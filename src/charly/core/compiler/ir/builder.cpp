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

#include "charly/utils/buffer.h"
#include "charly/utils/colorwriter.h"
#include "charly/utils/argumentparser.h"
#include "charly/core/compiler/ir/builder.h"

namespace charly::core::compiler::ir {

using namespace charly::core::compiler::ast;

using Color = utils::Color;

uint16_t Builder::register_string(const std::string& string) {
  ref<IRFunction> function = m_active_function;

  // duplicates check
  uint16_t index = 0;
  for (const IRStringTableEntry& entry : function->string_table) {
    if (entry.value.compare(string) == 0) {
      return index;
    }

    index++;
  }

  register_symbol(string);
  function->string_table.emplace_back(IRStringTableEntry(string));
  return index;
}

void Builder::register_symbol(const std::string& string) {
  ref<IRModule> module = m_module;

  // duplicates check
  for (const std::string& entry : module->symbol_table) {
    if (entry.compare(string) == 0) {
      return;
    }
  }

  module->symbol_table.emplace_back(string);
}

void Builder::push_exception_handler(Label handler) {
  m_exception_handlers.push(handler);
  ref<IRBasicBlock> block = new_basic_block();
}

void Builder::pop_exception_handler() {
  m_exception_handlers.pop();
  ref<IRBasicBlock> block = new_basic_block();
}

ref<IRFunction> Builder::active_function() const {
  assert(m_active_function && "no active function");
  return m_active_function;
}

ref<IRBasicBlock> Builder::active_block() const {
  assert(m_active_block && "no active block");
  return m_active_block;
}

void Builder::begin_function(Label head, ref<ast::Function> ast) {
  m_active_function = make<IRFunction>(head, ast);
  m_module->functions.push_back(m_active_function);
  register_symbol(ast->name->value);
  new_basic_block();
}

void Builder::finish_function() {

  if (!utils::ArgumentParser::is_flag_set("opt_disable")) {

    // trim dead instructions in basic blocks that occur after terminating opcodes
    trim_dead_instructions();

    // remove basic blocks with no instructions
    remove_empty_blocks();
  }

  // build the control flow graph, storing the incoming and outgoing
  // branches for each basic block
  build_cfg();

  if (!utils::ArgumentParser::is_flag_set("opt_disable")) {
    // remove branch chains
    //
    //    jmp .L1
    //  .L1
    //    jmp .L3
    //  .L3
    //    <body>
    //
    // the above can be rewritten into:
    //
    //    jmp .L3
    //  .L1
    //    jmp .L3
    //  .L3
    //    <body>
    //
    //
    // this may result in some blocks becoming unreachable, causing
    // them to be removed in the later stages
    rewrite_chained_branches();

    // remove blocks with no incoming branches from the graph
    // repeat this process for as long as blocks are being removed
    remove_dead_blocks();

    // remove useless jumps
    //
    //    jmp .L1
    //  .L1
    //
    // the above jump opcode can be completely removed
    remove_useless_jumps();
  }

  // emit the exceptions tables for the basic blocks of this function
  //
  // each exception table entry maps a certain region of code
  // to a hanler address, if an exception were to occur within that region
  new_basic_block();
  emit_nop();
  emit_exception_tables();

  // allocate the inline cache slots
  allocate_inline_caches();

  // reset builder for next function
  reset_stack_height();
  m_block_id_counter = 0;
  m_labelled_blocks.clear();
  m_active_function = nullptr;
  m_active_block = nullptr;
}

void Builder::remove_empty_blocks() {
  auto it = m_active_function->basic_blocks.begin();
  while (it != m_active_function->basic_blocks.end()) {
    ref<IRBasicBlock> block = *it;

    // remove empty blocks
    if (block->instructions.size() == 0) {

      // propagate labels to next block
      if (block->next_block && block->labels.size()) {
        for (Label label : block->labels) {
          m_labelled_blocks.insert_or_assign(label, block->next_block);
        }
        block->next_block->labels.insert(block->labels.begin(), block->labels.end());
      }

      IRBasicBlock::unlink(block);
      it = m_active_function->basic_blocks.erase(it);
      continue;
    }

    it++;
  }
}

void Builder::trim_dead_instructions() {
  for (const ref<IRBasicBlock>& block : m_active_function->basic_blocks) {

    bool block_terminated = false;
    auto inst_it = block->instructions.begin();
    while (inst_it != block->instructions.end()) {
      const ref<IRInstruction>& inst = *inst_it;

      // remove instructions after terminating opcode
      if (block_terminated) {
        inst_it = block->instructions.erase(inst_it);
        continue;
      }

      if (kTerminatingOpcodes.count(inst->opcode)) {
        block_terminated = true;
      }

      inst_it++;
    }
  }
}

void Builder::build_cfg() {
  for (ref<IRBasicBlock> block : m_active_function->basic_blocks) {

    // empty blocks fallthrough to the next block
    if (block->instructions.size() == 0) {
      IRBasicBlock::link(block, block->next_block);
      continue;
    }

    ref<IRInstruction> op = block->instructions.back();
    switch (op->opcode) {
      case Opcode::jmp: {
        Label target_label = cast<IROperandOffset>(op->operands[0])->value;
        ref<IRBasicBlock> target_block = m_labelled_blocks.at(target_label);
        IRBasicBlock::link(block, target_block);
        continue;
      }
      case Opcode::jmpf:
      case Opcode::jmpt: {
        Label target_label = cast<IROperandOffset>(op->operands[0])->value;
        ref<IRBasicBlock> target_block = m_labelled_blocks.at(target_label);
        assert(block->next_block);
        IRBasicBlock::link(block, target_block);
        IRBasicBlock::link(block, block->next_block);
        continue;
      }
      case Opcode::testjmp:
      case Opcode::testjmpstrict: {
        Label target_label = cast<IROperandOffset>(op->operands[1])->value;
        ref<IRBasicBlock> target_block = m_labelled_blocks.at(target_label);
        assert(block->next_block);
        IRBasicBlock::link(block, target_block);
        IRBasicBlock::link(block, block->next_block);
        continue;
      }
      case Opcode::ret:
      case Opcode::throwex:
      case Opcode::panic: {
        // opcode ends function
        continue;
      }
      default: {
        IRBasicBlock::link(block, block->next_block);
        break;
      }
    }
  }
}

void Builder::rewrite_chained_branches() {
  bool updated_jmp = false;

  do {
    updated_jmp = false;
    for (const ref<IRBasicBlock>& block : m_active_function->basic_blocks) {

      // skip empty blocks
      if (block->instructions.size() == 0) {
        continue;
      }

      // jmp instructions that branch to a basic block that contains
      // only a single jmp instruction, can be replaced with the
      // second jmp instruction
      const ref<IRInstruction>& instruction = block->instructions.back();
      if (instruction->opcode == Opcode::jmp) {
        ref<IROperandOffset> original_operand = cast<IROperandOffset>(instruction->operands[0]);
        ref<IRBasicBlock> target_block = *block->outgoing_blocks.begin();

        // catch infinite loops
        if (target_block.get() == block.get()) {
          continue;
        }

        // check if the target block contains a single instruction
        if (target_block->instructions.size() == 1) {
          const ref<IRInstruction>& target_instruction = target_block->instructions.back();

          if (target_instruction->opcode == Opcode::jmp) {

            // determine target block of the second jmp and update the original jmp instruction
            Label second_target_label = cast<IROperandOffset>(target_instruction->operands[0])->value;
            ref<IRBasicBlock> new_target_block = m_labelled_blocks.at(second_target_label);

            // catch infinite loops
            if (target_block.get() == new_target_block.get()) {
              continue;
            }

            // unlink original branch
            IRBasicBlock::unlink(block, target_block);
            IRBasicBlock::link(block, new_target_block);
            original_operand->value = second_target_label;
            updated_jmp = true;
          } else if (target_instruction->opcode == Opcode::ret) {

            // replace original jump with a ret
            instruction->opcode = Opcode::ret;
            instruction->operands.clear();
            IRBasicBlock::unlink(block, target_block);
          }
        }
      }
    }
  } while (updated_jmp);
}

void Builder::remove_useless_jumps() {
  for (const ref<IRBasicBlock>& block : m_active_function->basic_blocks) {
    if (block->instructions.size()) {
      ref<IRInstruction> op = block->instructions.back();
      switch (op->opcode) {
        case Opcode::jmp: {
          Label target_label = cast<IROperandOffset>(op->operands[0])->value;
          if (block->next_block && block->next_block->labels.count(target_label)) {
            block->instructions.pop_back();
          }

          continue;
        }
        default: {
          break;
        }
      }
    }
  }
}

void Builder::remove_dead_blocks() {

  // mark all reachable blocks
  std::list<ref<IRBasicBlock>> reachable_blocks;
  reachable_blocks.push_back(m_active_function->basic_blocks.front());

  // mark function exception table handlers as reachable
  for (const IRExceptionTableEntry& entry : m_active_function->exception_table) {
    if (m_labelled_blocks.count(entry.handler)) {
      reachable_blocks.push_back(m_labelled_blocks.at(entry.handler));
    }
  }

  // mark reachable blocks
  while (reachable_blocks.size()) {
    ref<IRBasicBlock> block = reachable_blocks.front();
    reachable_blocks.pop_front();

    if (!block->reachable) {
      block->reachable = true;

      for (const ref<IRBasicBlock>& outgoing : block->outgoing_blocks) {
        reachable_blocks.push_back(outgoing);
      }

      if (block->exception_handler) {
        reachable_blocks.push_back(m_labelled_blocks.at(block->exception_handler.value()));
      }
    }
  }

  // delete all non-reachable blocks
  auto it = m_active_function->basic_blocks.begin();
  while (it != m_active_function->basic_blocks.end()) {
    ref<IRBasicBlock> block = *it;

    // remove block
    if (!block->reachable) {

      // unlink block from list
      if (block->previous_block) {
        block->previous_block->next_block = block->next_block;
      }
      if (block->next_block) {
        block->next_block->previous_block = block->previous_block;
      }
      it = m_active_function->basic_blocks.erase(it);

      // remove from incoming blocks list of outgoing branches
      IRBasicBlock::unlink(block);
      continue;
    }

    it++;
  }
}

void Builder::emit_exception_tables() {
  for (const ref<IRBasicBlock>& block : m_active_function->basic_blocks) {
    if (block->exception_handler) {
      assert(block->labels.size());
      assert(block->next_block);
      assert(block->next_block->labels.size());

      Label begin = *block->labels.begin();
      Label end = *block->next_block->labels.begin();
      Label handler = block->exception_handler.value();

      // extend previous table if possible
      if (m_active_function->exception_table.size()) {
        IRExceptionTableEntry& previous = m_active_function->exception_table.back();

        if (previous.end == begin && previous.handler == handler) {
          previous.end = end;
        } else {
          m_active_function->exception_table.emplace_back(IRExceptionTableEntry(begin, end, handler));
        }
      } else {
        m_active_function->exception_table.emplace_back(IRExceptionTableEntry(begin, end, handler));
      }
    }
  }
}

void Builder::allocate_inline_caches() {
  for (const auto& block : m_active_function->basic_blocks) {
    for (const auto& instruction : block->instructions) {
      ICType type = kOpcodeInlineCaches[static_cast<uint8_t>(instruction->opcode)];
      if (type != ICNone) {
        instruction->inline_cache_index = m_active_function->inline_cache_table.size();
        m_active_function->inline_cache_table.emplace_back(IRInlineCacheTableEntry(type));
      }
    }
  }
}

ref<IRBasicBlock> Builder::new_basic_block() {
  assert(m_active_function);

  ref<IRBasicBlock> block = make<IRBasicBlock>(m_block_id_counter++);
  block->previous_block = m_active_block;

  if (m_exception_handlers.size()) {
    block->exception_handler = m_exception_handlers.top();
  }

  if (m_active_block) {
    m_active_block->next_block = block;

    // each block must have at least one label
    if (m_active_block->labels.size() == 0) {
      m_active_block->labels.insert(reserve_label());
    }
  }

  m_active_function->basic_blocks.push_back(block);
  m_active_block = block;
  return block;
}

Label Builder::reserve_label() {
  return m_label_counter++;
}

void Builder::place_label(Label label) {

  // if the current basic block is still empty
  // we can add the label to the current block
  if (m_active_block->instructions.size() == 0) {
    m_active_block->labels.insert(label);
    m_labelled_blocks.insert({label, m_active_block});
    return;
  }

  // create a new block
  ref<IRBasicBlock> new_block = new_basic_block();
  new_block->labels.insert(label);
  m_labelled_blocks.insert({label, new_block});
}

// machine control
ref<IRInstruction> Builder::emit_nop() {
  return emit(Opcode::nop);
}

ref<IRInstruction> Builder::emit_panic() {
  return emit(Opcode::panic);
}

// misc. instructions
ref<IRInstruction> Builder::emit_import() {
  return emit(Opcode::import);
}

ref<IRInstruction> Builder::emit_stringconcat(OpCount8 count) {
  return emit(Opcode::stringconcat, IROperandCount8::make(count));
}

ref<IRInstruction> Builder::emit_declareglobal(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::declareglobal, IROperandSymbol::make(symbol));
}

