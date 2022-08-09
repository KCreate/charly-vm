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
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"

using namespace std::chrono_literals;

namespace charly::core::runtime {

GarbageCollector::GarbageCollector(Runtime* runtime) :
  m_runtime(runtime),
  m_heap(runtime->heap()),
  m_collection_mode(CollectionMode::None),
  m_gc_cycle(1),
  m_wants_collection(false),
  m_wants_exit(false),
  m_thread(std::thread(&GarbageCollector::main, this)) {}

GarbageCollector::~GarbageCollector() {
  DCHECK(!m_thread.joinable());
}

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
    debugln("mapped regions = %", m_heap->m_mapped_regions.size());
    debugln("free regions = %", m_heap->m_free_regions.size());
    debugln("eden regions = %", m_heap->m_eden_regions.size());
    debugln("intermediate regions = %", m_heap->m_intermediate_regions.size());
    debugln("old regions = %", m_heap->m_old_regions.size());
    double start_time = (double)get_steady_timestamp_micro() / 1000;
    collect();
    double end_time = (double)get_steady_timestamp_micro() / 1000;
    debugln("GC finished in %ms", end_time - start_time);
    debugln("mapped regions = %", m_heap->m_mapped_regions.size());
    debugln("free regions = %", m_heap->m_free_regions.size());
    debugln("eden regions = %", m_heap->m_eden_regions.size());
    debugln("intermediate regions = %", m_heap->m_intermediate_regions.size());
    debugln("old regions = %", m_heap->m_old_regions.size());

    m_collection_mode = CollectionMode::None;
    DCHECK(m_mark_queue.empty());
    m_target_intermediate_regions.clear();
    m_target_old_regions.clear();
    m_gc_cycle++;
    m_wants_collection = false;
    m_cv.notify_all();

    m_runtime->scheduler()->start_the_world();
  }
}

