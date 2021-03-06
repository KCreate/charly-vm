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
#include "charly/core/compiler/ir/builder.h"

namespace charly::core::compiler::ir {

using namespace charly::core::compiler::ast;

using Color = utils::Color;

Label Builder::reserve_label() {
  Label l = m_label_counter++;
  m_pending_labels.insert(l);
  return l;
}

Label Builder::label() {
  Label l = reserve_label();
  place_label(l);
  return l;
}

void Builder::place_label(Label label) {
  m_statements.push_back(IRStatement(label));
  assert(m_pending_labels.count(label) && "label was already set or is unknown");
  m_pending_labels.erase(label);
}

void Builder::begin_function(Label head_label, const ref<Function>& function) {
  place_label(head_label);

  m_functions[head_label] = {
    .head_label = head_label,
    .body_label = label(),
    .end_label = reserve_label(),
    .function_ast = function,
  };
}

void Builder::end_function(Label start_label) {
  place_label(m_functions[start_label].end_label);
}

void Builder::emit(Opcode opcode) {
  assert(kOpcodeLength[(uint16_t)opcode] == 1);
  m_statements.push_back(IRStatement(Instruction(opcode)));
}

void Builder::emit_nop() {
  m_statements.push_back(IRStatement(Instruction(Opcode::nop)));
}

void Builder::emit_load(VALUE value) {
  m_statements.push_back(IRStatement(Instruction(Opcode::load, Operand::Immediate(value))));
}

void Builder::emit_jmp(Label label) {
  m_statements.push_back(IRStatement(Instruction(Opcode::jmp, Operand::Label(label))));
}

void Builder::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);

  bool last_statement_was_instruction = false;
  for (const IRStatement& statement : m_statements) {
    if (const Instruction* instruction = std::get_if<Instruction>(&statement)) {

      // emit opcode name
      out << "  ";
      writer.fg(Color::White, kOpcodeNames[(uint16_t)instruction->opcode]);

      // emit opcode operands
      for (const Operand& operand : instruction->operands) {
        out << " ";
        switch (operand.type) {
          case OperandType::Local: {
            writer.fg(Color::Magenta, (uint32_t)(operand.as.Local.value));
            break;
          }
          case OperandType::Symbol: {
            writer.fg(Color::Red, operand.as.Symbol.value);
            break;
          }
          case OperandType::Label: {
            writer.fg(Color::Yellow, ".L", operand.as.Label.value);
            break;
          }
          case OperandType::Immediate: {
            out << std::hex;
            writer.fg(Color::Cyan, "0x");
            writer.fg(Color::Cyan, operand.as.Immediate.value);
            out << std::dec;
            break;
          }
          default: {
            assert(false && "unexpected operand type");
            break;
          }
        }
      }

      last_statement_was_instruction = true;

      out << std::endl;
    } else if (const Label* stmt = std::get_if<Label>(&statement)) {
      Label label = *stmt;

      // group consecutive labels together
      if (last_statement_was_instruction) {
        out << std::endl;
      }

      // display function info if this is the begin label
      // of a function
      if (m_functions.count(label)) {
        const FunctionData& fdata = m_functions.at(label);

        if (fdata.function_ast->name->value.size()) {
          writer.fg(Color::Grey, "; function '", fdata.function_ast->name->value, "' at ");
          writer.fg(Color::Grey, fdata.function_ast->location(), '\n');
        } else {
          writer.fg(Color::Grey, "; arrow function at ");
          writer.fg(Color::Grey, fdata.function_ast->location(), '\n');
        }

        assert(fdata.function_ast->ir_info.valid);
        writer.fg(Color::Grey, ";   ", fdata.function_ast->ir_info, '\n');
        writer.fg(Color::Grey, ";", '\n');
        writer.fg(Color::Grey, ";   head: .L", fdata.head_label, '\n');
        writer.fg(Color::Grey, ";   body: .L", fdata.body_label, '\n');
        writer.fg(Color::Grey, ";   end: .L", fdata.end_label, '\n');
      }

      writer.fg(Color::Yellow, ".L", label);

      out << std::endl;

      last_statement_was_instruction = false;

    } else {
      assert(false && "unexpected type");
    }
  }
}

}  // namespace charly::core::compiler::ir
