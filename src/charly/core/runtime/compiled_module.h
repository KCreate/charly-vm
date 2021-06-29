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

#include "charly/utils/memoryblock.h"

#pragma once

namespace charly::core::runtime {

struct ExceptionTableEntry {
  ExceptionTableEntry(uint32_t begin, uint32_t end, uint32_t handler) : begin(begin), end(end), handler(handler) {}

  uint32_t begin;
  uint32_t end;
  uint32_t handler;
};

struct SourceMapEntry {
  SourceMapEntry(uint32_t instruction_offset, uint16_t row, uint16_t column, uint16_t end_row, uint16_t end_column) :
    instruction_offset(instruction_offset), row(row), column(column), end_row(end_row), end_column(end_column) {}

  uint32_t instruction_offset;
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

// emitted into bytecode stream
struct __attribute__((packed)) InlineCacheEntry {
  uint64_t padd1;
  uint64_t padd2;
  uint64_t padd3;
  uint64_t padd4;

  // TODO: implement real data structure
};

// emitted into bytecode stream
struct CompiledFunction;
struct __attribute__((packed)) BytecodeFunctionHeader {
  CompiledFunction* shared_compiled_function;
  uint32_t inline_cache_offset;
  uint32_t bytecode_offset;
};

struct CompiledModule;
struct CompiledFunction {
  CompiledModule* owner_module;

  SYMBOL name;
  compiler::ir::FunctionInfo ir_info;

  std::vector<ExceptionTableEntry> exception_table;
  std::vector<SourceMapEntry> sourcemap_table;
  std::vector<StringTableEntry> string_table;

  uint32_t inline_cache_offset;
  uint32_t bytecode_offset;
};

struct CompiledModule {
  std::string filename;
  std::vector<std::string> symbol_table;
  std::vector<CompiledFunction*> function_table;

  // collective buffer containing all the bytecodes and inline cache tables
  // for all the modules compiled functions
  //
  // the struct 'CompiledFunction' contains offsets into this buffer
  ref<utils::MemoryBlock> buffer;

  void dump(std::ostream& out) const;
};

}  // namespace charly::core::runtime
