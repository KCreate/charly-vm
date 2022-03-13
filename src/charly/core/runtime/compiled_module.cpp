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

#include "charly/core/runtime/compiled_module.h"
#include "charly/core/compiler/ir/bytecode.h"
#include "charly/utils/colorwriter.h"

namespace charly::core::runtime {

using namespace utils;
using namespace compiler::ir;

void CompiledModule::dump(std::ostream& out) const {
  ColorWriter writer(out);
  writer.fg(Color::Grey, "; assembled module for file ");
  writer.fg(Color::Yellow, "'", this->filename, "'", "\n");
  writer.fg(Color::Grey, "; bytecode buffer size = ");
  writer.fg(Color::Yellow, this->buffer.size(), "\n");

  out << '\n';

  // disassemble functions
  for (const SharedFunctionInfo* function : this->function_table) {
    function->dump(out);
  }
}

void SharedFunctionInfo::dump(std::ostream& out) const {
  ColorWriter writer(out);

  const uint8_t* function_bytecodes = (uint8_t*)this->bytecode_base_ptr;
  const uint8_t* function_end = (uint8_t*)this->end_ptr;
  size_t function_bytecodes_length = function_end - function_bytecodes;

  writer.fg(Color::Grey, "; function ");
  writer.fg(Color::Yellow, "'", this->name, "'", "\n");

  writer.fg(Color::Grey, "; bytecodes ptr = ");
  writer.fg(Color::Yellow, std::hex, (uintptr_t)function_bytecodes, std::dec, "\n");

  writer.fg(Color::Grey, "; bytecodes length = ");
  writer.fg(Color::Yellow, function_bytecodes_length, "\n");

  writer.fg(Color::Grey, "; bytecode hash = ");
  writer.fg(Color::Yellow, std::hex,
            crc32::hash_block(bitcast<const char*>(function_bytecodes), function_bytecodes_length), std::dec, "\n");

  writer.fg(Color::Grey, "; ir_info = ");
  writer.fg(Color::Yellow, this->ir_info, "\n");

  // decode individual bytecodes
  const uint8_t* next_opcode = function_bytecodes;
  while (next_opcode < function_end) {
    const Opcode opcode = *(Opcode*)next_opcode;
    DCHECK(opcode < Opcode::OpcodeCount);
    size_t opcode_length = kInstructionLength;
    const std::string& opcode_name = kOpcodeNames[opcode];

    writer.fg(Color::Grey, ";  ", std::setw(20), std::left, opcode_name, std::setw(1));
    out << termcolor::yellow;
    Buffer::hexdump((const char*)next_opcode, opcode_length, out, true);
    out << termcolor::reset;

    next_opcode = next_opcode + opcode_length;
  }

  out << '\n';
}

}  // namespace charly::core::runtime
