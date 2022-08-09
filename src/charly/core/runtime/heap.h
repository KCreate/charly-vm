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

#include <array>
#include <bitset>
#include <cstdlib>
#include <list>
#include <mutex>
#include <set>
#include <unordered_set>
#include <vector>

#include "charly/value.h"

#pragma once

namespace charly::core::runtime {

class Runtime;

static const size_t kHeapSize = kGb * 64;
static const size_t kHeapRegionSize = kKb * 512;
static const size_t kHeapRegionCount = kHeapSize / kHeapRegionSize;
static const size_t kHeapInitialMappedRegionCount = 16;

static const size_t kHeapRegionSpanSize = kKb;
static const size_t kHeapRegionSpanCount = kHeapRegionSize / kHeapRegionSpanSize;

static const uintptr_t kHeapRegionPointerMask = ~(kHeapRegionSize - 1);
static const uintptr_t kHeapRegionMagicNumber = 0xdeadbeefcafebabe;

static const size_t kGarbageCollectionAttempts = 4;

static const float kHeapExpectedFreeToMappedRatio = 0.30f;

class Heap;

struct HeapRegion {
  HeapRegion() = delete;

  enum class Type : uint8_t {
    Unused,        // region is not in use, currently in a freelist
    Eden,          // mutator threads allocate new objects in eden regions
    Intermediate,  // holds objects that survived one GC cycle
    Old            // holds objects that survived two or more GC cycles
  };

  // returns the id of this region
  uint32_t id() const;

  // allocate block of memory
  uintptr_t allocate(size_t size);

  // check if this region has enough space left for size bytes
  bool fits(size_t size) const;

  // returns the remaining amount of memory left in this region
  size_t remaining_space() const;

  void reset();

  // returns a pointer to the beginning of the buffer segment (after all the metadata)
  uintptr_t buffer_base() const;

  // check wether a given pointer points into this region
  bool pointer_points_into_region(uintptr_t pointer) const;

  size_t span_check_index_in_use(size_t span_index) const;
  size_t span_get_index_for_pointer(uintptr_t pointer) const;
  uintptr_t span_get_last_alloc_pointer(size_t span_index) const;
  bool span_get_dirty_flag(size_t span_index) const;

  void span_set_last_alloc_pointer(size_t span_index, uintptr_t pointer);
  void span_set_dirty_flag(size_t span_index, bool dirty = true);

  void each_object(std::function<void(ObjectHeader*)> callback);
  void each_object_in_span(size_t span_index, std::function<void(ObjectHeader*)> callback);

  uintptr_t magic;
  Heap* heap;
  Type type;
  size_t used;

  // stores last known object offset and dirty flag for each span
  // encoded as (offset << 32) | dirty
  //
  // object offset is relative to the HeapRegion::buffer field
  // spans are relative to the entire HeapRegion (metadata included)
  //
  // VM write barriers set the dirty flag on a span if they write a
  // reference to a young object to an object contained in an old region
  //
  // the object offsets for each span are populated during allocation
  // and are used by the GC to determine the start offset of the last
  // object in that span. because the beginning of a span might be in the
  // middle of an object, we can't just start scanning there but must
  // begin the scan at the offset of that previous object
  static constexpr size_t kSpanTableInvalidOffset = 0xffffffff;
  static constexpr size_t kSpanTableOffsetShift = 32;
  static constexpr size_t kSpanTableDirtyFlagMask = 0x1;
  std::array<size_t, kHeapRegionSpanCount> span_table;
  uint8_t buffer alignas(kObjectAlignment)[];
};
static_assert(sizeof(HeapRegion) % kObjectAlignment == 0);
static_assert(offsetof(HeapRegion, buffer) % kObjectAlignment == 0);
static_assert(offsetof(HeapRegion, buffer) == sizeof(HeapRegion));

// the first couple region spans are taken up by the heap region metadata
static const size_t kHeapRegionFirstUsableSpanIndex = sizeof(HeapRegion) / kHeapRegionSpanSize;

// amount of bytes that can be used in a heap region
static const size_t kHeapRegionUsableSize = kHeapRegionSize - sizeof(HeapRegion);

// usable size for object payloads (size excluding object header)
static const size_t kHeapRegionUsableSizeForPayload = kHeapRegionUsableSize - sizeof(ObjectHeader);

class ThreadAllocationBuffer;
class GarbageCollector;

class Heap {
  friend class GarbageCollector;

public:
  explicit Heap(Runtime* runtime);
  ~Heap();

  HeapRegion* acquire_region(Thread* thread, HeapRegion::Type type);
  HeapRegion* acquire_region_internal(HeapRegion::Type type);
  HeapRegion* pop_free_region();
  HeapRegion* map_new_region();

  void adjust_heap_size();
  void unmap_free_region();

  // check wether a given pointer points into the charly heap
  bool is_heap_pointer(uintptr_t pointer) const;

  // check wether a given pointer points into a live region
  bool is_valid_pointer(uintptr_t pointer) const;

  const void* heap_base() const {
    return m_heap_base;
  }

private:
  Runtime* m_runtime;
  std::mutex m_mutex;

  // pointer to the base of the heap address space
  void* m_heap_base;

  // region mappings
  std::set<HeapRegion*> m_unmapped_regions;
  std::set<HeapRegion*> m_mapped_regions;
  std::set<HeapRegion*> m_free_regions;

  std::set<HeapRegion*> m_eden_regions;
  std::set<HeapRegion*> m_intermediate_regions;
  std::set<HeapRegion*> m_old_regions;
};

class Thread;
class ThreadAllocationBuffer {
  friend class GarbageCollector;
public:
  explicit ThreadAllocationBuffer(Heap* heap);
  ~ThreadAllocationBuffer();

  uintptr_t allocate(Thread* thread, size_t size);

private:
  void release_owned_region();
  void acquire_new_region(Thread* thread);

private:
  Heap* m_heap;
  HeapRegion* m_region;
};

}  // namespace charly::core::runtime
