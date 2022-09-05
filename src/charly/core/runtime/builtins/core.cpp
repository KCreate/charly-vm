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

#include <filesystem>
#include <fstream>

#include "charly/core/compiler/compiler.h"
#include "charly/core/runtime/builtins/core.h"
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"

namespace charly::core::runtime::builtin::core {

namespace fs = std::filesystem;
using namespace charly::core::compiler;
using namespace std::chrono_literals;

void initialize(Thread* thread) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  DEF_BUILTIN_CORE(REGISTER_BUILTIN_FUNCTION)
}

RawValue currentfiber(Thread* thread, BuiltinFrame*) {
  return thread->fiber();
}

RawValue transplantbuiltinclass(Thread* thread, BuiltinFrame* frame) {
  Runtime* runtime = thread->runtime();

  HandleScope scope(thread);
  Class klass(scope, frame->arguments[0]);
  Class static_class(scope, klass.klass(thread));
  Class donor_class(scope, frame->arguments[1]);
  Class static_donor_class(scope, donor_class.klass(thread));

  if (!is_builtin_shape(klass.shape_instance().own_shape_id())) {
    return thread->throw_message("Expected base class to be a builtin class");
  }

  if (klass.function_table().length()) {
    return thread->throw_message("Expected base class function table to be empty");
  }

  if (donor_class.parent() != runtime->get_builtin_class(ShapeId::kInstance)) {
    return thread->throw_message("The donor class shall not be a subclass");
  }

  if (donor_class.shape_instance() != runtime->lookup_shape(ShapeId::kInstance)) {
    return thread->throw_message("The donor class shall not declare any new properties");
  }

  auto donor_constructor = donor_class.constructor();
  if (donor_constructor.isFunction()) {
    auto constructor = RawFunction::cast(donor_constructor);
    constructor.set_host_class(klass);
  }
  klass.set_constructor(donor_constructor);

  klass.set_function_table(donor_class.function_table());
  for (uint32_t i = 0; i < klass.function_table().length(); i++) {
    auto method = klass.function_table().field_at<RawFunction>(i);
    auto overload_table = method.overload_table();
    if (overload_table.isTuple()) {
      auto overload_tuple = RawTuple::cast(overload_table);
      for (uint32_t j = 0; j < overload_tuple.length(); j++) {
        auto overloaded_method = overload_tuple.field_at<RawFunction>(j);
        overloaded_method.set_host_class(klass);
      }
    } else {
      method.set_host_class(klass);
    }
  }
  donor_class.set_flags(RawClass::kFlagNonConstructable | RawClass::kFlagFinal);
  donor_class.set_constructor(kNull);
  donor_class.set_function_table(RawTuple::create_empty(thread));

  auto builtin_class_class = runtime->get_builtin_class(ShapeId::kClass);
  if (static_donor_class != builtin_class_class) {
    if (static_class.function_table().length()) {
      return thread->throw_message("Expected base static class function table to be empty");
    }

    if (static_donor_class.shape_instance() != runtime->lookup_shape(ShapeId::kClass)) {
      return thread->throw_message("The donor class shall not declare any new static properties");
    }

    static_class.set_function_table(static_donor_class.function_table());
    static_donor_class.set_function_table(RawTuple::create_empty(thread));
    for (uint32_t i = 0; i < static_class.function_table().length(); i++) {
      auto method = static_class.function_table().field_at<RawFunction>(i);
      auto overload_table = method.overload_table();
      if (overload_table.isTuple()) {
        auto overload_tuple = RawTuple::cast(overload_table);
        for (uint32_t j = 0; j < overload_tuple.length(); j++) {
          auto overloaded_method = overload_tuple.field_at<RawFunction>(j);
          overloaded_method.set_host_class(static_class);
        }
      } else {
        method.set_host_class(static_class);
      }
    }
  }

  return kNull;
}

