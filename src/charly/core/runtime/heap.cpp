/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>

#include "charly/core/runtime/heap.h"
#include "charly/core/runtime/runtime.h"

#include "charly/utils/allocator.h"

using namespace std::chrono_literals;

namespace charly::core::runtime {

using utils::Allocator;

uint32_t HeapRegion::id() const {
  auto ptr = bitcast<uintptr_t>(this);
  ptr = ptr % kHeapTotalSize;
  return ptr / kHeapRegionSize;
}

uintptr_t HeapRegion::allocate(size_t size) {
  DCHECK(fits(size));

  auto buffer_start = bitcast<uintptr_t>(&this->buffer);
  uintptr_t buffer_next = buffer_start + this->used;
  this->used += size;

  return buffer_next;
}

bool HeapRegion::fits(size_t size) const {
  return size <= remaining_space();
}

size_t HeapRegion::remaining_space() const {
  return kHeapRegionUsableSize - this->used;
}

void HeapRegion::reset() {
  this->type = HeapRegion::Type::Unused;
  this->state = HeapRegion::State::Unused;
  this->used = 0;
  this->card_table.reset();
}

Heap::Heap(Runtime* runtime) : m_runtime(runtime) {
  std::lock_guard<std::mutex> locker(m_mutex);

  m_heap_base = Allocator::mmap_self_aligned(kHeapTotalSize);
  DCHECK((uintptr_t)m_heap_base % kHeapTotalSize == 0, "invalid heap alignment");

  // fill unmapped regions list
  auto base = bitcast<uintptr_t>(m_heap_base);
  for (size_t i = 0; i < kHeapRegionCount; i++) {
    auto* region = bitcast<HeapRegion*>(base + (i * kHeapRegionSize));
    m_unmapped_regions.push_back(region);
  }

  // map an initial amount of heap regions
  for (size_t i = 0; i < kHeapInitialMappedRegionCount; i++) {
    m_free_regions.push_back(map_new_region());
  }
}

Heap::~Heap() {
  std::unique_lock<std::mutex> locker(m_mutex);

  Allocator::munmap(m_heap_base, kHeapTotalSize);
  debugln("unmapped % heap regions", m_mapped_regions.size());

  m_heap_base = nullptr;
  m_unmapped_regions.clear();
  m_mapped_regions.clear();
  m_free_regions.clear();
  m_live_eden_regions.clear();
  m_released_eden_regions.clear();
  m_survivor_regions.clear();
  m_old_regions.clear();
}

HeapRegion* Heap::allocate_eden_region() {
  std::lock_guard<std::mutex> locker(m_mutex);

  HeapRegion* region = pop_free_region();
  if (region == nullptr) {
    region = map_new_region();

    if (region == nullptr) {
      return nullptr;
    }
  }

  DCHECK(region != nullptr);
  DCHECK(region->type == HeapRegion::Type::Unused);
  DCHECK(region->state == HeapRegion::State::Unused);
  region->type = HeapRegion::Type::Eden;
  region->state = HeapRegion::State::Owned;
  m_live_eden_regions.insert(region);

  return region;
}

void Heap::release_eden_region(HeapRegion* region) {
  std::lock_guard<std::mutex> locker(m_mutex);

  DCHECK(region != nullptr);
  DCHECK(region->type == HeapRegion::Type::Eden);
  DCHECK(region->state == HeapRegion::State::Owned);
  DCHECK(m_live_eden_regions.count(region) > 0);

  m_live_eden_regions.erase(region);
  m_released_eden_regions.insert(region);

  region->state = HeapRegion::State::Released;
}

HeapRegion* Heap::pop_free_region() {
  if (!m_free_regions.empty()) {
    HeapRegion* region = m_free_regions.front();
    m_free_regions.pop_front();
    return region;
  }

  m_runtime->gc()->request_gc();

  return nullptr;
}

HeapRegion* Heap::map_new_region() {
  if (!m_unmapped_regions.empty()) {
    HeapRegion* region = m_unmapped_regions.front();
    m_unmapped_regions.pop_front();

    Allocator::mmap_address(region, kHeapRegionSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED | MAP_FIXED);

    region->heap = this;
    region->type = HeapRegion::Type::Unused;
    region->state = HeapRegion::State::Unused;
    region->used = 0;
    region->card_table.reset();

    m_mapped_regions.insert(region);

    return region;
  }

  return nullptr;
}

ThreadAllocationBuffer::ThreadAllocationBuffer(Heap* heap) {
  m_heap = heap;
  m_region = nullptr;
  acquire_new_region();
}

ThreadAllocationBuffer::~ThreadAllocationBuffer() {
  if (m_region) {
    release_owned_region();
  }
  DCHECK(m_region == nullptr);
}

bool ThreadAllocationBuffer::allocate(size_t size, uintptr_t* address_out) {
  DCHECK(size % kObjectAlignment == 0, "allocation not aligned correctly");
  DCHECK(size <= kHeapObjectMaxSize, "allocation is too big");

  // release current region if it cannot fulfill the requested allocation
  if (m_region != nullptr && !m_region->fits(size)) {
    release_owned_region();
    DCHECK(m_region == nullptr);
  }

  if (m_region == nullptr) {
    acquire_new_region();

    // could not allocate a new region
    if (m_region == nullptr) {
      return false;
    }
  }

  DCHECK(m_region != nullptr);
  DCHECK(m_region->fits(size));

  uintptr_t addr = m_region->allocate(size);
  *address_out = addr;
  return true;
}

void ThreadAllocationBuffer::release_owned_region() {
  m_heap->release_eden_region(m_region);
  m_region = nullptr;
}

void ThreadAllocationBuffer::acquire_new_region() {
  DCHECK(m_region == nullptr);

  m_region = m_heap->allocate_eden_region();
  if (m_region == nullptr) {
    FAIL("out of memory!");
  }

  DCHECK(m_region->type == HeapRegion::Type::Eden);
  DCHECK(m_region->state == HeapRegion::State::Owned);
}

}  // namespace charly::core::runtime
