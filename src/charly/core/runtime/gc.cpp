/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2021 Leonard Schütz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "charly/core/runtime/scheduler.h"
#include "charly/core/runtime/gc.h"

using namespace std::chrono_literals;

namespace charly::core::runtime {

void GarbageCollector::initialize() {
  static GarbageCollector collector;
  GarbageCollector::instance = &collector;

  // allocate initial set of heap regions
  for (size_t i = 0; i < kHeapInitialRegionCount; i++) {
    collector.add_heap_region();
  }
}

void* GarbageCollector::alloc_size(size_t size) {
  HeapRegion* region = g_worker->active_region();

  // no region or not enough space left in current region for object of *size*
  if (region == nullptr || !region->fits(size)) {
    g_worker->clear_gc_region();

    if (should_begin_collection()) {
      // safeprint("requesting gc cycle");
      m_concurrent_worker.request_gc();
    }

    region = allocate_heap_region();
    if (region == nullptr) {
      return nullptr;
    }

    g_worker->set_gc_region(region);
    safeprint("worker % allocated region %", g_worker->id, region->id());
  }

  // serve the allocation from our currently active region
  void* head = region->allocate(size);

  safeprint(
    "allocated % (% bytes) (% bytes left) from region % (%/%) in worker %",
    head,
    size,
    kHeapRegionSize - region->used(),
    region,
    region->id(),
    m_allocated_regions.load(std::memory_order_relaxed),
    g_worker->id
  );

  return head;
}

void GarbageCollector::grow_heap() {
  size_t regions_to_add = m_allocated_regions.load(std::memory_order_acquire);
  for (size_t i = 0; i < regions_to_add; i++) {
    add_heap_region();
  }
}

GCPhase GarbageCollector::phase() const {
  return m_concurrent_worker.phase();
}

bool GarbageCollector::can_allocate() {
  return m_free_regions.load(std::memory_order_relaxed);
}

void GarbageCollector::free_region(HeapRegion* region) {
  std::unique_lock<std::mutex> locker(m_mutex);
  region->reset();
  m_freelist.push_back(region);
  m_free_regions++;
}

HeapRegion* GarbageCollector::allocate_heap_region() {
  std::unique_lock<std::mutex> locker(m_mutex);

  if (should_grow()) {
    safeprint("GC growing heap");
    locker.unlock();
    grow_heap();
    locker.lock();
  }

  // if there are no free regions, wait for the GC to complete a cycle
  if (m_freelist.size() == 0) {
    for (size_t tries = kHeapAllocationAttempts; tries > 0; tries--) {
      locker.unlock();
      safeprint("worker % waiting for GC cycle (%)", g_worker->id, tries);
      g_worker->pause();
      locker.lock();

      if (m_freelist.size()) {
        safeprint("worker % finished waiting for GC cycle", g_worker->id);
        break;
      }
    }

    if (m_freelist.size() == 0) {
      return nullptr;
    }
  }

  HeapRegion* region = m_freelist.front();
  m_freelist.pop_front();
  m_free_regions--;
  return region;
}

void GarbageCollector::add_heap_region() {
  std::unique_lock<std::mutex> locker(m_mutex);

  // heap limit reached
  if (m_regions.size() >= kHeapRegionLimit) {
    return;
  }

  HeapRegion* region = new HeapRegion();
  m_regions.push_back(region);
  m_allocated_regions++;

  locker.unlock();

  free_region(region);
}

float GarbageCollector::utilization() {
  size_t total_regions = m_allocated_regions.load(std::memory_order_acquire);
  size_t full_regions = total_regions - m_free_regions.load(std::memory_order_acquire);
  return (float)full_regions / (float)total_regions;
}

}  // namespace charly::core::runtime
