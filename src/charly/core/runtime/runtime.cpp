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

#include <fstream>

#include "charly/core/compiler/compiler.h"
#include "charly/core/runtime/builtins/core.h"
#include "charly/core/runtime/builtins/future.h"
#include "charly/core/runtime/builtins/readline.h"
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"
#include "charly/utils/argumentparser.h"

namespace charly::core::runtime {

using namespace charly::core::compiler;
using namespace charly::core::compiler::ir;

int32_t Runtime::run() {
  Runtime runtime;
  return runtime.join();
}

Runtime::Runtime() :
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
  initialize_stdlib_paths();
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
  declare_symbol(thread, "CHARLY_STDLIB");

  CHECK(declare_global_variable(thread, SYM("CHARLY_STDLIB"), true).is_error_ok());
  CHECK(set_global_variable(thread, SYM("CHARLY_STDLIB"), create_string(thread, m_stdlib_directory)).is_error_ok());
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
  builtin::future::initialize(thread);
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
  auto builtin_shape_immediate = RawShape::cast(create_instance(thread, ShapeId::kShape, RawShape::kFieldCount));
  builtin_shape_immediate.set_parent(kNull);
  builtin_shape_immediate.set_keys(create_tuple(thread, 0));
  builtin_shape_immediate.set_additions(create_tuple(thread, 0));
  register_shape(builtin_shape_immediate);

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

  auto builtin_shape_instance =
    create_shape(thread, builtin_shape_immediate, { { "klass", RawShape::kKeyFlagInternal } });

  auto builtin_shape_builtin_instance =
    create_shape(thread, builtin_shape_immediate, { { "__charly_klass", RawShape::kKeyFlagInternal } });

  auto builtin_shape_huge_bytes =
    create_shape(thread, builtin_shape_builtin_instance,
                 { { "data", RawShape::kKeyFlagInternal }, { "length", RawShape::kKeyFlagInternal } });

  auto builtin_shape_huge_string =
    create_shape(thread, builtin_shape_builtin_instance,
                 { { "data", RawShape::kKeyFlagInternal }, { "length", RawShape::kKeyFlagInternal } });

  auto builtin_shape_class = create_shape(thread, builtin_shape_builtin_instance,
                                          { { "flags", RawShape::kKeyFlagInternal },
                                            { "ancestor_table", RawShape::kKeyFlagReadOnly },
                                            { "name", RawShape::kKeyFlagReadOnly },
                                            { "parent", RawShape::kKeyFlagReadOnly },
                                            { "shape", RawShape::kKeyFlagReadOnly },
                                            { "function_table", RawShape::kKeyFlagReadOnly },
                                            { "constructor", RawShape::kKeyFlagReadOnly } });

  auto builtin_shape_shape = create_shape(thread, builtin_shape_builtin_instance,
                                          { { "id", RawShape::kKeyFlagReadOnly },
                                            { "parent", RawShape::kKeyFlagReadOnly },
                                            { "keys", RawShape::kKeyFlagReadOnly },
                                            { "additions", RawShape::kKeyFlagReadOnly } });

  auto builtin_shape_function = create_shape(thread, builtin_shape_builtin_instance,
                                             { { "name", RawShape::kKeyFlagReadOnly },
                                               { "context", RawShape::kKeyFlagReadOnly },
                                               { "saved_self", RawShape::kKeyFlagReadOnly },
                                               { "host_class", RawShape::kKeyFlagReadOnly },
                                               { "overload_table", RawShape::kKeyFlagReadOnly },
                                               { "shared_info", RawShape::kKeyFlagInternal } });

  auto builtin_shape_builtin_function = create_shape(thread, builtin_shape_builtin_instance,
                                                     { { "function", RawShape::kKeyFlagReadOnly },
                                                       { "name", RawShape::kKeyFlagReadOnly },
                                                       { "argc", RawShape::kKeyFlagReadOnly } });

  auto builtin_shape_fiber = create_shape(thread, builtin_shape_builtin_instance,
                                          { { "thread", RawShape::kKeyFlagInternal },
                                            { "function", RawShape::kKeyFlagReadOnly },
                                            { "context", RawShape::kKeyFlagReadOnly },
                                            { "arguments", RawShape::kKeyFlagReadOnly },
                                            { "result_future", RawShape::kKeyFlagReadOnly } });

  auto builtin_shape_future = create_shape(thread, builtin_shape_builtin_instance,
                                           { { "wait_queue", RawShape::kKeyFlagInternal },
                                             { "result", RawShape::kKeyFlagReadOnly },
                                             { "exception", RawShape::kKeyFlagReadOnly } });

  auto builtin_shape_exception = create_shape(thread, builtin_shape_builtin_instance,
                                              { { "message", RawShape::kKeyFlagNone },
                                                { "stack_trace", RawShape::kKeyFlagNone },
                                                { "cause", RawShape::kKeyFlagReadOnly } });

  auto builtin_shape_import_exception =
    create_shape(thread, builtin_shape_exception, { { "errors", RawShape::kKeyFlagReadOnly } });

