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

// The signature of an internal method
struct MethodSignature {
  std::string name;
  size_t argc;
  void* func_pointer;
};

// Stores runtime lookup tables for internals
struct Index {
  static std::unordered_map<std::string, std::string> standard_libraries;
  static std::unordered_map<VALUE, MethodSignature> methods;
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
VALUE write(VM& vm, VALUE value);
VALUE getn(VM& vm);
VALUE dirname(VM& vm);
VALUE exit(VM& vm, VALUE status_code);
VALUE register_worker_task(VM& vm, VALUE v, VALUE cb);
}  // namespace Internals
}  // namespace Charly
