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

#include "charly/core/runtime/allocator.h"
#include "charly/core/runtime/scheduler.h"
#include "charly/core/runtime/gc.h"

using namespace std::chrono_literals;

namespace charly::core::runtime {

MemoryAllocator::MemoryAllocator() {
  for (size_t i = 0; i < kHeapInitialRegionCount; i++) {
    HeapRegion* region = allocate_new_region();
    assert(region);
    free_region(region);
  }
}

MemoryAllocator::~MemoryAllocator() {
  std::lock(m_freelist_mutex, m_regions_mutex);
  m_freelist.clear();

  for (HeapRegion* region : m_regions) {
    delete region;
  }

  m_regions.clear();
}

void* MemoryAllocator::allocate_memory(size_t size) {
  assert(sizeof(HeapHeader) + size <= kHeapRegionSize && "object too big");

  std::unique_lock<std::mutex> global_region_lock(m_global_region_mutex, std::defer_lock);
  HeapRegion** region_ptr;
  HeapRegion* region;

  // acquire the region that serves this allocation
  if (Processor* proc = Scheduler::instance->processor()) {
    region_ptr = &proc->active_region;
    region = *region_ptr;
  } else {
    region_ptr = &m_global_region;
    region = *region_ptr;
    global_region_lock.lock();
  }

  // release region if it cannot fulfill the allocation
  if (region && !region->fits(size)) {
    safeprint("releasing region %", region->id);
    assert(region->state == HeapRegion::State::Used);
    region->state.acas(HeapRegion::State::Used, HeapRegion::State::Released);
    region = nullptr;
    *region_ptr = nullptr;
  }

  // allocate new region if no current region
  if (region == nullptr) {
    region = acquire_region();

    // failed to allocate new region
    if (!region) {
      return nullptr;
    }

    *region_ptr = region;
    safeprint("acquired region %", region->id);
  }

  return region->allocate(size);
}

HeapHeader* MemoryAllocator::object_header(void* object) {
  HeapHeader* header = (HeapHeader*)((uintptr_t)object - (uintptr_t)(sizeof(HeapHeader)));
#ifndef NDEBUG
  assert(header->magic_number == kHeapHeaderMagicNumber);
#endif
  return header;
}

HeapRegion* MemoryAllocator::acquire_region() {
  float util = utilization();

  // start GC worker thread if threshold is reached
  if (util > kHeapUtilizationCollectionTrigger) {
    GarbageCollector::instance->request_gc();
  }

  // aggressively expand heap if threshold is reached
  if (util > kHeapUtilizationGrowTrigger) {
    expand_heap();
  }

  // FIXME: there could be a race condition where the newly allocated regions created
  // by the call to expand_heap() are already taken off the freelist by the time
  // this thread checks for them

  std::unique_lock<std::mutex> locker(m_freelist_mutex);
  if (m_freelist.size()) {
    HeapRegion* region = m_freelist.front();
    m_freelist.pop_front();
    m_free_regions--;

    region->state.acas(HeapRegion::State::Available, HeapRegion::State::Used);
    return region;
  }

  // FIXME: implement some mechanism to wait for the GC to complete a cycle,
  // in the hope that there will be available free regions by then
  //
  // if multiple GC iterations fail to provide any new regions, fail the allocation

  return nullptr;
}

HeapRegion* MemoryAllocator::allocate_new_region() {
  std::lock_guard<std::mutex> locker(m_regions_mutex);
  HeapRegion* region = new HeapRegion();
  m_regions.push_back(region);
  m_allocated_regions++;
  return region;
}

bool MemoryAllocator::expand_heap() {

  // calculate new target region count
  size_t target_region_count = m_allocated_regions * kHeapGrowFactor;
  if (target_region_count > kHeapRegionLimit) {
    target_region_count = kHeapRegionLimit;
  }

  // allocate new regions
  bool allocated_one = false;
  while (m_allocated_regions < target_region_count) {
    HeapRegion* region = allocate_new_region();
    assert(region);
    free_region(region);
    allocated_one = true;
  }

  return allocated_one;
}

void MemoryAllocator::free_region(HeapRegion* region) {

  // reset region
  region->state.store(HeapRegion::State::Available);
  region->next = 0;

  // append to region freelist
  {
    std::unique_lock<std::mutex> locker(m_freelist_mutex);
    m_freelist.push_back(region);
    m_free_regions++;
  }

  m_freelist_cv.notify_one();
}

float MemoryAllocator::utilization() {
  float amount_of_regions = m_allocated_regions;
  float amount_of_free_regions = m_free_regions;
  float factor_free_regions = amount_of_free_regions / amount_of_regions;
  float factor_used_regions = 1.0 - factor_free_regions;
  return factor_used_regions;
}

}  // namespace charly::core::runtime
