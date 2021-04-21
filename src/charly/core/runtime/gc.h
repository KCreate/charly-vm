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
#include <vector>
#include <mutex>

#include "charly/charly.h"
#include "charly/atomic.h"

#include "charly/core/runtime/heapvalue.h"
#include "charly/core/runtime/gcworker.h"

#pragma once

namespace charly::core::runtime {

// forward declarations
class Worker;
class GarbageCollector;

// amount of cells per heap region
static const size_t kHeapRegionCellCount = 2048;

// initial amount of heap regions allocated when starting the machine
static const size_t kHeapInitialRegionCount = 64;

// the maximum amount of heap regions allowed to be allocated. any allocation
// performed after that issue was reached will fail
static const size_t kHeapRegionLimit = 1024;

// heap fill percentage at which to begin concurrent collection
static const float kHeapGCTrigger = 0.4;

// heap fill percentage at which to grow the heap
static const float kHeapGCGrowTrigger = 0.9;

// the amount of times an allocation should wait for the next GC cycle
// before it fails
static const size_t kHeapAllocationAttempts = 10;

struct HeapCell {
  HeapCell() {}
  ~HeapCell() {}

  union {
    HeapTestType test;
  } as;
};

struct HeapRegion {
  HeapRegion() {
    static uint32_t id_counter = 1;
    id = id_counter++;
  }

  uint32_t id;
  charly::atomic<Worker*> allocated_worker = nullptr;
  size_t next_cell = 0;
  HeapCell buffer[kHeapRegionCellCount];
};

class GarbageCollector {
  friend class Worker;
  friend class GCConcurrentWorker;
public:
  GarbageCollector() : m_concurrent_worker(this) {}

  static void initialize();

  void start_background_worker() {
    m_concurrent_worker.start_thread();
  }

  void stop_background_worker() {
    m_concurrent_worker.stop_thread();
  }

  inline static GarbageCollector* instance = nullptr;

  HeapCell* allocate();

  // checks wether the garbage collector might be able to allocate a value right now
  bool can_allocate();

  // grow the heap to contain a certain amount of regions
  void grow_heap();

private:
  // append a region to the freelist
  void free_region(HeapRegion* region);

  // reserve a heap region for a worker
  // returns nullptr if the memory limit was reached
  HeapRegion* allocate_heap_region();

  // allocates a new heap region and appends it to the freelist
  void add_heap_region();

  // percentage (0.0 - 1.0) of allocated regions that are currently in use
  float utilization();

  // checks wether a GC collection should be started
  bool should_begin_collection() {
    return utilization() > kHeapGCTrigger;
  }

  // wether the gc should alloate more regions
  bool should_grow() {
    return utilization() > kHeapGCGrowTrigger;
  }

private:
  GCConcurrentWorker m_concurrent_worker;

  std::mutex m_mutex;
  std::condition_variable m_cv;

  std::list<HeapRegion*> m_freelist;
  std::list<HeapRegion*> m_regions;

  charly::atomic<size_t> m_free_regions = 0;
  charly::atomic<size_t> m_allocated_regions = 0;
};

}  // namespace charly::core::runtime