ref<IRInstruction> Builder::emit_declareglobalconst(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::declareglobalconst, IROperandSymbol::make(symbol));
}

ref<IRInstruction> Builder::emit_type() {
  return emit(Opcode::type);
}

// stack management
ref<IRInstruction> Builder::emit_pop() {
  return emit(Opcode::pop);
}

ref<IRInstruction> Builder::emit_dup() {
  return emit(Opcode::dup);
}

ref<IRInstruction> Builder::emit_dup2() {
  return emit(Opcode::dup2);
}

// control flow
ref<IRInstruction> Builder::emit_jmp(OpOffset label) {
  return emit(Opcode::jmp, IROperandOffset::make(label));
}

ref<IRInstruction> Builder::emit_jmpf(OpOffset label) {
  return emit(Opcode::jmpf, IROperandOffset::make(label));
}

ref<IRInstruction> Builder::emit_jmpt(OpOffset label) {
  return emit(Opcode::jmpt, IROperandOffset::make(label));
}

ref<IRInstruction> Builder::emit_testjmp(OpImmediate value, OpOffset label) {
  return emit(Opcode::testjmp, IROperandImmediate::make(value), IROperandOffset::make(label));
}

ref<IRInstruction> Builder::emit_testjmpstrict(OpImmediate value, OpOffset label) {
  return emit(Opcode::testjmpstrict, IROperandImmediate::make(value), IROperandOffset::make(label));
}

