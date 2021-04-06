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
using namespace charly::utils;

ref<MemoryBlock> Assembler::assemble() {
  Label function_table_label = reserve_label();
  write_label_reference(function_table_label);

  // module filename
  write_label_reference(m_module->filename_label);

  // string table
  assert(m_module->string_table.size() <= (size_t)0x0000ffff);
  m_buffer->write_u16(m_module->string_table.size());
  for (const IRStringTableEntry& entry : m_module->string_table) {
    write_label_reference(entry.label);
  }
  for (const IRStringTableEntry& entry : m_module->string_table) {
    align_to_pointer();
    place_label(entry.label);
    m_buffer->write_u32(entry.hash);
    m_buffer->write_u32(entry.value.size());
    m_buffer->write_string(entry.value);
  }

  // function table
  assert(m_module->functions.size() <= (size_t)0x0000ffff);
  place_label(function_table_label);
  m_buffer->write_u16(m_module->functions.size());
  for (const ref<IRFunction>& function : m_module->functions) {
    write_label_reference(function->head);
  }

  // emit functions
  for (const ref<IRFunction>& function : m_module->functions) {
    Label exception_tables = reserve_label();
    Label sourcemap_label = reserve_label();
    Label inline_cache_label = reserve_label();
    Label bytecodes_label = reserve_label();

    align_to_pointer();
    place_label(function->head);

    // function runtime shared info pointer placeholder
    m_buffer->write_u64(0);

    // section forward ptrs
    write_label_reference(exception_tables);
    write_label_reference(sourcemap_label);
    write_label_reference(inline_cache_label);
    write_label_reference(bytecodes_label);

    // function info
    m_buffer->write_u32(SYM(function->ast->name->value));
    m_buffer->write_u8(function->ast->ir_info.local_variables);
    m_buffer->write_u8(function->ast->ir_info.argc);
    m_buffer->write_u8(function->ast->ir_info.minargc);
    m_buffer->write_u8(function->ast->ir_info.stacksize);
    m_buffer->write_u8(function->ast->ir_info.spread_argument);
    m_buffer->write_u8(function->ast->ir_info.arrow_function);
    m_buffer->write_u8(function->ast->ir_info.leaked);

    // exception tables
    align_to_pointer();
    place_label(exception_tables);
    assert((function->exception_table.size() <= (size_t)0x0000ffff) && "function exceeds maximum exception table count");
    m_buffer->write_u16(function->exception_table.size());
    for (const IRExceptionTableEntry& entry : function->exception_table) {
      m_buffer->write_u32(entry.begin);
      m_buffer->write_u32(entry.end);
      m_buffer->write_u32(entry.handler);
    }

    // sourcemap
    // TODO: compress this somehow
    align_to_pointer();
    place_label(sourcemap_label);
    m_buffer->write_u16(0xff);
    uint16_t sourcemap_count = 0;
    for (const ref<IRBasicBlock>& block : function->basic_blocks) {
      for (const ref<IRInstruction>& instruction : block->instructions) {
        if (instruction->location.valid) {
          Label l = reserve_label();
          instruction->assembled_offset = l;

          write_label_reference(l);
          m_buffer->write_u16(instruction->location.row);
          m_buffer->write_u16(instruction->location.column);
          m_buffer->write_u16(instruction->location.end_row);
          m_buffer->write_u16(instruction->location.end_column);

          sourcemap_count++;
        }
      }
    }

    // patch sourcemap entry count
    m_buffer->seek(m_placed_labels.at(sourcemap_label));
    m_buffer->write_u16(sourcemap_count);
    m_buffer->seek(-1);

    // inline caches
    place_label(inline_cache_label);
    m_buffer->write_u32(function->inline_cache_table.size());
    for (const auto& entry : function->inline_cache_table) {
      (void)entry;
      // allocate 32 bytes for each inline cache side table entry
      m_buffer->write_u64(0x00);
      m_buffer->write_u64(0x00);
      m_buffer->write_u64(0x00);
      m_buffer->write_u64(0x00);
    }

    // emit instructions
    place_label(bytecodes_label);
    for (const ref<IRBasicBlock>& block : function->basic_blocks) {
      for (Label label : block->labels) {
        place_label(label);
      }

      for (const ref<IRInstruction>& instruction : block->instructions) {
        encode_instruction(instruction);
      }
    }
  }

  // fill placeholders with the actual offsets to the labels
  patch_unresolved_labels();

  m_buffer->seek(-1);

  return m_buffer;
}

void Assembler::align_to_pointer() {
  while (m_buffer->size() % 8) {
    m_buffer->write_u8(0);
  }
}

void Assembler::encode_instruction(const ref<IRInstruction>& instruction) {

  // place label for sourcemap
  if (instruction->assembled_offset) {
    place_label(instruction->assembled_offset.value());
  }

  // opcode
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
      case OperandType::Symbol: {
        m_buffer->write_u32(SYM(cast<IROperandSymbol>(op)->value));
        break;
      }
      case OperandType::Offset: {
        write_label_reference(cast<IROperandOffset>(op)->value);
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

void Assembler::write_label_reference(Label label) {
  m_unresolved_labels.insert({m_buffer->cursor(), label});
  m_buffer->write_u32(0x0);
}

void Assembler::patch_unresolved_labels() {
  for (const auto& entry : m_unresolved_labels) {
    uint32_t offset = entry.first;
    Label label = entry.second;

    assert(m_placed_labels.count(label));
    m_buffer->seek(offset);
    m_buffer->write_u32(m_placed_labels.at(label));
  }
}

}  // namespace charly::core::compiler::ir