  auto class_value_shape = create_shape(thread, builtin_shape_class, {});
  RawClass class_value = RawClass::unsafe_cast(create_instance(thread, class_value_shape));
  class_value.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_value.set_ancestor_table(create_tuple(thread, 0));
  class_value.set_name(RawSymbol::make(declare_symbol(thread, "Value")));
  class_value.set_parent(kNull);
  class_value.set_shape_instance(builtin_shape_value);
  class_value.set_function_table(create_tuple(thread, 0));
  class_value.set_constructor(kNull);

#define DEFINE_BUILTIN_CLASS(S, N, P, F, SP)                                              \
  auto class_##S##_shape = create_shape(thread, builtin_shape_class, SP);                 \
  RawClass class_##S = RawClass::unsafe_cast(create_instance(thread, class_##S##_shape)); \
  class_##S.set_flags(F);                                                                 \
  class_##S.set_ancestor_table(concat_tuple_value(thread, P.ancestor_table(), P));        \
  class_##S.set_name(RawSymbol::make(declare_symbol(thread, #N)));                        \
  class_##S.set_parent(P);                                                                \
  class_##S.set_shape_instance(builtin_shape_##S);                                        \
  class_##S.set_function_table(create_tuple(thread, 0));                                  \
  class_##S.set_constructor(kNull);

