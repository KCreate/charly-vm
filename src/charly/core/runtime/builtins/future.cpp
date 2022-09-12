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

#include "charly/core/runtime/builtins/future.h"
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"

namespace charly::core::runtime::builtin::future {

void initialize(Thread* thread) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  DEF_BUILTIN_FUTURE(REGISTER_BUILTIN_FUNCTION)
}

RawValue create(Thread* thread, BuiltinFrame*) {
  return RawFuture::create(thread);
}

RawValue resolve(Thread* thread, BuiltinFrame* frame) {
  CHECK(frame->arguments[0].isFuture());
  RawFuture future = RawFuture::cast(frame->arguments[0]);
  RawValue result = frame->arguments[1];
  return future.resolve(thread, result);
}

RawValue reject(Thread* thread, BuiltinFrame* frame) {
  CHECK(frame->arguments[0].isFuture());
  CHECK(frame->arguments[1].isException());

  HandleScope scope(thread);
  Future future(scope, frame->arguments[0]);
  Exception exception(scope, frame->arguments[1]);
  return future.reject(thread, exception);
}

}  // namespace charly::core::runtime::builtin::future
