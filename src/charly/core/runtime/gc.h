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

#include <condition_variable>
#include <mutex>
#include <queue>
#include <set>
#include <thread>

#include "charly/charly.h"
#include "charly/core/runtime/heap.h"
#include "charly/value.h"

#pragma once

namespace charly::core::runtime {

class Runtime;
class Thread;

constexpr size_t kGCObjectMaxSurvivorCount = 15;

// if the minor GC cycle failed to free at least this ratio of free to mapped regions, trigger major GC
constexpr float kGCFreeToMappedRatioMajorTrigger = 0.25f;

// force a major GC cycle each nth cycle
constexpr size_t kGCForceMajorGCEachNthCycle = 16;

// if the time elapsed since the last collection is below this threshold, grow the heap
constexpr size_t kGCHeapGrowTimeMajorThreshold = 1000 * 5;
constexpr size_t kGCHeapGrowTimeMinorThreshold = 500;

// if the time elapsed since the last collection is above this threshold, shrink the heap
constexpr size_t kGCHeapShrinkTimeMajorThreshold = 1000 * 30;

// if a thread fails to retrieve a free region, repeatedly invoke the garbage collector
// if after this many attempts, there are still no free regions, crash with an out of memory error
constexpr size_t kGCCollectionAttempts = 4;

class GarbageCollector {
  friend class Heap;

public:
  enum class CollectionMode {
    Minor,
    Major
  };

  explicit GarbageCollector(Runtime* runtime);
  ~GarbageCollector();

  void shutdown();
  void join();
  void perform_gc(Thread* thread);

private:
  void main();

  void collect(CollectionMode mode);

  void mark_live_objects();
  void mark_runtime_roots();
  void mark_dirty_span_roots();
  void mark_queue_value(RawValue value, bool force_mark = false);

  void compact_object(RawObject object);
  HeapRegion* get_target_intermediate_region(size_t alloc_size);
  HeapRegion* get_target_old_region(size_t alloc_size);

  void update_old_references() const;

  // updates stale references to moved objects
  // returns true if any intermediate references remain
  bool update_object_references(RawObject object) const;

  // updates references stored in runtime roots
  void update_root_references() const;

  void deallocate_heap_ressources() const;

  void deallocate_object_heap_ressources(RawObject object) const;

  void recycle_collected_regions() const;

  void adjust_heap() const;

  void validate_heap_and_roots() const;

private:
  Runtime* m_runtime;
  Heap* m_heap;

  CollectionMode m_collection_mode = CollectionMode::Minor;
  std::queue<RawObject> m_mark_queue;
  std::set<HeapRegion*> m_target_intermediate_regions;
  std::set<HeapRegion*> m_target_old_regions;

  size_t m_last_collection_time;

  atomic<size_t> m_gc_cycle = 1;
  atomic<bool> m_has_initialized = false;
  atomic<bool> m_wants_collection = false;

  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::thread m_thread;
};

}  // namespace charly::core::runtime
