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
  m_heap(std::make_unique<Heap>(this)),
  m_gc(std::make_unique<GarbageCollector>(this)),
  m_scheduler(std::make_unique<Scheduler>(this)) {
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
  declare_symbol(thread, "??");
  declare_symbol(thread, "charly.baseclass");
  declare_symbol(thread, "CHARLY_STDLIB");
  declare_symbol(thread, "klass");
  declare_symbol(thread, "length");
  declare_symbol(thread, "ARGV");

  CHECK(declare_global_variable(thread, SYM("CHARLY_STDLIB"), true, RawString::create(thread, m_stdlib_directory))
          .is_error_ok());
}

void Runtime::initialize_argv_tuple(Thread* thread) {
  HandleScope scope(thread);

  auto& argv = utils::ArgumentParser::USER_FLAGS;
  Tuple argv_tuple(scope, RawTuple::create(thread, argv.size()));
  for (uint32_t i = 0; i < argv.size(); i++) {
    const std::string& arg = argv[i];
    auto arg_string = RawString::create(thread, arg.data(), arg.size(), crc32::hash_string(arg));
    argv_tuple.set_field_at(i, arg_string);
  }
  CHECK(declare_global_variable(thread, SYM("ARGV"), true, argv_tuple).is_error_ok());
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
  RawShape builtin_shape_immediate =
    RawShape::unsafe_cast(RawInstance::create(thread, ShapeId::kShape, RawShape::kFieldCount));
  builtin_shape_immediate.set_parent(kNull);
  builtin_shape_immediate.set_keys(RawTuple::create_empty(thread));
  builtin_shape_immediate.set_additions(RawTuple::create_empty(thread));
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
    RawShape::create(thread, builtin_shape_immediate, { { "klass", RawShape::kKeyFlagInternal } });

  auto builtin_shape_huge_bytes = RawShape::create(thread, builtin_shape_immediate,
                                                   { { "__charly_huge_bytes_klass", RawShape::kKeyFlagInternal },
                                                     { "data", RawShape::kKeyFlagInternal },
                                                     { "length", RawShape::kKeyFlagInternal } });

  auto builtin_shape_huge_string =
    RawShape::create(thread, builtin_shape_immediate,
                     { { "data", RawShape::kKeyFlagInternal }, { "length", RawShape::kKeyFlagInternal } });

  auto builtin_shape_class = RawShape::create(thread, builtin_shape_immediate,
                                              { { "__charly_class_klass", RawShape::kKeyFlagInternal },
                                                { "flags", RawShape::kKeyFlagInternal },
                                                { "ancestor_table", RawShape::kKeyFlagReadOnly },
                                                { "name", RawShape::kKeyFlagReadOnly },
                                                { "parent", RawShape::kKeyFlagReadOnly },
                                                { "shape", RawShape::kKeyFlagReadOnly },
                                                { "function_table", RawShape::kKeyFlagReadOnly },
                                                { "constructor", RawShape::kKeyFlagReadOnly } });

  auto builtin_shape_shape = RawShape::create(thread, builtin_shape_immediate,
                                              { { "__charly_shape_klass", RawShape::kKeyFlagInternal },
                                                { "id", RawShape::kKeyFlagReadOnly },
                                                { "parent", RawShape::kKeyFlagReadOnly },
                                                { "keys", RawShape::kKeyFlagReadOnly },
                                                { "additions", RawShape::kKeyFlagReadOnly } });

  auto builtin_shape_function = RawShape::create(thread, builtin_shape_immediate,
                                                 { { "__charly_function_klass", RawShape::kKeyFlagInternal },
                                                   { "name", RawShape::kKeyFlagReadOnly },
                                                   { "context", RawShape::kKeyFlagReadOnly },
                                                   { "saved_self", RawShape::kKeyFlagReadOnly },
                                                   { "host_class", RawShape::kKeyFlagReadOnly },
                                                   { "overload_table", RawShape::kKeyFlagReadOnly },
                                                   { "shared_info", RawShape::kKeyFlagInternal } });

  auto builtin_shape_builtin_function =
    RawShape::create(thread, builtin_shape_immediate,
                     { { "__charly_builtin_function_klass", RawShape::kKeyFlagInternal },
                       { "function", RawShape::kKeyFlagReadOnly },
                       { "name", RawShape::kKeyFlagReadOnly },
                       { "argc", RawShape::kKeyFlagReadOnly } });

  auto builtin_shape_fiber = RawShape::create(thread, builtin_shape_immediate,
                                              { { "__charly_fiber_klass", RawShape::kKeyFlagInternal },
                                                { "thread", RawShape::kKeyFlagInternal },
                                                { "function", RawShape::kKeyFlagReadOnly },
                                                { "context", RawShape::kKeyFlagReadOnly },
                                                { "arguments", RawShape::kKeyFlagReadOnly },
                                                { "result_future", RawShape::kKeyFlagReadOnly } });

  auto builtin_shape_future = RawShape::create(thread, builtin_shape_immediate,
                                               { { "__charly_future_klass", RawShape::kKeyFlagInternal },
                                                 { "wait_queue", RawShape::kKeyFlagInternal },
                                                 { "result", RawShape::kKeyFlagReadOnly },
                                                 { "exception", RawShape::kKeyFlagReadOnly } });

  auto builtin_shape_exception = RawShape::create(thread, builtin_shape_instance,
                                                  { { "message", RawShape::kKeyFlagNone },
                                                    { "stack_trace", RawShape::kKeyFlagNone },
                                                    { "cause", RawShape::kKeyFlagReadOnly } });

  auto builtin_shape_import_exception =
    RawShape::create(thread, builtin_shape_exception, { { "errors", RawShape::kKeyFlagReadOnly } });

  auto builtin_shape_assertion_exception = RawShape::create(thread, builtin_shape_exception,
                                                            { { "left_hand_side", RawShape::kKeyFlagReadOnly },
                                                              { "right_hand_side", RawShape::kKeyFlagReadOnly },
                                                              { "operation_name", RawShape::kKeyFlagReadOnly } });

  // patch shapes table and assign correct shape ids to shape instances
  register_shape(ShapeId::kSmallString, builtin_shape_immediate);
  register_shape(ShapeId::kLargeString, builtin_shape_immediate);
  register_shape(ShapeId::kSmallBytes, builtin_shape_immediate);
  register_shape(ShapeId::kLargeBytes, builtin_shape_immediate);
  register_shape(ShapeId::kInstance, builtin_shape_instance);
  register_shape(ShapeId::kHugeBytes, builtin_shape_huge_bytes);
  register_shape(ShapeId::kHugeString, builtin_shape_huge_string);
  register_shape(ShapeId::kClass, builtin_shape_class);
  register_shape(ShapeId::kShape, builtin_shape_shape);
  register_shape(ShapeId::kFunction, builtin_shape_function);
  register_shape(ShapeId::kBuiltinFunction, builtin_shape_builtin_function);
  register_shape(ShapeId::kFiber, builtin_shape_fiber);
  register_shape(ShapeId::kFuture, builtin_shape_future);
  register_shape(ShapeId::kException, builtin_shape_exception);
  register_shape(ShapeId::kImportException, builtin_shape_import_exception);
  register_shape(ShapeId::kAssertionException, builtin_shape_assertion_exception);

  RawShape class_value_shape = builtin_shape_class;
  RawClass class_value = RawClass::unsafe_cast(RawInstance::create(thread, builtin_shape_class));
  class_value.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);
  class_value.set_ancestor_table(RawTuple::create_empty(thread));
  class_value.set_name(RawSymbol::create(declare_symbol(thread, "Value")));
  class_value.set_parent(kNull);
  class_value.set_shape_instance(builtin_shape_value);
  class_value.set_function_table(RawTuple::create_empty(thread));
  class_value.set_constructor(kNull);

