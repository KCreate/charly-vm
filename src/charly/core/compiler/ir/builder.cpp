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

IRFunction& Builder::active_function() const {
  assert(m_active_function && "no active function");
  return *m_active_function;
}

IRFunction& Builder::begin_function(Label head, ast::ref<ast::Function> ast) {
  m_module->functions.emplace_back(head, ast);
  m_active_function = &m_module->functions.back();
  return active_function();
}

void Builder::end_function() {

  // emit string table
  std::unordered_map<SYMBOL, Label> emitted_strings;
  for (const IRFunction::StringTableEntry& entry : active_function().string_table) {
    SYMBOL string_hash = SYM(entry.string);

    if (emitted_strings.count(string_hash)) {
      place_label_at_label(entry.label, emitted_strings.at(string_hash));
      continue;
    }

    place_label(entry.label);
    emit_string_data(entry.string);
    emitted_strings[string_hash] = entry.label;
  }

  // reset active function
  m_active_function = nullptr;
}

Label Builder::register_string(const std::string& string) {
  IRFunction& func = active_function();
  Label string_label = reserve_label();
  func.string_table.emplace_back(string_label, string);
  return string_label;
}

Label Builder::reserve_label() {
  return m_label_counter++;
}

Label Builder::label() {
  Label l = reserve_label();
  place_label(l);
  return l;
}

void Builder::place_label(Label label) {
  emit_label_definition(label);
}

void Builder::place_label_at_label(Label label, Label target_label) {

  // search for the target label
  IRFunction& func = active_function();
  for (auto it = func.statements.begin(); it != func.statements.end(); it++) {
    const std::shared_ptr<IRStatement>& ptr = *it;

    if (ptr->get_type() == IRStatement::Type::LabelDefinition) {
      const IRLabelDefinition* label_def = dynamic_cast<IRLabelDefinition*>(ptr.get());
      if (label_def->label == target_label) {
        func.statements.insert(it, std::make_shared<IRLabelDefinition>(label));
        return;
      }
    }
  }

  assert(false && "unknown label");
}

void Builder::emit_string_data(const std::string& string) {
  IRFunction& func = active_function();
  func.statements.push_back(std::make_shared<IRStringData>(string));
}

void Builder::emit_label_definition(Label label) {
  IRFunction& func = active_function();
  func.statements.push_back(std::make_shared<IRLabelDefinition>(label));
}

// machine control
void Builder::emit_nop() {
  emit(Opcode::nop);
}

// misc. instructions

// stack management
void Builder::emit_pop() {
  emit(Opcode::pop);
}

// control flow
void Builder::emit_jmp(OpOffset label) {
  emit(Opcode::jmp, IROperand::Offset(label));
}

void Builder::emit_jmpf(OpOffset label) {
  emit(Opcode::jmpf, IROperand::Offset(label));
}

void Builder::emit_jmpt(OpOffset label) {
  emit(Opcode::jmpt, IROperand::Offset(label));
}

void Builder::emit_throwex() {
  emit(Opcode::throwex);
}

// function control flow
void Builder::emit_ret() {
  emit(Opcode::ret);
}

// load operations
void Builder::emit_load(OpImmediate value) {
  emit(Opcode::load, IROperand::Immediate(value));
}

void Builder::emit_loadglobal(OpSymbol symbol) {
  emit(Opcode::loadglobal, IROperand::Symbol(symbol));
}

void Builder::emit_loadlocal(OpCount8 offset) {
  emit(Opcode::loadlocal, IROperand::Count8(offset));
}

void Builder::emit_loadfar(OpCount8 depth, OpCount8 offset) {
  emit(Opcode::loadfar, IROperand::Count8(depth), IROperand::Count8(offset));
}

// write operations
void Builder::emit_setglobal(OpSymbol symbol) {
  emit(Opcode::setglobal, IROperand::Symbol(symbol));
}

void Builder::emit_setlocal(OpCount8 offset) {
  emit(Opcode::setlocal, IROperand::Count8(offset));
}

