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

#include <sstream>
#include <iomanip>
#include <memory>

#include "charly/core/compiler/ir/ir.h"
#include "charly/utils/argumentparser.h"
#include "charly/utils/buffer.h"
#include "charly/utils/colorwriter.h"

namespace charly::core::compiler::ir {

using Color = utils::Color;

void IRStatement::at(const Location& location) {
  this->location = location;
}

void IRStatement::at(const ref<ast::Node>& node) {
  this->location = node->location();
}

void IROperandCount8::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  out << std::hex;
  writer.fg(Color::Yellow, (uint32_t)this->value);
  out << std::dec;
}

void IROperandCount16::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  out << std::hex;
  writer.fg(Color::Yellow, (uint32_t)this->value);
  out << std::dec;
}

void IROperandSymbol::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  out << std::hex;
  writer.fg(Color::Red, "'", this->value, "'");
  out << std::dec;
}

void IROperandOffset::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Yellow, ".L", this->value);
}

void IROperandImmediate::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  // writer.fg(Color::Cyan, std::hex, this->value.raw, std::dec);
  // return;

  if (this->value.is_pointer()) {
    writer.fg(Color::Cyan, this->value.to_pointer());
    return;
  }

  if (this->value.is_int()) {
    writer.fg(Color::Cyan, this->value.to_int());
    return;
  }

  if (this->value.is_float()) {
    writer.fg(Color::Red, this->value.to_float());
    return;
  }

  if (this->value.is_char()) {
    utils::Buffer buf(4);
    buf.append_utf8(this->value.to_char());

    writer.fg(Color::Red, "'", buf.buffer_string(), "'");

    writer.fg(Color::Grey, " [");
    for (size_t i = 0; i < buf.size(); i++) {
      uint32_t character = *(buf.data() + i) & 0xFF;

      if (i) {
        out << " ";
      }

      writer.fg(Color::Red, "0x", std::hex, character, std::dec);
    }
    writer.fg(Color::Grey, "]");

    return;
  }

  if (this->value.is_symbol()) {
    writer.fg(Color::Red, std::hex, this->value.to_symbol(), std::dec);
    return;
  }

  if (this->value.is_bool()) {
    if (this->value.to_bool()) {
      writer.fg(Color::Green, "true");
    } else {
      writer.fg(Color::Red, "false");
    }
    return;
  }

  if (this->value.is_null()) {
    writer.fg(Color::Grey, "null");
    return;
  }

  writer.fg(Color::Grey, "???");
}

uint32_t IRInstruction::popped_values() const {
  uint32_t arg0 = 0;
  uint32_t arg1 = 0;
  uint32_t arg2 = 0;
  uint32_t arg3 = 0;

  if (this->operands.size() >= 1 && this->operands[0]->get_type() == OperandType::Count16) {
    arg0 = cast<IROperandCount16>(this->operands[0])->value;
  }
  if (this->operands.size() >= 1 && this->operands[0]->get_type() == OperandType::Count8) {
    arg0 = cast<IROperandCount8>(this->operands[0])->value;
  }
  if (this->operands.size() >= 2 && this->operands[1]->get_type() == OperandType::Count16) {
    arg1 = cast<IROperandCount16>(this->operands[1])->value;
  }
  if (this->operands.size() >= 2 && this->operands[1]->get_type() == OperandType::Count8) {
    arg1 = cast<IROperandCount8>(this->operands[1])->value;
  }
  if (this->operands.size() >= 3 && this->operands[2]->get_type() == OperandType::Count16) {
    arg2 = cast<IROperandCount16>(this->operands[2])->value;
  }
  if (this->operands.size() >= 3 && this->operands[2]->get_type() == OperandType::Count8) {
    arg2 = cast<IROperandCount8>(this->operands[2])->value;
  }
  if (this->operands.size() >= 4 && this->operands[3]->get_type() == OperandType::Count16) {
    arg3 = cast<IROperandCount16>(this->operands[3])->value;
  }
  if (this->operands.size() >= 4 && this->operands[3]->get_type() == OperandType::Count8) {
    arg3 = cast<IROperandCount8>(this->operands[3])->value;
  }

  switch (this->opcode) {
#define PAIR(name, STACKPOP, STACKPUSH, ...) case Opcode::name: { return STACKPOP; }
  FOREACH_OPCODE(PAIR)
#undef PAIR
  }
}

uint32_t IRInstruction::pushed_values() const {
  uint32_t arg0 = 0;
  uint32_t arg1 = 0;
  uint32_t arg2 = 0;
  uint32_t arg3 = 0;

  if (this->operands.size() >= 1 && this->operands[0]->get_type() == OperandType::Count16) {
    arg0 = cast<IROperandCount16>(this->operands[0])->value;
  }
  if (this->operands.size() >= 1 && this->operands[0]->get_type() == OperandType::Count8) {
    arg0 = cast<IROperandCount8>(this->operands[0])->value;
  }
  if (this->operands.size() >= 2 && this->operands[1]->get_type() == OperandType::Count16) {
    arg1 = cast<IROperandCount16>(this->operands[1])->value;
  }
  if (this->operands.size() >= 2 && this->operands[1]->get_type() == OperandType::Count8) {
    arg1 = cast<IROperandCount8>(this->operands[1])->value;
  }
  if (this->operands.size() >= 3 && this->operands[2]->get_type() == OperandType::Count16) {
    arg2 = cast<IROperandCount16>(this->operands[2])->value;
  }
  if (this->operands.size() >= 3 && this->operands[2]->get_type() == OperandType::Count8) {
    arg2 = cast<IROperandCount8>(this->operands[2])->value;
  }
  if (this->operands.size() >= 4 && this->operands[3]->get_type() == OperandType::Count16) {
    arg3 = cast<IROperandCount16>(this->operands[3])->value;
  }
  if (this->operands.size() >= 4 && this->operands[3]->get_type() == OperandType::Count8) {
    arg3 = cast<IROperandCount8>(this->operands[3])->value;
  }

  switch (this->opcode) {
#define PAIR(name, STACKPOP, STACKPUSH, ...) case Opcode::name: { return STACKPUSH; }
  FOREACH_OPCODE(PAIR)
#undef PAIR
  }
}