ref<IRInstruction> Builder::emit_throwex() {
  return emit(Opcode::throwex);
}

ref<IRInstruction> Builder::emit_getexception() {
  return emit(Opcode::getexception);
}

// function control flow
ref<IRInstruction> Builder::emit_call(OpCount8 count) {
  return emit(Opcode::call, IROperandCount8::make(count));
}

ref<IRInstruction> Builder::emit_callspread(OpCount8 count) {
  return emit(Opcode::callspread, IROperandCount8::make(count));
}

ref<IRInstruction> Builder::emit_ret() {
  return emit(Opcode::ret);
}

// load operations
ref<IRInstruction> Builder::emit_load(OpImmediate value) {
  return emit(Opcode::load, IROperandImmediate::make(value));
}

ref<IRInstruction> Builder::emit_loadsymbol(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::loadsymbol, IROperandSymbol::make(symbol));
}

ref<IRInstruction> Builder::emit_loadself() {
  return emit(Opcode::loadself);
}

ref<IRInstruction> Builder::emit_loadargc() {
  return emit(Opcode::loadargc);
}

ref<IRInstruction> Builder::emit_loadglobal(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::loadglobal, IROperandSymbol::make(symbol));
}

ref<IRInstruction> Builder::emit_loadlocal(OpCount8 offset) {
  return emit(Opcode::loadlocal, IROperandCount8::make(offset));
}

