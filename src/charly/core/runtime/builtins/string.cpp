// This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
//
// MIT License
//
// Copyright (c) 2017 - 2025 Leonard Schütz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <filesystem>
#include <fstream>
#include <string_view>
#include <ranges>
#include <iomanip>
#include <iostream>

#include "charly/core/runtime/builtins/string.h"
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"

namespace charly::core::runtime::builtin::string {

void initialize(Thread* thread) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  DEF_BUILTIN_STRING(REGISTER_BUILTIN_FUNCTION)
}

RawValue index_of(Thread*, BuiltinFrame* frame) {
  CHECK(frame->arguments[0].isString());
  CHECK(frame->arguments[1].isString());

  const auto self = RawString::cast(frame->arguments[0]).str();
  const auto search = RawString::cast(frame->arguments[1]).str();
  std::string::size_type found = self.find(search, 0);

  if (found == std::string::npos) {
    return RawInt::create(-1);
  }

  return RawInt::create(found);
}

RawValue split(Thread* thread, BuiltinFrame* frame) {
  CHECK(frame->arguments[0].isString());
  CHECK(frame->arguments[1].isString());

  const auto self = RawString::cast(frame->arguments[0]).str();
  const auto delimiter = RawString::cast(frame->arguments[1]).str();

  HandleScope scope(thread);
  List result(scope, RawList::create(thread));

  const auto self_view = std::string_view(self);
  const auto delimiter_view = std::string_view(delimiter);

  for (const auto entry : std::views::split(self_view, delimiter_view)) {
    if (entry.size() > 0) {
      auto foo = std::string(entry.data(), entry.size());
      result.push_value(thread, RawString::create(thread, foo));
    }
  }

  return *result;
}

RawValue substring(Thread* thread, BuiltinFrame* frame) {
  CHECK(frame->arguments[0].isString());
  CHECK(frame->arguments[1].isNumber());
  CHECK(frame->arguments[2].isNumber());

  HandleScope scope(thread);
  String self(scope, frame->arguments[0]);
  int64_t start_index = frame->arguments[1].int_value();
  int64_t char_count = frame->arguments[2].int_value();
  int64_t length = self.codepoint_length();

  // start index below 0 gets truncated
  if (start_index < 0) {
    char_count += start_index;
    start_index = 0;
  }

  // substring has size 0 or start index is after the strings end
  if (char_count <= 0 || start_index >= length) {
    return RawSmallString::create_empty();
  }

  const char* str_begin_ptr = RawString::data(&self);
  const char* str_end_ptr = str_begin_ptr + self.byte_length();

  // determine the start ptr of the subrange
  const char* range_begin_ptr = str_begin_ptr;
  CHECK(utf8::advance_to_nth_codepoint(range_begin_ptr, str_end_ptr, start_index));

  // advance the end ptr until we reach the desired amount of characters or the end of the string
  const char* range_end_ptr = range_begin_ptr;
  int32_t chars_read = 0;
  while (chars_read < char_count && range_end_ptr < str_end_ptr) {
    CHECK(utf8::next(range_end_ptr, str_end_ptr));
    chars_read++;
  }

  // determine the amount of bytes read
  size_t range_begin_off = reinterpret_cast<size_t>(range_begin_ptr);
  size_t range_end_off = reinterpret_cast<size_t>(range_end_ptr);
  size_t bytes_read = range_end_off - range_begin_off;

  return RawString::create(thread, range_begin_ptr, bytes_read);
}

}  // namespace charly::core::runtime::builtin::string
