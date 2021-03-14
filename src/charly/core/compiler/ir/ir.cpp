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

#include "charly/utils/colorwriter.h"
#include "charly/utils/buffer.h"
#include "charly/utils/stringindent.h"
#include "charly/core/compiler/ir/ir.h"

namespace charly::core::compiler::ir {

using Color = utils::Color;

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

  out << '\n';
}

void IRLabelDefinition::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Yellow, ".L", this->label, '\n');
}

void IRStringData::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Red, ".string '", this->data, "'");
  writer.fg(Color::Grey, " length = ", this->data.size(), '\n');
}

void IRFunction::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Yellow, ".L", this->head, '\n');
  writer.fg(Color::Yellow, "function {", "\n");
  writer.fg(Color::Grey, ".name = ");
  writer.fg(Color::Red, "'", this->ast->name->value, "'\n");
  writer.fg(Color::Grey, ".locals = ");
  writer.fg(Color::Green, (uint32_t)this->ast->ir_info.local_variables, "\n");
  writer.fg(Color::Grey, ".argc = ");
  writer.fg(Color::Green, (uint32_t)this->ast->ir_info.argc, "\n");
  writer.fg(Color::Grey, ".minargc = ");
  writer.fg(Color::Green, (uint32_t)this->ast->ir_info.minargc, "\n");
  writer.fg(Color::Grey, ".spread = ");
  writer.fg(Color::Green, this->ast->ir_info.arrow_function ? "true" : "false", "\n");
  writer.fg(Color::Grey, ".arrow = ");
  writer.fg(Color::Green, this->ast->ir_info.spread_argument ? "true" : "false", "\n");
  writer.fg(Color::Yellow, ".body", "\n");

  IRStatement::Type last_type = IRStatement::Type::LabelDefinition;
  for (const auto& stmt : this->statements) {

    // put a space between label definitions and the last statement
    if (stmt->get_type() == IRStatement::Type::LabelDefinition) {
      if (last_type != IRStatement::Type::LabelDefinition) {
        writer.fg(Color::Grey, "  |\n");
      }
      writer.fg(Color::Grey, "  | ");
    } else {

      // indent instructions and stringdata
      writer.fg(Color::Grey, "  |   ");
    }

    stmt->dump(out);

    last_type = stmt->get_type();
  }

  writer.fg(Color::Yellow, "}\n");
}

void IRModule::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Blue, "module {", '\n');

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
    std::stringstream stream;
    termcolor::colorlike(stream, out);

    func.dump(stream);

    std::stringstream indent_stream;
    utils::indent_stream(2, stream, out);
  }

  writer.fg(Color::Blue, "}", '\n');
}

}  // namespace charly::core::compiler::ir
