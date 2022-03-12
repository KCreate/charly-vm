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

Runtime::Runtime() :
  m_start_timestamp(get_steady_timestamp()),
  m_init_flag(m_mutex),
  m_exit_flag(m_mutex),
  m_exit_code(0),
  m_wants_exit(false),
  m_heap(new Heap(this)),
  m_gc(new GarbageCollector(this)),
  m_scheduler(new Scheduler(this)) {
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

  debuglnf("runtime exited after % milliseconds", get_steady_timestamp() - m_start_timestamp);

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

void Runtime::register_module(Thread* thread, const ref<CompiledModule>& module) {
  std::lock_guard<std::mutex> locker(m_mutex);
  m_compiled_modules.push_back(module);

  // register symbols in the functions string tables
  for (const SharedFunctionInfo* func : module->function_table) {
    for (const StringTableEntry& entry : func->string_table) {
      declare_symbol(thread, entry.value);
    }
  }
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

RawObject Runtime::create_instance(Thread* thread, ShapeId shape_id, size_t field_count) {
  // determine the allocation size
  DCHECK(field_count <= RawInstance::kMaximumFieldCount);
  size_t object_size = field_count * kPointerSize;
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
  ObjectHeader::initialize_header(memory, shape_id, field_count);

  // initialize fields to null
  uintptr_t object = memory + header_size;
  RawValue* object_fields = bitcast<RawValue*>(object);
  for (uint32_t i = 0; i < field_count; i++) {
    object_fields[i] = kNull;
  }

  return RawObject::make_from_ptr(object);
}

RawValue Runtime::create_string(Thread* thread, const char* data, size_t size, SYMBOL hash) {
  if (size <= RawSmallString::kMaxLength) {
    return RawSmallString::make_from_memory(data, size);
  } else if (size <= RawLargeString::kMaxLength) {
    return create_large_string(thread, data, size, hash);
  } else {
    return create_huge_string(thread, data, size, hash);
  }
}

RawValue Runtime::acquire_string(Thread* thread, char* data, size_t size, SYMBOL hash) {
  if (size <= RawLargeString::kMaxLength) {
    auto value = create_string(thread, data, size, hash);
    std::free(data);
    return value;
  } else {
    return create_huge_string_acquire(thread, data, size, hash);
  }
}

RawLargeString Runtime::create_large_string(Thread* thread, const char* data, size_t size, SYMBOL hash) {
  DCHECK(size <= RawData::kMaxLength);
  DCHECK(size > RawSmallString::kMaxLength);
  RawLargeString data_object = RawLargeString::cast(create_data(thread, ShapeId::kLargeString, size));
  std::memcpy((char*)data_object.address(), data, size);
  data_object.header()->cas_hashcode((SYMBOL)0, hash);
  return data_object;
}

RawHugeString Runtime::create_huge_string(Thread* thread, const char* data, size_t size, SYMBOL hash) {
  DCHECK(size > RawLargeString::kMaxLength);
  char* copy = static_cast<char*>(std::malloc(size));
  std::memcpy(copy, data, size);
  return create_huge_string_acquire(thread, copy, size, hash);
}

RawHugeString Runtime::create_huge_string_acquire(Thread* thread, char* data, size_t size, SYMBOL hash) {
  DCHECK(size > RawLargeString::kMaxLength);
  RawHugeString object = RawHugeString::cast(create_instance(thread, ShapeId::kHugeString, RawHugeString::kFieldCount));
  object.set_data(data);
  object.set_length(size);
  object.header()->cas_hashcode((SYMBOL)0, hash);
  return object;
}

RawObject Runtime::create_tuple(Thread* thread, uint32_t count) {
  return create_instance(thread, ShapeId::kTuple, count);
}

RawObject Runtime::create_function(Thread* thread, RawValue context, SharedFunctionInfo* shared_info) {
  RawFunction func = RawFunction::cast(create_instance(thread, ShapeId::kFunction, RawFunction::kFieldCount));
  func.set_context(context);
  func.set_shared_info(shared_info);
  return RawObject::cast(func);
}

RawObject Runtime::create_builtin_function(Thread* thread, BuiltinFunctionType function, RawSymbol name, uint8_t argc) {
  RawValue inst = create_instance(thread, ShapeId::kBuiltinFunction, RawBuiltinFunction::kFieldCount);
  RawBuiltinFunction func = RawBuiltinFunction::cast(inst);
  func.set_function(function);
  func.set_name(name);
  func.set_argc(argc);
  return RawObject::cast(func);
}

RawObject Runtime::create_fiber(Thread* thread, RawFunction function, RawValue context, RawValue arguments) {
  RawFiber fiber = RawFiber::cast(create_instance(thread, ShapeId::kFiber, RawFiber::kFieldCount));
  Thread* fiber_thread = scheduler()->get_free_thread();
  fiber_thread->init_fiber_thread(fiber);
  fiber.set_thread(fiber_thread);
  fiber.set_function(function);
  fiber.set_context(context);
  fiber.set_arguments(arguments);
  fiber.set_result(kNull);

  // schedule the fiber for execution
  fiber_thread->ready();
  scheduler()->schedule_thread(fiber_thread, thread->worker()->processor());

  return RawObject::cast(fiber);
}

RawValue Runtime::join_fiber(Thread* thread, RawFiber _fiber) {
  HandleScope scope(thread);
  Fiber fiber(scope, _fiber);

  {
    std::lock_guard lock(fiber);

    // fiber has already terminated
    if (fiber.has_finished()) {
      return fiber.result();
    }

    // register to be woken up again when this fiber finishes
    fiber.thread()->m_waiting_threads.push_back(thread);
    thread->m_state.acas(Thread::State::Running, Thread::State::Waiting);
  }

  thread->enter_scheduler();
  return fiber.result();
}

RawValue Runtime::declare_global_variable(Thread*, SYMBOL name, bool constant) {
  std::unique_lock<std::shared_mutex> locker(m_globals_mutex);

  if (m_global_variables.count(name) == 1) {
    return kErrorException;
  }

  m_global_variables[name] = {kNull, constant, false};
  return kErrorOk;
}

RawValue Runtime::read_global_variable(Thread*, SYMBOL name) {
  std::shared_lock<std::shared_mutex> locker(m_globals_mutex);

  if (m_global_variables.count(name) == 0) {
    return kErrorNotFound;
  }

  GlobalVariable& var = m_global_variables.at(name);
  return var.value;
}

RawValue Runtime::set_global_variable(Thread*, SYMBOL name, RawValue value) {
  std::unique_lock<std::shared_mutex> locker(m_globals_mutex);

  if (m_global_variables.count(name) == 0) {
    return kErrorNotFound;
  }

  auto& var = m_global_variables.at(name);
  if (var.constant) {
    if (var.initialized) {
      return kErrorReadOnly;
    }
  }

  var.value = value;
  var.initialized = true;
  return kErrorOk;
}

RawSymbol Runtime::declare_symbol(Thread* thread, const char* data, size_t size) {
  std::lock_guard<std::mutex> locker(m_symbols_mutex);

  SYMBOL symbol = crc32::hash_block(data, size);

  if (m_symbol_table.count(symbol) > 0) {
    return RawSymbol::make(symbol);
  }

  HandleScope scope(thread);
  String sym_string(scope, create_string(thread, data, size, symbol));

  m_symbol_table[symbol] = sym_string;

  return RawSymbol::make(symbol);
}

RawValue Runtime::lookup_symbol(SYMBOL symbol) {
  std::lock_guard<std::mutex> locker(m_symbols_mutex);

  if (m_symbol_table.count(symbol)) {
    return m_symbol_table.at(symbol);
  }

  return kNull;
}

}  // namespace charly::core::runtime