ref<IRInstruction> Builder::emit_loadfar(OpCount8 depth, OpCount8 offset) {
  return emit(Opcode::loadfar, IROperandCount8::make(depth), IROperandCount8::make(offset));
}

ref<IRInstruction> Builder::emit_loadattr(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::loadattr, IROperandSymbol::make(symbol));
}

ref<IRInstruction> Builder::emit_loadattrsym() {
  return emit(Opcode::loadattrsym);
}

ref<IRInstruction> Builder::emit_loadsuperconstructor() {
  return emit(Opcode::loadsuperconstructor);
}

ref<IRInstruction> Builder::emit_loadsuperattr(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::loadsuperattr, IROperandSymbol::make(symbol));
}

// write operations
ref<IRInstruction> Builder::emit_setglobal(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::setglobal, IROperandSymbol::make(symbol));
}

ref<IRInstruction> Builder::emit_setlocal(OpCount8 offset) {
  return emit(Opcode::setlocal, IROperandCount8::make(offset));
}

ref<IRInstruction> Builder::emit_setreturn() {
  return emit(Opcode::setreturn);
}

ref<IRInstruction> Builder::emit_setfar(OpCount8 depth, OpCount8 offset) {
  return emit(Opcode::setfar, IROperandCount8::make(depth), IROperandCount8::make(offset));
}

