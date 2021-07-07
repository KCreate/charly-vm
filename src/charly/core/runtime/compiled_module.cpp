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

#include "charly/utils/colorwriter.h"
#include "charly/core/runtime/compiled_module.h"
#include "charly/core/compiler/ir/bytecode.h"

namespace charly::core::runtime {

using namespace utils;
using namespace compiler::ir;

void CompiledModule::dump(std::ostream& out) const {
  ColorWriter writer(out);
  writer.fg(Color::Grey, "; assembled module for file ");
  writer.fg(Color::Yellow, "'", this->filename, "'", "\n");
  out << '\n';

  // disassemble functions
  for (const CompiledFunction* function : this->function_table) {
    writer.fg(Color::Grey, "; function ");
    writer.fg(Color::Yellow, "'", function->name, "'", "\n");

    char* function_bytecodes = (char*)function->bytecode_base_ptr;
    char* function_end = (char*)function->end_ptr;

    // decode individual bytecodes
    char* next_opcode = function_bytecodes;
    while (next_opcode < function_end) {
      Opcode opcode = *(Opcode*)next_opcode;
      assert(opcode < Opcode::__Count);
      size_t opcode_length = kOpcodeLength[opcode];
      const std::string& opcode_name = kOpcodeNames[opcode];

      writer.fg(Color::Grey, ";  ", std::setw(16), std::left, opcode_name, std::setw(1));
      out << termcolor::yellow;
      MemoryBlock::hexdump(next_opcode, opcode_length, out, true);
      out << termcolor::reset;

      next_opcode = next_opcode + opcode_length;
    }

    out << '\n';
  }
}

}  // namespace charly::core::runtime
