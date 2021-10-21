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

#include "charly/core/runtime/runtime.h"

namespace charly::core::runtime {

Runtime::Runtime() : m_init_flag(m_mutex), m_exit_flag(m_mutex), m_heap(nullptr), m_scheduler(nullptr), m_gc(nullptr) {
  m_start_timestamp = get_steady_timestamp();
  m_exit_code = 0;
  m_wants_exit = false;

  m_heap = new Heap(this);
  m_gc = new GarbageCollector(this);
  m_scheduler = new Scheduler(this);

  m_init_flag.signal();
}

Heap* Runtime::heap() {
  return m_heap;
}

Scheduler* Runtime::scheduler() {
  return m_scheduler;
}

GarbageCollector* Runtime::gc() {
  return m_gc;
}

bool Runtime::wants_exit() const {
  return m_wants_exit;
}

int32_t Runtime::join() {
  m_exit_flag.wait();

  m_gc->shutdown();
  m_scheduler->join();
  m_gc->join();

  return m_exit_code;
}

void Runtime::abort(int32_t status_code) {
  if (m_wants_exit.cas(false, true)) {
    {
      std::lock_guard<std::mutex> locker(m_mutex);
      m_exit_code = status_code;
    }
    m_exit_flag.signal();
  }
}

void Runtime::wait_for_initialization() {
  m_init_flag.wait();
}

void Runtime::register_module(const ref<CompiledModule>& module) {
  std::lock_guard<std::mutex> locker(m_mutex);
  m_compiled_modules.push_back(module);
}

RawData Runtime::create_data(Thread* thread, ShapeId shape_id, size_t size) {

  // determine the total allocation size
  DCHECK(size <= RawData::kMaxLength);
  size_t header_size = sizeof(ObjectHeader);
  size_t total_size = align_to_size(header_size + size, kObjectAlignment);

  Worker* worker = thread->worker();
  Processor* proc = worker->processor();
  ThreadAllocationBuffer* tab = proc->tab();

  // attempt to allocate memory for the object
  uintptr_t memory = 0;
  if (!tab->allocate(total_size, &memory)) {
    FAIL("allocation failed");
  }
  DCHECK(memory);

  // initialize header
  ObjectHeader::initialize_header(memory, shape_id, size);
  return RawData::cast(RawObject::make_from_ptr(memory + header_size));
}

RawObject Runtime::create_instance(Thread* thread, ShapeId shape_id, size_t object_size) {

  // determine the allocation size
  DCHECK(object_size % kPointerSize == 0);
  size_t num_fields = object_size / kPointerSize;
  DCHECK(num_fields <= RawInstance::kMaximumFieldCount);
  size_t header_size = sizeof(ObjectHeader);
  size_t total_size = align_to_size(header_size + object_size, kObjectAlignment);

  Worker* worker = thread->worker();
  Processor* proc = worker->processor();
  ThreadAllocationBuffer* tab = proc->tab();

  // attempt to allocate memory for the object
  uintptr_t memory = 0;
  if (!tab->allocate(total_size, &memory)) {
    FAIL("allocation failed");
  }
  DCHECK(memory);

  // initialize header
  ObjectHeader::initialize_header(memory, shape_id, num_fields);

  // initialize fields to null
  uintptr_t object = memory + header_size;
  RawValue* object_fields = bitcast<RawValue*>(object);
  for (uint32_t i = 0; i < num_fields; i++) {
    object_fields[i] = kNull;
  }

  return RawObject::make_from_ptr(object);
}

RawValue Runtime::create_string(Thread* thread, const char* data, size_t size, SYMBOL hash) {
  if (size <= RawSmallString::kMaxLength) {
    return RawSmallString::make_from_memory(data, size);
  } else if (size <= RawLargeString::kMaxLength) {
    return create_heap_string(thread, data, size, hash);
  } else {
    FAIL("string is too big and RawHugeString is not implemented yet");
  }
}

RawObject Runtime::create_heap_string(Thread* thread, const char* data, size_t size, SYMBOL hash) {
  DCHECK(size <= RawData::kMaxLength);
  RawData data_object = RawData::cast(create_data(thread, ShapeId::kLargeString, size));
  std::memcpy((char*)data_object.address(), data, size);
  data_object.header()->cas_hashcode((SYMBOL)0, hash);
  return RawObject::cast(data_object);
}

RawObject Runtime::create_tuple(Thread* thread, uint32_t count) {
  return create_instance(thread, ShapeId::kTuple, count * kPointerSize);
}

RawObject Runtime::create_function(Thread* thread, RawValue context, SharedFunctionInfo* shared_info) {
  RawFunction func = RawFunction::cast(create_instance(thread, ShapeId::kFunction, RawFunction::kSize));
  func.set_context(context);
  func.set_shared_info(shared_info);
  return RawObject::cast(func);
}

RawObject Runtime::create_fiber(Thread* thread, RawFunction function) {
  RawFiber fiber = RawFiber::cast(create_instance(thread, ShapeId::kFiber, RawFiber::kSize));
  fiber.set_thread(nullptr);
  fiber.set_function(function);
  return RawObject::cast(fiber);
}

}  // namespace charly::core::runtime
