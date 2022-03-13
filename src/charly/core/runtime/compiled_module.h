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
#include <iostream>
#include <string>
#include <vector>

#include "charly/value.h"

#include "charly/core/compiler/ir/bytecode.h"
#include "charly/core/compiler/ir/functioninfo.h"

#include "charly/utils/buffer.h"

#pragma once

namespace charly::core::runtime {

struct ExceptionTableEntry {
  ExceptionTableEntry(uint32_t b, uint32_t _end, uint32_t _handler) :
    begin_offset(b), end_offset(_end), handler_offset(_handler), begin_ptr(0), end_ptr(0), handler_ptr(0) {}

  uint32_t begin_offset;
  uint32_t end_offset;
  uint32_t handler_offset;
  uintptr_t begin_ptr;
  uintptr_t end_ptr;
  uintptr_t handler_ptr;
};

struct SourceMapEntry {
  SourceMapEntry(uint32_t offset, uint16_t row, uint16_t column, uint16_t end_row, uint16_t end_column) :
    instruction_offset(offset),
    instruction_ptr(0),
    row(row),
    column(column),
    end_row(end_row),
    end_column(end_column) {}

  uint32_t instruction_offset;
  uintptr_t instruction_ptr;
  uint16_t row;
  uint16_t column;
  uint16_t end_row;
  uint16_t end_column;
};

struct StringTableEntry {
  explicit StringTableEntry(const std::string& value) : hash(crc32::hash_string(value)), value(value) {}

  SYMBOL hash;
  std::string value;
};

struct CompiledModule;
struct SharedFunctionInfo {
  CompiledModule* owner_module;

  std::string name;
  SYMBOL name_symbol;
  compiler::ir::FunctionInfo ir_info;

  std::vector<ExceptionTableEntry> exception_table;
  std::vector<SourceMapEntry> sourcemap_table;
  std::vector<StringTableEntry> string_table;
  std::vector<RawValue> constant_table;

  uintptr_t buffer_base_ptr;    // pointer to the base of the containing modules buffer
  uintptr_t bytecode_base_ptr;  // pointer to the functions first opcode
  uintptr_t end_ptr;            // function end pointer

  uint32_t bytecode_offset;  // offset into module buffer where the functions opcodes are located
  uint32_t end_offset;       // function end offset

  void dump(std::ostream& out) const;
};

struct CompiledModule {
  CompiledModule() : buffer(32) {}
  ~CompiledModule() {
    for (SharedFunctionInfo* func : function_table) {
      delete func;
    }
  }

  std::string filename;
  std::vector<SharedFunctionInfo*> function_table;

  // buffer containing the bytecodes
  // the struct 'SharedFunctionInfo' contains offsets into this buffer
  utils::Buffer buffer;

  void dump(std::ostream& out) const;
};

}  // namespace charly::core::runtime
