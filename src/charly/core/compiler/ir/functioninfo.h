/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2021 Leonard Schütz
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
#include <vector>

#pragma once

namespace charly::core::compiler::ir {

// keep track of local variable info of functions
struct FunctionInfo {
  bool valid = false;
  uint8_t local_variables = 0;
  uint8_t argc = 0;
  uint8_t minargc = 0;
  bool spread_argument = false;
  bool arrow_function = false;
  bool leaked = false;
  uint32_t stacksize = 0;

  // write a formatted version to the stream:
  //
  // (lvars=5, argc=3, minargc=2, spread=false)
  friend std::ostream& operator<<(std::ostream& out, const FunctionInfo& info) {
    out << "(";
    out << "lvars=" << static_cast<int32_t>(info.local_variables) << ", ";
    out << "argc=" << static_cast<int32_t>(info.argc) << ", ";
    out << "minargc=" << static_cast<int32_t>(info.minargc) << ", ";
    out << "spread=" << (info.spread_argument ? "true" : "false") << ", ";
    out << "arrow=" << (info.arrow_function ? "true" : "false");
    out << "leaked=" << (info.leaked ? "true" : "false");
    out << ")";

    return out;
  }
};

}  // namespace charly::core::compiler