void IRInstruction::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);

  // emit opcode mnemonic
  writer.fg(Color::White, kOpcodeNames[(uint16_t)this->opcode]);

  // emit instruction operands
  bool first_operand = true;
  for (const auto& operand : this->operands) {
    if (first_operand) {
      out << " ";
    } else {
      out << ", ";
    }
    first_operand = false;
    operand->dump(out);
  }

  // instruction source location
  if (this->location.valid) {
    writer.fg(Color::Grey, " ; at ", this->location);
  }
}

void IRLabelDefinition::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Yellow, ".L", this->label);
}

void IRStringData::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Red, ".string '", this->data, "'");
  writer.fg(Color::Grey, " length = ", this->data.size());
}

void IRFunction::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Yellow, "  .L", this->head, '\n');
  writer.fg(Color::Yellow, "  function {", "\n");
  writer.fg(Color::Grey, "    name = ");
  writer.fg(Color::Red, "'", this->ast->name->value, "'\n");
  writer.fg(Color::Grey, "    stacksize = ");
  writer.fg(Color::Green, (uint32_t)this->ast->ir_info.stacksize, "\n");
  writer.fg(Color::Grey, "    locals = ");
  writer.fg(Color::Green, (uint32_t)this->ast->ir_info.local_variables, "\n");
  writer.fg(Color::Grey, "    argc = ");
  writer.fg(Color::Green, (uint32_t)this->ast->ir_info.argc, "\n");
  writer.fg(Color::Grey, "    minargc = ");
  writer.fg(Color::Green, (uint32_t)this->ast->ir_info.minargc, "\n");
  writer.fg(Color::Grey, "    spread = ");
  writer.fg(Color::Green, this->ast->ir_info.spread_argument ? "true" : "false", "\n");
  writer.fg(Color::Grey, "    arrow = ");
  writer.fg(Color::Green, this->ast->ir_info.arrow_function ? "true" : "false", "\n");
  writer.fg(Color::Grey, "    constructor = ");
  writer.fg(Color::Green, this->ast->class_constructor ? "true" : "false", "\n");
  writer.fg(Color::Grey, "    static = ");
  writer.fg(Color::Green, this->ast->class_static_function ? "true" : "false", "\n");
  out << "\n";

  writer.fg(Color::Yellow, "  exception_table", "\n");
  for (const auto& entry : this->exception_table) {
    writer.fg(Color::Grey, "    (");
    writer.fg(Color::Yellow, ".L", std::get<0>(entry), " ");
    writer.fg(Color::Yellow, ".L", std::get<1>(entry), " ");
    writer.fg(Color::Yellow, ".L", std::get<2>(entry));
    writer.fg(Color::Grey, ")");
    out << "\n";
  }
  out << "\n";

  writer.fg(Color::Yellow, "  body", "\n");

  uint32_t current_stack_size = 0;

  IRStatement::Type last_type = IRStatement::Type::LabelDefinition;
  for (const auto& stmt : this->statements) {

    // put a space between label definitions and the last statement
    if (stmt->get_type() == IRStatement::Type::LabelDefinition) {
      if (last_type != IRStatement::Type::LabelDefinition) {
        writer.fg(Color::Grey, "    |\n");
      }
      writer.fg(Color::Grey, "    | ");
    } else {

      // indent instructions and stringdata
      writer.fg(Color::Grey, "    |   ");
    }

    // emit stack information for instructions
    if (utils::ArgumentParser::is_flag_set("dump_asm_stack")) {
      if (stmt->get_type() == IRStatement::Type::Instruction) {
        ref<IRInstruction> inst = cast<IRInstruction>(stmt);
        current_stack_size += inst->pushed_values() - inst->popped_values();

        writer.fg(Color::Grey, "[");
        for (uint32_t i = 0; i < current_stack_size; i++) {
          writer.fg(Color::Yellow, "#");
        }
        for (uint32_t i = 0; i < this->ast->ir_info.stacksize - current_stack_size; i++) {
          out << " ";
        }
        writer.fg(Color::Grey, "] ");
      }
    }

    stmt->dump(out);
    out << '\n';

    last_type = stmt->get_type();
  }

  writer.fg(Color::Yellow, "  }\n");
}

void IRModule::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Blue, "module ");
  writer.fg(Color::Yellow, "'", this->filename, "'");
  writer.fg(Color::Blue, " {", '\n');

  writer.fg(Color::Grey, "; symbol table", '\n');
  for (const auto& entry : this->symbol_table) {
    writer.fg(Color::Grey, "[");
    writer.fg(Color::Cyan, "0x", std::hex, std::setw(8), std::setfill('0'), entry.first, std::setw(0),
              std::setfill(' '), std::dec);
    out << " ";
    writer.fg(Color::Red, "'", entry.second, "'");
    writer.fg(Color::Grey, "]\n");
  }
  out << '\n';

  writer.fg(Color::Grey, "; functions", '\n');
  for (const IRFunction& func : this->functions) {
    func.dump(out);
  }

  writer.fg(Color::Blue, "}", '\n');
}

}  // namespace charly::core::compiler::ir
