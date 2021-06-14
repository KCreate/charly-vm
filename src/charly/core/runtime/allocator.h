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
#include "charly/value.h"

#include "charly/core/runtime/scheduler.h"

#pragma once

namespace charly::core::runtime {

// forward declarations
struct Worker;
class MemoryAllocator;
class GarbageCollector;

// amount of cells per heap region
// static const size_t kHeapRegionSize = 1024 * 128; // 128 kilobyte regions
static const size_t kHeapRegionSize = 1024;

// initial amount of heap regions allocated when starting the machine
static const size_t kHeapInitialRegionCount = 2;

// the maximum amount of heap regions allowed to be allocated. any allocation
// performed after that issue was reached will fail
static const size_t kHeapRegionLimit = 4;

// heap fill percentage at which to begin concurrent collection
static const float kHeapUtilizationCollectionTrigger = 0.5;

// heap fill percentage at which to aggressively expand the heap
static const float kHeapUtilizationGrowTrigger = 0.9;

// factor used to calculate the new amount of heap regions that should be allocated
static const float kHeapGrowFactor = 2;

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
    this->state.store(State::Available);
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

  enum class State : uint8_t {
    Available,  // region contains no live data and can be acquired
    Used,       // region may contain live data and is currently owned by a worker
    Released    // region may contain live data and is not owned by a worker
  };

  uint64_t id;
  size_t next;
  uint8_t buffer[kHeapRegionSize];
  charly::atomic<State> state;
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
  MemoryAllocator();
  ~MemoryAllocator();

  static void initialize() {
    static MemoryAllocator allocator;
    MemoryAllocator::instance = &allocator;
  }
  inline static MemoryAllocator* instance = nullptr;

  // allocate object of type T
  template <typename T, typename... Args>
  static T* allocate(Args&&... params) {

    // allocate memory for the object + header
    size_t header_size = sizeof(HeapHeader);
    size_t object_size = sizeof(T);
    size_t total_size = header_size + object_size;
    void* memory = MemoryAllocator::instance->allocate_memory(total_size);

    if (memory == nullptr) {
      return nullptr;
    }

    HeapHeader* header = (HeapHeader*)memory;
    T* object = (T*)((uintptr_t)memory + sizeof(HeapHeader));

    // initialize heap header
#ifndef NDEBUG
    header->magic_number = kHeapHeaderMagicNumber;
#endif
    header->forward_ptr.store((void*)object);
    header->type.store(T::heap_value_type());
    header->gcmark.store(MarkColor::Black); // newly allocated values are colored black

    // call object constructor
    new ((T*)object)T(std::forward<Args>(params)...);

    return object;
  }

  // returns a pointer to the header of a heap allocated object
  static HeapHeader* object_header(void* object);

private:

  // allocate *size* bytes if memory from either the current processors current region
  // or the global region
  void* allocate_memory(size_t size);

  // acquire a new region. either reuses an already allocated region that has been cleared
  // or allocates a new region on the system heap
  HeapRegion* acquire_region();

  // allocate a new region from the system heap
  HeapRegion* allocate_new_region();

  // attempt to aggressively expand the heap by growing it by some set factor
  //
  // returns false if the total limit for allocated regions was hit and the heap could
  // not be expanded
  bool expand_heap();

  // append a region to the freelist
  void free_region(HeapRegion* region);

  // heap utilization (amount of heap regions that contain live data)
  // returns a float between 0.0 - 1.0
  float utilization();

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
