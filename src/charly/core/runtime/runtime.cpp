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
#include "charly/core/runtime/interpreter.h"
#include "charly/utils/argumentparser.h"

namespace charly::core::runtime {

int32_t Runtime::run() {
  Runtime runtime;
  return runtime.join();
}

Runtime::Runtime() :
  m_start_timestamp(get_steady_timestamp()),
  m_init_flag(m_mutex),
  m_exit_flag(m_mutex),
  m_exit_code(0),
  m_wants_exit(false),
  m_heap(nullptr),
  m_gc(nullptr),
  m_scheduler(nullptr) {
  m_heap = std::make_unique<Heap>(this);
  m_gc = std::make_unique<GarbageCollector>(this);
  m_scheduler = std::make_unique<Scheduler>(this);
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
  // insert shape placeholders for immediate types
  for (uint32_t i = 0; i <= static_cast<uint32_t>(ShapeId::kLastBuiltinShapeId); i++) {
    m_shapes.push_back(kNull);
  }

  // initialize builtin classes array
  for (uint32_t i = 0; i < m_builtin_classes.size(); i++) {
    m_builtin_classes[i] = kNull;
  }

  // initialize base shapes
  auto builtin_shape_immediate = create_shape(thread, kNull, {});
  auto builtin_shape_value = builtin_shape_immediate;
  auto builtin_shape_number = builtin_shape_immediate;
  auto builtin_shape_int = builtin_shape_immediate;
  auto builtin_shape_float = builtin_shape_immediate;
  auto builtin_shape_bool = builtin_shape_immediate;
  auto builtin_shape_symbol = builtin_shape_immediate;
  auto builtin_shape_null = builtin_shape_immediate;
  auto builtin_shape_string = builtin_shape_immediate;
  auto builtin_shape_bytes = builtin_shape_immediate;
  auto builtin_shape_tuple = builtin_shape_immediate;

  auto builtin_shape_instance = create_shape(thread, builtin_shape_immediate, { "klass" });
  builtin_shape_instance.set_key_flag(RawInstance::kKlassOffset, RawShape::kKeyFlagInternal);

  auto builtin_shape_builtin_instance = create_shape(thread, builtin_shape_immediate, { "__charly_klass" });
  builtin_shape_builtin_instance.set_key_flag(RawInstance::kKlassOffset, RawShape::kKeyFlagInternal);

  auto builtin_shape_huge_bytes = create_shape(thread, builtin_shape_builtin_instance, { "data", "length" });

  auto builtin_shape_huge_string = create_shape(thread, builtin_shape_builtin_instance, { "data", "length" });

  auto builtin_shape_class =
    create_shape(thread, builtin_shape_builtin_instance,
                 { "flags", "name", "parent", "shape", "functions", "constructor" });
  //  builtin_shape_class.set_key_flag(RawClass::kShapeOffset, RawShape::kKeyFlagInternal);
  //  builtin_shape_class.set_key_flag(RawClass::kFunctionTableOffset, RawShape::kKeyFlagInternal);
  //  builtin_shape_class.set_key_flag(RawClass::kConstructorOffset, RawShape::kKeyFlagInternal);

  auto builtin_shape_shape =
    create_shape(thread, builtin_shape_builtin_instance, { "id", "parent", "keys", "additions" });

  auto builtin_shape_function =
    create_shape(thread, builtin_shape_builtin_instance, { "name", "context", "saved_self", "host_class", "shared_info" });
  builtin_shape_function.set_key_flag(RawFunction::kContextHeapVariablesOffset, RawShape::kKeyFlagInternal);
  builtin_shape_function.set_key_flag(RawFunction::kSavedSelfOffset, RawShape::kKeyFlagInternal);
  builtin_shape_function.set_key_flag(RawFunction::kSharedInfoOffset, RawShape::kKeyFlagInternal);

  auto builtin_shape_builtin_function =
    create_shape(thread, builtin_shape_builtin_instance, { "function", "name", "argc" });
  builtin_shape_builtin_function.set_key_flag(RawBuiltinFunction::kFunctionPtrOffset, RawShape::kKeyFlagInternal);
  builtin_shape_builtin_function.set_key_flag(RawBuiltinFunction::kArgcOffset, RawShape::kKeyFlagInternal);

  auto builtin_shape_fiber =
    create_shape(thread, builtin_shape_builtin_instance, { "thread", "function", "context", "arguments", "result" });

  builtin_shape_fiber.set_key_flag(RawFiber::kThreadPointerOffset, RawShape::kKeyFlagInternal);
  auto builtin_shape_exception = create_shape(thread, builtin_shape_builtin_instance, { "message", "stack_trace" });

#define DEFINE_BUILTIN_CLASS(S, N, P, SP)                                                 \
  auto class_##S##_shape = create_shape(thread, builtin_shape_class, SP);                 \
  RawClass class_##S = RawClass::unsafe_cast(create_instance(thread, class_##S##_shape)); \
  class_##S.set_name(RawSymbol::make(declare_symbol(thread, #N)));                        \
  class_##S.set_parent(P);                                                                \
  class_##S.set_shape_instance(builtin_shape_##S);                                        \
  class_##S.set_function_table(create_tuple(thread, 0));                                  \
  class_##S.set_constructor(kNull);

  DEFINE_BUILTIN_CLASS(value, Value, kNull, {})
  DEFINE_BUILTIN_CLASS(number, Number, class_value, {})
  DEFINE_BUILTIN_CLASS(int, Int, class_number, {})
  DEFINE_BUILTIN_CLASS(float, Float, class_number, {})
  DEFINE_BUILTIN_CLASS(bool, Bool, class_value, {})
  DEFINE_BUILTIN_CLASS(symbol, Symbol, class_value, {})
  DEFINE_BUILTIN_CLASS(null, Null, class_value, {})
  DEFINE_BUILTIN_CLASS(string, String, class_value, {})
  DEFINE_BUILTIN_CLASS(bytes, Bytes, class_value, {})
  DEFINE_BUILTIN_CLASS(tuple, Tuple, class_value, {})
  DEFINE_BUILTIN_CLASS(instance, Instance, class_value, {})
  DEFINE_BUILTIN_CLASS(class, Class, class_instance, {})
  DEFINE_BUILTIN_CLASS(shape, Shape, class_instance, {})
  DEFINE_BUILTIN_CLASS(function, Function, class_instance, {})
  DEFINE_BUILTIN_CLASS(builtin_function, BuiltinFunction, class_instance, {})
  DEFINE_BUILTIN_CLASS(fiber, Fiber, class_instance, {})
  DEFINE_BUILTIN_CLASS(exception, Exception, class_instance, {})
#undef DEFINE_BUILTIN_CLASS

  // define the static classes for the builtin classes
#define DEFINE_STATIC_CLASS(S, N)                                                                          \
  auto static_class_##S = RawClass::cast(create_instance(thread, ShapeId::kClass, RawClass::kFieldCount)); \
  static_class_##S.set_name(RawSymbol::make(declare_symbol(thread, #N)));                                  \
  static_class_##S.set_parent(class_class);                                                                \
  static_class_##S.set_shape_instance(class_##S##_shape);                                                  \
  static_class_##S.set_function_table(create_tuple(thread, 0));                                            \
  static_class_##S.set_constructor(kNull);

  DEFINE_STATIC_CLASS(value, Value)
  DEFINE_STATIC_CLASS(number, Number)
  DEFINE_STATIC_CLASS(int, Int)
  DEFINE_STATIC_CLASS(float, Float)
  DEFINE_STATIC_CLASS(bool, Bool)
  DEFINE_STATIC_CLASS(symbol, Symbol)
  DEFINE_STATIC_CLASS(null, Null)
  DEFINE_STATIC_CLASS(string, String)
  DEFINE_STATIC_CLASS(bytes, Bytes)
  DEFINE_STATIC_CLASS(tuple, Tuple)
  DEFINE_STATIC_CLASS(instance, Instance)
  DEFINE_STATIC_CLASS(class, Class)
  DEFINE_STATIC_CLASS(shape, Shape)
  DEFINE_STATIC_CLASS(function, Function)
  DEFINE_STATIC_CLASS(builtin_function, BuiltinFunction)
  DEFINE_STATIC_CLASS(fiber, Fiber)
  DEFINE_STATIC_CLASS(exception, Exception)
#undef DEFINE_STATIC_CLASS

  // fix up the class pointers in the class hierarchy
  class_value.set_klass_field(static_class_value);
  class_number.set_klass_field(static_class_number);
  class_int.set_klass_field(static_class_int);
  class_float.set_klass_field(static_class_float);
  class_bool.set_klass_field(static_class_bool);
  class_symbol.set_klass_field(static_class_symbol);
  class_null.set_klass_field(static_class_null);
  class_string.set_klass_field(static_class_string);
  class_bytes.set_klass_field(static_class_bytes);
  class_tuple.set_klass_field(static_class_tuple);
  class_instance.set_klass_field(static_class_instance);
  class_class.set_klass_field(static_class_class);
  class_shape.set_klass_field(static_class_shape);
  class_function.set_klass_field(static_class_function);
  class_builtin_function.set_klass_field(static_class_builtin_function);
  class_fiber.set_klass_field(static_class_fiber);
  class_exception.set_klass_field(static_class_exception);

  // set builtin classes flags
  class_value.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_number.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_int.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_float.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_bool.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_symbol.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_null.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_string.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_bytes.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_tuple.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_instance.set_flags(RawClass::kFlagNone);
  class_class.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_shape.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_function.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_builtin_function.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_fiber.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_exception.set_flags(RawClass::kFlagNone);

  // mark internal shape keys with internal flag

  set_builtin_class(thread, ShapeId::kInt, class_int);
  set_builtin_class(thread, ShapeId::kFloat, class_float);
  set_builtin_class(thread, ShapeId::kBool, class_bool);
  set_builtin_class(thread, ShapeId::kSymbol, class_symbol);
  set_builtin_class(thread, ShapeId::kNull, class_null);
  set_builtin_class(thread, ShapeId::kSmallString, class_string);
  set_builtin_class(thread, ShapeId::kSmallBytes, class_bytes);
  set_builtin_class(thread, ShapeId::kLargeString, class_string);
  set_builtin_class(thread, ShapeId::kLargeBytes, class_bytes);
  set_builtin_class(thread, ShapeId::kInstance, class_instance);
  set_builtin_class(thread, ShapeId::kHugeBytes, class_bytes);
  set_builtin_class(thread, ShapeId::kHugeString, class_string);
  set_builtin_class(thread, ShapeId::kTuple, class_tuple);
  set_builtin_class(thread, ShapeId::kClass, class_class);
  set_builtin_class(thread, ShapeId::kShape, class_shape);
  set_builtin_class(thread, ShapeId::kFunction, class_function);
  set_builtin_class(thread, ShapeId::kBuiltinFunction, class_builtin_function);
  set_builtin_class(thread, ShapeId::kFiber, class_fiber);
  set_builtin_class(thread, ShapeId::kException, class_exception);

  // fix shapes for string and bytes types
  register_shape(ShapeId::kSmallString, builtin_shape_immediate);
  register_shape(ShapeId::kLargeString, builtin_shape_immediate);
  register_shape(ShapeId::kHugeString, builtin_shape_huge_string);
  register_shape(ShapeId::kSmallBytes, builtin_shape_immediate);
  register_shape(ShapeId::kLargeBytes, builtin_shape_immediate);
  register_shape(ShapeId::kHugeBytes, builtin_shape_huge_bytes);

  // register builtin classes as global variables
  CHECK(declare_global_variable(thread, SYM("Value"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Number"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Int"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Float"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Bool"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Symbol"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Null"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("String"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Bytes"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Tuple"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Instance"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Class"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Shape"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Function"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("BuiltinFunction"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Fiber"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Exception"), true).is_error_ok());

  CHECK(set_global_variable(thread, SYM("Value"), class_value).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Number"), class_number).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Int"), class_int).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Float"), class_float).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Bool"), class_bool).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Symbol"), class_symbol).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Null"), class_null).is_error_ok());
  CHECK(set_global_variable(thread, SYM("String"), class_string).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Bytes"), class_bytes).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Tuple"), class_tuple).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Instance"), class_instance).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Class"), class_class).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Shape"), class_shape).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Function"), class_function).is_error_ok());
  CHECK(set_global_variable(thread, SYM("BuiltinFunction"), class_builtin_function).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Fiber"), class_fiber).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Exception"), class_exception).is_error_ok());
}

