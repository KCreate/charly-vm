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

#include "charly/value.h"

#pragma once

namespace charly::core::runtime::builtin {

#define REGISTER_BUILTIN_FUNCTION(L, N, A)                                                           \
  {                                                                                                  \
    auto builtin_name = runtime->declare_symbol(thread, "charly.builtin." #L "." #N);                \
    BuiltinFunction builtin_func(scope, RawBuiltinFunction::create(thread, N, builtin_name, A));     \
    CHECK(runtime->declare_global_variable(thread, builtin_name, true, builtin_func).is_error_ok()); \
  }

#define DEFINE_BUILTIN_METHOD_DECLARATIONS(L, N, A) RawValue N(Thread* thread, BuiltinFrame* frame);

}  // namespace charly::core::runtime::builtin