ref<IRInstruction> Builder::emit_setattr(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::setattr, IROperandSymbol::make(symbol));
}

ref<IRInstruction> Builder::emit_setattrsym() {
  return emit(Opcode::setattrsym);
}

// value destructuring operations
ref<IRInstruction> Builder::emit_unpacksequence(OpCount8 count) {
  return emit(Opcode::unpacksequence, IROperandCount8::make(count));
}

ref<IRInstruction> Builder::emit_unpacksequencespread(OpCount8 before, OpCount8 after) {
  return emit(Opcode::unpacksequencespread, IROperandCount8::make(before), IROperandCount8::make(after));
}

ref<IRInstruction> Builder::emit_unpackobject(OpCount8 count) {
  return emit(Opcode::unpackobject, IROperandCount8::make(count));
}

ref<IRInstruction> Builder::emit_unpackobjectspread(OpCount8 count) {
  return emit(Opcode::unpackobjectspread, IROperandCount8::make(count));
}

// value allocation
ref<IRInstruction> Builder::emit_makefunc(OpOffset offset) {
  return emit(Opcode::makefunc, IROperandOffset::make(offset));
}

ref<IRInstruction> Builder::emit_makeclass(OpSymbol name,
                                                     OpCount8 funccount,
                                                     OpCount8 propcount,
                                                     OpCount8 staticpropcount) {
  register_symbol(name);
  return emit(Opcode::makeclass, IROperandSymbol::make(name), IROperandCount8::make(funccount),
              IROperandCount8::make(propcount), IROperandCount8::make(staticpropcount));
}

ref<IRInstruction> Builder::emit_makesubclass(OpSymbol name,
                                                        OpCount8 funccount,
                                                        OpCount8 propcount,
                                                        OpCount8 staticpropcount) {
  register_symbol(name);
  return emit(Opcode::makesubclass, IROperandSymbol::make(name), IROperandCount8::make(funccount),
              IROperandCount8::make(propcount), IROperandCount8::make(staticpropcount));
}

ref<IRInstruction> Builder::emit_makestr(OpIndex16 index) {
  return emit(Opcode::makestr, IROperandIndex16::make(index));
}

ref<IRInstruction> Builder::emit_makelist(OpCount16 count) {
  return emit(Opcode::makelist, IROperandCount16::make(count));
}

ref<IRInstruction> Builder::emit_makelistspread(OpCount16 count) {
  return emit(Opcode::makelistspread, IROperandCount16::make(count));
}

ref<IRInstruction> Builder::emit_makedict(OpCount16 count) {
  return emit(Opcode::makedict, IROperandCount16::make(count));
}

ref<IRInstruction> Builder::emit_makedictspread(OpCount16 count) {
  return emit(Opcode::makedictspread, IROperandCount16::make(count));
}

ref<IRInstruction> Builder::emit_maketuple(OpCount16 count) {
  return emit(Opcode::maketuple, IROperandCount16::make(count));
}