RawValue writevalue(Thread*, BuiltinFrame* frame) {
  for (size_t i = 0; i < frame->argc; i++) {
    frame->arguments[i].to_string(std::cout);
    if (i < frame->argc - 1) {
      std::cout << " ";
    }
  }

  return kNull;
}

RawValue currentworkingdirectory(Thread* thread, BuiltinFrame*) {
  fs::path cwd = fs::current_path();
  return RawString::create(thread, cwd);
}

RawValue getbacktrace(Thread* thread, BuiltinFrame*) {
  return thread->create_backtrace();
}

RawValue disassemble(Thread*, BuiltinFrame* frame) {
  DCHECK(frame->arguments[0].isFunction());

  auto func = RawFunction::cast(frame->arguments[0]);
  SharedFunctionInfo* info = func.shared_info();
  info->dump(std::cout);

  return kNull;
}

RawValue createtuple(Thread* thread, BuiltinFrame* frame) {
  DCHECK(frame->arguments[0].isNumber());

  int64_t length = frame->arguments[0].int_value();
  if (length < 0) {
    return thread->throw_message("Expected length to be positive, got %", length);
  }

  if ((size_t)length > kHeapRegionMaximumObjectFieldCount) {
    return thread->throw_message("Expected length to be smaller than the maximum tuple capacity of %, got %",
                                 kHeapRegionMaximumObjectFieldCount, length);
  }

  HandleScope scope(thread);
  Value initial(scope, frame->arguments[1]);
  Tuple tuple(scope, RawTuple::create(thread, length));
  for (int64_t i = 0; i < length; i++) {
    tuple.set_field_at(i, initial);
  }

  return tuple;
}

RawValue createtuplewith(Thread* thread, BuiltinFrame* frame) {
  DCHECK(frame->arguments[0].isNumber());
  DCHECK(frame->arguments[1].isFunction());

  int64_t length = frame->arguments[0].int_value();
  if (length < 0) {
    return thread->throw_message("Expected length to be positive, got %", length);
  }

  if ((size_t)length > kHeapRegionMaximumObjectFieldCount) {
    return thread->throw_message("Expected length to be smaller than the maximum tuple capacity of %, got %",
                                 kHeapRegionMaximumObjectFieldCount, length);
  }

  HandleScope scope(thread);
  Function callback(scope, frame->arguments[1]);
  Tuple tuple(scope, RawTuple::create(thread, length));
  for (int64_t i = 0; i < length; i++) {
    auto index = RawInt::create(i);
    RawValue value = Interpreter::call_function(thread, kNull, callback, &index, 1);

    if (value.is_error_exception()) {
      return kErrorException;
    }

    tuple.set_field_at(i, value);
  }

  return tuple;
}

RawValue exit(Thread* thread, BuiltinFrame* frame) {
  DCHECK(frame->arguments[0].isInt());
  thread->abort((int32_t)RawInt::cast(frame->arguments[0]).value());
}

RawValue performgc(Thread* thread, BuiltinFrame*) {
  thread->runtime()->gc()->perform_gc(thread);
  return kNull;
}

RawValue getsteadytimestampmicro(Thread*, BuiltinFrame*) {
  return RawInt::create(get_steady_timestamp_micro());
}

RawValue compile(Thread* thread, BuiltinFrame* frame) {
  Runtime* runtime = thread->runtime();

  CHECK(frame->arguments[0].isString());
  CHECK(frame->arguments[1].isString());
  RawString source = RawString::cast(frame->arguments[0]);
  std::string name = RawString::cast(frame->arguments[1]).str();

  utils::Buffer buf;
  source.to_string(buf);
  auto unit = Compiler::compile(name, buf, CompilationUnit::Type::ReplInput);

  if (unit->console.has_errors()) {
    return thread->throw_exception(RawImportException::create(thread, name, unit));
  }

  auto module = unit->compiled_module;
  CHECK(!module->function_table.empty());
  runtime->register_module(thread, module);
  return RawFunction::create(thread, kNull, module->function_table.front());
}

}  // namespace charly::core::runtime::builtin::core
