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

#include <vector>
#include <string>
#include <unordered_map>

#include "charly/core/compiler/ir/builder.h"
#include "charly/core/compiler/ir/ir.h"

#include "charly/utils/memoryblock.h"

#pragma once

namespace charly::core::compiler::ir {

/*
struct ModuleHeader {
  void* function_table;   // pointer to beginning of function table
  String* filename_label; // label to filename inside string table

  // string table
  uint16_t symtable_size;
  struct StringTableEntry {
    SYMBOL symbol;
    uint32_t length;
    char data[length];
  } stringtable[stringtable_size];

  // function table
  uint16_t functable_size;
  struct FunctionTableEntry {
    Function* label;
  } functable[functable_size];

  // functions
  struct Function {

    // forward pointers
    void* inline_cache_section;
    void* sourcemap_section;
    void* bytecodes_section;

    // ir info
    SYMBOL name;
    uint8_t lvars;
    uint8_t argc;
    uint8_t minargc;
    uint8_t stacksize;
    bool spread;
    bool arrow;
    bool leaked;

    // exception tables
    uint8_t exctable_size;
    struct {
      Label begin;
      Label end;
      Label handler;
    } exctable[exctable_size];

    // inline caches
    uint8_t inline_cache_size;
    struct {
      uint64_t padding[4];
    } inline_caches[inline_cache_size];

    // sourcemap
    uint16_t sourcemap_entries;
    struct {
      Label instruction;
      uint16_t row;
      uint16_t column;
      uint16_t end_row;
      uint16_t end_column;
    } sourcemap[sourcemap_entries];

    // bytecodes
    uint32_t bytecodes_size;
    char bytecodes[bytecodes_size];
  } functions[functable_size];
};
*/

class Assembler {
public:
  Assembler(const ref<IRModule>& module) :
    m_buffer(make<utils::MemoryBlock>()), m_module(module), m_label_counter(module->next_label) {}

  ref<utils::MemoryBlock> assemble();

private:

  // encode instruction into buffer
  void encode_instruction(const ref<IRInstruction>& instruction);

  // emit some padding bytes to align the next write to a pointer boundary
  void align_to_pointer();

  // reserve a new label
  Label reserve_label();
  void place_label(Label label);
  void write_label_reference(Label label);

  // resolve all unresolved label references in the bytecode buffer
  void patch_unresolved_labels();

  ref<utils::MemoryBlock> m_buffer;
  ref<IRModule> m_module;

  Label m_label_counter;

  // maps Labels to their offset in the bytecode buffer
  std::unordered_map<Label, uint32_t> m_placed_labels;

  // maps unresolved offsets in the bytecode to their label
  std::unordered_map<uint32_t, Label> m_unresolved_labels;
};

}  // namespace charly::core::compiler::ir