#define DEFINE_BUILTIN_CLASS(S, N, P, F)                                                      \
  auto class_##S##_shape = builtin_shape_class;                                               \
  RawClass class_##S = RawClass::unsafe_cast(RawInstance::create(thread, class_##S##_shape)); \
  class_##S.set_flags(F);                                                                     \
  class_##S.set_ancestor_table(RawTuple::concat_value(thread, P.ancestor_table(), P));        \
  class_##S.set_name(RawSymbol::create(declare_symbol(thread, #N)));                          \
  class_##S.set_parent(P);                                                                    \
  class_##S.set_shape_instance(builtin_shape_##S);                                            \
  class_##S.set_function_table(RawTuple::create_empty(thread));                               \
  class_##S.set_constructor(kNull);

  DEFINE_BUILTIN_CLASS(number, Number, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(int, Int, class_number, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(float, Float, class_number, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(bool, Bool, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(symbol, Symbol, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(null, Null, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(string, String, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(bytes, Bytes, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(tuple, Tuple, class_value, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(instance, Instance, class_value, RawClass::kFlagNone)
  DEFINE_BUILTIN_CLASS(class, Class, class_instance, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(shape, Shape, class_instance, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(function, Function, class_instance, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(builtin_function, BuiltinFunction, class_instance,
                       RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(fiber, Fiber, class_instance, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(future, Future, class_instance, RawClass::kFlagFinal | RawClass::kFlagNonConstructable)
  DEFINE_BUILTIN_CLASS(exception, Exception, class_instance, RawClass::kFlagNone)
  DEFINE_BUILTIN_CLASS(import_exception, ImportException, class_exception, RawClass::kFlagFinal)
  DEFINE_BUILTIN_CLASS(assertion_exception, AssertionException, class_exception, RawClass::kFlagFinal)
#undef DEFINE_BUILTIN_CLASS

// define the static classes for the builtin classes
#define DEFINE_STATIC_CLASS(S, N)                                                                                 \
  auto static_class_##S =                                                                                         \
    RawClass::cast(RawInstance::create(thread, ShapeId::kClass, RawClass::kFieldCount, class_class));             \
  static_class_##S.set_flags(RawClass::kFlagFinal | RawClass::kFlagNonConstructable);                             \
  static_class_##S.set_ancestor_table(RawTuple::concat_value(thread, class_class.ancestor_table(), class_class)); \
  static_class_##S.set_name(RawSymbol::create(declare_symbol(thread, #N)));                                       \
  static_class_##S.set_parent(class_class);                                                                       \
  static_class_##S.set_shape_instance(class_##S##_shape);                                                         \
  static_class_##S.set_function_table(RawTuple::create_empty(thread));                                            \
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
  DEFINE_STATIC_CLASS(assertion_exception, AssertionException)
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
  class_assertion_exception.set_klass_field(static_class_assertion_exception);

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
  set_builtin_class(ShapeId::kAssertionException, class_assertion_exception);

  // patch klass field of all shape instances created up until this point
  for (auto entry : m_shapes) {
    if (!entry.isNull()) {
      auto shape = RawShape::cast(entry);

      if (shape.klass_field().isNull()) {
        shape.set_klass_field(get_builtin_class(ShapeId::kShape));
        DCHECK(shape.klass_field().isClass());
      }
    }
  }

  // validate shape ids
  {
    size_t begin_index = static_cast<size_t>(ShapeId::kFirstBuiltinShapeId);
    size_t end_index = static_cast<size_t>(ShapeId::kFirstUserDefinedShapeId);

    auto begin = m_shapes.begin();
    auto it = std::next(begin, begin_index);
    auto end_it = std::next(begin, end_index);

    size_t index = begin_index;
    for (; it != end_it; it++) {
      auto shape = RawShape::cast(*it);
      DCHECK(shape.own_shape_id() == static_cast<ShapeId>(index));
      index++;
    }
  }

  // register builtin classes as global variables
  CHECK(declare_global_variable(thread, SYM("Value"), true, class_value).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Number"), true, class_number).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Int"), true, class_int).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Float"), true, class_float).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Bool"), true, class_bool).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Symbol"), true, class_symbol).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Null"), true, class_null).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("String"), true, class_string).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Bytes"), true, class_bytes).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Tuple"), true, class_tuple).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Instance"), true, class_instance).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Class"), true, class_class).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Shape"), true, class_shape).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Function"), true, class_function).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("BuiltinFunction"), true, class_builtin_function).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Fiber"), true, class_fiber).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Future"), true, class_future).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("Exception"), true, class_exception).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("ImportException"), true, class_import_exception).is_error_ok());
  CHECK(declare_global_variable(thread, SYM("AssertionException"), true, class_assertion_exception).is_error_ok());
}