  DEFINE_BUILTIN_CLASS(number, Number, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(int, Int, class_number, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(float, Float, class_number, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(bool, Bool, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(symbol, Symbol, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(null, Null, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(string, String, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(bytes, Bytes, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(tuple, Tuple, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(instance, Instance, class_value, RawClass::kFlagNone, {})
  DEFINE_BUILTIN_CLASS(class, Class, class_instance, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(shape, Shape, class_instance, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(function, Function, class_instance, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(builtin_function, BuiltinFunction, class_instance,
                       RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(fiber, Fiber, class_instance, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(future, Future, class_instance, RawClass::kFlagFinal | RawClass::kFlagNonConstructable, {})
  DEFINE_BUILTIN_CLASS(exception, Exception, class_instance, RawClass::kFlagNone, {})
  DEFINE_BUILTIN_CLASS(import_exception, ImportException, class_exception, RawClass::kFlagNone, {})
#undef DEFINE_BUILTIN_CLASS

// define the static classes for the builtin classes
#define DEFINE_STATIC_CLASS(S, N)                                                                             \
  auto static_class_##S =                                                                                     \
    RawClass::cast(create_instance(thread, ShapeId::kClass, RawClass::kFieldCount, class_class));             \
  static_class_##S.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);                         \
  static_class_##S.set_ancestor_table(concat_tuple_value(thread, class_class.ancestor_table(), class_class)); \
  static_class_##S.set_name(RawSymbol::make(declare_symbol(thread, #N)));                                     \
  static_class_##S.set_parent(class_class);                                                                   \
  static_class_##S.set_shape_instance(class_##S##_shape);                                                     \
  static_class_##S.set_function_table(create_tuple(thread, 0));                                               \
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
  DEFINE_STATIC_CLASS(future, Future)
  DEFINE_STATIC_CLASS(exception, Exception)
  DEFINE_STATIC_CLASS(import_exception, ImportException)
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
  class_future.set_klass_field(static_class_future);
  class_exception.set_klass_field(static_class_exception);
  class_import_exception.set_klass_field(static_class_import_exception);

  // mark internal shape keys with internal flag
  set_builtin_class(ShapeId::kInt, class_int);
  set_builtin_class(ShapeId::kFloat, class_float);
  set_builtin_class(ShapeId::kBool, class_bool);
  set_builtin_class(ShapeId::kSymbol, class_symbol);
  set_builtin_class(ShapeId::kNull, class_null);
  set_builtin_class(ShapeId::kSmallString, class_string);
  set_builtin_class(ShapeId::kSmallBytes, class_bytes);
  set_builtin_class(ShapeId::kLargeString, class_string);
  set_builtin_class(ShapeId::kLargeBytes, class_bytes);
  set_builtin_class(ShapeId::kInstance, class_instance);
  set_builtin_class(ShapeId::kHugeBytes, class_bytes);
  set_builtin_class(ShapeId::kHugeString, class_string);
  set_builtin_class(ShapeId::kTuple, class_tuple);
  set_builtin_class(ShapeId::kClass, class_class);
  set_builtin_class(ShapeId::kShape, class_shape);
  set_builtin_class(ShapeId::kFunction, class_function);
  set_builtin_class(ShapeId::kBuiltinFunction, class_builtin_function);
  set_builtin_class(ShapeId::kFiber, class_fiber);
  set_builtin_class(ShapeId::kFuture, class_future);
  set_builtin_class(ShapeId::kException, class_exception);
  set_builtin_class(ShapeId::kImportException, class_import_exception);

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
  CHECK(declare_global_variable(thread, SYM("Future"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Exception"), true).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("ImportException"), true).is_error_ok());

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
  CHECK(set_global_variable(thread, SYM("Future"), class_future).is_error_ok());
  CHECK(set_global_variable(thread, SYM("Exception"), class_exception).is_error_ok());
  CHECK(set_global_variable(thread, SYM("ImportException"), class_import_exception).is_error_ok());

  // patch klass field of all shape instances created up until this point
  // no lock is required here while interacting with m_shapes since no other threads are active yet
  {
    for (auto entry : m_shapes) {
      if (entry.isShape()) {
        auto shape = RawShape::cast(entry);
        shape.set_klass_field(get_builtin_class(ShapeId::kShape));
      }
    }
  }
}

void Runtime::initialize_stdlib_paths() {
  auto CHARLYVMDIR = utils::ArgumentParser::get_environment_for_key("CHARLYVMDIR");
  CHECK(CHARLYVMDIR.has_value());
  m_source_code_directory = fs::path(CHARLYVMDIR.value());
  m_stdlib_directory = m_source_code_directory / "src" / "charly" / "stdlib";
  m_builtin_libraries_paths["testlib"] = m_stdlib_directory / "libs" / "testlib.ch";
}

RawData Runtime::create_data(Thread* thread, ShapeId shape_id, size_t size) {
  // determine the total allocation size
  DCHECK(size <= RawData::kMaxLength);
  size_t header_size = sizeof(ObjectHeader);
  size_t total_size = align_to_size(header_size + size, kObjectAlignment);

  uintptr_t memory = thread->allocate(total_size);
  DCHECK(memory);

  // initialize header
  ObjectHeader::initialize_header(memory, shape_id, size);
  return RawData::cast(RawObject::make_from_ptr(memory + header_size, true));
}

RawInstance Runtime::create_instance(Thread* thread, ShapeId shape_id, size_t field_count, RawValue klass) {
  // determine the allocation size
  DCHECK(field_count >= 1);
  DCHECK(field_count <= RawInstance::kMaximumFieldCount);
  size_t object_size = field_count * kPointerSize;
  size_t header_size = sizeof(ObjectHeader);
  size_t total_size = align_to_size(header_size + object_size, kObjectAlignment);

  uintptr_t memory = thread->allocate(total_size);
  DCHECK(memory);

  // initialize header
  ObjectHeader::initialize_header(memory, shape_id, field_count);

  // initialize fields to null
  uintptr_t object = memory + header_size;
  auto* object_fields = bitcast<RawValue*>(object);
  for (uint32_t i = 0; i < field_count; i++) {
    object_fields[i] = kNull;
  }

  auto instance = RawInstance::cast(RawObject::make_from_ptr(object, true));
  instance.set_klass_field(klass);
  return instance;
}

RawInstance Runtime::create_instance(Thread* thread, RawShape shape, RawValue klass) {
  return create_instance(thread, shape.own_shape_id(), shape.keys().size(), klass);
}

RawInstance Runtime::create_instance(Thread* thread, RawClass klass) {
  return create_instance(thread, klass.shape_instance(), klass);
}

RawString Runtime::create_string(Thread* thread, const char* data, size_t size, SYMBOL hash) {
  if (size <= RawSmallString::kMaxLength) {
    return RawString::cast(RawSmallString::make_from_memory(data, size));
  } else if (size <= RawLargeString::kMaxLength) {
    return RawString::cast(create_large_string(thread, data, size, hash));
  } else {
    return RawString::cast(create_huge_string(thread, data, size, hash));
  }
}

RawString Runtime::acquire_string(Thread* thread, char* data, size_t size, SYMBOL hash) {
  if (size <= RawLargeString::kMaxLength) {
    auto value = create_string(thread, data, size, hash);
    std::free(data);
    return value;
  } else {
    return RawString::cast(create_huge_string_acquire(thread, data, size, hash));
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
  RawHugeString object = RawHugeString::cast(
    create_instance(thread, ShapeId::kHugeString, RawHugeString::kFieldCount, get_builtin_class(ShapeId::kHugeString)));
  object.set_data(data);
  object.set_length(size);
  object.header()->cas_hashcode((SYMBOL)0, hash);
  return object;
}

RawValue Runtime::create_class(Thread* thread,
                               SYMBOL name,
                               RawValue parent_value,
                               RawFunction constructor,
                               RawTuple member_props,
                               RawTuple member_funcs,
                               RawTuple static_prop_keys,
                               RawTuple static_prop_values,
                               RawTuple static_funcs,
                               uint32_t flags) {
  if (parent_value.is_error_no_base_class()) {
    parent_value = get_builtin_class(ShapeId::kInstance);
  }

  if (!parent_value.isClass()) {
    thread->throw_value(create_string_from_template(thread, "extended value is not a class"));
    return kErrorException;
  }

  RawClass parent_class = RawClass::cast(parent_value);
  RawShape parent_shape = parent_class.shape_instance();

  if (parent_class.flags() & RawClass::kFlagFinal) {
    thread->throw_value(
      create_string_from_template(thread, "cannot subclass class '%', it is marked final", parent_class.name()));
    return kErrorException;
  }

  // check for duplicate member properties
  auto parent_keys_tuple = parent_shape.keys();
  for (uint8_t i = 0; i < member_props.size(); i++) {
    auto encoded = member_props.field_at<RawInt>(i);
    SYMBOL prop_name;
    uint8_t prop_flags;
    RawShape::decode_shape_key(encoded, prop_name, prop_flags);
    for (uint32_t pi = 0; pi < parent_keys_tuple.size(); pi++) {
      SYMBOL parent_key_symbol;
      uint8_t parent_key_flags;
      RawShape::decode_shape_key(parent_keys_tuple.field_at<RawInt>(pi), parent_key_symbol, parent_key_flags);
      if (parent_key_symbol == prop_name) {
        thread->throw_value(
          create_string_from_template(thread, "cannot redeclare property '%', parent class '%' already contains it",
                                      RawSymbol::make(prop_name), parent_class.name()));
        return kErrorException;
      }
    }
  }

  // ensure new class doesn't exceed the class member property limit
  size_t new_member_count = parent_shape.field_count() + member_props.size();
  if (new_member_count > RawInstance::kMaximumFieldCount) {
    // for some reason, RawInstance::kMaximumFieldCount needs to be casted to its own
    // type, before it can be used in here. this is some weird template thing...
    thread->throw_value(
      create_string_from_template(thread, "newly created class '%' has % properties, but the limit is %",
                                  RawSymbol::make(name), new_member_count, (size_t)RawInstance::kMaximumFieldCount));
    return kErrorException;
  }

  auto object_shape = create_shape(thread, parent_shape, member_props);

  RawTuple parent_function_table = parent_class.function_table();
  RawTuple new_function_table;

  // can simply reuse parent function table if no new functions are being added
  if (member_funcs.size() == 0) {
    new_function_table = parent_function_table;
  } else {
    std::unordered_map<SYMBOL, uint32_t> parent_function_indices;
    for (uint32_t j = 0; j < parent_function_table.size(); j++) {
      auto parent_function = parent_function_table.field_at<RawFunction>(j);
      parent_function_indices[parent_function.name()] = j;
    }

    // calculate the amount of functions being added that are not already present in the parent class
    std::unordered_set<SYMBOL> new_function_names;
    size_t newly_added_functions = 0;
    for (uint32_t i = 0; i < member_funcs.size(); i++) {
      auto method_name = RawFunction::cast(member_funcs.field_at<RawTuple>(i).field_at(0)).name();
      new_function_names.insert(method_name);

      if (parent_function_indices.count(method_name) == 0) {
        newly_added_functions++;
      }
    }

    size_t new_function_table_size = parent_function_table.size() + newly_added_functions;
    new_function_table = create_tuple(thread, new_function_table_size);

    // functions which are not being overriden in the new class can be copied to the new table
    size_t new_function_table_offset = 0;
    for (uint32_t j = 0; j < parent_function_table.size(); j++) {
      auto parent_function = parent_function_table.field_at<RawFunction>(j);
      if (new_function_names.count(parent_function.name()) == 0) {
        new_function_table.set_field_at(new_function_table_offset++, parent_function);
      }
    }

    // insert new methods into function table
    // the overload tables of functions that override functions from the parent class must be merged
    // with the overload tables from their parents
    for (uint32_t i = 0; i < member_funcs.size(); i++) {
      auto new_overload_table = member_funcs.field_at<RawTuple>(i);
      auto first_overload = new_overload_table.field_at<RawFunction>(0);
      auto new_overload_name = first_overload.name();

      // new function does not override any parent functions
      if (parent_function_indices.count(new_overload_name) == 0) {
        for (uint32_t j = 0; j < new_overload_table.size(); j++) {
          new_overload_table.field_at<RawFunction>(j).set_overload_table(new_overload_table);
        }
        new_function_table.set_field_at(new_function_table_offset++, first_overload);
        continue;
      }

      uint32_t parent_function_index = parent_function_indices[new_overload_name];
      auto parent_function = parent_function_table.field_at<RawFunction>(parent_function_index);
      auto parent_overload_table = RawTuple::cast(parent_function.overload_table());

      auto max_parent_overload = parent_overload_table.last_field<RawFunction>();
      auto max_new_overload = new_overload_table.last_field<RawFunction>();

      uint32_t parent_max_argc = max_parent_overload.shared_info()->ir_info.argc;
      uint32_t new_max_argc = max_new_overload.shared_info()->ir_info.argc;
      uint32_t max_argc = std::max(parent_max_argc, new_max_argc);
      uint32_t merged_size = max_argc + 2;
      auto merged_table = create_tuple(thread, merged_size);

      // merge the parents and the new functions overload tables
      RawFunction highest_overload;
      RawFunction first_new_overload = new_overload_table.first_field<RawFunction>();
      for (uint32_t argc = 0; argc < merged_size; argc++) {
        uint32_t parent_table_index = std::min(parent_overload_table.size() - 1, argc);
        uint32_t new_table_index = std::min(new_overload_table.size() - 1, argc);

        auto parent_method = parent_overload_table.field_at<RawFunction>(parent_table_index);
        auto new_method = new_overload_table.field_at<RawFunction>(new_table_index);

        if (new_method.check_accepts_argc(argc)) {
          new_method.set_overload_table(merged_table);
          merged_table.set_field_at(argc, new_method);
          highest_overload = new_method;
          continue;
        }

        if (parent_method.check_accepts_argc(argc)) {
          auto method_copy =
            create_function(thread, parent_method.context(), parent_method.shared_info(), parent_method.saved_self());

          method_copy.set_overload_table(merged_table);
          method_copy.set_host_class(parent_method.host_class());
          merged_table.set_field_at(argc, method_copy);
          highest_overload = method_copy;
          continue;
        }

        merged_table.set_field_at(argc, kNull);
      }

      // patch holes in the merged table with their next highest applicable overload
      // trailing holes are filled with the next lowest overload
      RawValue next_highest_method = kNull;
      for (int64_t k = merged_size - 1; k >= 0; k--) {
        auto field = merged_table.field_at(k);
        if (field.isNull()) {
          if (next_highest_method.isNull()) {
            merged_table.set_field_at(k, highest_overload);
          } else {
            merged_table.set_field_at(k, next_highest_method);
          }
        } else {
          next_highest_method = field;
        }
      }

      // remove overload tuple if only one function is present
      // TODO: ideally the overload tuple shouldn't be constructed
      //       in the first place if this is the case
      auto merged_first_overload = merged_table.first_field<RawFunction>();
      bool overload_tuple_is_homogenic = true;
      for (uint32_t oi = 0; oi < merged_table.size(); oi++) {
        if (merged_table.field_at(oi) != merged_first_overload) {
          overload_tuple_is_homogenic = false;
          break;
        }
      }
      if (merged_table.size() == 1 || overload_tuple_is_homogenic) {
        merged_first_overload.set_overload_table(kNull);
        new_function_table.set_field_at(new_function_table_offset++, merged_first_overload);
      } else {
        new_function_table.set_field_at(new_function_table_offset++, first_new_overload);
      }
    }
  }

  // build overload tables for static functions
  auto static_function_table = create_tuple(thread, static_funcs.size());
  for (uint32_t i = 0; i < static_funcs.size(); i++) {
    auto overload_tuple = static_funcs.field_at<RawTuple>(i);
    DCHECK(overload_tuple.size() >= 1);

    // remove overload tuple if only one function is present
    auto first_overload = overload_tuple.first_field<RawFunction>();
    bool overload_tuple_is_homogenic = true;
    for (uint32_t oi = 0; oi < overload_tuple.size(); oi++) {
      if (overload_tuple.field_at(oi) != first_overload) {
        overload_tuple_is_homogenic = false;
        break;
      }
    }
    if (overload_tuple.size() == 1 || overload_tuple_is_homogenic) {
      first_overload.set_overload_table(kNull);
      static_function_table.set_field_at(i, first_overload);
      continue;
    }

    // point functions to the same overload tuple
    for (uint32_t j = 0; j < overload_tuple.size(); j++) {
      auto func = overload_tuple.field_at<RawFunction>(j);
      func.set_overload_table(overload_tuple);
    }

    // put the lowest overload into the functions lookup table
    // since every function part of the overload has a reference to the overload table
    // it doesn't really matter which function we put in there
    static_function_table.set_field_at(i, first_overload);
  }

  // if there are any static properties or functions in this class
  // a special intermediate class needs to be created that contains those static properties
  // the class instance returned is an instance of that intermediate class
  DCHECK(static_prop_keys.size() == static_prop_values.size());
  auto builtin_class_instance = get_builtin_class(ShapeId::kClass);
  RawClass constructed_class;
  if (static_prop_keys.size() || static_funcs.size()) {
    auto builtin_class_shape = builtin_class_instance.shape_instance();
    auto static_shape = create_shape(thread, builtin_class_shape, static_prop_keys);
    auto static_class = RawClass::cast(create_instance(thread, builtin_class_instance));
    static_class.set_flags(flags | RawClass::kFlagStatic);
    static_class.set_ancestor_table(
      concat_tuple_value(thread, builtin_class_instance.ancestor_table(), builtin_class_instance));
    static_class.set_name(RawSymbol::make(name));
    static_class.set_parent(builtin_class_instance);
    static_class.set_shape_instance(static_shape);
    static_class.set_function_table(static_function_table);
    static_class.set_constructor(kNull);

    // build instance of newly created static shape
    auto actual_class = RawClass::cast(create_instance(thread, static_shape, static_class));
    actual_class.set_flags(flags);
    actual_class.set_ancestor_table(concat_tuple_value(thread, parent_class.ancestor_table(), parent_class));
    actual_class.set_name(RawSymbol::make(name));
    actual_class.set_parent(parent_class);
    actual_class.set_shape_instance(object_shape);
    actual_class.set_function_table(new_function_table);
    actual_class.set_constructor(constructor);

    // initialize static properties
    for (uint32_t i = 0; i < static_prop_values.size(); i++) {
      RawValue value = static_prop_values.field_at(i);
      actual_class.set_field_at(RawClass::kFieldCount + i, value);
    }

    // patch host class field on static functions
    for (uint32_t i = 0; i < static_function_table.size(); i++) {
      auto entry = static_function_table.field_at<RawFunction>(i);
      auto overloads_value = entry.overload_table();
      if (overloads_value.isTuple()) {
        auto overloads = RawTuple::cast(overloads_value);
        for (uint32_t j = 0; j < overloads.size(); j++) {
          auto func = overloads.field_at<RawFunction>(j);
          if (func.host_class().isNull()) {
            func.set_host_class(static_class);
          }
        }
      }
    }

    constructed_class = actual_class;
  } else {
    auto klass = RawClass::cast(create_instance(thread, builtin_class_instance));
    klass.set_flags(flags);
    klass.set_ancestor_table(concat_tuple_value(thread, parent_class.ancestor_table(), parent_class));
    klass.set_name(RawSymbol::make(name));
    klass.set_parent(parent_class);
    klass.set_shape_instance(object_shape);
    klass.set_function_table(new_function_table);
    klass.set_constructor(constructor);
    constructed_class = klass;
  }

  // set host class fields on functions
  constructor.set_host_class(constructed_class);
  for (uint32_t i = 0; i < new_function_table.size(); i++) {
    auto entry = new_function_table.field_at<RawFunction>(i);
    auto overloads_value = entry.overload_table();
    if (overloads_value.isTuple()) {
      auto overloads = RawTuple::cast(overloads_value);
      for (uint32_t j = 0; j < overloads.size(); j++) {
        auto func = overloads.field_at<RawFunction>(j);
        if (func.host_class().isNull()) {
          func.set_host_class(constructed_class);
        }
      }
    }
  }

  return constructed_class;
}

RawShape Runtime::create_shape(Thread* thread, RawValue parent, RawTuple key_table) {
  // find preexisting shape with same layout or create new shape
  auto parent_shape = RawShape::cast(parent);
  auto target_shape = parent_shape;
  for (uint32_t i = 0; i < key_table.size(); i++) {
    auto encoded = key_table.field_at<RawInt>(i);
    RawShape next_shape;
    {
      std::lock_guard lock(target_shape);

      // find the shape to transition to when adding the new key
      RawTuple additions = target_shape.additions();
      bool found_next = false;
      for (uint32_t ai = 0; ai < additions.size(); ai++) {
        auto entry = additions.field_at<RawTuple>(ai);
        auto entry_encoded_key = entry.field_at<RawInt>(RawShape::kAdditionsKeyOffset);
        if (encoded == entry_encoded_key) {
          next_shape = entry.field_at<RawShape>(RawShape::kAdditionsNextOffset);
          found_next = true;
          break;
        }
      }

      // create new shape for added key
      if (!found_next) {
        next_shape = RawShape::cast(create_instance(thread, ShapeId::kShape, RawShape::kFieldCount));

        // Runtime::create_shape gets called during runtime initialization
        // at that point no builtin classes are registered yet
        // the klass field gets patched once runtime initialization has finished
        if (builtin_class_is_registered(ShapeId::kShape)) {
          next_shape.set_klass_field(get_builtin_class(ShapeId::kShape));
        }
        next_shape.set_parent(target_shape);
        next_shape.set_keys(concat_tuple_value(thread, target_shape.keys(), encoded));
        next_shape.set_additions(create_tuple(thread, 0));
        register_shape(next_shape);

        // add new shape to additions table of previous base
        target_shape.set_additions(
          concat_tuple_value(thread, additions, create_tuple(thread, { encoded, next_shape })));
      }
    }

    target_shape = next_shape;
  }

  return target_shape;
}

RawShape Runtime::create_shape(Thread* thread,
                               RawValue parent,
                               std::initializer_list<std::tuple<std::string, uint8_t>> keys) {
  auto key_tuple = create_tuple(thread, keys.size());
  for (size_t i = 0; i < keys.size(); i++) {
    auto& entry = std::data(keys)[i];
    auto& name = std::get<0>(entry);
    auto flags = std::get<1>(entry);
    key_tuple.set_field_at(i, RawShape::encode_shape_key(declare_symbol(thread, name), flags));
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

RawTuple Runtime::concat_tuple_value(Thread* thread, RawTuple left, RawValue value) {
  uint32_t left_size = left.size();
  uint64_t new_size = left_size + 1;
  CHECK(new_size <= kInt32Max);

  auto new_tuple = create_tuple(thread, new_size);
  for (uint32_t i = 0; i < left_size; i++) {
    new_tuple.set_field_at(i, left.field_at(i));
  }
  new_tuple.set_field_at(new_size - 1, value);

  return new_tuple;
}

RawFunction Runtime::create_function(Thread* thread,
                                     RawValue context,
                                     SharedFunctionInfo* shared_info,
                                     RawValue saved_self) {
  RawFunction func = RawFunction::cast(create_instance(thread, get_builtin_class(ShapeId::kFunction)));
  func.set_name(RawSymbol::make(shared_info->name_symbol));
  func.set_context(context);
  func.set_saved_self(saved_self);
  func.set_host_class(kNull);
  func.set_overload_table(kNull);
  func.set_shared_info(shared_info);
  return func;
}

RawBuiltinFunction Runtime::create_builtin_function(Thread* thread,
                                                    BuiltinFunctionType function,
                                                    SYMBOL name,
                                                    uint8_t argc) {
  RawValue inst = create_instance(thread, get_builtin_class(ShapeId::kBuiltinFunction));
  RawBuiltinFunction func = RawBuiltinFunction::cast(inst);
  func.set_function(function);
  func.set_name(RawSymbol::make(name));
  func.set_argc(argc);
  return func;
}

RawFiber Runtime::create_fiber(Thread* thread, RawFunction function, RawValue self, RawValue arguments) {
  RawFiber fiber = RawFiber::cast(create_instance(thread, get_builtin_class(ShapeId::kFiber)));
  Thread* fiber_thread = scheduler()->get_free_thread();
  fiber_thread->init_fiber_thread(fiber);
  fiber.set_thread(fiber_thread);
  fiber.set_function(function);
  fiber.set_context(self);
  fiber.set_arguments(arguments);
  fiber.set_result_future(create_future(thread));

  // schedule the fiber for execution
  fiber_thread->ready();
  scheduler()->schedule_thread(fiber_thread, thread->worker()->processor());

  return fiber;
}

RawFuture Runtime::create_future(Thread* thread) {
  RawFuture future = RawFuture::cast(create_instance(thread, get_builtin_class(ShapeId::kFuture)));
  future.set_wait_queue(new std::vector<Thread*>());
  future.set_result(kNull);
  future.set_exception(kNull);
  return future;
}

RawValue Runtime::create_exception(Thread* thread, RawValue value) {
  if (value.isString()) {
    auto instance = RawException::cast(create_instance(thread, get_builtin_class(ShapeId::kException)));
    instance.set_message(RawString::cast(value));
    instance.set_stack_trace(create_stack_trace(thread));
    instance.set_cause(kNull);
    return instance;
  } else if (value.isException()) {
    return value;
  }

  thread->throw_value(
    this->create_string_from_template(thread, "expected thrown value to be an exception or a string"));
  return kErrorException;
}

RawImportException Runtime::create_import_exception(Thread* thread,
                                                    const std::string& module_path,
                                                    const ref<compiler::CompilationUnit>& unit) {
  const auto& messages = unit->console.messages();
  size_t size = messages.size();
  auto error_tuple = create_tuple(thread, size);
  for (size_t i = 0; i < size; i++) {
    auto& msg = messages[i];
    std::string type;
    switch (msg.type) {
      case compiler::DiagnosticType::Info: type = "info"; break;
      case compiler::DiagnosticType::Warning: type = "warning"; break;
      case compiler::DiagnosticType::Error: type = "error"; break;
    }
    auto filepath = msg.filepath;
    auto message = msg.message;

    utils::Buffer location_buffer;
    utils::Buffer annotated_source_buffer;
    if (msg.location.valid) {
      location_buffer << msg.location;
      unit->console.write_annotated_source(annotated_source_buffer, msg);
    }

    auto message_tuple = create_tuple(thread, 5);
    message_tuple.set_field_at(0, create_string(thread, type));                     // type
    message_tuple.set_field_at(1, create_string(thread, filepath));                 // filepath
    message_tuple.set_field_at(2, create_string(thread, message));                  // message
    message_tuple.set_field_at(3, create_string(thread, annotated_source_buffer));  // source
    message_tuple.set_field_at(4, create_string(thread, location_buffer));          // location
    error_tuple.set_field_at(i, message_tuple);
  }

  // allocate exception instance
  auto instance = RawImportException::cast(create_instance(thread, get_builtin_class(ShapeId::kImportException)));
  instance.set_message(create_string_from_template(thread, "Encountered an error while importing '%'", module_path));
  instance.set_stack_trace(create_stack_trace(thread));
  instance.set_cause(kNull);
  instance.set_errors(error_tuple);

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

RawValue Runtime::await_future(Thread* thread, RawFuture _future) {
  HandleScope scope(thread);
  Future future(scope, _future);

  {
    std::unique_lock lock(future);

    // wait for the future to finish
    if (!future.has_finished()) {
      auto wait_queue = future.wait_queue();
      wait_queue->push_back(thread);
      thread->m_state.acas(Thread::State::Running, Thread::State::Waiting);

      lock.unlock();
      thread->enter_scheduler();
      lock.lock();
    }
  }

  if (!future.exception().isNull()) {
    thread->throw_value(future.exception());
    return kErrorException;
  }
  return future.result();
}

RawValue Runtime::await_fiber(Thread* thread, RawFiber fiber) {
  return await_future(thread, fiber.result_future());
}

RawValue Runtime::resolve_future(Thread* thread, RawFuture future, RawValue result) {
  auto runtime = thread->runtime();

  {
    std::lock_guard lock(future);
    auto wait_queue = future.wait_queue();
    if (wait_queue == nullptr) {
      thread->throw_value(runtime->create_string_from_template(thread, "Future has already completed"));
      return kErrorException;
    }

    future.set_result(result);
    future.set_exception(kNull);
    future_wake_waiting_threads(thread, future);
  }

  return future;
}

RawValue Runtime::reject_future(Thread* thread, RawFuture future, RawException exception) {
  auto runtime = thread->runtime();

  {
    std::lock_guard lock(future);
    auto wait_queue = future.wait_queue();
    if (wait_queue == nullptr) {
      thread->throw_value(runtime->create_string_from_template(thread, "Future has already completed"));
      return kErrorException;
    }

    future.set_result(kNull);
    future.set_exception(exception);
    future_wake_waiting_threads(thread, future);
  }

  return future;
}

void Runtime::future_wake_waiting_threads(Thread* thread, RawFuture future) {
  DCHECK(future.is_locked());
  auto runtime = thread->runtime();
  auto worker = thread->worker();
  auto scheduler = runtime->scheduler();
  auto processor = worker->processor();

  auto wait_queue = future.wait_queue();
  for (Thread* waiting_thread : *wait_queue) {
    DCHECK(waiting_thread->state() == Thread::State::Waiting);
    waiting_thread->ready();
    scheduler->schedule_thread(waiting_thread, processor);
  }
  wait_queue->clear();
  delete wait_queue;
  future.set_wait_queue(nullptr);
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

RawShape Runtime::lookup_shape(ShapeId id) {
  std::shared_lock<std::shared_mutex> lock(m_shapes_mutex);
  auto index = static_cast<size_t>(id);
  CHECK(index < m_shapes.size());
  return RawShape::cast(m_shapes.at(index));
}

RawClass Runtime::lookup_class(RawValue value) {
  if (value.isInstance()) {
    auto instance = RawInstance::cast(value);
    auto klass_field = instance.klass_field();
    DCHECK(!klass_field.isNull());
    return RawClass::cast(klass_field);
  }

  return get_builtin_class(value.shape_id());
}

RawValue Runtime::lookup_symbol(SYMBOL symbol) {
  std::lock_guard<std::mutex> locker(m_symbols_mutex);

  if (m_symbol_table.count(symbol)) {
    return m_symbol_table.at(symbol);
  }

  return kNull;
}

bool Runtime::builtin_class_is_registered(ShapeId shape_id) {
  std::shared_lock<std::shared_mutex> lock(m_shapes_mutex);
  auto offset = static_cast<uint32_t>(shape_id);
  DCHECK(offset < kBuiltinClassCount);
  return !m_builtin_classes.at(offset).isNull();
}

void Runtime::set_builtin_class(ShapeId shape_id, RawClass klass) {
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

RawClass Runtime::get_builtin_class(ShapeId shape_id) {
  std::shared_lock<std::shared_mutex> lock(m_shapes_mutex);
  auto offset = static_cast<uint32_t>(shape_id);
  DCHECK(offset < kBuiltinClassCount);
  return RawClass::cast(m_builtin_classes.at(offset));
}

uint32_t Runtime::check_private_access_permitted(Thread* thread, RawInstance value) {
  // a given value can read the private members of another value if either:
  // - they are the same value
  // - the class of the reader and the accessed klass share a common ancestor

  RawValue self = thread->frame()->self;
  RawClass self_class = lookup_class(self);
  if (self == value) {
    return self_class.shape_instance().keys().size();
  }

  RawClass other_class = lookup_class(value);
  if (self_class == other_class) {
    return self_class.shape_instance().keys().size();
  }

  RawTuple self_ancestors = self_class.ancestor_table();
  RawTuple other_ancestors = other_class.ancestor_table();

  uint32_t min_ancestor = std::min(self_ancestors.size(), other_ancestors.size());
  CHECK(min_ancestor >= 1, "expected at least one common class");
  uint32_t highest_allowed_private_member = 0;
  for (uint32_t i = 0; i < min_ancestor; i++) {
    auto ancestor_self = self_ancestors.field_at<RawClass>(i);
    auto ancestor_other = other_ancestors.field_at<RawClass>(i);
    if (ancestor_self == ancestor_other) {
      highest_allowed_private_member = ancestor_self.shape_instance().keys().size();
    }
  }

  return highest_allowed_private_member;
}

std::optional<fs::path> Runtime::resolve_module(const fs::path& module_path, const fs::path& origin_path) const {
  if (m_builtin_libraries_paths.count(module_path)) {
    return m_builtin_libraries_paths.at(module_path);
  } else if (module_path.is_absolute()) {
    return module_path;
  } else {
    CHECK(origin_path.has_filename() && origin_path.has_parent_path() && origin_path.is_absolute(),
          "malformed origin path");
    auto origin_directory = origin_path.parent_path();
    auto search_directory = origin_directory;

    // search for the module by traversing the filesystem hierarchy
    for (;;) {
      if (module_path.has_extension()) {
        fs::path search_path = search_directory / module_path;
        if (fs::is_regular_file(search_path)) {
          return search_path;
        }
      } else {
        fs::path search_path_direct = search_directory / module_path;
        fs::path search_path_with_extension = search_path_direct;
        fs::path search_path_module = search_path_direct / "index.ch";
        search_path_with_extension.replace_extension("ch");

        if (fs::is_regular_file(search_path_direct)) {
          return search_path_direct;
        }

        if (fs::is_regular_file(search_path_with_extension)) {
          return search_path_with_extension;
        }

        if (fs::is_regular_file(search_path_module)) {
          return search_path_module;
        }
      }

      // exit if we just searched the root directory
      if (search_directory == "/") {
        break;
      }

      // next higher directory
      auto next_search_directory = search_directory.parent_path();
      search_directory = next_search_directory;
    }
  }

  return std::nullopt;
}

RawValue Runtime::import_module(Thread* thread, const fs::path& path, bool treat_as_repl) {
  std::unique_lock lock(m_cached_modules_mutex);
  fs::file_time_type mtime = fs::last_write_time(path);

  // check if module cache has an entry for this module
  auto path_hash = fs::hash_value(path);
  if (m_cached_modules.count(path_hash)) {
    const auto& entry = m_cached_modules.at(path_hash);

    // file is unchanged since last import
    if (fs::last_write_time(path) == entry.mtime) {
      lock.unlock();
      auto result = await_future(thread, entry.module);
      return result;
    }
  }

  HandleScope scope(thread);
  Future future(scope, create_future(thread));
  m_cached_modules[path_hash] = { .path = path, .mtime = mtime, .module = future };

  lock.unlock();

  std::ifstream import_source(path);
  if (!import_source.is_open()) {
    auto exception =
      create_exception(thread, create_string_from_template(thread, "Could not open the file at '%'", path));
    thread->throw_value(exception);
    reject_future(thread, future, exception);
    return kErrorException;
  }

  utils::Buffer buf;
  buf << import_source.rdbuf();
  import_source.close();

  double start_time = (double)get_steady_timestamp_micro() / 1000;
  auto compilation_type = treat_as_repl ? CompilationUnit::Type::ReplInput : CompilationUnit::Type::Module;
  auto unit = Compiler::compile(path, buf, compilation_type);
  if (unit->console.has_errors()) {
    auto exception = create_import_exception(thread, path, unit);
    thread->throw_value(exception);
    reject_future(thread, future, exception);
    return kErrorException;
  }
  double end_time = (double)get_steady_timestamp_micro() / 1000;
  debugln("compiled module at % in %ms", path, end_time - start_time);

  if (utils::ArgumentParser::is_flag_set("skipexec")) {
    resolve_future(thread, future, kNull);
    return kNull;
  }

  auto module = unit->compiled_module;
  CHECK(!module->function_table.empty());
  register_module(thread, module);
  auto module_function = create_function(thread, kNull, module->function_table.front());
  auto module_fiber = create_fiber(thread, module_function, kNull, kNull);

  auto rval = await_fiber(thread, module_fiber);
  if (rval.is_error_exception()) {
    reject_future(thread, future, RawException::cast(thread->pending_exception()));
    return kErrorException;
  }

  resolve_future(thread, future, rval);
  return rval;
}

const fs::path& Runtime::source_code_directory() const {
  return m_source_code_directory;
}

const fs::path& Runtime::stdlib_directory() const {
  return m_stdlib_directory;
}

}  // namespace charly::core::runtime
