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
#include <thread>
#include <queue>
#include <set>

#include "charly/charly.h"
#include "charly/value.h"
#include "charly/core/runtime/heap.h"

#pragma once

namespace charly::core::runtime {

class Runtime;
class Thread;

static const size_t kGCObjectMaxSurvivorCount = 2;

class GarbageCollector {
  friend class Heap;
public:

  enum class CollectionMode {
    None,
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

  void collect();

  void determine_collection_mode();

  void mark_runtime_roots();
  void mark_dirty_span_roots();
  void mark_queue_value(RawValue value, bool force_mark = false);

  void compact_object(RawObject object);
  HeapRegion* get_target_intermediate_region(size_t alloc_size);
  HeapRegion* get_target_old_region(size_t alloc_size);

  // updates stale references to moved objects
  // returns true if any intermediate references remain
  bool update_object_references(RawObject object) const;

  // updates references stored in runtime roots
  void update_root_references() const;

  void deallocate_external_heap_resources(RawObject object) const;

  void recycle_old_regions() const;

  void validate_heap_and_roots() const;

private:
  Runtime* m_runtime;
  Heap* m_heap;

  CollectionMode m_collection_mode;
  std::queue<RawObject> m_mark_queue;
  std::set<HeapRegion*> m_target_intermediate_regions;
  std::set<HeapRegion*> m_target_old_regions;

  atomic<uint64_t> m_gc_cycle;
  atomic<bool> m_has_initialized;
  atomic<bool> m_wants_collection;
  atomic<bool> m_wants_exit;

  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::thread m_thread;
};

}  // namespace charly::core::runtime