ref<IRInstruction> Builder::emit_maketuplespread(OpCount16 count) {
  return emit(Opcode::maketuplespread, IROperandCount16::make(count));
}

// fiber management
ref<IRInstruction> Builder::emit_fiberspawn() {
  return emit(Opcode::fiberspawn);
}

ref<IRInstruction> Builder::emit_fiberyield() {
  return emit(Opcode::fiberyield);
}

ref<IRInstruction> Builder::emit_fibercall() {
  return emit(Opcode::fibercall);
}

ref<IRInstruction> Builder::emit_fiberpause() {
  return emit(Opcode::fiberpause);
}

ref<IRInstruction> Builder::emit_fiberresume() {
  return emit(Opcode::fiberresume);
}

ref<IRInstruction> Builder::emit_fiberawait() {
  return emit(Opcode::fiberawait);
}

// cast operations
ref<IRInstruction> Builder::emit_caststring() {
  return emit(Opcode::caststring);
}

ref<IRInstruction> Builder::emit_castsymbol() {
  return emit(Opcode::castsymbol);
}

ref<IRInstruction> Builder::emit_castiterator() {
  return emit(Opcode::castiterator);
}

// iterator operations
ref<IRInstruction> Builder::emit_iteratornext() {
  return emit(Opcode::iteratornext);
}

// arithmetic operations
ref<IRInstruction> Builder::emit_add() {
  return emit(Opcode::add);
}

ref<IRInstruction> Builder::emit_sub() {
  return emit(Opcode::sub);
}

ref<IRInstruction> Builder::emit_mul() {
  return emit(Opcode::mul);
}

ref<IRInstruction> Builder::emit_div() {
  return emit(Opcode::div);
}

ref<IRInstruction> Builder::emit_mod() {
  return emit(Opcode::mod);
}

ref<IRInstruction> Builder::emit_pow() {
  return emit(Opcode::pow);
}

ref<IRInstruction> Builder::emit_usub() {
  return emit(Opcode::usub);
}

// arithmetic operations
ref<IRInstruction> Builder::emit_eq() {
  return emit(Opcode::eq);
}

ref<IRInstruction> Builder::emit_neq() {
  return emit(Opcode::neq);
}

ref<IRInstruction> Builder::emit_lt() {
  return emit(Opcode::lt);
}

ref<IRInstruction> Builder::emit_gt() {
  return emit(Opcode::gt);
}

ref<IRInstruction> Builder::emit_le() {
  return emit(Opcode::le);
}

ref<IRInstruction> Builder::emit_ge() {
  return emit(Opcode::ge);
}

ref<IRInstruction> Builder::emit_unot() {
  return emit(Opcode::unot);
}

// bitwise operations
ref<IRInstruction> Builder::emit_shl() {
  return emit(Opcode::shl);
}

ref<IRInstruction> Builder::emit_shr() {
  return emit(Opcode::shr);
}

ref<IRInstruction> Builder::emit_shru() {
  return emit(Opcode::shru);
}

ref<IRInstruction> Builder::emit_band() {
  return emit(Opcode::band);
}

ref<IRInstruction> Builder::emit_bor() {
  return emit(Opcode::bor);
}

ref<IRInstruction> Builder::emit_bxor() {
  return emit(Opcode::bxor);
}

ref<IRInstruction> Builder::emit_ubnot() {
  return emit(Opcode::ubnot);
}

uint32_t Builder::maximum_stack_height() const {
  return m_maximum_stack_height;
};

void Builder::reset_stack_height() {
  m_maximum_stack_height = 0;
  assert(m_current_stack_height == 0);
}

void Builder::update_stack(int32_t amount) {
  if (amount < 0) {
    assert((uint32_t)-amount <= m_current_stack_height);
  }

  // std::cout << " stack " << (int)m_current_stack_height << " += " << amount << std::endl;
  m_current_stack_height += amount;

  if (m_current_stack_height > m_maximum_stack_height) {
    m_maximum_stack_height = m_current_stack_height;
  }
}

}  // namespace charly::core::compiler::ir
