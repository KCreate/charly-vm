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

#include "charly/utils/colorwriter.h"
#include "charly/utils/stringindent.h"
#include "charly/core/compiler/ir/ir.h"

namespace charly::core::compiler::ir {

using Color = utils::Color;

void IROperand::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);

  switch (this->type) {
    case OperandType::Count8: {
      out << std::hex;
      writer.fg(Color::Yellow, (uint32_t)this->as.Count8.value);
      out << std::dec;
      break;
    }
    case OperandType::Count16: {
      out << std::hex;
      writer.fg(Color::Yellow, (uint32_t)this->as.Count16.value);
      out << std::dec;
      break;
    }
    case OperandType::Symbol: {
      out << std::hex;
      writer.fg(Color::Red, "0x", this->as.Symbol.value);
      out << std::dec;
      break;
    }
    case OperandType::Offset: {
      writer.fg(Color::Yellow, ".L", this->as.Offset.value);
      break;
    }
    case OperandType::Immediate: {
      out << std::hex;
      writer.fg(Color::Cyan, "0x", this->as.Immediate.value);
      out << std::dec;
      break;
    }
    default: {
      assert(false && "unexpected operand type");
      break;
    }
  }
}

void IRInstruction::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);

  // emit opcode mnemonic
  writer.fg(Color::White, kOpcodeNames[(uint16_t)this->opcode]);

  // emit instruction operands
  for (const IROperand& operand : this->operands) {
    out << " ";
    operand.dump(out);
  }

  // print special info for some instructions

  out << '\n';
}

void IRLabelDefinition::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Yellow, ".L", this->label, '\n');
}

void IRStringData::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Red, ".string '", this->data, "'");
  writer.fg(Color::Grey, " length=", this->data.size(), '\n');
}

void IRFunction::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Yellow, ".L", this->head, "\n");
  writer.fg(Color::Yellow, "function {", "\n");
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
  out << "\n";
  writer.fg(Color::Yellow, ".ENTRY", "\n");

  IRStatement::Type last_type = IRStatement::Type::LabelDefinition;
  for (const auto& stmt : this->statements) {

    // put a space between label definitions and the last statement
    if (stmt->get_type() == IRStatement::Type::LabelDefinition) {
      if (last_type != IRStatement::Type::LabelDefinition) {
        out << "\n";
      }
    } else {

      // indent instructions and stringdata
      out << "  ";
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
  for (const SymbolTableEntry& entry : this->symbol_table) {
    writer.fg(Color::Red, ".symbol 0x", entry.symbol, " ", entry.string, "'", '\n');
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