void Runtime::initialize_stdlib_paths() {
  auto CHARLYVMDIR = utils::ArgumentParser::get_environment_for_key("CHARLYVMDIR");
  CHECK(CHARLYVMDIR.has_value());
  m_stdlib_directory = fs::path(CHARLYVMDIR.value()) / "src" / "charly" / "stdlib";
  m_builtin_libraries_paths["testlib"] = m_stdlib_directory / "libs" / "testlib.ch";
}

RawValue Runtime::declare_global_variable(Thread*, SYMBOL name, bool constant, RawValue value) {
  std::unique_lock<std::shared_mutex> locker(m_globals_mutex);

  if (m_global_variables.count(name) == 1) {
    return kErrorException;
  }

  m_global_variables[name] = { value, constant };
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
    return kErrorReadOnly;
  }

  var.value = value;
  return kErrorOk;
}

SYMBOL Runtime::declare_symbol(Thread* thread, const char* data, size_t size) {
  std::lock_guard<std::mutex> locker(m_symbols_mutex);

  SYMBOL symbol = crc32::hash_block(data, size);

  if (m_symbol_table.count(symbol) > 0) {
    return symbol;
  }

  m_symbol_table[symbol] = RawString::create(thread, data, size, symbol);
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
  auto offset = static_cast<uint32_t>(shape_id);
  DCHECK(shape_id <= ShapeId::kLastBuiltinShapeId);
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
  RawClass self_class = self.klass(thread);
  if (self == value) {
    return lookup_shape(self.shape_id()).keys().size();
  }

  RawClass other_class = value.klass(thread);
  if (self_class == other_class) {
    return lookup_shape(value.shape_id()).keys().size();
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

  return {};
}

RawValue Runtime::import_module_at_path(Thread* thread, const fs::path& path, bool treat_as_repl) {
  std::unique_lock lock(m_cached_modules_mutex);

  auto path_hash = fs::hash_value(path);
  std::ifstream import_source(path);
  if (!import_source.is_open()) {
    if (m_cached_modules.count(path_hash)) {
      m_cached_modules.erase(path_hash);
    }

    return thread->throw_message("Could not open the file at '%'", path);
  }

  fs::file_time_type mtime = fs::last_write_time(path);

  if (m_cached_modules.count(path_hash)) {
    const auto& entry = m_cached_modules.at(path_hash);
    if (mtime == entry.mtime) {
      lock.unlock();
      return entry.module.await(thread);
    }
  }

  HandleScope scope(thread);
  Future future(scope, RawFuture::create(thread));
  m_cached_modules[path_hash] = { .path = path, .mtime = mtime, .module = future };

  lock.unlock();

  utils::Buffer buf;
  buf << import_source.rdbuf();
  import_source.close();

  auto compilation_type = treat_as_repl ? CompilationUnit::Type::ReplInput : CompilationUnit::Type::Module;
  auto unit = Compiler::compile(path, buf, compilation_type);
  if (unit->console.has_errors()) {
    ImportException exception(scope, RawImportException::create(thread, path, unit));
    future.reject(thread, exception);
    return thread->throw_exception(exception);
  }

  if (utils::ArgumentParser::is_flag_set("skipexec")) {
    future.resolve(thread, kNull);
    return kNull;
  }

  auto module = unit->compiled_module;
  CHECK(!module->function_table.empty());
  register_module(thread, module);

  Function module_function(scope, RawFunction::create(thread, kNull, module->function_table.front()));
  Fiber module_fiber(scope, RawFiber::create(thread, module_function, kNull, kNull));

  Value rval(scope, module_fiber.await(thread));
  if (rval.is_error_exception()) {
    future.reject(thread, RawException::cast(thread->pending_exception()));
    return kErrorException;
  }

  future.resolve(thread, rval);
  return rval;
}

const fs::path& Runtime::stdlib_directory() const {
  return m_stdlib_directory;
}

void Runtime::each_root(std::function<void(RawValue& value)> callback) {
  // processor symbol tables
  for (auto* proc : m_scheduler->m_processors) {
    for (auto& entry : proc->m_symbol_table) {
      callback(entry.second);
    }
  }

  // thread handle scopes and stackframes
  for (auto* thread : m_scheduler->m_threads) {
    callback(thread->m_fiber);
    callback(thread->m_pending_exception);

    auto* handle = thread->handles()->head();
    while (handle) {
      callback(*handle);
      handle = handle->next();
    }

    auto* frame = thread->frame();
    while (frame) {
      callback(frame->self);
      // frame->arguments pointer is not explicitly traversed
      // referenced objects are already traversed via either stack traversal or frame->argument_tuple
      callback(frame->argument_tuple);

      if (frame->is_interpreter_frame()) {
        auto* interpreter_frame = static_cast<InterpreterFrame*>(frame);
        callback(interpreter_frame->function);
        callback(interpreter_frame->context);
        callback(interpreter_frame->return_value);

        const auto* shared_info = interpreter_frame->shared_function_info;
        DCHECK(shared_info);

        uint8_t locals = shared_info->ir_info.local_variables;
        DCHECK(interpreter_frame->locals);
        for (uint8_t li = 0; li < locals; li++) {
          callback(interpreter_frame->locals[li]);
        }

        uint8_t stacksize = shared_info->ir_info.stacksize;
        DCHECK(interpreter_frame->stack);
        for (uint8_t si = 0; si < stacksize && si < interpreter_frame->sp; si++) {
          callback(interpreter_frame->stack[si]);
        }
      } else {
        auto* builtin_frame = static_cast<BuiltinFrame*>(frame);
        callback(builtin_frame->function);
      }

      frame = frame->parent;
    }
  }

  // runtime symbol table
  for (auto& entry : m_symbol_table) {
    callback(entry.second);
  }

  // runtime shape table
  for (auto& shape : m_shapes) {
    callback(shape);
  }

  // runtime class table
  for (auto& klass : m_builtin_classes) {
    callback(klass);
  }

  // global variables
  for (auto& entry : m_global_variables) {
    callback(entry.second.value);
  }

  // cached modules table
  for (auto& entry : m_cached_modules) {
    callback(entry.second.module);
  }
}

}  // namespace charly::core::runtime
