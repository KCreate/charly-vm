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
#include <unordered_set>
#include <vector>
#include <mutex>
#include <bitset>

#include "charly/value.h"

#pragma once

namespace charly::core::runtime {

class Runtime;

static const size_t kKb = 1024;
static const size_t kMb = kKb * 1024;
static const size_t kGb = kMb * 1024;

static const size_t kHeapTotalSize = kGb * 2;
static const size_t kHeapRegionSize = kKb * 512;
static const size_t kHeapRegionCount = kHeapTotalSize / kHeapRegionSize;
static const size_t kHeapRegionPageCount = kHeapRegionSize / kPageSize;
static const size_t kHeapRegionPossibleObjectAddresses = kHeapRegionSize / kObjectAlignment;

static const size_t kHeapInitialMappedRegionCount = 16;

class Heap;

struct HeapRegion {
  enum class Type : uint8_t {
    Unused,    // region isn't used and doesn't have a type yet
    Eden,      // new objects are allocated in eden regions
    Survivor,  // holds objects that have a survived a minor GC cycle
    Old        // holds objects that have survived many minor GC cycles
  };

  enum class State : uint8_t {
    Unused,    // region is on a freelist and can be acquired
    Owned,     // region is exclusively owned by a processor
    Released,  // region has been released from the processor
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

  Heap* heap;
  Type type;
  State state;
  size_t used;
  std::bitset<kHeapRegionPossibleObjectAddresses> card_table;
  uint8_t buffer alignas(kObjectAlignment)[];
};

static const size_t kHeapRegionUsableSize = kHeapRegionSize - sizeof(HeapRegion);
static const size_t kHeapObjectMaxSize = kHeapRegionUsableSize;

class ThreadAllocationBuffer;

class Heap {
public:
  Heap(Runtime* runtime);
  ~Heap();

  // allocates a free region from the heaps freelist and marks it
  // as a live eden region
  HeapRegion* allocate_eden_region();

  // release a live eden region
  void release_eden_region(HeapRegion* region);

  // checks wether address points into the heap
  bool contains_address(uintptr_t address);

private:

  // pops a free region from the freelist
  HeapRegion* pop_free_region();

  // maps a new region into memory
  HeapRegion* map_new_region();

private:
  Runtime* m_runtime;
  std::mutex m_mutex;

  // pointer to the base of the heap address space
  void* m_heap_base = nullptr;

  // region mappings
  std::list<HeapRegion*> m_unmapped_regions;         // regions which aren't mapped into memory
  std::unordered_set<HeapRegion*> m_mapped_regions;  // regions which are mapped into memory

  std::list<HeapRegion*> m_free_regions;                    // regions that are unused and can be acquired
  std::unordered_set<HeapRegion*> m_live_eden_regions;      // eden regions that are owned
  std::unordered_set<HeapRegion*> m_released_eden_regions;  // eden regions that are no longer owned
  std::unordered_set<HeapRegion*> m_survivor_regions;       // survivor regions
  std::unordered_set<HeapRegion*> m_old_regions;            // old regions
};

class ThreadAllocationBuffer {
public:
  ThreadAllocationBuffer(Heap* heap);
  ~ThreadAllocationBuffer();

  bool allocate(size_t size, uintptr_t* address_out);

private:
  void release_owned_region();
  void acquire_new_region();

private:
  Heap* m_heap;
  HeapRegion* m_region;
};

}  // namespace charly::core::runtime
