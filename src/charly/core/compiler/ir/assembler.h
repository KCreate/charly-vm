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

#include <string>
#include <unordered_map>
#include <vector>

#include "charly/core/compiler/ir/builder.h"
#include "charly/core/compiler/ir/ir.h"

#include "charly/utils/buffer.h"

#include "charly/core/runtime/compiled_module.h"

#pragma once

namespace charly::core::compiler::ir {

class Assembler {
public:
  static ref<runtime::CompiledModule> compile_module(const ref<IRModule>& module);

private:
  explicit Assembler(const ref<IRModule>& module) : m_ir_module(module), m_label_counter(module->next_label) {}

  // assemble set ir module
  void assemble();

  // encode op into buffer
  void encode_instruction(const ref<IRInstruction>& op);

  // emit some padding bytes to align the next write to a pointer boundary
  void align_to_pointer();

  // reserve a new label
  Label reserve_label();
  void place_label(Label label);

  // resolve all unresolved label references in the bytecode buffer
  void patch_unresolved_labels();

  // returns the offset a given label refers to
  // asserts that the label was placed, programming error otherwise
  uint32_t offset_of_label(Label label);

  struct UnresolvedLabel {
    Label label;
    Label base;
  };

  ref<runtime::CompiledModule> m_runtime_module = nullptr;
  ref<IRModule> m_ir_module = nullptr;
  Label m_label_counter;
  std::unordered_map<Label, uint32_t> m_placed_labels;
  std::unordered_map<uint32_t, UnresolvedLabel> m_unresolved_labels;
};

}  // namespace charly::core::compiler::ir
