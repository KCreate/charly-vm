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

RawValue currentfiber(Thread* thread, const RawValue*, uint8_t argc) {
  CHECK(argc == 0);
  return thread->fiber();
}

RawValue transplantbuiltinclass(Thread* thread, const RawValue* args, uint8_t argc) {
  Runtime* runtime = thread->runtime();
  CHECK(argc == 2);
  auto klass = RawClass::cast(args[0]);
  auto static_class = runtime->lookup_class(thread, klass);
  auto donor_class = RawClass::cast(args[1]);
  auto static_donor_class = runtime->lookup_class(thread, donor_class);

  if (!is_builtin_shape(klass.shape_instance().own_shape_id())) {
    thread->throw_value(
      runtime->create_exception_with_message(thread, "expected base class to be a builtin class"));
    return kErrorException;
  }

  if (klass.function_table().size()) {
    thread->throw_value(
      runtime->create_exception_with_message(thread, "expected base class function table to be empty"));
    return kErrorException;
  }
  klass.set_constructor(donor_class.constructor());
  klass.set_function_table(donor_class.function_table());
  for (uint32_t i = 0; i < klass.function_table().size(); i++) {
    auto method = klass.function_table().field_at<RawFunction>(i);
    auto overload_table = method.overload_table();
    if (overload_table.isTuple()) {
      auto overload_tuple = RawTuple::cast(overload_table);
      for (uint32_t j = 0; j < overload_tuple.size(); j++) {
        auto overloaded_method = overload_tuple.field_at<RawFunction>(j);
        overloaded_method.set_host_class(klass);
      }
    } else {
      method.set_host_class(klass);
    }
  }
  donor_class.set_flags(RawClass::kFlagNonConstructable | RawClass::kFlagFinal);
  donor_class.set_constructor(kNull);
  donor_class.set_function_table(runtime->create_tuple(thread, 0));

  if (static_class != runtime->get_builtin_class(thread, ShapeId::kClass)) {
    if (static_class.function_table().size()) {
      thread->throw_value(
        runtime->create_exception_with_message(thread, "expected base static class function table to be empty"));
      return kErrorException;
    }
    static_class.set_function_table(static_donor_class.function_table());
    static_donor_class.set_function_table(runtime->create_tuple(thread, 0));
    for (uint32_t i = 0; i < static_class.function_table().size(); i++) {
      auto method = static_class.function_table().field_at<RawFunction>(i);
      auto overload_table = method.overload_table();
      if (overload_table.isTuple()) {
        auto overload_tuple = RawTuple::cast(overload_table);
        for (uint32_t j = 0; j < overload_tuple.size(); j++) {
          auto overloaded_method = overload_tuple.field_at<RawFunction>(j);
          overloaded_method.set_host_class(static_class);
        }
      } else {
        method.set_host_class(static_class);
      }
    }
  }

  return thread->fiber();
}

RawValue writeline(Thread*, const RawValue* args, uint8_t argc) {
  CHECK(argc == 1);
  args[0].to_string(std::cout);
  std::cout << std::endl;
  return kNull;
}

RawValue writevalue(Thread*, const RawValue* args, uint8_t argc) {
  CHECK(argc == 1);
  args[0].to_string(std::cout);
  return kNull;
}

RawValue writevaluesync(Thread*, const RawValue* args, uint8_t argc) {
  CHECK(argc == 1);
  utils::Buffer buf;
  args[0].to_string(buf);
  debuglnf("%", buf);
  return kNull;
}

RawValue readfile(Thread* thread, const RawValue* args, uint8_t argc) {
  Runtime* runtime = thread->runtime();
  CHECK(argc == 1);
  DCHECK(args[0].isString());
  std::string name = RawString::cast(args[0]).str();

  std::ifstream stream(name);
  if (!stream.is_open()) {
    thread->throw_value(
      runtime->create_exception_with_message(thread, "could not open file at path %", name));
    return kErrorException;
  }

  utils::Buffer buffer;
  std::string line;
  while (std::getline(stream, line)) {
    buffer << line << '\n';
  }

  size_t size = buffer.size();
  uint32_t hash = buffer.hash();
  char* ptr = buffer.release_buffer();
  return runtime->acquire_string(thread, ptr, size, hash);
}

RawValue currentworkingdirectory(Thread* thread, const RawValue*, uint8_t argc) {
  CHECK(argc == 0);
  fs::path cwd = fs::current_path();
  return thread->runtime()->create_string(thread, cwd);
}

RawValue getstacktrace(Thread* thread, const RawValue* args, uint8_t argc) {
  CHECK(argc == 1);
  uint32_t trim = RawInt::cast(args[0]).value();
  return thread->runtime()->create_stack_trace(thread, trim);
}

RawValue compile(Thread* thread, const RawValue* args, uint8_t argc) {
  Runtime* runtime = thread->runtime();

  CHECK(argc == 2);
  CHECK(args[0].isString());
  CHECK(args[1].isString());
  RawString source = RawString::cast(args[0]);
  std::string name = RawString::cast(args[1]).str();

  utils::Buffer buf;
  source.to_string(buf);
  auto unit = Compiler::compile(name, buf, CompilationUnit::Type::ReplInput);

  if (unit->console.has_errors()) {
    auto& messages = unit->console.messages();
    size_t size = messages.size();
    auto error_tuple = runtime->create_tuple(thread, size);
    for (size_t i = 0; i < size; i++) {
      error_tuple.set_field_at(i, runtime->create_string(thread, messages[i].message));
    }

    thread->throw_value(error_tuple);
    return kErrorException;
  }

  auto module = unit->compiled_module;
  CHECK(!module->function_table.empty());
  runtime->register_module(thread, module);
  return runtime->create_function(thread, kNull, module->function_table.front());
}

RawValue disassemble(Thread*, const RawValue* args, uint8_t argc) {
  CHECK(argc == 1);
  DCHECK(args[0].isFunction());

  auto func = RawFunction::cast(args[0]);
  SharedFunctionInfo* info = func.shared_info();
  info->dump(std::cout);

  return kNull;
}

RawValue makelist(Thread* thread, const RawValue* args, uint8_t argc) {
  CHECK(argc == 2);
  DCHECK(args[0].isInt() || args[0].isFloat());

  int64_t size;
  if (args[0].isFloat()) {
    size = static_cast<int64_t>(RawFloat::cast(args[0]).value());
  } else {
    size = static_cast<int64_t>(RawInt::cast(args[0]).value());
  }
  CHECK(size >= 0, "expected size to be positive number");

  Runtime* runtime = thread->runtime();
  auto tuple = runtime->create_tuple(thread, size);
  RawValue initial = args[1];
  for (int64_t i = 0; i < size; i++) {
    tuple.set_field_at(i, initial);
  }

  return tuple;
}

RawValue exit(Thread* thread, const RawValue* args, uint8_t argc) {
  CHECK(argc == 1);
  DCHECK(args[0].isInt());
  thread->abort((int32_t)RawInt::cast(args[0]).value());
}

}  // namespace charly::core::runtime::builtin::core