void Builder::emit_setfar(OpCount8 depth, OpCount8 offset) {
  emit(Opcode::setfar, IROperand::Count8(depth), IROperand::Count8(offset));
}

// value destructuring operations

// value allocation
void Builder::emit_makefunc(OpOffset offset) {
  emit(Opcode::makefunc, IROperand::Offset(offset));
}
void Builder::emit_makestr(OpOffset offset) {
  emit(Opcode::makestr, IROperand::Offset(offset));
}

// fiber management

// cast operations









// void Builder::dump(std::ostream& out) const {
//   utils::ColorWriter writer(out);
//
//   for (const BasicBlock& block : m_blocks) {
//     if (!first_block) {
//       out << '\n';
//     } else {
//       first_block = false;
//     }
//
//     // print all block labels
//     for (Label label : block.labels) {
//       writer.fg(Color::Yellow, ".L", label);
//       out << '\n';
//     }
//
//     // print block statements
//     for (const IRStatement& statement : block.statements) {
//       if (const Instruction* instruction = std::get_if<Instruction>(&statement)) {
//
//         // emit opcode name
//         out << "  ";
//         writer.fg(Color::White, kOpcodeNames[(uint16_t)instruction->opcode]);
//
//         // emit opcode operands
//         for (const Operand& operand : instruction->operands) {
//           out << " ";
//           switch (operand.type) {
//             case OperandType::Count8: {
//               out << std::hex;
//               writer.fg(Color::Yellow, (uint32_t)operand.as.Count8.value);
//               out << std::dec;
//               break;
//             }
//             case OperandType::Count16: {
//               out << std::hex;
//               writer.fg(Color::Yellow, (uint32_t)operand.as.Count16.value);
//               out << std::dec;
//               break;
//             }
//             case OperandType::Symbol: {
//               out << std::hex;
//               writer.fg(Color::Red, "0x");
//               writer.fg(Color::Red, operand.as.Symbol.value);
//               out << std::dec;
//               break;
//             }
//             case OperandType::Offset: {
//               writer.fg(Color::Yellow, ".L", operand.as.Offset.value);
//               break;
//             }
//             case OperandType::Immediate: {
//               out << std::hex;
//               writer.fg(Color::Cyan, "0x");
//               writer.fg(Color::Cyan, operand.as.Immediate.value);
//               out << std::dec;
//               break;
//             }
//             default: {
//               assert(false && "unexpected operand type");
//               break;
//             }
//           }
//         }
//
//         out << std::endl;
//       } else if (const FunctionData* function_data = std::get_if<FunctionData>(&statement)) {
//         const FunctionData& fdata = *function_data;
//         writer.fg(Color::Yellow, ".function {", '\n');
//
//         writer.fg(Color::Grey, "  name = ");
//         writer.fg(Color::Red, "'", fdata.ast->name->value, "'", '\n');
//
//         writer.fg(Color::Grey, "  ir_info = ");
//         writer.fg(Color::Magenta, fdata.ast->ir_info, '\n');
//
//         writer.fg(Color::Grey, "  head = ");
//         writer.fg(Color::Yellow, ".L", fdata.head, '\n');
//
//         writer.fg(Color::Grey, "  end = ");
//         writer.fg(Color::Yellow, ".L", fdata.end, '\n');
//
//         writer.fg(Color::Grey, "  string_table = ");
//         writer.fg(Color::Yellow, ".L", fdata.string_table, '\n');
//
//         writer.fg(Color::Grey, "  body = ");
//         writer.fg(Color::Yellow, ".L", fdata.body, '\n');
//
//         writer.fg(Color::Yellow, "}", '\n');
//       } else if (const StringData* stmt = std::get_if<StringData>(&statement)) {
//         const StringData& string_data = *stmt;
//
//         writer.fg(Color::Red, ".string '", string_data.data, "'");
//         writer.fg(Color::Grey, " length=", string_data.data.size());
//         writer.fg(Color::Grey, " hash=0x", std::hex, string_data.hash, std::dec, '\n');
//       } else {
//         assert(false && "unexpected type");
//       }
//     }
//   }
// }

}  // namespace charly::core::compiler::ir
