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

constexpr size_t kHeapSize = kGb * 64;
constexpr size_t kHeapRegionSize = kKb * 512;
constexpr size_t kHeapRegionCount = kHeapSize / kHeapRegionSize;
constexpr size_t kHeapMinimumMappedRegionCount = 32;

constexpr size_t kHeapRegionSpanSize = kKb;
constexpr size_t kHeapRegionSpanCount = kHeapRegionSize / kHeapRegionSpanSize;

constexpr uintptr_t kHeapRegionPointerMask = ~(kHeapRegionSize - 1);
constexpr uintptr_t kHeapRegionMagicNumber = 0xdeadbeefcafebabe;

constexpr float kHeapGrowthFactor = 1.5f;
constexpr float kHeapShrinkFactor = 0.5f;

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
  uintptr_t allocate(size_t size, bool contains_external_heap_pointers = false);

  // check if this region has enough space left for size bytes
  bool fits(size_t size) const;

  // returns the remaining amount of memory left in this region
  size_t remaining_space() const;

  void reset();

  // returns a pointer to the beginning of the buffer segment (after all the metadata)
  uintptr_t buffer_base() const;

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

  // contains pointers to all objects (starting at header) within this region
  // that could contain a pointer to the regular heap (e.g. HugeString, HugeBytee, Future, etc)
  // this table is populated during allocation of objects and used during GC to prevent
  // having to traverse every single object on the heap in the search of dead objects
  std::vector<uintptr_t> objects_with_external_heap_pointers;

  uint8_t buffer alignas(kObjectAlignment)[];
};
static_assert(sizeof(HeapRegion) % kObjectAlignment == 0);
static_assert(offsetof(HeapRegion, buffer) % kObjectAlignment == 0);
static_assert(offsetof(HeapRegion, buffer) == sizeof(HeapRegion));

// the first couple region spans are taken up by the heap region metadata
constexpr size_t kHeapRegionFirstUsableSpanIndex = sizeof(HeapRegion) / kHeapRegionSpanSize;

// amount of bytes that can be used in a heap region
constexpr size_t kHeapRegionUsableSize = kHeapRegionSize - sizeof(HeapRegion);

// usable size for object payloads (size excluding object header)
constexpr size_t kHeapRegionUsableSizeForPayload = kHeapRegionUsableSize - sizeof(ObjectHeader);

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

  void grow_heap();
  void shrink_heap();

  void unmap_free_region();

  // check wether a given pointer points into the charly heap
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

  uintptr_t allocate(Thread* thread, size_t size, bool contains_external_heap_pointers = false);

private:
  void release_owned_region();
  void acquire_new_region(Thread* thread);

private:
  Heap* m_heap;
  HeapRegion* m_region;
};

}  // namespace charly::core::runtime
