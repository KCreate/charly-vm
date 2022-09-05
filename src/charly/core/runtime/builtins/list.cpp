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

#include "charly/core/runtime/builtins/list.h"
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"

namespace charly::core::runtime::builtin::list {

void initialize(Thread* thread) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  DEF_BUILTIN_LIST(REGISTER_BUILTIN_FUNCTION)
}

RawValue create(Thread* thread, BuiltinFrame* frame) {
  CHECK(frame->arguments[0].isNumber());

  RawValue size_value = frame->arguments[0];
  int64_t size = size_value.int_value();
  RawValue initial_value = frame->arguments[1];

  if (size < 0) {
    return thread->throw_message("Expected length to be positive, got %", size_value);
  }

  if (size > (int64_t)RawList::kMaximumCapacity) {
    return thread->throw_message("List exceeded max size");
  }

  return RawList::create_with(thread, size, initial_value);
}

RawValue insert(Thread* thread, BuiltinFrame* frame) {
  CHECK(frame->arguments[0].isList());
  CHECK(frame->arguments[1].isNumber());

  auto list = RawList::cast(frame->arguments[0]);
  int64_t index = frame->arguments[1].int_value();

  return list.insert_at(thread, index, frame->arguments[2]);
}

RawValue erase(Thread* thread, BuiltinFrame* frame) {
  CHECK(frame->arguments[0].isList());
  CHECK(frame->arguments[1].isNumber());
  CHECK(frame->arguments[2].isNumber());

  auto list = RawList::cast(frame->arguments[0]);
  int64_t start = frame->arguments[1].int_value();
  int64_t count = frame->arguments[2].int_value();
  return list.erase_at(thread, start, count);
}

RawValue push(Thread* thread, BuiltinFrame* frame) {
  CHECK(frame->arguments[0].isList());
  auto list = RawList::cast(frame->arguments[0]);
  return list.push_value(thread, frame->arguments[1]);
}

RawValue pop(Thread* thread, BuiltinFrame* frame) {
  CHECK(frame->arguments[0].isList());
  auto list = RawList::cast(frame->arguments[0]);
  return list.pop_value(thread);
}

}  // namespace charly::core::runtime::builtin::list