void GarbageCollector::collect() {
  // validate heap
  if (kIsDebugBuild) {
    validate_heap_and_roots();
  }

  determine_collection_mode();
  mark_runtime_roots();
  if (m_collection_mode == CollectionMode::Minor) {
    mark_dirty_span_roots();
  }

  // mark and evacuate reachable objects
  while (!m_mark_queue.empty()) {
    auto object = m_mark_queue.front();
    m_mark_queue.pop();
    auto* header = object.header();

    // skip if object has already been marked
    if (header->is_reachable()) {
      continue;
    }
    header->set_is_reachable();

    if (header->is_young_generation()) {
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

    compact_object(object);
  }

  if (m_collection_mode == CollectionMode::Minor) {
    for (auto* region : m_heap->m_old_regions) {
      if (m_target_old_regions.count(region) == 0) {
        for (size_t si = kHeapRegionFirstUsableSpanIndex; si < kHeapRegionSpanCount; si++) {
          if (region->span_get_dirty_flag(si)) {
            bool contains_young_references = false;
            region->each_object_in_span(si, [&](ObjectHeader* header) {
              header->clear_is_reachable();
              if (update_object_references(header->object())) {
                contains_young_references = true;
              }
            });
            region->span_set_dirty_flag(si, contains_young_references);
          }
        }
      }
    }
  }

  for (auto* region : m_target_old_regions) {
    for (size_t si = kHeapRegionFirstUsableSpanIndex; si < kHeapRegionSpanCount; si++) {
      bool contains_young_references = false;
      region->each_object_in_span(si, [&](ObjectHeader* header) {
        if (update_object_references(header->object())) {
          contains_young_references = true;
        }
      });
      region->span_set_dirty_flag(si, contains_young_references);
    }
  }

  for (auto* region : m_target_intermediate_regions) {
    region->each_object([&](ObjectHeader* header) {
      update_object_references(header->object());
    });
  }

  update_root_references();

  if (m_collection_mode == CollectionMode::Major) {
    for (auto* region : m_heap->m_old_regions) {
      if (m_target_old_regions.count(region) == 0) {
        region->each_object([&](ObjectHeader* header) {
          if (!header->is_reachable()) {
            deallocate_external_heap_resources(header->object());
          }
        });
      }
    }
  }

  for (auto* region : m_heap->m_intermediate_regions) {
    if (m_target_intermediate_regions.count(region) == 0) {
      region->each_object([&](ObjectHeader* header) {
        if (!header->is_reachable()) {
          deallocate_external_heap_resources(header->object());
        }
      });
    }
  }

  for (auto* region : m_heap->m_eden_regions) {
    region->each_object([&](ObjectHeader* header) {
      if (!header->is_reachable()) {
        deallocate_external_heap_resources(header->object());
      }
    });
  }

  recycle_old_regions();

  for (auto* proc : m_runtime->scheduler()->m_processors) {
    proc->tab()->m_region = nullptr;
  }

  // grow or shrink heap according to heuristics
  m_heap->adjust_heap_size();

  // validate heap
  if (kIsDebugBuild) {
    validate_heap_and_roots();
  }
}

void GarbageCollector::determine_collection_mode() {
  bool is_major = m_gc_cycle % 4 == 0;
  if (is_major) {
    m_collection_mode = CollectionMode::Major;
  } else {
    m_collection_mode = CollectionMode::Minor;
  }
}

void GarbageCollector::mark_runtime_roots() {
  m_runtime->each_root([&](RawValue root) {
    mark_queue_value(root);
  });
}

void GarbageCollector::mark_dirty_span_roots() {
  if (m_collection_mode == CollectionMode::Minor) {
    for (auto* region : m_heap->m_old_regions) {
      for (size_t si = kHeapRegionFirstUsableSpanIndex; si < kHeapRegionSpanCount; si++) {
        if (region->span_get_dirty_flag(si)) {
          region->each_object_in_span(si, [&](ObjectHeader* header) {
            mark_queue_value(header->object(), true);
          });
        }
      }
    }
  }
}

void GarbageCollector::mark_queue_value(RawValue value, bool force_mark) {
  if (value.isObject()) {
    auto object = RawObject::cast(value);
    DCHECK(m_heap->is_valid_pointer(object.base_address()));

    auto* header = object.header();
    DCHECK((size_t)header->shape_id() < m_runtime->m_shapes.size());

    if (m_collection_mode == CollectionMode::Minor) {
      if (object.is_old_pointer() && !force_mark) {
        return;
      }
    }

    m_mark_queue.push(object);
  }
}

void GarbageCollector::compact_object(RawObject object) {
  auto* header = object.header();
  auto* region = header->heap_region();

  using Type = HeapRegion::Type;
  size_t alloc_size = header->alloc_size();

  // determine target region for compaction
  HeapRegion* target_region;
  switch (region->type) {
    case Type::Eden: {
      target_region = get_target_intermediate_region(alloc_size);
      break;
    }
    case Type::Intermediate: {
      target_region = get_target_old_region(alloc_size);
      break;
    }
    case Type::Old: {
      // do not compact old objects during minor GC
      if (m_collection_mode == CollectionMode::Minor) {
        return;
      }
      target_region = get_target_old_region(alloc_size);
      break;
    }
    default: FAIL("unexpected region type");
  }

  // copy object into target region
  auto target = target_region->allocate(alloc_size);
  DCHECK(target);
  memcpy(bitcast<void*>(target), bitcast<void*>(header), alloc_size);

  auto target_header = bitcast<ObjectHeader*>(target);
  target_header->clear_is_reachable();
  if (target_region->type == Type::Old) {
    target_header->clear_is_young_generation();
    target_header->clear_survivor_count();

    // set dirty flag on spans in non-target old regions
    if (m_collection_mode == CollectionMode::Minor) {
      auto target_span_index = target_region->span_get_index_for_pointer(target);
      target_region->span_set_dirty_flag(target_span_index);
    }
  }

  header->set_forward_target(target_header->object());
  DCHECK(header->forward_target() == target_header->object());
  DCHECK(header->forward_target().shape_id() > ShapeId::kLastImmediateShape);
  DCHECK(header->shape_id() == header->forward_target().shape_id());
}

HeapRegion* GarbageCollector::get_target_intermediate_region(size_t alloc_size) {
  for (auto* region : m_target_intermediate_regions) {
    if (region->fits(alloc_size)) {
      return region;
    }
  }

  auto* region = m_heap->acquire_region_internal(HeapRegion::Type::Intermediate);
  m_target_intermediate_regions.insert(region);
  return region;
}

HeapRegion* GarbageCollector::get_target_old_region(size_t alloc_size) {
  if (m_collection_mode == CollectionMode::Minor) {
    for (auto* region : m_heap->m_old_regions) {
      if (region->fits(alloc_size)) {
        return region;
      }
    }
  }

  for (auto* region : m_target_old_regions) {
    if (region->fits(alloc_size)) {
      return region;
    }
  }

  auto* region = m_heap->acquire_region_internal(HeapRegion::Type::Old);
  m_target_old_regions.insert(region);
  return region;
}

bool GarbageCollector::update_object_references(RawObject object) const {
  bool contains_young_references = false;

  if (object.isInstance() || object.isTuple()) {
    uint32_t field_count = object.count();
    for (size_t index = 0; index < field_count; index++) {
      RawValue& value = object.field_at(index);
      if (value.isObject()) {
        auto referenced_object = RawObject::cast(value);
        if (referenced_object.header()->has_forward_target()) {
          RawObject forwarded_object = referenced_object.header()->forward_target();
          bool forwarded_is_young = forwarded_object.header()->is_young_generation();
          if (forwarded_is_young) {
            contains_young_references = true;
          }
          value = forwarded_object;
        }
      }
    }
  }

  return contains_young_references;
}

void GarbageCollector::update_root_references() const {
  auto check_forwarded = [](RawValue value, std::function<void(RawObject)> callback) {
    if (value.isObject()) {
      auto object = RawObject::cast(value);
      auto* header = object.header();
      if (header->has_forward_target()) {
        RawObject target = header->forward_target();
        DCHECK(!target.header()->has_forward_target());
        DCHECK(header->shape_id() == target.header()->shape_id());
        callback(target);
      }
    }
  };

  m_runtime->each_root([&](RawValue& root) {
    check_forwarded(root, [&](RawObject object) {
      root = object;
    });
  });

  // patch frame argument pointers
  for (auto* thread : m_runtime->scheduler()->m_threads) {
    auto* frame = thread->frame();
    while (frame) {
      if (frame->argument_tuple.isTuple()) {
        DCHECK(frame->arguments);
        DCHECK(m_heap->is_valid_pointer(bitcast<uintptr_t>(frame->arguments)));
        auto argtup = RawTuple::cast(frame->argument_tuple);
        DCHECK(!argtup.header()->has_forward_target());
        DCHECK(argtup.size() >= frame->argc);
        frame->arguments = argtup.data();
      } else {
        if (frame->arguments != nullptr) {
          DCHECK(thread->stack()->pointer_points_into_stack(frame->arguments));
        }
      }

      frame = frame->parent;
    }
  }
}

void GarbageCollector::deallocate_external_heap_resources(RawObject object) const {
  if (object.isHugeBytes()) {
    auto huge_bytes = RawHugeBytes::cast(object);
    utils::Allocator::free(bitcast<void*>(const_cast<uint8_t*>(huge_bytes.data())));
    huge_bytes.set_data(nullptr);
  } else if (object.isHugeString()) {
    auto huge_string = RawHugeString::cast(object);
    utils::Allocator::free(bitcast<void*>(const_cast<char*>(huge_string.data())));
    huge_string.set_data(nullptr);
  } else if (object.isFuture()) {
    auto future = RawFuture::cast(object);
    // TODO: throw exceptions in threads that can no longer be awoken
    if (future.wait_queue()) {
      delete future.wait_queue();
      future.set_wait_queue(nullptr);
    }
  }
}

void GarbageCollector::recycle_old_regions() const {
  for (auto* region : m_heap->m_eden_regions) {
    region->reset();
    m_heap->m_free_regions.insert(region);
  }
  m_heap->m_eden_regions.clear();

  auto intermediate_it = m_heap->m_intermediate_regions.begin();
  auto intermediate_end = m_heap->m_intermediate_regions.end();
  while (intermediate_it != intermediate_end) {
    auto* region = *intermediate_it;
    if (m_target_intermediate_regions.count(region) == 0) {
      region->reset();
      m_heap->m_free_regions.insert(region);
      intermediate_it = m_heap->m_intermediate_regions.erase(intermediate_it);
      continue;
    }
    intermediate_it++;
  }

  if (m_collection_mode == CollectionMode::Major) {
    auto old_it = m_heap->m_old_regions.begin();
    auto old_end = m_heap->m_old_regions.end();
    while (old_it != old_end) {
      auto* region = *old_it;
      if (m_target_old_regions.count(region) == 0) {
        region->reset();
        m_heap->m_free_regions.insert(region);
        old_it = m_heap->m_old_regions.erase(old_it);
        continue;
      }
      old_it++;
    }
  }
}

void GarbageCollector::validate_heap_and_roots() const {
  auto validate_reference = [&](RawValue value) {
    if (value.isObject()) {
      auto object = RawObject::cast(value);
      auto* header = object.header();
      DCHECK(m_heap->is_valid_pointer(bitcast<uintptr_t>(header)), "invalid reference (%) points to region #%", header,
             header->heap_region()->id());
      DCHECK(!header->has_forward_target(), "expected reference to point to a non-forwarded object");
      DCHECK(object.is_young_pointer() == header->is_young_generation(), "mismatched pointer tag");
    }
  };

  // validate heap objects
  for (auto* region : m_heap->m_mapped_regions) {
    if (region->type != HeapRegion::Type::Unused) {
      region->each_object([&](ObjectHeader* header) {
        auto object = header->object();
        DCHECK(object.isObject());
        DCHECK((size_t)header->shape_id() < m_runtime->m_shapes.size(), "got %", (void*)header->shape_id());
        DCHECK(header->shape_id() > ShapeId::kLastImmediateShape, "got %", (uint32_t)header->shape_id());
        DCHECK(header->survivor_count() <= 2, "got %", (uint32_t)header->survivor_count());
        DCHECK(header->has_forward_target() == false);
        DCHECK(header->is_reachable() == false);

        if (object.isInstance()) {
          DCHECK(RawInstance::cast(object).klass_field() != kNull);
        }

        if (object.isInstance() || object.isTuple()) {
          uint32_t field_count = object.count();
          for (size_t index = 0; index < field_count; index++) {
            RawValue field = object.field_at(index);
            validate_reference(field);
            if (field.is_young_pointer() && region->type == HeapRegion::Type::Old) {
              DCHECK(region->span_get_dirty_flag(region->span_get_index_for_pointer(bitcast<uintptr_t>(header))));
            }
          }
        }
      });
    }
  }

  m_runtime->each_root([&](RawValue& root) {
    validate_reference(root);
  });
}

}  // namespace charly::core::runtime
