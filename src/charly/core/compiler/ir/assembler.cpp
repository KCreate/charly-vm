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

#include "charly/core/compiler/ir/assembler.h"

namespace charly::core::compiler::ir {

using namespace charly::core::compiler::ast;
using namespace charly::core::compiler::ir;
using namespace charly::core::runtime;
using namespace charly::utils;

ref<CompiledModule> Assembler::compile_module(const ref<IRModule>& module) {
  Assembler assembler(module);
  assembler.assemble();
  return assembler.m_runtime_module;
}

void Assembler::assemble() {
  m_runtime_module = make<CompiledModule>();
  m_runtime_module->filename = m_ir_module->filename;

  // emit functions
  CHECK(m_ir_module->functions.size() <= kUInt16Max);
  for (const ref<IRFunction>& function : m_ir_module->functions) {
    auto shared_info = new SharedFunctionInfo();
    m_runtime_module->function_table.push_back(shared_info);
    shared_info->name = function->ast->name->value;
    shared_info->name_symbol = crc32::hash_string(function->ast->name->value);
    shared_info->ir_info = function->ast->ir_info;
    DCHECK(function->ast->ir_info.valid);

    // build string table
    CHECK(function->string_table.size() <= kUInt16Max);
    for (const auto& entry : function->string_table) {
      shared_info->string_table.emplace_back(entry.value);
    }

    // build constant table
    CHECK(function->constant_table.size() <= kUInt16Max);
    for (const auto& value : function->constant_table) {
      shared_info->constant_table.push_back(value);
    }

    // emit shared info struct pointer into bytecode stream
    // the offset inside makefunc instructions points to this pointer value
    Label end_label = reserve_label();
    Label bytecode_section = reserve_label();
    align_to_pointer();
    place_label(function->head);
    m_runtime_module->buffer.write_ptr((uintptr_t)shared_info);

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
    place_label(end_label);

    shared_info->bytecode_offset = offset_of_label(bytecode_section);
    shared_info->end_offset = offset_of_label(end_label);

    // build exception table
    for (const IRExceptionTableEntry& entry : function->exception_table) {
      shared_info->exception_table.emplace_back(offset_of_label(entry.begin), offset_of_label(entry.end),
                                                offset_of_label(entry.handler));
    }

    // build sourcemap table
    for (const ref<IRBasicBlock>& block : function->basic_blocks) {
      for (const ref<IRInstruction>& instruction : block->instructions) {
        if (instruction->location.valid) {
          DCHECK(instruction->assembled_at_label.has_value());
          uint32_t instruction_offset = offset_of_label(instruction->assembled_at_label.value());
          shared_info->sourcemap_table.emplace_back(instruction_offset, instruction->location.row,
                                                    instruction->location.column, instruction->location.end_row,
                                                    instruction->location.end_column);
        }
      }
    }
  }

  // fill placeholders with the actual offsets to the labels
  patch_unresolved_labels();

  // enable memory protection
  m_runtime_module->buffer.protect();

  // write final pointers into function structs
  auto base_address = (uintptr_t)m_runtime_module->buffer.data();
  for (SharedFunctionInfo* func : m_runtime_module->function_table) {
    func->buffer_base_ptr = base_address;
    func->bytecode_base_ptr = base_address + func->bytecode_offset;
    func->end_ptr = base_address + func->end_offset;

    // populate exception table pointers
    for (ExceptionTableEntry& entry : func->exception_table) {
      entry.begin_ptr = base_address + entry.begin_offset;
      entry.end_ptr = base_address + entry.end_offset;
      entry.handler_ptr = base_address + entry.handler_offset;
    }

    // populate source map pointers
    for (SourceMapEntry& entry : func->sourcemap_table) {
      entry.instruction_ptr = base_address + entry.instruction_offset;
    }
  }
}

void Assembler::align_to_pointer() {
  while (m_runtime_module->buffer.size() % 8) {
    m_runtime_module->buffer.write_u8(0);
  }
}

void Assembler::encode_instruction(const ref<IRInstruction>& op) {
  Label instruction_label = reserve_label();
  op->assembled_at_label = instruction_label;

  auto& buffer = m_runtime_module->buffer;

  place_label(instruction_label);
  switch (op->opcode) {
    case Opcode::jmp:
    case Opcode::jmpf:
    case Opcode::jmpt:
    case Opcode::makefunc: {
      Label label = op->as_iaax()->arg;
      m_unresolved_labels.insert({ m_runtime_module->buffer.tellp(), { label, instruction_label } });
      break;
    }
    case Opcode::argcjmp: {
      Label label = op->as_iabb()->arg2;
      m_unresolved_labels.insert({ m_runtime_module->buffer.tellp(), { label, instruction_label } });
      break;
    }
    default: break;
  }
  buffer.write_u32(op->encode());
}

Label Assembler::reserve_label() {
  return m_label_counter++;
}

void Assembler::place_label(Label label) {
  DCHECK(m_placed_labels.count(label) == 0);
  m_placed_labels.insert({ label, m_runtime_module->buffer.tellp() });
}

void Assembler::patch_unresolved_labels() {
  utils::Buffer& buf = m_runtime_module->buffer;

  auto p = buf.tellp();
  auto g = buf.tellg();

  for (const auto& entry : m_unresolved_labels) {
    Label referenced_label = entry.second.label;
    Label base = entry.second.base;

    // calculate the relative offset
    int64_t referenced_offset = offset_of_label(referenced_label);
    int64_t base_offset = offset_of_label(base);
    int64_t relative_offset = referenced_offset - base_offset;
    CHECK(relative_offset >= kInt16Min && relative_offset <= kInt16Max);

    // read the opcode at the patch offset to determine how to update it
    uint32_t patch_offset = entry.first;
    buf.seekg(patch_offset);
    auto read_byte = buf.peek_char();
    CHECK(read_byte != EOF);

    auto opcode = (Opcode)read_byte;
    switch (opcode) {
      case Opcode::jmp:
      case Opcode::jmpt:
      case Opcode::jmpf:
      case Opcode::makefunc: {
        buf.seekp(patch_offset);
        buf.write_u32(encode_iaax(opcode, relative_offset));
        break;
      }
      case Opcode::argcjmp: {
        auto read_arg = buf.peek_char(1);
        CHECK(read_arg != EOF);
        buf.seekp(patch_offset);
        buf.write_u32(encode_iabb(opcode, read_arg, relative_offset));
        break;
      }
      default: {
        FAIL("unexpected opcode % (%)", kOpcodeNames[opcode], (void*)(uintptr_t)opcode);
      }
    }

    buf.seekp(p);
  }

  buf.seekp(p);
  buf.seekg(g);
}

uint32_t Assembler::offset_of_label(Label label) {
  DCHECK(m_placed_labels.count(label));
  return m_placed_labels.at(label);
}

}  // namespace charly::core::compiler::ir
