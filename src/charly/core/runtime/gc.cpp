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

#include "charly/utils/timedsection.h"
#include "charly/utils/argumentparser.h"

#include "charly/core/runtime/gc.h"
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"

using namespace std::chrono_literals;

namespace charly::core::runtime {

GarbageCollector::GarbageCollector(Runtime* runtime) :
  m_runtime(runtime),
  m_heap(runtime->heap()),
  m_last_collection_time(get_steady_timestamp()),
  m_thread(std::thread(&GarbageCollector::main, this)) {}

GarbageCollector::~GarbageCollector() {
  DCHECK(!m_thread.joinable());
}

void GarbageCollector::shutdown() {
  m_cv.notify_all();
}

void GarbageCollector::join() {
  m_thread.join();
}

void GarbageCollector::perform_gc(Thread* thread) {
  size_t old_gc_cycle = m_gc_cycle;

  DCHECK(m_has_initialized);

  thread->native_section([&]() {
    // wake GC thread
    std::unique_lock locker(m_mutex);
    if (!m_wants_collection) {
      if (m_wants_collection.cas(false, true)) {
        m_cv.notify_all();
      }
    }

    // wait for GC thread to finish
    m_cv.wait(locker, [&]() {
      return (m_gc_cycle != old_gc_cycle) || m_runtime->wants_exit();
    });
  });

  thread->checkpoint();
}

void GarbageCollector::main() {
  m_runtime->wait_for_initialization();
  m_has_initialized = true;

  double collection_time_sum = 0;
  size_t collection_count = 0;
  while (!m_runtime->wants_exit()) {
    {
      std::unique_lock locker(m_mutex);
      m_cv.wait(locker, [&]() {
        return m_wants_collection || m_runtime->wants_exit();
      });
    }

    if (!m_wants_collection) {
      continue;
    }

    collection_time_sum += utils::TimedSection::run("GC collection", [&] {
      m_runtime->scheduler()->stop_the_world();

      collect(CollectionMode::Minor);

      // trigger major GC if minor GC failed to free enough free regions
      size_t free_count = m_heap->m_free_regions.size();
      size_t mapped_count = m_heap->m_mapped_regions.size();
      float free_to_mapped_ratio = (float)free_count / (float)mapped_count;
      bool below_minimum_ratio = free_to_mapped_ratio < kGCFreeToMappedRatioMajorTrigger;
      bool below_hw_concurrency = free_count < Scheduler::hardware_concurrency();
      bool force_major_cycle = m_gc_cycle % kGCForceMajorGCEachNthCycle == 0;
      if (below_minimum_ratio || below_hw_concurrency || force_major_cycle) {
        collect(CollectionMode::Major);
      }

      m_last_collection_time = get_steady_timestamp();
      m_wants_collection = false;
      m_cv.notify_all();
      m_runtime->scheduler()->start_the_world();
    });

    // deallocate memory blocks in the deallocation queue concurrently with worker threads
    for (void* data : m_deallocation_queue) {
      utils::Allocator::free(data);
    }
    m_deallocation_queue.clear();

    collection_count++;
  }

  if (collection_count > 0) {
    debuglnf("collection average time %ms", collection_time_sum / collection_count);
  }
}

void GarbageCollector::collect(CollectionMode mode) {
  m_collection_mode = mode;

  if (utils::ArgumentParser::is_flag_set("validate_heap")) {
    validate_heap_and_roots();
  }

  mark_runtime_roots();

  if (mode == CollectionMode::Minor) {
    mark_dirty_span_roots();
  }

  mark_live_objects();
  DCHECK(m_mark_queue.empty());

  update_old_references();
  deallocate_heap_ressources();
  recycle_collected_regions();

  adjust_heap();

  m_target_intermediate_regions.clear();
  m_target_old_regions.clear();
  m_gc_cycle++;

  if (utils::ArgumentParser::is_flag_set("validate_heap")) {
    validate_heap_and_roots();
  }
}

void GarbageCollector::mark_live_objects() {
  while (!m_mark_queue.empty()) {
    RawObject object = m_mark_queue.front();
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

    if (object.isInstance() || object.isTuple()) {
      auto field_count = object.count();
      for (size_t i = 0; i < field_count; i++) {
        mark_queue_value(object.field_at(i));
      }

      if (object.isList()) {
        auto list = RawList::cast(object);
        RawValue* data = list.data();
        size_t length = list.length();
        for (size_t i = 0; i < length; i++) {
          mark_queue_value(data[i]);
        }
      }
    }

    compact_object(object);
  }
}

void GarbageCollector::mark_runtime_roots() {
  m_runtime->each_root([&](RawValue root) {
    mark_queue_value(root);
  });
}

void GarbageCollector::mark_dirty_span_roots() {
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
  using Type = HeapRegion::Type;
  auto* header = object.header();
  auto* region = header->heap_region();

  // do not compact old objects during minor GC
  if (m_collection_mode == CollectionMode::Minor && region->type == Type::Old) {
    return;
  }

  size_t alloc_size = header->alloc_size();

  // determine target region for compaction
  HeapRegion* target_region;
  switch (region->type) {
    case Type::Eden: {
      target_region = get_target_intermediate_region(alloc_size);
      break;
    }
    case Type::Intermediate: {
      if (header->survivor_count() >= kGCObjectMaxSurvivorCount) {
        target_region = get_target_old_region(alloc_size);
      } else {
        target_region = get_target_intermediate_region(alloc_size);
      }

      break;
    }
    case Type::Old: {
      target_region = get_target_old_region(alloc_size);
      break;
    }
    default: FAIL("unexpected region type");
  }

  // copy object into target region
  auto target = target_region->allocate(alloc_size, object.contains_external_heap_pointers());
  DCHECK(target);
  memcpy(bitcast<void*>(target), bitcast<void*>(header), alloc_size);

  auto* target_header = ObjectHeader::header_at_address(target);
  target_header->clear_is_reachable();
  if (target_region->type == Type::Old) {
    target_header->clear_is_young_generation();
    target_header->clear_survivor_count();

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

void GarbageCollector::update_old_references() const {
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
}

bool GarbageCollector::update_object_references(RawObject object) const {
  bool contains_young_references = false;

  auto follow_and_update_ref = [&](RawValue& value) {
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
  };

  if (object.isInstance() || object.isTuple()) {
    uint32_t field_count = object.count();
    for (size_t index = 0; index < field_count; index++) {
      RawValue& value = object.field_at(index);
      follow_and_update_ref(value);
    }

    if (object.isList()) {
      auto list = RawList::cast(object);
      RawValue* data = list.data();
      size_t length = list.length();
      for (size_t i = 0; i < length; i++) {
        follow_and_update_ref(data[i]);
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
        DCHECK(argtup.length() >= frame->argc);
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

void GarbageCollector::deallocate_heap_ressources() {
  if (m_collection_mode == CollectionMode::Major) {
    for (auto* region : m_heap->m_old_regions) {
      if (m_target_old_regions.count(region) == 0) {
        for (uintptr_t pointer : region->objects_with_external_heap_pointers) {
          auto* header = ObjectHeader::header_at_address(pointer);
          if (!header->is_reachable()) {
            queue_object_memory_for_deallocation(header->object());
          }
        }
      }
    }
  }

  for (auto* region : m_heap->m_intermediate_regions) {
    if (m_target_intermediate_regions.count(region) == 0) {
      for (uintptr_t pointer : region->objects_with_external_heap_pointers) {
        auto* header = ObjectHeader::header_at_address(pointer);
        if (!header->is_reachable()) {
          queue_object_memory_for_deallocation(header->object());
        }
      }
    }
  }

  for (auto* region : m_heap->m_eden_regions) {
    for (uintptr_t pointer : region->objects_with_external_heap_pointers) {
      auto* header = ObjectHeader::header_at_address(pointer);
      if (!header->is_reachable()) {
        queue_object_memory_for_deallocation(header->object());
      }
    }
  }
}

void GarbageCollector::queue_object_memory_for_deallocation(RawObject object) {
  if (object.isHugeBytes()) {
    auto huge_bytes = RawHugeBytes::cast(object);
    auto* data = bitcast<void*>(const_cast<uint8_t*>(huge_bytes.data()));
    DCHECK(data);
    m_deallocation_queue.push_back(data);
    huge_bytes.set_data(nullptr);
  } else if (object.isHugeString()) {
    auto huge_string = RawHugeString::cast(object);
    auto* data = bitcast<void*>(const_cast<char*>(huge_string.data()));
    DCHECK(data);
    m_deallocation_queue.push_back(data);
    huge_string.set_data(nullptr);
  } else if (object.isFuture()) {
    auto future = RawFuture::cast(object);
    if (auto* wait_queue = future.wait_queue()) {
      DCHECK(wait_queue->used == 0);
      m_deallocation_queue.push_back(wait_queue);
      future.set_wait_queue(nullptr);
    }
  } else if (object.isList()) {
    auto list = RawList::cast(object);
    if (auto* data = list.data()) {
      m_deallocation_queue.push_back(data);
      list.set_data(nullptr);
    }
  }
}

void GarbageCollector::recycle_collected_regions() const {
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

  for (auto* proc : m_runtime->scheduler()->m_processors) {
    proc->tab()->m_region = nullptr;
  }
}

void GarbageCollector::adjust_heap() const {
  size_t current_time = get_steady_timestamp();
  size_t time_passed_since_last_collection = current_time - m_last_collection_time;

  if (m_collection_mode == CollectionMode::Major) {
    if (time_passed_since_last_collection > kGCHeapShrinkTimeMajorThreshold) {
      m_heap->shrink_heap();
    } else if (time_passed_since_last_collection < kGCHeapGrowTimeMajorThreshold) {
      m_heap->grow_heap();
    }
  } else {
    if (time_passed_since_last_collection < kGCHeapGrowTimeMinorThreshold) {
      m_heap->grow_heap();
    }
  }
}

void GarbageCollector::validate_heap_and_roots() const {
  auto validate_reference = [&](RawValue value) {
    if (value.isObject()) {
      auto object = RawObject::cast(value);
      auto* header = object.header();
      DCHECK(m_heap->is_valid_pointer(bitcast<uintptr_t>(header)));
      DCHECK(object.is_young_pointer() == header->is_young_generation());
      DCHECK((size_t)header->shape_id() < m_runtime->m_shapes.size());
      DCHECK(header->shape_id() > ShapeId::kLastImmediateShape);
      DCHECK(header->survivor_count() <= kGCObjectMaxSurvivorCount);
      DCHECK(!header->has_forward_target());
      DCHECK(!header->is_reachable());

      if (object.isInstance()) {
        auto instance = RawInstance::cast(object);
        auto klass = RawClass::cast(instance.klass_field());
        auto class_shape_id = klass.shape_instance().own_shape_id();
        auto header_id = header->shape_id();

        switch (header_id) {
          // RawHugeString and RawHugeBytes instances point to the 'String', 'Bytes' class instances
          // Those instances are general classes and not specific to any one string / byte subtype
          // and will not contain the correct shape id that would be expected
          case ShapeId::kHugeString:
          case ShapeId::kHugeBytes: {
            break;
          }
          default: {
            DCHECK(class_shape_id == header_id);
            break;
          }
        }
      }

      if (object.isInstance() || object.isTuple()) {
        HeapRegion* region = header->heap_region();
        uint32_t field_count = object.count();
        for (size_t index = 0; index < field_count; index++) {
          RawValue field = object.field_at(index);
          if (field.is_young_pointer() && region->type == HeapRegion::Type::Old) {
            DCHECK(region->span_get_dirty_flag(region->span_get_index_for_pointer(bitcast<uintptr_t>(header))));
          }
        }

        if (object.isList()) {
          auto list = RawList::cast(object);
          RawValue* data = list.data();
          size_t length = list.length();

          for (size_t i = 0; i < length; i++) {
            RawValue field = data[i];
            if (field.is_young_pointer() && region->type == HeapRegion::Type::Old) {
              DCHECK(region->span_get_dirty_flag(region->span_get_index_for_pointer(bitcast<uintptr_t>(header))));
            }
          }
        }
      }
    }
  };

  // validate heap objects
  for (auto* region : m_heap->m_mapped_regions) {
    if (region->type != HeapRegion::Type::Unused) {
      region->each_object([&](ObjectHeader* header) {
        auto object = header->object();
        validate_reference(object);
      });
    }
  }

  m_runtime->each_root([&](RawValue& root) {
    validate_reference(root);
  });
}

}  // namespace charly::core::runtime
