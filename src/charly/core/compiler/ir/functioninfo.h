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

#include <cstdint>
#include <iostream>

#include "charly/symbol.h"

#pragma once

namespace charly::core::compiler::ir {

// keep track of local variable info of functions
struct FunctionInfo {
  SYMBOL name = SYM("");
  bool valid = false;
  uint8_t stacksize = 0;
  uint8_t local_variables = 0;
  uint8_t heap_variables = 0;
  uint8_t argc = 0;
  uint8_t minargc = 0;
  bool has_frame_context = false;
  bool spread_argument = false;
  bool arrow_function = false;
  bool private_function = false;
  bool is_constructor = false;

  // write a formatted version to the stream:
  //
  // (lvars=5, argc=3, minargc=2, spread=false)
  friend std::ostream& operator<<(std::ostream& out, const FunctionInfo& info) {
    out << "( ";
    out << "name=" << bitcast<void*>(static_cast<uintptr_t>(info.name)) << " ";
    out << "lvars=" << static_cast<int32_t>(info.local_variables) << " ";
    out << "hvars=" << static_cast<int32_t>(info.heap_variables) << " ";
    out << "argc=" << static_cast<int32_t>(info.argc) << " ";
    out << "minargc=" << static_cast<int32_t>(info.minargc) << " ";

    if (info.has_frame_context) {
      out << "has_context ";
    }
    if (info.spread_argument) {
      out << "spread ";
    }
    if (info.arrow_function) {
      out << "arrow ";
    }
    if (info.private_function) {
      out << "private ";
    }
    if (info.is_constructor) {
      out << "constructor ";
    }
    out << ")";

    return out;
  }
};

}  // namespace charly::core::compiler::ir
