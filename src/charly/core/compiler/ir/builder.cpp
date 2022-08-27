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

#include <deque>

#include "charly/core/compiler/ir/builder.h"
#include "charly/utils/argumentparser.h"
#include "charly/utils/buffer.h"
#include "charly/utils/colorwriter.h"

namespace charly::core::compiler::ir {

using namespace charly::core::compiler::ast;
using namespace charly::core::runtime;

using Color = utils::Color;

uint16_t Builder::register_string(const std::string& string) {
  ref<IRFunction> function = m_active_function;

  // duplicates check
  uint16_t index = 0;
  for (const IRStringTableEntry& entry : function->string_table) {
    if (entry.value == string) {
      return index;
    }

    index++;
  }

  function->string_table.emplace_back(IRStringTableEntry(string));
  return index;
}

SYMBOL Builder::register_symbol(const std::string& string) {
  uint16_t index = register_string(string);
  return m_active_function->string_table.at(index).hash;
}

uint16_t Builder::register_constant(RawValue value) {
  ref<IRFunction> function = m_active_function;

  auto& table = function->constant_table;

  // check if the value is already in the table
  for (size_t i = 0; i < table.size(); i++) {
    if (table[i].raw() == value.raw()) {
      return i;
    }
  }

  table.push_back(value);
  return table.size() - 1;
}

void Builder::push_exception_handler(Label handler) {
  m_exception_handlers.push(handler);
  new_basic_block();
}

void Builder::pop_exception_handler() {
  m_exception_handlers.pop();
  new_basic_block();
}

ref<IRFunction> Builder::active_function() const {
  DCHECK(m_active_function);
  return m_active_function;
}

ref<IRBasicBlock> Builder::active_block() const {
  DCHECK(m_active_block);
  return m_active_block;
}

void Builder::begin_function(Label head, const ref<ast::Function>& ast) {
  m_active_function = make<IRFunction>(head, ast);
  m_module->functions.push_back(m_active_function);
  register_symbol(ast->name->value);
  new_basic_block();
}

void Builder::finish_function() {
  if (!utils::ArgumentParser::is_flag_set("no_ir_opt")) {
    // trim dead instructions in basic blocks that occur after terminating opcodes
    trim_dead_instructions();

    // remove basic blocks with no instructions
    remove_empty_blocks();
  }

  // build the control flow graph, storing the incoming and outgoing
  // branches for each basic block
  build_cfg();

  if (!utils::ArgumentParser::is_flag_set("no_ir_opt")) {
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

    // merges continuous blocks into a single block
    //
    //    loadsmi 25
    //    pop
    //  .L2
    //    loadsmi 25
    //    pop
    //
    // becomes just (given that no other instructions reference L2)
    //
    //    loadsmi 25
    //    pop
    //    loadsmi 25
    //    pop
    merge_continuous_blocks();
  }

  // emit the exceptions tables for the basic blocks of this function
  //
  // each exception table entry maps a certain region of code
  // to a handler address, if an exception were to occur within that region
  new_basic_block();
  emit_nop();
  emit_exception_tables();

  // validate stack interactions and determine the maximum height of the stack
  determine_max_stack_height();

  // reset builder for next function
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
    if (block->instructions.empty()) {
      // propagate labels to next block
      if (block->next_block && !block->labels.empty()) {
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
  for (const ref<IRBasicBlock>& block : m_active_function->basic_blocks) {
    // empty blocks fallthrough to the next block
    if (block->instructions.empty()) {
      IRBasicBlock::link(block, block->next_block);
      continue;
    }

    // populate exception handler source table
    if (block->exception_handler.has_value()) {
      auto handler_block = m_labelled_blocks.at(block->exception_handler.value());
      handler_block->exception_handler_sources.insert(block);
    }

    ref<IRInstruction> op = block->instructions.back();
    switch (op->opcode) {
      case Opcode::jmp: {
        Label target_label = op->as_jmp()->arg;
        ref<IRBasicBlock> target_block = m_labelled_blocks.at(target_label);
        IRBasicBlock::link(block, target_block);
        continue;
      }
      case Opcode::jmpf:
      case Opcode::jmpt: {
        Label target_label = op->as_iaax()->arg;
        ref<IRBasicBlock> target_block = m_labelled_blocks.at(target_label);
        DCHECK(block->next_block);
        IRBasicBlock::link(block, target_block);
        IRBasicBlock::link(block, block->next_block);
        continue;
      }
      case Opcode::argcjmp: {
        Label target_label = op->as_argcjmp()->arg2;
        ref<IRBasicBlock> target_block = m_labelled_blocks.at(target_label);
        DCHECK(block->next_block);
        IRBasicBlock::link(block, target_block);
        IRBasicBlock::link(block, block->next_block);
        continue;
      }
      case Opcode::ret:
      case Opcode::throwex:
      case Opcode::rethrowex:
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
  bool updated_jmp;

  do {
    updated_jmp = false;
    for (const ref<IRBasicBlock>& block : m_active_function->basic_blocks) {
      // skip empty blocks
      if (block->instructions.empty()) {
        continue;
      }

      // jmp instructions that branch to a basic block that contains
      // only a single jmp op, can be replaced with the
      // second jmp op
      ref<IRInstruction>& op = block->instructions.back();
      if (op->opcode == Opcode::jmp) {
        ref<IRBasicBlock> target_block = *block->outgoing_blocks.begin();

        // catch infinite loops
        if (target_block.get() == block.get()) {
          continue;
        }

        // check if the target block contains a single op
        if (target_block->instructions.size() == 1) {
          const ref<IRInstruction>& target_op = target_block->instructions.back();

          if (target_op->opcode == Opcode::jmp) {
            // determine target block of the second jmp and update the original jmp op
            Label second_target_label = target_op->as_jmp()->arg;
            ref<IRBasicBlock> new_target_block = m_labelled_blocks.at(second_target_label);

            // catch infinite loops
            if (target_block.get() == new_target_block.get()) {
              continue;
            }

            // unlink original branch
            IRBasicBlock::unlink(block, target_block);
            IRBasicBlock::link(block, new_target_block);
            op->as_jmp()->arg = second_target_label;
            updated_jmp = true;
          } else if (target_op->opcode == Opcode::ret) {
            // replace source jmp opcode with a ret
            op = make<IRInstruction_ret>();
            IRBasicBlock::unlink(block, target_block);
          }
        }
      }
    }
  } while (updated_jmp);
}

void Builder::remove_useless_jumps() {
  for (const ref<IRBasicBlock>& block : m_active_function->basic_blocks) {
    if (!block->instructions.empty()) {
      ref<IRInstruction> op = block->instructions.back();
      switch (op->opcode) {
        case Opcode::jmp: {
          Label target_label = op->as_jmp()->arg;
          if (block->next_block && block->next_block->labels.count(target_label)) {
            block->instructions.pop_back();
          }

          continue;
        }
        case Opcode::jmpf: {
          Label target_label = op->as_jmpf()->arg;
          if (block->next_block && block->next_block->labels.count(target_label)) {
            block->instructions.pop_back();
            emit_pop(block);
          }

          continue;
        }
        case Opcode::jmpt: {
          Label target_label = op->as_jmpt()->arg;
          if (block->next_block && block->next_block->labels.count(target_label)) {
            block->instructions.pop_back();
            emit_pop(block);
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
  while (!reachable_blocks.empty()) {
    ref<IRBasicBlock> block = reachable_blocks.front();
    reachable_blocks.pop_front();

    if (!block->reachable) {
      block->reachable = true;

      for (const ref<IRBasicBlock>& outgoing : block->outgoing_blocks) {
        reachable_blocks.push_back(outgoing);
      }

      if (block->exception_handler.has_value()) {
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

void Builder::merge_continuous_blocks() {
  auto it = m_active_function->basic_blocks.begin();
  while (it != m_active_function->basic_blocks.end()) {
    auto block = *it;

    // link continuous blocks
    auto next_block = block->next_block;
    if (next_block == nullptr) {
      it++;
      continue;
    }

    DCHECK(next_block == *std::next(it));
    DCHECK(block == next_block->previous_block);

    // blocks with exception handlers or blocks that handle exceptions cannot be merged
    if (!block->exception_handler.has_value() && block->exception_handler_sources.empty()) {
      if (!next_block->exception_handler.has_value() && next_block->exception_handler_sources.empty()) {
        // blocks must be directly connected
        if (block->outgoing_blocks.size() == 1 && next_block->incoming_blocks.size() == 1) {
          if (*block->outgoing_blocks.begin() == next_block && block == *next_block->incoming_blocks.begin()) {
            auto& next_ops = next_block->instructions;
            block->instructions.insert(block->instructions.end(), next_ops.begin(), next_ops.end());

            IRBasicBlock::unlink(block, next_block);
            auto& next_outgoing_blocks = next_block->outgoing_blocks;
            for (auto& next_outgoing_block : next_outgoing_blocks) {
              IRBasicBlock::unlink(next_block, next_outgoing_block);
              IRBasicBlock::link(block, next_outgoing_block);
            }

            it = m_active_function->basic_blocks.erase(std::next(it));
            continue;
          }
        }
      }
    }

    it++;
  }
}

void Builder::emit_exception_tables() {
  for (const ref<IRBasicBlock>& block : m_active_function->basic_blocks) {
    if (block->exception_handler.has_value()) {
      DCHECK(!block->labels.empty());
      DCHECK(block->next_block);
      DCHECK(!block->next_block->labels.empty());

      Label begin = *block->labels.begin();
      Label end = *block->next_block->labels.begin();
      Label handler = block->exception_handler.value();

      // extend previous table if possible
      if (!m_active_function->exception_table.empty()) {
        IRExceptionTableEntry& previous = m_active_function->exception_table.back();

        if (previous.end == begin && previous.handler == handler) {
          previous.end = end;
        } else {
          m_active_function->exception_table.emplace_back(IRExceptionTableEntry{ begin, end, handler });
        }
      } else {
        m_active_function->exception_table.emplace_back(IRExceptionTableEntry{ begin, end, handler });
      }
    }
  }
}

void Builder::determine_max_stack_height() {
  std::deque<ref<IRBasicBlock>> queue;
  std::unordered_set<ref<IRBasicBlock>> visited_blocks;
  std::unordered_map<ref<IRBasicBlock>, uint32_t> block_initial_heights;
  uint32_t max_stack_height = 0;

  auto func = active_function();
  auto initial_block = func->basic_blocks.front();
  queue.push_back(initial_block);
  block_initial_heights[initial_block] = 0;

  // push exception handler blocks as they are not reachable via regular control flow
  for (const IRExceptionTableEntry& entry : func->exception_table) {
    DCHECK(m_labelled_blocks.count(entry.handler) == 1);
    auto block = m_labelled_blocks.at(entry.handler);
    queue.push_back(block);
    block_initial_heights[block] = 0;
  }

  while (queue.size()) {
    auto block = queue.front();
    queue.pop_front();

    if (visited_blocks.count(block) == 1) {
      continue;
    }

    visited_blocks.insert(block);

    DCHECK(block_initial_heights.count(block) == 1);
    uint32_t stack_height = block_initial_heights[block];

    for (const auto& op : block->instructions) {
      uint32_t popped_values = op->popped_values();
      uint32_t pushed_values = op->pushed_values();

      DCHECK(stack_height >= popped_values);
      stack_height -= popped_values;
      stack_height += pushed_values;

      if (stack_height > max_stack_height) {
        max_stack_height = stack_height;
      }
    }

    if (block->outgoing_blocks.empty()) {
      DCHECK(stack_height == 0, "function %: expected stack height of terminating block % to be 0, got % instead",
             func->ast->name->value, block->id, stack_height);
    }

    // set the initial block height of following blocks
    // also make sure that previous values match the currently calculated one
    for (const auto& next_block : block->outgoing_blocks) {
      if (block_initial_heights.count(next_block) == 1) {
        uint32_t next_height = block_initial_heights[next_block];
        DCHECK(stack_height == next_height, "function %: invalid stack heights for blocks % and % (got % and %)",
               func->ast->name->value, block->id, next_block->id, stack_height, next_height);
      } else {
        queue.push_back(next_block);
        block_initial_heights[next_block] = stack_height;
      }
    }
  }

  DCHECK(max_stack_height < 256, "function % exceeded maximum stack height of 256 values", func->ast->name->value);
  active_function()->ast->ir_info.stacksize = max_stack_height;
}

ref<IRBasicBlock> Builder::new_basic_block() {
  DCHECK(m_active_function);

  ref<IRBasicBlock> block = make<IRBasicBlock>(m_block_id_counter++);
  block->previous_block = m_active_block;

  if (!m_exception_handlers.empty()) {
    block->exception_handler = m_exception_handlers.top();
  }

  if (m_active_block) {
    m_active_block->next_block = block;

    // each block must have at least one label
    if (m_active_block->labels.empty()) {
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
  if (m_active_block->instructions.empty()) {
    m_active_block->labels.insert(label);
    m_labelled_blocks.insert({ label, m_active_block });
    return;
  }

  // create a new block
  ref<IRBasicBlock> new_block = new_basic_block();
  new_block->labels.insert(label);
  m_labelled_blocks.insert({ label, new_block });
}

ref<IRInstruction> Builder::emit_load_value(RawValue value) {
  // check if immediate value fits into the 3 byte opcode argument
  if ((value.raw() & 0xffffffffff000000) == 0) {
    return emit_loadsmi(value.raw());
  } else {
    return emit_loadconst(register_constant(value));
  }
}

ref<IRInstruction> Builder::emit_instruction_impl(const ref<IRBasicBlock>& block,
                                                  const ref<IRInstruction>& instruction) {
  if (block->instructions.empty()) {
    place_label(reserve_label());
  }

  block->instructions.push_back(instruction);

  if (kBranchingOpcodes.count(instruction->opcode)) {
    new_basic_block();
  }

  return instruction;
}

}  // namespace charly::core::compiler::ir
