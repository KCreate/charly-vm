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

#include "charly/core/compiler/ir/assembler.h"

namespace charly::core::compiler::ir {

using namespace charly::core::compiler::ast;
using namespace charly::core::compiler::ir;
using namespace charly::core::runtime;
using namespace charly::utils;

CompiledModule* Assembler::assemble() {

  // setup module
  CompiledModule* compiled_module = new CompiledModule();
  compiled_module->filename = m_module->filename;
  compiled_module->symbol_table = m_module->symbol_table;

  // emit functions
  assert(m_module->functions.size() <= (size_t)0xffff);
  for (const ref<IRFunction>& function : m_module->functions) {

    // init function entry
    CompiledFunction* compiled_func = new CompiledFunction();
    compiled_func->owner_module = compiled_module;
    compiled_module->function_table.push_back(compiled_func);
    compiled_func->name = SYM(function->ast->name->value);
    compiled_func->ir_info = function->ast->ir_info;
    assert(function->ast->ir_info.valid);

    // build string table
    assert(function->string_table.size() <= (size_t)0x0000ffff);
    for (const auto& entry : function->string_table) {
      compiled_func->string_table.push_back(StringTableEntry(entry.value));
    }

    // generate the inline BytecodeFunctionHeader struct
    Label inline_cache_section = reserve_label();
    Label bytecode_section = reserve_label();
    align_to_pointer();
    place_label(function->head);
    m_buffer->write_ptr(compiled_func);
    write_relative_label_reference(inline_cache_section, function->head);
    write_relative_label_reference(bytecode_section, function->head);

    // emit inline cache section
    align_to_pointer();
    place_label(inline_cache_section);
    assert(function->inline_cache_table.size() <= (size_t)0x0000ffff);
    for (const auto& entry : function->inline_cache_table) {
      (void)entry;
      m_buffer->write_zeroes(sizeof(InlineCacheEntry));
    }

    // emit bytecodes
    align_to_pointer();
    place_label(bytecode_section);
    for (const ref<IRBasicBlock>& block : function->basic_blocks) {
      for (Label label : block->labels) {
        place_label(label);
      }

      for (const ref<IRInstruction>& instruction : block->instructions) {
        encode_instruction(instruction);
      }
    }

    compiled_func->inline_cache_offset = offset_of_label(inline_cache_section);
    compiled_func->bytecode_offset = offset_of_label(bytecode_section);

    // build exception table
    for (const IRExceptionTableEntry& entry : function->exception_table) {
      compiled_func->exception_table.emplace_back(
        offset_of_label(entry.begin),
        offset_of_label(entry.end),
        offset_of_label(entry.handler)
      );
    }

    // build sourcemap table
    for (const ref<IRBasicBlock>& block : function->basic_blocks) {
      for (const ref<IRInstruction>& instruction : block->instructions) {
        if (instruction->location.valid) {
          assert(instruction->assembled_at_label.has_value());
          uint32_t instruction_offset = offset_of_label(instruction->assembled_at_label.value());
          compiled_func->sourcemap_table.emplace_back(
            instruction_offset,
            instruction->location.row,
            instruction->location.column,
            instruction->location.end_row,
            instruction->location.end_column
          );
        }
      }
    }
  }

  // fill placeholders with the actual offsets to the labels
  patch_unresolved_labels();

  return compiled_module;
}

void Assembler::align_to_pointer() {
  while (m_buffer->size() % 8) {
    m_buffer->write_u8(0);
  }
}

void Assembler::encode_instruction(const ref<IRInstruction>& instruction) {
  Label instruction_label = reserve_label();
  instruction->assembled_at_label = instruction_label;

  // opcode
  place_label(instruction_label);
  m_buffer->write_u8(static_cast<uint8_t>(instruction->opcode));

  // operands
  for (const ref<IROperand>& op : instruction->operands) {
    switch (op->get_type()) {
      case OperandType::Count8: {
        m_buffer->write_u8(cast<IROperandCount8>(op)->value);
        break;
      }
      case OperandType::Count16: {
        m_buffer->write_u16(cast<IROperandCount16>(op)->value);
        break;
      }
      case OperandType::Index16: {
        m_buffer->write_u16(cast<IROperandIndex16>(op)->value);
        break;
      }
      case OperandType::Symbol: {
        m_buffer->write_u32(SYM(cast<IROperandSymbol>(op)->value));
        break;
      }
      case OperandType::Offset: {
        write_relative_label_reference(cast<IROperandOffset>(op)->value, instruction_label);
        break;
      }
      case OperandType::Immediate: {
        m_buffer->write_ptr(cast<IROperandImmediate>(op)->value.raw);
        break;
      }
    }
  }

  // inline cache index
  if (instruction->inline_cache_index) {
    m_buffer->write_u16(instruction->inline_cache_index.value());
  }
}

Label Assembler::reserve_label() {
  return m_label_counter++;
}

void Assembler::place_label(Label label) {
  assert(m_placed_labels.count(label) == 0);
  m_placed_labels.insert({label, m_buffer->size()});
}

void Assembler::write_relative_label_reference(Label label, Label other) {
  m_unresolved_labels.insert({m_buffer->cursor(), { label, true, other }});
  m_buffer->write_u32(0x0);
}

void Assembler::write_absolute_label_reference(Label label) {
  m_unresolved_labels.insert({m_buffer->cursor(), { label, false, std::nullopt }});
  m_buffer->write_u32(0x0);
}

void Assembler::patch_unresolved_labels() {
  for (const auto& entry : m_unresolved_labels) {
    uint32_t patch_offset = entry.first;
    Label referenced_label = entry.second.label;
    bool relative_reference = entry.second.relative;

    // get target offset
    assert(m_placed_labels.count(referenced_label));
    uint32_t referenced_offset = m_placed_labels.at(referenced_label);

    if (relative_reference) {
      int32_t relative_offset = referenced_offset - patch_offset;
      m_buffer->seek(patch_offset);
      m_buffer->write_i32(relative_offset);
    } else {
      m_buffer->seek(patch_offset);
      m_buffer->write_u32(referenced_offset);
    }
  }

  m_buffer->seek(-1);
}

uint32_t Assembler::offset_of_label(Label label) {
  assert(m_placed_labels.count(label));
  return m_placed_labels.at(label);
}

}  // namespace charly::core::compiler::ir
