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

void* MemoryAllocator::allocate(HeapType type, size_t size) {
  assert(sizeof(HeapHeader) + size <= kHeapRegionSize && "object too big");

  if (Scheduler::instance->worker()) {
    return allocate_worker(type, size);
  } else {
    return allocate_global(type, size);
  }
}

void* MemoryAllocator::allocate_worker(HeapType type, size_t size) {
  Processor* proc = Scheduler::instance->processor();
  HeapRegion* region = proc->active_region;

  if (region == nullptr || !region->fits(sizeof(HeapHeader) + size)) {
    if (region) {
      region->release();
    }

    if (!(region = acquire_region())) {
      return nullptr;
    }
    region->acquire();
    proc->active_region = region;
  }

  // prepend heap header to user object
  HeapHeader* header = (HeapHeader*)region->allocate(sizeof(HeapHeader));
  void* user_obj = region->allocate(size);

  // initialize header
#ifndef NDEBUG
  header->magic_number = kHeapHeaderMagicNumber;
#endif
  header->forward_ptr.store(user_obj);
  header->type.store(type);
  header->gcmark.store(MarkColor::Black); // newly allocated values are colored black

  return user_obj;
}

void* MemoryAllocator::allocate_global(HeapType type, size_t size) {
  std::unique_lock<std::mutex> locker(m_global_region_mutex);
  HeapRegion* region = m_global_region;

  if (region == nullptr || !region->fits(sizeof(HeapHeader) + size)) {
    if (region) {
      region->release();
    }

    if (!(region = acquire_region())) {
      return nullptr;
    }
    region->acquire();
    m_global_region = region;
  }

  // prepend heap header to user object
  HeapHeader* header = (HeapHeader*)region->allocate(sizeof(HeapHeader));
  void* user_obj = region->allocate(size);

  // initialize header
#ifndef NDEBUG
  header->magic_number = kHeapHeaderMagicNumber;
#endif
  header->forward_ptr.store(user_obj);
  header->type.store(type);
  header->gcmark.store(MarkColor::Black); // newly allocated values are colored black

  return user_obj;
}

HeapHeader* MemoryAllocator::object_header(void* object) {
  HeapHeader* header = (HeapHeader*)((uintptr_t)object - (uintptr_t)(sizeof(HeapHeader)));
#ifndef NDEBUG
  assert(header->magic_number == kHeapHeaderMagicNumber);
#endif
  return header;
}

HeapRegion* MemoryAllocator::acquire_region() {
  {
    std::lock_guard<std::mutex> locker(m_freelist_mutex);

    // TODO: signal GC worker if freelist gets too low

    // check freelist
    if (m_freelist.size()) {
      HeapRegion* region = m_freelist.front();
      m_freelist.pop_front();
      m_free_regions--;
      return region;
    }
  }

  // attempt to allocate new region
  HeapRegion* region = allocate_new_region();
  if (region) {
    return region;
  }

  // TODO: pause the current processor until the next GC cycle has completed
  //
  // for (uint32_t i = 0; i < kHeapAllocationAttempts; i++) {
  //
  // }

  return nullptr;
}

HeapRegion* MemoryAllocator::allocate_new_region() {
  std::lock_guard<std::mutex> locker(m_regions_mutex);

  // check if the maximum allowed heap size was reached
  if (m_regions.size() == kHeapRegionLimit) {
    return nullptr;
  }

  HeapRegion* region = new HeapRegion();
  m_regions.push_back(region);
  m_allocated_regions++;
  return region;
}

void MemoryAllocator::free_region(HeapRegion* region) {
  assert(region->state == HeapRegion::State::Available);
  {
    std::lock_guard<std::mutex> locker(m_freelist_mutex);
    m_freelist.push_back(region);
    m_free_regions++;
  }
  m_freelist_cv.notify_one();
}

}  // namespace charly::core::runtime
