/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include <iomanip>

#include "block.h"
#include "opcode.h"
#include "symboltable.h"

#pragma once

namespace Charly::Compilation {
class Disassembler {
public:
  Disassembler(SymbolTable& s) : symtable(s) {
  }

  // Prints a disassembly of the passed block into stream
  //
  // Example:
  // 00000000: ReadLocal 0, 1
  // 00000009: ReadLocal 0, 2
  // 000000nn: Add
  // ...
  template <class T>
  inline void disassemble(InstructionBlock* block, T&& stream) {
    uint32_t offset = 0;

    while (offset < block->writeoffset) {
      Opcode opcode = static_cast<Opcode>(block->uint8_at(offset));

      this->print_offset(offset, stream);
      stream << ": " << kOpcodeMnemonics[opcode] << " ";

      switch (opcode) {
        case Opcode::ReadLocal:
        case Opcode::SetLocal: {
          stream << block->uint32_at(offset + 1) << ", " << block->uint32_at(offset + 1 + sizeof(uint32_t));
          break;
        }
        case Opcode::ReadMemberSymbol:
        case Opcode::SetMemberSymbol: {
          this->print_symbol(block->value_at(offset + 1), stream);
          break;
        }
        case Opcode::ReadArrayIndex:
        case Opcode::SetArrayIndex: {
          stream << block->uint32_at(offset + 1);
          break;
        }
        case Opcode::PutValue: {
          this->print_hex(block->value_at(offset + 1), stream);
          break;
        }
        case Opcode::PutFloat: {
          this->print_value(block->double_at(offset + 1), stream);
          break;
        }
        case Opcode::PutString: {
          this->print_value(block->uint32_at(offset + 1), stream);
          stream << ", ";
          this->print_value(block->uint32_at(offset + 1 + sizeof(uint32_t)), stream);
          break;
        }
        case Opcode::PutFunction: {
          this->print_symbol(block->value_at(offset + 1), stream);
          stream << ", ";
          this->print_offset(block->int32_at(offset + 1 + sizeof(VALUE)), stream);
          stream << ", ";
          this->print_value(block->bool_at(offset + 1 + sizeof(VALUE) + sizeof(uint32_t)), stream);
          stream << ", ";
          this->print_value(block->uint32_at(offset + 1 + sizeof(VALUE) + sizeof(uint32_t) + sizeof(bool)), stream);
          stream << ", ";
          this->print_value(block->uint32_at(offset + 1 + sizeof(VALUE) + sizeof(uint32_t) + sizeof(uint32_t)), stream);
          break;
        }
        case Opcode::PutCFunction: {
          this->print_symbol(block->value_at(offset + 1), stream);
          stream << ", ";
          this->print_hex(block->voidptr_at(offset + 1 + sizeof(VALUE)), stream);
          stream << ", ";
          this->print_value(block->uint32_at(offset + 1 + sizeof(VALUE) + sizeof(void*)), stream);
          break;
        }
        case Opcode::PutClass: {
          this->print_symbol(block->value_at(offset + 1), stream);
          stream << ", ";
          this->print_value(block->uint32_at(offset + 1 + sizeof(VALUE)), stream);
          stream << ", ";
          this->print_value(block->uint32_at(offset + 1 + sizeof(VALUE) + sizeof(uint32_t) * 1), stream);
          stream << ", ";
          this->print_value(block->uint32_at(offset + 1 + sizeof(VALUE) + sizeof(uint32_t) * 2), stream);
          stream << ", ";
          this->print_value(block->uint32_at(offset + 1 + sizeof(VALUE) + sizeof(uint32_t) * 3), stream);
          stream << ", ";
          this->print_value(block->uint32_at(offset + 1 + sizeof(VALUE) + sizeof(uint32_t) * 4), stream);
          stream << ", ";
          break;
        }
        case Opcode::PutArray:
        case Opcode::PutHash:
        case Opcode::Pop:
        case Opcode::Topn:
        case Opcode::Setn:
        case Opcode::Call:
        case Opcode::CallMember: {
          this->print_value(block->uint32_at(offset + 1), stream);
          break;
        }
        case Opcode::RegisterCatchTable:
        case Opcode::Branch:
        case Opcode::BranchIf:
        case Opcode::BranchUnless: {
          this->print_value(block->int32_at(offset + 1), stream);
          break;
        }
      }

      stream << '\n';
      offset += kInstructionLengths[opcode];
    }
  }

private:
  template <class T>
  inline void print_offset(int32_t offset, T&& stream) {
    stream.fill('0');
    stream << std::setw(8) << offset;
    stream << std::setw(1);
  }

  template <class T, typename V>
  inline void print_hex(V value, T&& stream) {
    stream << std::hex << value << std::dec;
  }

  template <class T, typename V>
  inline void print_value(V value, T&& stream) {
    stream << value;
  }

  template <class T>
  inline void print_symbol(VALUE offset, T&& stream) {
    stream << this->symtable(offset).value_or(kUndefinedSymbolString);
  }

private:
  SymbolTable& symtable;
};
}
