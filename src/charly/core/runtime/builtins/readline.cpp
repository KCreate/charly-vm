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

#include <readline/history.h>
#include <readline/readline.h>
#include <cstdio>

#include "charly/core/runtime/builtins/readline.h"
#include "charly/core/runtime/runtime.h"

namespace charly::core::runtime::builtin::readline {

void initialize(Thread* thread) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  DEF_BUILTIN_READLINE(REGISTER_BUILTIN_FUNCTION)
}

RawValue prompt(Thread* thread, const RawValue* args, uint8_t argc) {
  CHECK(argc == 1);
  DCHECK(args[0].isString());
  Runtime* runtime = thread->runtime();

  std::string prompt = RawString::cast(args[0]).str();

  char* raw_line = nullptr;
  thread->native_section([&] {
    raw_line = ::readline(prompt.c_str());
  });

  // EOF passed on empty line
  if (raw_line == nullptr) {
    return kNull;
  }

  size_t length = std::strlen(raw_line);
  SYMBOL hash = crc32::hash_block(raw_line, length);
  return runtime->acquire_string(thread, raw_line, length, hash);
}

RawValue add_history(Thread* thread, const RawValue* args, uint8_t argc) {
  CHECK(argc == 1);
  DCHECK(args[0].isString());

  std::string name = RawString::cast(args[0]).str();
  thread->native_section([&] {
    ::add_history(name.c_str());
  });

  return kNull;
}

RawValue clear_history(Thread* thread, const RawValue*, uint8_t argc) {
  CHECK(argc == 0);

  thread->native_section([&] {
    ::clear_history();
  });

  return kNull;
}

}  // namespace charly::core::runtime::builtin::readline