void Runtime::initialize_main_fiber(Thread* thread, SharedFunctionInfo* info) {
  RawFunction function = RawFunction::cast(create_function(thread, kNull, info));
  RawFiber mainfiber = RawFiber::cast(create_fiber(thread, function, kNull, kNull));
  CHECK(declare_global_variable(thread, SYM("charly.mainfiber"), true).is_error_ok());
  CHECK(set_global_variable(thread, SYM("charly.mainfiber"), mainfiber).is_error_ok());
}

void Runtime::initialize_global_variables(Thread*) {
  // TODO: implement
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

RawInstance Runtime::create_instance(Thread* thread, ShapeId shape_id, size_t field_count, RawValue klass) {
  // determine the allocation size
  DCHECK(field_count >= 1);
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

  auto instance = RawInstance::cast(RawObject::make_from_ptr(object));
  instance.set_klass_field(klass);
  return instance;
}

RawInstance Runtime::create_instance(Thread* thread, RawShape shape, RawValue klass) {
  return create_instance(thread, shape.own_shape_id(), shape.keys().size(), klass);
}

RawInstance Runtime::create_instance(Thread* thread, RawClass klass) {
  return create_instance(thread, klass.shape_instance(), klass);
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

RawClass Runtime::create_class(Thread* thread,
                               SYMBOL name,
                               RawClass parent,
                               RawFunction constructor,
                               RawTuple member_props,
                               RawTuple member_funcs,
                               RawTuple static_prop_keys,
                               RawTuple static_prop_values,
                               RawTuple static_funcs,
                               uint32_t flags) {
  auto object_shape = create_shape(thread, parent.shape_instance(), member_props);

  // if there are any static properties or functions in this class
  // a special intermediate class needs to be created that contains those static properties
  // the class instance returned is an instance of that intermediate class
  DCHECK(static_prop_keys.size() == static_prop_values.size());
  auto builtin_class_instance = get_builtin_class(thread, ShapeId::kClass);
  if (static_prop_keys.size() || static_funcs.size()) {
    auto builtin_class_shape = builtin_class_instance.shape_instance();
    auto static_shape = create_shape(thread, builtin_class_shape, static_prop_keys);
    auto static_class = RawClass::cast(create_instance(thread, builtin_class_instance));
    static_class.set_flags(flags);
    static_class.set_name(RawSymbol::make(name));
    static_class.set_parent(builtin_class_instance);
    static_class.set_shape_instance(static_shape);
    static_class.set_function_table(static_funcs);
    static_class.set_constructor(kNull);

    // build instance of newly created static shape
    auto actual_class = RawClass::cast(create_instance(thread, static_shape, static_class));
    actual_class.set_flags(flags);
    actual_class.set_name(RawSymbol::make(name));
    actual_class.set_parent(parent);
    actual_class.set_shape_instance(object_shape);
    actual_class.set_function_table(member_funcs);
    actual_class.set_constructor(constructor);

    // initialize static properties
    for (uint32_t i = 0; i < static_prop_values.size(); i++) {
      RawValue value = static_prop_values.field_at(i);
      actual_class.set_field_at(RawClass::kFieldCount + i, value);
    }

    return actual_class;
  } else {
    auto klass = RawClass::cast(create_instance(thread, builtin_class_instance));
    klass.set_flags(flags);
    klass.set_name(RawSymbol::make(name));
    klass.set_parent(parent);
    klass.set_shape_instance(object_shape);
    klass.set_function_table(member_funcs);
    klass.set_constructor(constructor);
    return klass;
  }
}

RawShape Runtime::create_shape(Thread* thread, RawValue parent, RawTuple key_table) {
  // create empty base shape if parent is null
  if (!parent.isShape()) {
    auto shape = RawShape::cast(create_instance(thread, ShapeId::kShape, RawShape::kFieldCount));
    shape.set_parent(kNull);
    shape.set_keys(create_tuple(thread, 0));
    shape.set_additions(create_tuple(thread, 0));
    register_shape(shape);
    return create_shape(thread, shape, key_table);
  }

  // find preexisting shape with same layout or create new shape
  auto parent_shape = RawShape::cast(parent);
  auto target_shape = parent_shape;
  for (uint32_t i = 0; i < key_table.size(); i++) {
    auto key = RawSymbol::cast(key_table.field_at(i));
    RawShape next_shape;
    {
      std::lock_guard lock(target_shape);

      // find the shape to transition to when adding the new key
      RawTuple additions = target_shape.additions();
      bool found_next = false;
      for (uint32_t ai = 0; ai < additions.size(); ai++) {
        auto entry = RawTuple::cast(additions.field_at(ai));
        auto symbol = RawSymbol::cast(entry.field_at(RawShape::kAdditionsSymbolOffset));
        if (symbol == key) {
          next_shape = RawShape::cast(entry.field_at(RawShape::kAdditionsNextOffset));
          found_next = true;
          break;
        }
      }

      // create new shape for added key
      if (!found_next) {
        next_shape = RawShape::cast(create_instance(thread, ShapeId::kShape, RawShape::kFieldCount));
        next_shape.set_parent(target_shape);
        auto encoded = RawShape::encode_shape_key(key.value(), RawShape::kKeyFlagNone);
        next_shape.set_keys(concat_tuple(thread, target_shape.keys(), create_tuple(thread, { encoded })));
        next_shape.set_additions(create_tuple(thread, 0));
        register_shape(next_shape);

        // add new shape to additions table of previous base
        target_shape.set_additions(
          concat_tuple(thread, additions, create_tuple(thread, { create_tuple(thread, { key, next_shape }) })));
      }
    }

    target_shape = next_shape;
  }

  return target_shape;
}

RawShape Runtime::create_shape(Thread* thread, RawValue parent, std::initializer_list<std::string> keys) {
  auto key_tuple = create_tuple(thread, keys.size());
  for (size_t i = 0; i < keys.size(); i++) {
    key_tuple.set_field_at(i, RawSymbol::make(declare_symbol(thread, std::data(keys)[i])));
  }
  return create_shape(thread, parent, key_tuple);
}

RawTuple Runtime::create_tuple(Thread* thread, uint32_t count) {
  size_t size = count * kPointerSize;
  DCHECK(size <= RawData::kMaxLength);
  auto tuple = RawTuple::cast(create_data(thread, ShapeId::kTuple, size));
  CHECK(tuple.header()->cas_count(size, count));
  return tuple;
}

RawTuple Runtime::create_tuple(Thread* thread, std::initializer_list<RawValue> values) {
  auto tuple = create_tuple(thread, values.size());
  for (size_t i = 0; i < values.size(); i++) {
    tuple.set_field_at(i, std::data(values)[i]);
  }
  return tuple;
}

RawTuple Runtime::concat_tuple(Thread* thread, RawTuple left, RawTuple right) {
  uint32_t left_size = left.size();
  uint32_t right_size = right.size();
  uint64_t new_size = left_size + right_size;
  CHECK(new_size <= kInt32Max);

  auto new_tuple = create_tuple(thread, new_size);
  for (uint32_t i = 0; i < left_size; i++) {
    new_tuple.set_field_at(i, left.field_at(i));
  }
  for (uint32_t i = 0; i < right_size; i++) {
    new_tuple.set_field_at(left_size + i, right.field_at(i));
  }

  return new_tuple;
}

RawFunction Runtime::create_function(Thread* thread,
                                     RawValue context,
                                     SharedFunctionInfo* shared_info,
                                     RawValue saved_self) {
  RawFunction func = RawFunction::cast(create_instance(thread, ShapeId::kFunction, RawFunction::kFieldCount));
  func.set_name(RawSymbol::make(shared_info->name_symbol));
  func.set_context(context);
  func.set_saved_self(saved_self);
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
  fiber.set_context(self);
  fiber.set_arguments(arguments);
  fiber.set_result(kNull);

  // schedule the fiber for execution
  fiber_thread->ready();
  scheduler()->schedule_thread(fiber_thread, thread->worker()->processor());

  return fiber;
}

RawValue Runtime::create_exception(Thread* thread, RawValue message) {
  // allocate exception instance
  auto instance = RawException::cast(create_instance(thread, get_builtin_class(thread, ShapeId::kException)));
  instance.set_message(message);
  instance.set_stack_trace(create_stack_trace(thread));

  return instance;
}

RawTuple Runtime::create_stack_trace(Thread* thread, uint32_t trim) {
  std::vector<RawTuple> frames;
  uint64_t depth = 0;
  for (Frame* frame = thread->frame(); frame != nullptr; frame = frame->parent) {
    if (depth >= trim) {
      frames.push_back(create_tuple(thread, { frame->function }));
    }
    depth++;
  }

  RawTuple stack_trace = create_tuple(thread, frames.size());
  for (uint32_t i = 0; i < frames.size(); i++) {
    stack_trace.set_field_at(i, frames.at(i));
  }
  return stack_trace;
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

ShapeId Runtime::register_shape(RawShape shape) {
  std::unique_lock<std::shared_mutex> lock(m_shapes_mutex);
  // TODO: fail gracefully
  CHECK(m_shapes.size() < static_cast<size_t>(ShapeId::kMaxShapeCount), "exceeded max shapes count");

  uint32_t shape_count = m_shapes.size();
  ShapeId next_shape_id = static_cast<ShapeId>(shape_count);
  m_shapes.push_back(shape);
  shape.set_own_shape_id(next_shape_id);

  return next_shape_id;
}

void Runtime::register_shape(ShapeId id, RawShape shape) {
  std::unique_lock<std::shared_mutex> lock(m_shapes_mutex);
  auto index = static_cast<size_t>(id);
  DCHECK(index < m_shapes.size());
  m_shapes[index] = shape;
  shape.set_own_shape_id(id);
}

RawShape Runtime::lookup_shape(Thread*, ShapeId id) {
  // TODO: cache shapes in processor
  std::shared_lock<std::shared_mutex> lock(m_shapes_mutex);
  auto index = static_cast<size_t>(id);
  CHECK(index < m_shapes.size());
  return RawShape::cast(m_shapes.at(index));
}

RawClass Runtime::lookup_class(Thread* thread, RawValue value) {
  // user instances carry their class reference with them
  if (value.isInstance()) {
    auto instance = RawInstance::cast(value);
    auto klass_field = instance.klass_field();
    if (klass_field.isNull()) {
      instance.set_klass_field(get_builtin_class(thread, value.shape_id()));
    }

    return RawClass::cast(instance.klass_field());
  }

  return get_builtin_class(thread, value.shape_id());
}

RawValue Runtime::lookup_symbol(SYMBOL symbol) {
  std::lock_guard<std::mutex> locker(m_symbols_mutex);

  if (m_symbol_table.count(symbol)) {
    return m_symbol_table.at(symbol);
  }

  return kNull;
}

void Runtime::set_builtin_class(Thread*, ShapeId shape_id, RawClass klass) {
  std::unique_lock<std::shared_mutex> lock(m_shapes_mutex);
  RawShape shape = klass.shape_instance();

  auto offset = static_cast<uint32_t>(shape_id);
  DCHECK(shape_id <= ShapeId::kLastBuiltinShapeId);
  m_shapes[offset] = shape;
  shape.set_own_shape_id(shape_id);

  DCHECK(offset < kBuiltinClassCount);
  DCHECK(m_builtin_classes[offset].isNull());
  m_builtin_classes[offset] = klass;
}

RawClass Runtime::get_builtin_class(Thread*, ShapeId shape_id) {
  // TODO: cache via processor
  std::shared_lock<std::shared_mutex> lock(m_shapes_mutex);
  auto offset = static_cast<uint32_t>(shape_id);
  DCHECK(offset < kBuiltinClassCount);
  return RawClass::cast(m_builtin_classes.at(offset));
}

}  // namespace charly::core::runtime
