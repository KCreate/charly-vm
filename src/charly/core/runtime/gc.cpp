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

#include "charly/core/runtime/gc.h"
#include "charly/core/runtime/compiled_module.h"
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"

using namespace std::chrono_literals;

namespace charly::core::runtime {

void GarbageCollector::shutdown() {
  {
    std::unique_lock<std::mutex> locker(m_mutex);
    m_wants_exit = true;
  }
  m_cv.notify_all();
}

void GarbageCollector::join() {
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void GarbageCollector::perform_gc(Thread* thread) {
  uint64_t old_gc_cycle = m_gc_cycle;

  thread->native_section([&]() {
    // wake GC thread
    std::unique_lock<std::mutex> locker(m_mutex);
    if (!m_wants_collection) {
      if (m_wants_collection.cas(false, true)) {
        m_cv.notify_all();
      }
    }

    // wait for GC thread to finish
    m_cv.wait(locker, [&]() {
      return (m_gc_cycle != old_gc_cycle) || m_wants_exit;
    });
  });

  thread->checkpoint();
}

void GarbageCollector::main() {
  m_runtime->wait_for_initialization();

  while (!m_wants_exit) {
    {
      std::unique_lock<std::mutex> locker(m_mutex);
      m_cv.wait(locker, [&]() {
        return m_wants_collection || m_wants_exit;
      });
    }

    if (!m_wants_collection) {
      continue;
    }

    m_runtime->scheduler()->stop_the_world();
    debugln("GC running");

    determine_collection_mode();
    collect();

    debugln("GC finished");
    m_gc_cycle++;
    m_wants_collection = false;
    m_collection_mode = CollectionMode::None;
    m_cv.notify_all();

    m_runtime->scheduler()->start_the_world();
  }
}

void GarbageCollector::collect() {
  mark_roots();
  mark();
}

void GarbageCollector::determine_collection_mode() {
  // TODO: determine when to run minor and major
  m_collection_mode = CollectionMode::Major;
}

void GarbageCollector::mark() {
  while (!m_mark_queue.empty()) {
    auto object = m_mark_queue.front();
    m_mark_queue.pop();

    auto* header = object.header();
    if (header->is_reachable()) {
      continue;
    }
    header->set_is_reachable();

    if (object.is_eden_object()) {
      header->increment_survivor_count();
    }

    if (object.isInstance()) {
      auto instance = RawInstance::cast(object);
      auto field_count = instance.field_count();
      for (size_t index = 0; index < field_count; index++) {
        mark_queue_value(instance.field_at(index));
      }
    } else if (object.isTuple()) {
      auto tuple = RawTuple::cast(object);
      auto size = tuple.size();
      for (size_t index = 0; index < size; index++) {
        mark_queue_value(tuple.field_at(index));
      }
    }
  }
}

void GarbageCollector::mark_roots() {
  auto* scheduler = m_runtime->scheduler();

  // processor symbol tables
  for (auto* proc : scheduler->m_processors) {
    for (const auto& entry : proc->m_symbol_table) {
      mark_queue_value(entry.second);
    }
  }

  // thread handle scopes and stackframes
  for (auto* thread : scheduler->m_threads) {
    mark_queue_value(thread->fiber());
    mark_queue_value(thread->pending_exception());

    auto* handle = thread->handles()->head();
    while (handle) {
      mark_queue_value(*handle);
      handle = handle->next();
    }

    auto* frame = thread->frame();
    while (frame) {
      mark_queue_value(frame->function);
      mark_queue_value(frame->context);

      const auto& shared_info = frame->shared_function_info;
      const auto& ir_info = shared_info->ir_info;

      uint8_t locals = ir_info.local_variables;
      for (uint8_t li = 0; li < locals; li++) {
        mark_queue_value(frame->locals[li]);
      }

      uint8_t stacksize = ir_info.stacksize;
      for (uint8_t si = 0; si < stacksize; si++) {
        mark_queue_value(frame->stack[si]);
      }

      mark_queue_value(frame->return_value);

      frame = frame->parent;
    }
  }

  // runtime symbol table
  for (const auto& entry : m_runtime->m_symbol_table) {
    mark_queue_value(entry.second);
  }

  // runtime shape table
  for (auto shape : m_runtime->m_shapes) {
    mark_queue_value(shape);
  }

  // runtime class table
  for (auto klass : m_runtime->m_builtin_classes) {
    mark_queue_value(klass);
  }

  // global variables
  for (const auto& entry : m_runtime->m_global_variables) {
    mark_queue_value(entry.second.value);
  }

  // cached modules table
  for (const auto& entry : m_runtime->m_cached_modules) {
    mark_queue_value(entry.second.module);
  }

  // dirty spans of old heap regions
  if (m_collection_mode == CollectionMode::Minor) {
    auto* heap = m_runtime->heap();
    for (auto* region : heap->m_old_regions) {
      auto region_start = bitcast<uintptr_t>(region);
      auto used_end_pointer = region->buffer_base() + region->used;

      for (size_t si = 0; si < kHeapRegionSpanCount; si++) {
        // do not scan parts of the region that haven't been allocated to yet
        auto span_pointer = region_start + si * kHeapRegionSpanSize;
        auto span_end_pointer = span_pointer + kHeapRegionSpanSize;
        if (span_pointer >= used_end_pointer) {
          break;
        }

        // scan objects in the span if it was marked as dirty via a write barrier
        if (region->span_get_dirty_flag(si)) {
          // the beginning of the span might be in the middle of an
          // object from the previous span, so we consult the span table
          // for the address of the last object in that span and start
          // scanning from there
          uintptr_t scan;
          if (si == 0) {
            scan = region->buffer_base();
          } else {
            scan = region->span_get_last_object_pointer(si - 1);
          }

          while (scan < span_end_pointer && scan < used_end_pointer) {
            auto* header = bitcast<ObjectHeader*>(scan);
            mark_queue_value(header->object(), true);
            scan += sizeof(ObjectHeader) + header->object_size();
          }
        }
      }
    }
  }
}

void GarbageCollector::mark_queue_value(RawValue value, bool force_mark) {
  if (value.isObject()) {
    auto object = RawObject::cast(value);
    DCHECK(m_runtime->heap()->is_heap_pointer(object.address()));

    if (m_collection_mode == CollectionMode::Minor) {
      if (object.is_old_object() && !force_mark) {
        return;
      }
    }

    m_mark_queue.push(object);
  }
}

}  // namespace charly::core::runtime
