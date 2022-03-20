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
#include "charly/core/runtime/builtins/core.h"
#include "charly/core/runtime/builtins/readline.h"
#include "charly/utils/argumentparser.h"

namespace charly::core::runtime {

Runtime::Runtime() :
  m_start_timestamp(get_steady_timestamp()),
  m_init_flag(m_mutex),
  m_exit_flag(m_mutex),
  m_exit_code(0),
  m_wants_exit(false),
  m_heap(std::make_unique<Heap>(this)),
  m_gc(std::make_unique<GarbageCollector>(this)),
  m_scheduler(std::make_unique<Scheduler>(this)) {
  m_init_flag.signal();
}

Heap* Runtime::heap() {
  return m_heap.get();
}

Scheduler* Runtime::scheduler() {
  return m_scheduler.get();
}

GarbageCollector* Runtime::gc() {
  return m_gc.get();
}

bool Runtime::wants_exit() const {
  return m_wants_exit;
}

int32_t Runtime::join() {
  m_exit_flag.wait();

  m_gc->shutdown();
  m_scheduler->join();
  m_gc->join();

  debugln("runtime exited after % milliseconds", get_steady_timestamp() - m_start_timestamp);

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

void Runtime::initialize_symbol_table(Thread* thread) {
  // builtin types
#define DECL_SYM(N) declare_symbol(thread, #N);
  TYPE_NAMES(DECL_SYM)
#undef DECL_SYM

  // known global variables
  declare_symbol(thread, "");
  declare_symbol(thread, "charly.baseclass");
  declare_symbol(thread, "charly.mainfiber");
  declare_symbol(thread, "klass");
  declare_symbol(thread, "length");
  declare_symbol(thread, "ARGV");
}

void Runtime::initialize_argv_tuple(Thread* thread) {
  auto& argv = utils::ArgumentParser::USER_FLAGS;
  RawTuple argv_tuple = RawTuple::cast(create_tuple(thread, argv.size()));
  for (uint32_t i = 0; i < argv.size(); i++) {
    const std::string& arg = argv[i];
    RawString arg_string = RawString::cast(create_string(thread, arg.data(), arg.size(), crc32::hash_string(arg)));
    argv_tuple.set_field_at(i, arg_string);
  }
  CHECK(declare_global_variable(thread, SYM("ARGV"), true).is_error_ok());
  CHECK(set_global_variable(thread, SYM("ARGV"), argv_tuple).is_error_ok());
}

void Runtime::initialize_builtin_functions(Thread* thread) {
  builtin::core::initialize(thread);
  builtin::readline::initialize(thread);
}

void Runtime::initialize_builtin_types(Thread* thread) {
  // immediate type shapes placeholders
  RawShape shape_placeholder = create_shape(thread, kNull, create_tuple(thread, 0));
  for (uint32_t i = 0; i <= static_cast<uint32_t>(ShapeId::kLastNonInstance); i++) {
    register_shape(thread, shape_placeholder);
  }

  // builtin instances shapes placeholders
  uint32_t first_builtin_shape = static_cast<uint32_t>(ShapeId::kFirstBuiltinShapeId);
  uint32_t last_builtin_shape = static_cast<uint32_t>(ShapeId::kLastBuiltinShapeId);
  uint32_t builtin_shape_count = last_builtin_shape + 1 - first_builtin_shape;
  for (uint32_t i = 0; i < builtin_shape_count; i++) {
    register_shape(thread, shape_placeholder);
  }

  // initialize base class
  RawShape shape_object = create_shape(thread, kNull, create_tuple(thread));
  RawClass base_class = RawClass::cast(create_instance(thread, ShapeId::kClass, RawClass::kFieldCount));
  base_class.set_name(RawSymbol::make("charly.baseclass"));
  base_class.set_parent(kNull);
  base_class.set_shape_instance(shape_object);
  base_class.set_function_table(RawTuple::cast(create_tuple(thread, 0)));
  base_class.set_constructor(kNull);
  CHECK(declare_global_variable(thread, SYM("charly.baseclass"), true).is_error_ok());
  CHECK(set_global_variable(thread, SYM("charly.baseclass"), base_class).is_error_ok());
}

void Runtime::initialize_main_fiber(Thread* thread, SharedFunctionInfo* info) {
  RawFunction function = RawFunction::cast(create_function(thread, kNull, info));
  RawFiber mainfiber = RawFiber::cast(create_fiber(thread, function, kNull, kNull));
  CHECK(declare_global_variable(thread, SYM("charly.mainfiber"), true).is_error_ok());
  CHECK(set_global_variable(thread, SYM("charly.mainfiber"), mainfiber).is_error_ok());
}

void Runtime::initialize_global_variables(Thread* thread) {
#define REG_SHAPE_ID(N, T)                                              \
  auto id_##N = RawInt::make(static_cast<uint32_t>(ShapeId::T));        \
  auto name_##N = declare_symbol(thread, "charly.shapeid." #N);         \
  CHECK(declare_global_variable(thread, name_##N, true).is_error_ok()); \
  CHECK(set_global_variable(thread, name_##N, id_##N).is_error_ok());

  REG_SHAPE_ID(int, kInt)
  REG_SHAPE_ID(float, kFloat)
  REG_SHAPE_ID(bool, kBool)
  REG_SHAPE_ID(symbol, kSymbol)
  REG_SHAPE_ID(null, kNull)
  REG_SHAPE_ID(small_string, kSmallString)
  REG_SHAPE_ID(small_bytes, kSmallBytes)

  REG_SHAPE_ID(large_bytes, kLargeBytes)
  REG_SHAPE_ID(large_string, kLargeString)

  REG_SHAPE_ID(user_instance, kUserInstance)
  REG_SHAPE_ID(huge_bytes, kHugeBytes)
  REG_SHAPE_ID(huge_string, kHugeString)
  REG_SHAPE_ID(tuple, kTuple)
  REG_SHAPE_ID(class, kClass)
  REG_SHAPE_ID(shape, kShape)
  REG_SHAPE_ID(function, kFunction)
  REG_SHAPE_ID(builtin_function, kBuiltinFunction)
  REG_SHAPE_ID(fiber, kFiber)

  REG_SHAPE_ID(exception, kException)

#undef REG_SHAPE_ID
}

ShapeId Runtime::register_shape(Thread*, RawShape shape) {
  std::unique_lock lock(m_shapes_mutex);
  CHECK(m_shapes.size() < static_cast<size_t>(ShapeId::kMaxShapeCount), "exceeded max shapes count");

  uint32_t shape_count = m_shapes.size();
  ShapeId next_shape_id = static_cast<ShapeId>(shape_count);
  m_shapes.push_back(shape);
  shape.set_own_shape_id(next_shape_id);

  return next_shape_id;
}

RawShape Runtime::lookup_shape_id(Thread*, ShapeId id) {
  std::shared_lock lock(m_shapes_mutex);

  // TODO: cache shapes in processor

  auto index = static_cast<size_t>(id);
  CHECK(index < m_shapes.size());

  return m_shapes.at(index);
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

RawInstance Runtime::create_instance(Thread* thread, ShapeId shape_id, size_t field_count) {
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
  auto* object_fields = bitcast<RawValue*>(object);
  for (uint32_t i = 0; i < field_count; i++) {
    object_fields[i] = kNull;
  }

  return RawInstance::cast(RawObject::make_from_ptr(object));
}

RawUserInstance Runtime::create_user_instance(Thread* thread, RawClass klass) {
  RawShape shape = klass.shape_instance();
  ShapeId shape_id = shape.own_shape_id();
  uint32_t field_count = shape.key_table().field_count();
  auto instance = RawUserInstance::cast(create_instance(thread, shape_id, field_count));
  instance.set_klass(klass);
  return instance;
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

RawValue Runtime::create_class(Thread* thread,
                               SYMBOL name,
                               RawValue parent,
                               RawFunction constructor,
                               uint8_t member_func_count,
                               RawFunction* member_funcs,
                               uint8_t member_prop_count,
                               RawSymbol* member_props,
                               uint8_t static_prop_count,
                               RawSymbol* static_prop_names,
                               RawValue* static_prop_values) {
  // determine parent class
  RawClass parent_class;
  if (parent.is_error_no_base_class()) {
    parent_class = get_builtin_class(ShapeId::kUserInstance);
  } else {
    parent_class = RawClass::cast(parent);
  }

  // determine the actual amount of member properties in this class
  uint32_t parent_field_count = parent_class.shape_instance().key_table().field_count();
  uint32_t class_field_count = parent_field_count + member_prop_count;
  if (class_field_count > RawInstance::kMaximumFieldCount) {
    return kErrorOutOfBounds;
  }

  // build key table
  RawTuple parent_key_table = parent_class.shape_instance().key_table();
  RawTuple key_table = RawTuple::cast(create_tuple(thread, class_field_count));
  for (uint32_t i = 0; i < parent_field_count; i++) {
    key_table.set_field_at(i, parent_key_table.field_at(i));
  }
  for (uint32_t i = 0; i < member_prop_count; i++) {
    key_table.set_field_at(parent_field_count + i, member_props[i]);
  }

  // build Shape object
  RawShape shape = create_shape(thread, parent_class.shape_instance(), key_table);
  register_shape(thread, shape);

  // build function table
  RawTuple function_table = RawTuple::cast(create_tuple(thread, member_func_count));
  for (int16_t i = 0; i < member_func_count; i++) {
    function_table.set_field_at(i, member_funcs[i]);
  }

  // build Class object
  RawClass klass = RawClass::cast(create_instance(thread, ShapeId::kClass, RawClass::kFieldCount));
  klass.set_name(RawSymbol::make(name));
  klass.set_parent(parent_class);
  klass.set_shape_instance(shape);
  klass.set_function_table(function_table);
  klass.set_constructor(constructor);

  // TODO: use these values
  (void)static_prop_count;
  (void)static_prop_names;
  (void)static_prop_values;

  return klass;
}

RawShape Runtime::create_shape(Thread* thread, RawValue parent, RawTuple key_table) {
  RawShape shape = RawShape::cast(create_instance(thread, ShapeId::kShape, RawShape::kFieldCount));
  shape.set_parent(parent);
  shape.set_key_table(key_table);
  return shape;
}

RawTuple Runtime::create_tuple(Thread* thread, uint32_t count) {
  RawTuple tuple = RawTuple::cast(create_instance(thread, ShapeId::kTuple, count));
  return tuple;
}

RawTuple Runtime::create_tuple(Thread* thread, std::initializer_list<RawValue> values) {
  auto tuple = create_tuple(thread, values.size());
  for (size_t i = 0; i < values.size(); i++) {
    tuple.set_field_at(i, std::data(values)[i]);
  }
  return tuple;
}

RawFunction Runtime::create_function(Thread* thread, RawValue context, SharedFunctionInfo* shared_info) {
  RawFunction func = RawFunction::cast(create_instance(thread, ShapeId::kFunction, RawFunction::kFieldCount));
  func.set_context(context);
  func.set_shared_info(shared_info);
  return func;
}

RawBuiltinFunction Runtime::create_builtin_function(Thread* thread,
                                                    BuiltinFunctionType function,
                                                    SYMBOL name,
                                                    uint8_t argc) {
  RawValue inst = create_instance(thread, ShapeId::kBuiltinFunction, RawBuiltinFunction::kFieldCount);
  RawBuiltinFunction func = RawBuiltinFunction::cast(inst);
  func.set_function(function);
  func.set_name(RawSymbol::make(name));
  func.set_argc(argc);
  return func;
}

RawFiber Runtime::create_fiber(Thread* thread, RawFunction function, RawValue self, RawValue arguments) {
  RawFiber fiber = RawFiber::cast(create_instance(thread, ShapeId::kFiber, RawFiber::kFieldCount));
  Thread* fiber_thread = scheduler()->get_free_thread();
  fiber_thread->init_fiber_thread(fiber);
  fiber.set_thread(fiber_thread);
  fiber.set_function(function);
  fiber.set_self(self);
  fiber.set_arguments(arguments);
  fiber.set_result(kNull);

  // schedule the fiber for execution
  fiber_thread->ready();
  scheduler()->schedule_thread(fiber_thread, thread->worker()->processor());

  return fiber;
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

  m_global_variables[name] = { kNull, constant, false };
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

SYMBOL Runtime::declare_symbol(Thread* thread, const char* data, size_t size) {
  std::lock_guard<std::mutex> locker(m_symbols_mutex);

  SYMBOL symbol = crc32::hash_block(data, size);

  if (m_symbol_table.count(symbol) > 0) {
    return symbol;
  }

  HandleScope scope(thread);
  String sym_string(scope, create_string(thread, data, size, symbol));
  m_symbol_table[symbol] = sym_string;
  return symbol;
}

RawClass Runtime::lookup_class(RawValue value) {
  if (value.isUserInstance()) {
    return RawUserInstance::cast(value).klass();
  } else {
    return lookup_builtin_class(value.shape_id());
  }
}

RawClass Runtime::lookup_builtin_class(ShapeId id) {
  auto offset = static_cast<uint32_t>(id);
  DCHECK(id <= ShapeId::kLastBuiltinShapeId);
  return RawClass::cast(m_builtin_classes.at(offset));
}

RawValue Runtime::lookup_symbol(SYMBOL symbol) {
  std::lock_guard<std::mutex> locker(m_symbols_mutex);

  if (m_symbol_table.count(symbol)) {
    return m_symbol_table.at(symbol);
  }

  return kNull;
}

void Runtime::set_builtin_class(ShapeId shape_id, RawClass klass) {
  std::unique_lock lock(m_shapes_mutex);
  auto offset = static_cast<uint32_t>(shape_id);
  DCHECK(shape_id <= ShapeId::kLastBuiltinShapeId);
  m_builtin_classes.at(offset) = klass;
  m_shapes.at(offset) = klass.shape_instance();
  klass.set_constructor(kNull);
}

RawClass Runtime::get_builtin_class(ShapeId shape_id) {
  std::shared_lock lock(m_shapes_mutex);
  auto offset = static_cast<uint32_t>(shape_id);
  return RawClass::cast(m_builtin_classes.at(offset));
}

}  // namespace charly::core::runtime
