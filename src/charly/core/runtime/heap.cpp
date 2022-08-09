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

#include "charly/core/runtime/heap.h"
#include "charly/core/runtime/runtime.h"

#include "charly/utils/allocator.h"

using namespace std::chrono_literals;

namespace charly::core::runtime {

using utils::Allocator;

uint32_t HeapRegion::id() const {
  auto ptr = bitcast<uintptr_t>(this);
  ptr = ptr % kHeapSize;
  return ptr / kHeapRegionSize;
}

uintptr_t HeapRegion::allocate(size_t size) {
  DCHECK(fits(size));

  uintptr_t alloc_offset = this->used;
  uintptr_t alloc_pointer = buffer_base() + alloc_offset;
  this->used += size;
  uintptr_t alloc_end = alloc_pointer + size;

  // update last known object addresses in span table
  // also mark intermediate spans if the object takes up multiple spans
  size_t span_index_start = span_get_index_for_pointer(alloc_pointer);
  size_t span_index_end = kHeapRegionCount - 1;
  if (alloc_end < bitcast<uintptr_t>(this) + kHeapRegionSize) {
    span_index_end = span_get_index_for_pointer(alloc_end);
  }
  for (size_t index = span_index_start; index <= span_index_end && index < kHeapRegionSpanCount; index++) {
    span_set_last_alloc_pointer(index, alloc_pointer);
  }

  return alloc_pointer;
}

bool HeapRegion::fits(size_t size) const {
  return size <= remaining_space();
}

size_t HeapRegion::remaining_space() const {
  return kHeapRegionUsableSize - this->used;
}

void HeapRegion::reset() {
  this->type = HeapRegion::Type::Unused;
  this->used = 0;
  this->span_table.fill(kSpanTableInvalidOffset << kSpanTableOffsetShift);
  DCHECK(buffer_base() - (uintptr_t)(this) + kHeapRegionUsableSize == kHeapRegionSize);
  std::memset((void*)buffer_base(), 0, kHeapRegionUsableSize);
}

uintptr_t HeapRegion::buffer_base() const {
  return bitcast<uintptr_t>(&this->buffer);
}

bool HeapRegion::pointer_points_into_region(uintptr_t pointer) const {
  auto data = buffer_base();
  auto data_end = data + this->used;
  return pointer >= data && pointer < data_end;
}

size_t HeapRegion::span_get_index_for_pointer(uintptr_t pointer) const {
  pointer_points_into_region(pointer);
  size_t alloc_offset = static_cast<size_t>(pointer - bitcast<uintptr_t>(this));
  size_t span_index = alloc_offset / kHeapRegionSpanSize;
  DCHECK(span_index < kHeapRegionSpanCount);
  return span_index;
}

uintptr_t HeapRegion::span_get_last_alloc_pointer(size_t span_index) const {
  DCHECK(span_index < kHeapRegionSpanCount);
  auto offset = this->span_table[span_index] >> kSpanTableOffsetShift;
  DCHECK(offset != kSpanTableInvalidOffset);
  auto pointer = buffer_base() + offset;
  DCHECK(pointer_points_into_region(pointer));
  return pointer;
}

bool HeapRegion::span_get_dirty_flag(size_t span_index) const {
  DCHECK(span_index < kHeapRegionSpanCount);
  return this->span_table[span_index] & kSpanTableDirtyFlagMask;
}

void HeapRegion::span_set_last_alloc_pointer(size_t span_index, uintptr_t pointer) {
  DCHECK(span_index < kHeapRegionSpanCount);
  DCHECK(pointer_points_into_region(pointer));
  auto offset = pointer - buffer_base();
  auto dirty = span_get_dirty_flag(span_index);
  this->span_table[span_index] = offset << kSpanTableOffsetShift;
  span_set_dirty_flag(span_index, dirty);
}

void HeapRegion::span_set_dirty_flag(size_t span_index, bool dirty) {
  DCHECK(span_index < kHeapRegionSpanCount);
  size_t* entry = &this->span_table[span_index];
  bool* flag_pointer = static_cast<bool*>(static_cast<void*>(entry));
  *flag_pointer = dirty;
}

void HeapRegion::each_object(std::function<void(ObjectHeader*)> callback) {
  size_t alloc_size;
  auto scan_end = buffer_base() + this->used;
  for (uintptr_t scan = buffer_base(); scan < scan_end; scan += alloc_size) {
    auto* header = bitcast<ObjectHeader*>(scan);
    alloc_size = header->alloc_size();
    DCHECK(scan + alloc_size <= scan_end);
    callback(header);
  }
}

void HeapRegion::each_object_in_span(size_t span_index, std::function<void(ObjectHeader*)> callback) {
  DCHECK(span_index < kHeapRegionSpanCount);
  auto region_begin = bitcast<uintptr_t>(this);
  uintptr_t span_begin = region_begin + span_index * kHeapRegionSpanSize;
  uintptr_t span_end = span_begin + kHeapRegionSpanSize;
  uintptr_t buffer_begin = buffer_base();
  uintptr_t buffer_end = buffer_begin + this->used;

  // do not scan spans with no objects in them
  if (span_begin >= buffer_end || span_end <= buffer_begin) {
    return;
  }

  // the beginning of the span might be in the middle of an
  // object from the previous span, so we consult the span table
  // for the address of the last object in that span and start
  // scanning from there
  uintptr_t scan = buffer_begin;
  if (span_index > kHeapRegionFirstUsableSpanIndex) {
    uintptr_t last_alloc_ptr = span_get_last_alloc_pointer(span_index - 1);
    if (last_alloc_ptr != kSpanTableInvalidOffset) {
      auto* header = bitcast<ObjectHeader*>(last_alloc_ptr);
      scan = last_alloc_ptr + header->alloc_size();
    }
  }

  DCHECK(scan >= span_begin);
  while (scan < span_end && scan < buffer_end) {
    auto* header = bitcast<ObjectHeader*>(scan);
    callback(header);
    scan += header->alloc_size();
  }
}

Heap::Heap(Runtime* runtime) : m_runtime(runtime) {
  std::lock_guard<std::mutex> locker(m_mutex);

  m_heap_base = Allocator::mmap_self_aligned(kHeapSize);

  // fill unmapped regions list
  auto base = bitcast<uintptr_t>(m_heap_base);
  for (size_t i = 0; i < kHeapRegionCount; i++) {
    auto* region = bitcast<HeapRegion*>(base + (i * kHeapRegionSize));
    m_unmapped_regions.insert(region);
  }

  // map an initial amount of heap regions
  for (size_t i = 0; i < kHeapInitialMappedRegionCount; i++) {
    m_free_regions.insert(map_new_region());
  }
}

Heap::~Heap() {
  std::unique_lock<std::mutex> locker(m_mutex);
  Allocator::munmap(m_heap_base, kHeapSize);
  m_heap_base = nullptr;
  m_unmapped_regions.clear();
  m_mapped_regions.clear();
  m_free_regions.clear();
  m_eden_regions.clear();
  m_old_regions.clear();
}

HeapRegion* Heap::acquire_region(Thread* thread, HeapRegion::Type type) {
  std::unique_lock locker(m_mutex);

  HeapRegion* region = pop_free_region();
  if (region == nullptr) {
    for (size_t attempt = 0; attempt < kGarbageCollectionAttempts; attempt++) {
      locker.unlock();
      m_runtime->gc()->perform_gc(thread);
      locker.lock();

      region = pop_free_region();

      if (region) {
        break;
      }
    }

    if (region == nullptr) {
      FAIL("could not allocate free region");
    }
  }

  DCHECK(region != nullptr);
  DCHECK(region->type == HeapRegion::Type::Unused);
  region->type = type;

  switch (type) {
    case HeapRegion::Type::Eden: m_eden_regions.insert(region); break;
    case HeapRegion::Type::Intermediate: m_intermediate_regions.insert(region); break;
    case HeapRegion::Type::Old: m_old_regions.insert(region); break;
    default: FAIL("unknown region type");
  }

  return region;
}

HeapRegion* Heap::acquire_region_internal(HeapRegion::Type type) {
  std::unique_lock locker(m_mutex);

  HeapRegion* region = pop_free_region();
  if (region == nullptr) {
    region = map_new_region();
  }

  DCHECK(region != nullptr);
  DCHECK(region->type == HeapRegion::Type::Unused);
  region->type = type;

  switch (type) {
    case HeapRegion::Type::Eden: m_eden_regions.insert(region); break;
    case HeapRegion::Type::Intermediate: m_intermediate_regions.insert(region); break;
    case HeapRegion::Type::Old: m_old_regions.insert(region); break;
    default: FAIL("unknown region type");
  }

  return region;
}

void Heap::adjust_heap_size() {
  float mapped_region_count = m_mapped_regions.size();
  float free_region_count = m_free_regions.size();
  DCHECK(free_region_count <= mapped_region_count);
  float ratio_free_mapped = free_region_count / mapped_region_count;
  DCHECK(ratio_free_mapped >= 0.0 || ratio_free_mapped <= 1.0);

  // m = mapped regions
  // f = free regions
  // e = expected free/mapped ratio
  // x = change to total region count to reach ratio
  //
  // x = ceil((f - em) / (e - 1))
  float d1 = free_region_count - kHeapExpectedFreeToMappedRatio * mapped_region_count;
  float d2 = kHeapExpectedFreeToMappedRatio - 1;
  int64_t region_delta = std::ceil(d1 / d2);
  if (region_delta > 0) {
    size_t regions_to_add = region_delta;
    if (regions_to_add > m_unmapped_regions.size()) {
      regions_to_add = m_unmapped_regions.size();
    }

    for (size_t i = 0; i < regions_to_add; i++) {
      m_free_regions.insert(map_new_region());
    }
  } else {
    size_t regions_to_remove = -region_delta;
    DCHECK(mapped_region_count >= regions_to_remove);
    DCHECK(regions_to_remove < m_free_regions.size());
    size_t i = 0;
    for (i = 0; i < regions_to_remove && m_mapped_regions.size() > kHeapInitialMappedRegionCount; i++) {
      unmap_free_region();
    }
  }
}

void Heap::unmap_free_region() {
  DCHECK(m_free_regions.size() > 0);

  HeapRegion* region = *m_free_regions.rbegin();
  m_free_regions.erase(region);

  Allocator::munmap(region, kHeapRegionSize);
  Allocator::mmap_address(region, kHeapRegionSize);

  m_mapped_regions.erase(region);
  m_unmapped_regions.insert(region);
}

bool Heap::is_heap_pointer(uintptr_t pointer) const {
  auto heap_base = bitcast<const uintptr_t>(m_heap_base);
  return pointer >= heap_base && pointer < (heap_base + kHeapSize);
}

bool Heap::is_valid_pointer(uintptr_t pointer) const {
  if (!is_heap_pointer(pointer)) {
    return false;
  }

  for (auto* region : m_mapped_regions) {
    if (region->type != HeapRegion::Type::Unused) {
      if (region->pointer_points_into_region(pointer)) {
        return true;
      }
    }
  }

  return false;
}

HeapRegion* Heap::pop_free_region() {
  if (!m_free_regions.empty()) {
    HeapRegion* region = *m_free_regions.begin();
    m_free_regions.erase(region);
    return region;
  }

  return nullptr;
}

HeapRegion* Heap::map_new_region() {
  if (!m_unmapped_regions.empty()) {
    HeapRegion* region = *m_unmapped_regions.begin();
    m_unmapped_regions.erase(region);

    Allocator::mmap_address(region, kHeapRegionSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED | MAP_FIXED);

    region->magic = kHeapRegionMagicNumber;
    region->heap = this;
    region->reset();

    m_mapped_regions.insert(region);

    return region;
  }

  FAIL("out of memory!");
}

ThreadAllocationBuffer::ThreadAllocationBuffer(Heap* heap) {
  m_heap = heap;
  m_region = nullptr;
}

ThreadAllocationBuffer::~ThreadAllocationBuffer() {
  if (m_region) {
    release_owned_region();
  }
}

uintptr_t ThreadAllocationBuffer::allocate(Thread* thread, size_t size) {
  DCHECK(size % kObjectAlignment == 0, "allocation not aligned correctly");
  DCHECK(size <= kHeapRegionUsableSize, "allocation is too big");

  // release current region if it cannot fulfill the requested allocation
  if (m_region != nullptr && !m_region->fits(size)) {
    release_owned_region();
    DCHECK(m_region == nullptr);
  }

  if (m_region == nullptr) {
    acquire_new_region(thread);

    // could not allocate a new region
    if (m_region == nullptr) {
      FAIL("allocation failed");
    }
  }

  DCHECK(m_region != nullptr);
  DCHECK(m_region->fits(size));

  return m_region->allocate(size);
}

void ThreadAllocationBuffer::release_owned_region() {
  DCHECK(m_region);
  m_region = nullptr;
}

void ThreadAllocationBuffer::acquire_new_region(Thread* thread) {
  DCHECK(m_region == nullptr);
  m_region = m_heap->acquire_region(thread, HeapRegion::Type::Eden);
  DCHECK(m_region);
}

}  // namespace charly::core::runtime
