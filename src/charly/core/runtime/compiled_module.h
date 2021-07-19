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

#include <string>
#include <vector>
#include <cstdint>
#include <iostream>

#include "charly/value.h"

#include "charly/core/compiler/ir/functioninfo.h"
#include "charly/core/compiler/ir/bytecode.h"

#include "charly/utils/buffer.h"

#pragma once

namespace charly::core::runtime {

struct ExceptionTableEntry {
  ExceptionTableEntry(uint32_t b, uint32_t _end, uint32_t _handler) :
    begin_offset(b), end_offset(_end), handler_offset(_handler) {}

  uint32_t begin_offset;
  uint32_t end_offset;
  uint32_t handler_offset;
  uintptr_t begin_ptr;
  uintptr_t end_ptr;
  uintptr_t handler_ptr;
};

struct SourceMapEntry {
  SourceMapEntry(uint32_t offset, uint16_t row, uint16_t column, uint16_t end_row, uint16_t end_column) :
    instruction_offset(offset), row(row), column(column), end_row(end_row), end_column(end_column) {}

  uint32_t instruction_offset;
  uintptr_t instruction_ptr;
  uint16_t row;
  uint16_t column;
  uint16_t end_row;
  uint16_t end_column;
};

struct StringTableEntry {
  StringTableEntry(const std::string& value) : hash(SYM(value)), value(value) {}

  SYMBOL hash;
  std::string value;
};

// TODO: implement different inline cache types
struct InlineCacheEntry {
  InlineCacheEntry(compiler::ir::ICType type) : type(type) {}
  compiler::ir::ICType type;

  union {
    struct {
      uint64_t padd[4];
    } _padd;
  } as;
};

struct CompiledModule;
struct CompiledFunction {
  CompiledModule* owner_module;

  std::string name;
  SYMBOL name_symbol;
  compiler::ir::FunctionInfo ir_info;

  std::vector<ExceptionTableEntry> exception_table;
  std::vector<SourceMapEntry> sourcemap_table;
  std::vector<StringTableEntry> string_table;
  std::vector<InlineCacheEntry> inline_cache_table;

  uintptr_t buffer_base_ptr;        // pointer to the base of the containing modules buffer
  uintptr_t bytecode_base_ptr;      // pointer to the functions first opcode
  uintptr_t end_ptr;                // function end pointer

  uint32_t bytecode_offset;       // offset into module buffer where the functions opcodes are located
  uint32_t end_offset;            // function end offset

  void dump(std::ostream& out) const;
};

struct CompiledModule {
  CompiledModule() : buffer(make<utils::ProtectedBuffer>()) {}
  ~CompiledModule() {
    for (CompiledFunction* func : function_table) {
      if (func)
        delete func;
    }
  }

  std::string filename;
  std::vector<std::string> symbol_table;
  std::vector<CompiledFunction*> function_table;

  // collective buffer containing all the bytecodes and inline cache tables
  // for all the modules compiled functions
  //
  // the struct 'CompiledFunction' contains offsets into this buffer
  ref<utils::ProtectedBuffer> buffer;

  void dump(std::ostream& out) const;
};

}  // namespace charly::core::runtime
