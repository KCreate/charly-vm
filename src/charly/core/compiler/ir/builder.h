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
#include <list>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/ir/bytecode.h"
#include "charly/core/compiler/ir/ir.h"

#pragma once

namespace charly::core::compiler::ir {

class Builder {
public:
  explicit Builder(const std::string& filename) :
    m_maximum_stack_height(0),
    m_current_stack_height(0),
    m_label_counter(0),
    m_block_id_counter(0),
    m_active_function(nullptr),
    m_module(make<IRModule>(filename)) {}

  // register string in currently active function
  uint16_t register_string(const std::string& string);

  // register a symbol in the module symbol table
  SYMBOL register_symbol(const std::string& string);

  // register constant RawValue in current functions constant pool
  uint16_t register_constant(runtime::RawValue value);

  // basic block management
  ref<IRBasicBlock> new_basic_block();
  ref<IRBasicBlock> active_block() const;
  ref<IRFunction> active_function() const;
  void begin_function(Label head, const ref<ast::Function>& ast);
  void finish_function();

  // exception table building
  void push_exception_handler(Label handler);
  void pop_exception_handler();

  // cfg
  void build_cfg();
  void trim_dead_instructions();
  void remove_empty_blocks();
  void rewrite_chained_branches();
  void remove_useless_jumps();
  void remove_dead_blocks();
  void emit_exception_tables();

  // label management
  Label reserve_label();
  void place_label(Label label);

  ref<IRInstruction> emit(const ref<IRInstruction>& instruction) {
    return emit_instruction_impl(instruction);
  };

#define DEF_IXXX(O)                                          \
  ref<IRInstruction> emit_##O() {                            \
    return emit_instruction_impl(make<IRInstruction_##O>()); \
  }
#define DEF_IAXX(O)                                             \
  ref<IRInstruction> emit_##O(uint8_t arg) {                    \
    return emit_instruction_impl(make<IRInstruction_##O>(arg)); \
  }
#define DEF_IABX(O)                                                    \
  ref<IRInstruction> emit_##O(uint8_t arg1, uint8_t arg2) {            \
    return emit_instruction_impl(make<IRInstruction_##O>(arg1, arg2)); \
  }
#define DEF_IABC(O)                                                          \
  ref<IRInstruction> emit_##O(uint8_t arg1, uint8_t arg2, uint8_t arg3) {    \
    return emit_instruction_impl(make<IRInstruction_##O>(arg1, arg2, arg3)); \
  }
#define DEF_IABB(O)                                                    \
  ref<IRInstruction> emit_##O(uint8_t arg1, uint16_t arg2) {           \
    return emit_instruction_impl(make<IRInstruction_##O>(arg1, arg2)); \
  }
#define DEF_IAAX(O)                                             \
  ref<IRInstruction> emit_##O(uint16_t arg) {                   \
    return emit_instruction_impl(make<IRInstruction_##O>(arg)); \
  }
#define DEF_IAAA(O)                                             \
  ref<IRInstruction> emit_##O(uint32_t arg) {                   \
    return emit_instruction_impl(make<IRInstruction_##O>(arg)); \
  }
#define DEF_EMITS(N, T, ...) DEF_##T(N);
  FOREACH_OPCODE(DEF_EMITS)
#undef DEF_OPCODES
#undef DEF_IXXX
#undef DEF_IAXX
#undef DEF_IABX
#undef DEF_IABC
#undef DEF_IABB
#undef DEF_IAAX
#undef DEF_IAAA

  // helper method to emit a symbol constant load
  ref<IRInstruction> emit_loadsymbol(const std::string& string) {
    return emit_load(register_constant(runtime::RawSymbol::make(register_symbol(string))));
  };

  // helper method to emit loads of immediate values
  ref<IRInstruction> emit_load_value(runtime::RawValue value);

  ref<IRModule> get_module() const {
    return m_module;
  };

  Label next_label_id() const {
    return m_label_counter;
  }

  // keep track of maximum stack height
  uint32_t maximum_stack_height() const;
  void reset_stack_height();
  void update_stack(int32_t amount);

private:
  ref<IRInstruction> emit_instruction_impl(const ref<IRInstruction>& instruction);

private:
  uint32_t m_maximum_stack_height;
  uint32_t m_current_stack_height;

  Label m_label_counter;
  uint32_t m_block_id_counter;

  // maps Labels to basic blocks for the current function
  std::unordered_map<Label, ref<IRBasicBlock>> m_labelled_blocks;

  // stack of exception handlers
  std::stack<Label> m_exception_handlers;

  ref<IRFunction> m_active_function;
  ref<IRBasicBlock> m_active_block;
  ref<IRModule> m_module;
};

}  // namespace charly::core::compiler::ir
