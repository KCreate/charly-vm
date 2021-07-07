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
#include "charly/value.h"

#pragma once

namespace charly::core::compiler::ir {

class Builder {
public:
  Builder(const std::string& filename) :
    m_maximum_stack_height(0),
    m_current_stack_height(0),
    m_label_counter(0),
    m_block_id_counter(0),
    m_active_function(nullptr),
    m_module(make<IRModule>(filename)) {}

  // register string in currently active function
  uint16_t register_string(const std::string& string);

  // register a symbol in the module symbol table
  void register_symbol(const std::string& string);

  // basic block management
  ref<IRBasicBlock> new_basic_block();
  ref<IRBasicBlock> active_block() const;
  ref<IRFunction> active_function() const;
  void begin_function(Label head, ref<ast::Function> ast);
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
  void allocate_inline_caches();

  // label management
  Label reserve_label();
  void place_label(Label label);

  // emit an IR statement into the current function
  template <typename... Args>
  ref<IRInstruction> emit(Opcode opcode, Args&&... operands) {
    ref<IRBasicBlock> block = active_block();
    ref<IRInstruction> instruction = make<IRInstruction>(opcode, std::forward<Args>(operands)...);

    update_stack(-instruction->popped_values());
    update_stack(instruction->pushed_values());

    if (block->instructions.size() == 0) {
      place_label(reserve_label());
    }

    block->instructions.push_back(instruction);

    if (kBranchingOpcodes.count(opcode)) {
      new_basic_block();
    }

    return instruction;
  }

  ref<IRInstruction> emit_nop();
  ref<IRInstruction> emit_panic();
  ref<IRInstruction> emit_import();
  ref<IRInstruction> emit_stringconcat(IROpCount8 count);
  ref<IRInstruction> emit_declareglobal(IROpSymbol symbol);
  ref<IRInstruction> emit_declareglobalconst(IROpSymbol symbol);
  ref<IRInstruction> emit_type();
  ref<IRInstruction> emit_pop();
  ref<IRInstruction> emit_dup();
  ref<IRInstruction> emit_dup2();
  ref<IRInstruction> emit_jmp(IROpOffset label);
  ref<IRInstruction> emit_jmpf(IROpOffset label);
  ref<IRInstruction> emit_jmpt(IROpOffset label);
  ref<IRInstruction> emit_testjmp(IROpImmediate value, IROpOffset label);
  ref<IRInstruction> emit_testjmpstrict(IROpImmediate value, IROpOffset label);
  ref<IRInstruction> emit_throwex();
  ref<IRInstruction> emit_getexception();
  ref<IRInstruction> emit_call(IROpCount8 count);
  ref<IRInstruction> emit_callspread(IROpCount8 count);
  ref<IRInstruction> emit_ret();
  ref<IRInstruction> emit_load(IROpImmediate value);
  ref<IRInstruction> emit_loadsymbol(IROpSymbol symbol);
  ref<IRInstruction> emit_loadself();
  ref<IRInstruction> emit_loadargc();
  ref<IRInstruction> emit_loadglobal(IROpSymbol symbol);
  ref<IRInstruction> emit_loadlocal(IROpCount8 offset);
  ref<IRInstruction> emit_loadfar(IROpCount8 depth, IROpCount8 offset);
  ref<IRInstruction> emit_loadattr(IROpSymbol symbol);
  ref<IRInstruction> emit_loadattrsym();
  ref<IRInstruction> emit_loadsuperconstructor();
  ref<IRInstruction> emit_loadsuperattr(IROpSymbol symbol);
  ref<IRInstruction> emit_setglobal(IROpSymbol symbol);
  ref<IRInstruction> emit_setlocal(IROpCount8 offset);
  ref<IRInstruction> emit_setreturn();
  ref<IRInstruction> emit_setfar(IROpCount8 depth, IROpCount8 offset);
  ref<IRInstruction> emit_setattr(IROpSymbol symbol);
  ref<IRInstruction> emit_setattrsym();
  ref<IRInstruction> emit_unpacksequence(IROpCount8 count);
  ref<IRInstruction> emit_unpacksequencespread(IROpCount8 before, IROpCount8 after);
  ref<IRInstruction> emit_unpackobject(IROpCount8 count);
  ref<IRInstruction> emit_unpackobjectspread(IROpCount8 count);
  ref<IRInstruction> emit_makefunc(IROpOffset offset);
  ref<IRInstruction> emit_makeclass(IROpSymbol name,
                                    IROpCount8 funccount,
                                    IROpCount8 propcount,
                                    IROpCount8 staticpropcount);
  ref<IRInstruction> emit_makesubclass(IROpSymbol name,
                                       IROpCount8 funccount,
                                       IROpCount8 propcount,
                                       IROpCount8 staticpropcount);
  ref<IRInstruction> emit_makestr(IROpIndex16 index);
  ref<IRInstruction> emit_makelist(IROpCount16 count);
  ref<IRInstruction> emit_makelistspread(IROpCount16 count);
  ref<IRInstruction> emit_makedict(IROpCount16 count);
  ref<IRInstruction> emit_makedictspread(IROpCount16 count);
  ref<IRInstruction> emit_maketuple(IROpCount16 count);
  ref<IRInstruction> emit_maketuplespread(IROpCount16 count);
  ref<IRInstruction> emit_fiberspawn();
  ref<IRInstruction> emit_fiberyield();
  ref<IRInstruction> emit_fibercall();
  ref<IRInstruction> emit_fiberpause();
  ref<IRInstruction> emit_fiberresume();
  ref<IRInstruction> emit_fiberawait();
  ref<IRInstruction> emit_caststring();
  ref<IRInstruction> emit_castsymbol();
  ref<IRInstruction> emit_castiterator();
  ref<IRInstruction> emit_iteratornext();
  ref<IRInstruction> emit_add();
  ref<IRInstruction> emit_sub();
  ref<IRInstruction> emit_mul();
  ref<IRInstruction> emit_div();
  ref<IRInstruction> emit_mod();
  ref<IRInstruction> emit_pow();
  ref<IRInstruction> emit_usub();
  ref<IRInstruction> emit_eq();
  ref<IRInstruction> emit_neq();
  ref<IRInstruction> emit_lt();
  ref<IRInstruction> emit_gt();
  ref<IRInstruction> emit_le();
  ref<IRInstruction> emit_ge();
  ref<IRInstruction> emit_unot();
  ref<IRInstruction> emit_shl();
  ref<IRInstruction> emit_shr();
  ref<IRInstruction> emit_shru();
  ref<IRInstruction> emit_band();
  ref<IRInstruction> emit_bor();
  ref<IRInstruction> emit_bxor();
  ref<IRInstruction> emit_ubnot();

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
  uint32_t m_maximum_stack_height;
  uint32_t m_current_stack_height;

  Label m_label_counter;
  uint32_t m_block_id_counter;

  // maps Labels to basic blocks for the current function
  std::unordered_map<Label, ref<IRBasicBlock>> m_labelled_blocks;

  // stack of exception handlers
  std::stack<Label> m_exception_handlers;

  ref<IRFunction>   m_active_function;
  ref<IRBasicBlock> m_active_block;
  ref<IRModule>     m_module;
  };

}  // namespace charly::core::compiler::ir
