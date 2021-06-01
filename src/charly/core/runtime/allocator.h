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

#include <cstdlib>
#include <array>
#include <list>
#include <set>
#include <vector>
#include <mutex>

#include "charly/charly.h"
#include "charly/atomic.h"

#include "charly/core/runtime/scheduler.h"

#pragma once

namespace charly::core::runtime {

// forward declarations
struct Worker;
class MemoryAllocator;
class GarbageCollector;

// amount of cells per heap region
static const size_t kHeapRegionSize = 1024 * 16; // 16 kilobyte regions

// initial amount of heap regions allocated when starting the machine
static const size_t kHeapInitialRegionCount = 64;

// the maximum amount of heap regions allowed to be allocated. any allocation
// performed after that issue was reached will fail
static const size_t kHeapRegionLimit = 1024;

// heap fill percentage at which to begin concurrent collection
static const float kHeapGCTrigger = 0.5;

// heap fill percentage at which to grow the heap
static const float kHeapGCGrowTrigger = 0.9;

// the amount of times an allocation should wait for the next GC cycle
// before it fails
static const uint32_t kHeapAllocationAttempts = 10;

// heap object pointers are required to be aligned by 8 byte boundaries
// this ensures that the lower three bits of every heap pointer are set to 0
constexpr size_t kHeapObjectAlignment = static_cast<size_t>(1 << 3);

// heap regions are equal-sized chunks of memory that hold the charly
// heap allocated values
struct HeapRegion {
  HeapRegion() {
    static charly::atomic<uint64_t> region_id_counter = 0;
    this->id = region_id_counter++;
    this->next = 0;
    this->state = State::Available;
  }

  void* allocate(size_t size) {
    assert(this->next + size <= kHeapRegionSize);
    uint8_t* head = this->buffer + this->next;
    this->next += size;

    // align next allocation
    size_t count_unaligned_bytes = this->next % kHeapObjectAlignment;
    if (count_unaligned_bytes) {
      this->next += (kHeapObjectAlignment - count_unaligned_bytes);
    }

    return head;
  }

  bool fits(size_t size) {
    return this->next + size <= kHeapRegionSize;
  }

  void acquire() {
    assert(this->state == State::Available);
    this->state = State::Used;
  }

  void release() {
    assert(this->state == State::Used);
    this->state = State::Released;
  }

  void reset() {
    assert(this->state == State::Released);
    this->next = 0;
    this->state = State::Available;
  }

  enum class State : uint8_t {
    Available,  // region contains no live data and can be acquired by workers
    Used,       // region may contain live data and is currently in use by a worker
    Released    // region may contain live data but is no longer used by a worker
  };

  uint64_t id;
  size_t next;
  uint8_t buffer[kHeapRegionSize];
  State state;
};

enum class HeapType : uint8_t {
  Dead = 0,
  Fiber
};

enum class MarkColor : uint8_t {
  White = 0,    // value not reachable
  Grey,         // value currently being traversed
  Black         // value reachable
};

static const uint32_t kHeapHeaderMagicNumber = 0x4543494E; // ascii "NICE" in little-endian

// this header struct gets allocated before each user allocated value
// it stores data used by the runtime, such as the type field but also
// data used by the garbage collector and associated systems
struct HeapHeader {
  charly::atomic<void*> forward_ptr;        // points to itself or forwarded cell during evacuation phase
  charly::atomic<HeapType> type;            // stores the runtime type information
  charly::atomic<MarkColor> gcmark;         // stores GC mark color information

#ifndef NDEBUG
  uint32_t magic_number;  // magic number active during debugging
#endif
};
static_assert(sizeof(HeapHeader) % kHeapObjectAlignment == 0, "invalid heap header size");

class MemoryAllocator {
  friend struct Worker;
  friend class GarbageCollector;
public:
  MemoryAllocator() {

    // allocate initial set of free regions
    for (size_t i = 0; i < kHeapInitialRegionCount; i++) {
      HeapRegion* region = allocate_new_region();
      assert(region);
      free_region(region);
    }
  }

  static void initialize() {
    static MemoryAllocator allocator;
    MemoryAllocator::instance = &allocator;
  }
  inline static MemoryAllocator* instance = nullptr;

  // allocates a memory region of a given size
  // automatically prepends a heapheader to the allocation, so the total size
  // of the allocation is size + sizeof(HeapHeader)
  void* allocate(HeapType type, size_t size);

  // returns a pointer to the header of a heap allocated object
  static HeapHeader* object_header(void* object);

private:

  // worker / global allocation methods
  void* allocate_worker(HeapType type, size_t size);
  void* allocate_global(HeapType type, size_t size);

  // acquire a new region. either reuses an already allocated region that has been cleared
  // or allocates a new region on the system heap
  HeapRegion* acquire_region();

  // allocate a new region from the system heap
  HeapRegion* allocate_new_region();

  // append a region to the freelist
  void free_region(HeapRegion* region);

private:

  // list of free regions available to be given out to worker threads
  std::mutex m_freelist_mutex;
  std::condition_variable m_freelist_cv;
  std::list<HeapRegion*> m_freelist;
  charly::atomic<size_t> m_free_regions = 0;

  // list of all allocated regions
  std::mutex m_regions_mutex;
  std::list<HeapRegion*> m_regions;
  charly::atomic<size_t> m_allocated_regions = 0;

  // this heap region serves allocations that are performed outside of application
  // worker threads.
  std::mutex m_global_region_mutex;
  HeapRegion* m_global_region = nullptr;
};

}  // namespace charly::core::runtime
