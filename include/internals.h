/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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

#include <unordered_map>

#include "defines.h"
#include "value.h"

#pragma once

namespace Charly {
namespace Internals {

// Standard charly libraries
static const std::unordered_map<std::string, std::string> kStandardCharlyLibraries = {

    // Internal primitives
    {"_charly_array", "src/stdlib/primitives/array.ch"},
    {"_charly_value", "src/stdlib/primitives/value.ch"},
    {"_charly_boolean", "src/stdlib/primitives/boolean.ch"},
    {"_charly_class", "src/stdlib/primitives/class.ch"},
    {"_charly_function", "src/stdlib/primitives/function.ch"},
    {"_charly_generator", "src/stdlib/primitives/generator.ch"},
    {"_charly_null", "src/stdlib/primitives/null.ch"},
    {"_charly_number", "src/stdlib/primitives/number.ch"},
    {"_charly_object", "src/stdlib/primitives/object.ch"},
    {"_charly_string", "src/stdlib/primitives/string.ch"},

    // Helper stuff
    {"_charly_defer", "src/stdlib/libs/defer.ch"},

    // Libraries
    {"_charly_math", "src/stdlib/libs/math.ch"},
    {"_charly_time", "src/stdlib/libs/time.ch"},
    {"_charly_unittest", "src/stdlib/libs/unittest.ch"}
};

// The signature of an internal method
struct InternalMethodSignature {
  std::string name;
  size_t argc;
  void* func_pointer;
};

#define CHECK(T, V)                                             \
  {                                                             \
    if (!charly_is_##T(V)) {                                    \
      vm.throw_exception("Expected argument " #V " to be " #T); \
      return kNull;                                             \
    }                                                           \
  }

#define WARN_TYPE(T, V)                                             \
  {                                                             \
    if (!charly_is_##T(V)) {                                    \
      vm.context.out_stream << "Expected argument " #V " to be " #T; \
      return kNull;                                             \
    }                                                           \
  }

VALUE import(VM& vm, VALUE filename, VALUE source);
VALUE get_method(VM& vm, VALUE argument);
VALUE write(VM& vm, VALUE value);
VALUE getn(VM& vm);
VALUE dirname(VM& vm);
VALUE set_primitive_value(VM& vm, VALUE klass);
VALUE set_primitive_object(VM& vm, VALUE klass);
VALUE set_primitive_class(VM& vm, VALUE klass);
VALUE set_primitive_array(VM& vm, VALUE klass);
VALUE set_primitive_string(VM& vm, VALUE klass);
VALUE set_primitive_number(VM& vm, VALUE klass);
VALUE set_primitive_function(VM& vm, VALUE klass);
VALUE set_primitive_generator(VM& vm, VALUE klass);
VALUE set_primitive_boolean(VM& vm, VALUE klass);
VALUE set_primitive_null(VM& vm, VALUE klass);
VALUE to_s(VM& vm, VALUE value);

VALUE call_dynamic(VM& vm, VALUE func, VALUE ctx, VALUE args);

VALUE defer(VM& vm, VALUE cb, VALUE dur);
VALUE defer_interval(VM& vm, VALUE cb, VALUE period);
VALUE clear_timer(VM& vm, VALUE uid);
VALUE clear_interval(VM& vm, VALUE uid);
VALUE exit(VM& vm, VALUE status_code);

VALUE register_worker_task(VM& vm, VALUE v, VALUE cb);
}  // namespace Internals
}  // namespace Charly
