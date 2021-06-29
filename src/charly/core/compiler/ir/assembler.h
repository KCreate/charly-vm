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

#include "charly/core/runtime/compiled_module.h"

#pragma once

namespace charly::core::compiler::ir {

class Assembler {
public:
  Assembler(const ref<IRModule>& module) :
    m_buffer(make<utils::MemoryBlock>()), m_module(module), m_label_counter(module->next_label) {}

  runtime::CompiledModule* assemble();

private:

  // encode instruction into buffer
  void encode_instruction(const ref<IRInstruction>& instruction);

  // emit some padding bytes to align the next write to a pointer boundary
  void align_to_pointer();

  // reserve a new label
  Label reserve_label();
  void place_label(Label label);
  void write_relative_label_reference(Label label, Label other);  // int32_t relative offset to other label
  void write_absolute_label_reference(Label label);               // uint32_t absolute offset from buffer start

  // resolve all unresolved label references in the bytecode buffer
  void patch_unresolved_labels();

  // returns the offset a given label refers to
  // asserts that the label was placed, programming error otherwise
  uint32_t offset_of_label(Label label);

  ref<utils::MemoryBlock> m_buffer;
  ref<IRModule> m_module;

  Label m_label_counter;

  // maps Labels to their offset in the bytecode buffer
  std::unordered_map<Label, uint32_t> m_placed_labels;

  struct UnresolvedLabel {
    Label label;
    bool relative;
    std::optional<Label> relative_to;
  };

  // maps unresolved offsets in the bytecode to their label
  std::unordered_map<uint32_t, UnresolvedLabel> m_unresolved_labels;
};

}  // namespace charly::core::compiler::ir
