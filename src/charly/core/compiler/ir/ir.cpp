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

#include <iomanip>
#include <memory>

#include "charly/core/compiler/ir/ir.h"
#include "charly/utils/colorwriter.h"

namespace charly::core::compiler::ir {

using Color = utils::Color;

void IRInstruction::at(const Location& loc) {
  this->location = loc;
}

void IRInstruction::at(const ref<ast::Node>& node) {
  this->location = node->location();
}

void IRInstruction::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);

  // emit opcode mnemonic
  writer.fg(Color::White, kOpcodeNames[opcode]);

  dump_arguments(out);

  // instruction source location
  if (this->location.valid) {
    writer.fg(Color::Grey, " ; at ", this->location);
  }
}

ref<IRInstruction> IRInstruction::make(Opcode opcode,
                                       [[maybe_unused]] uint32_t arg1,
                                       [[maybe_unused]] uint32_t arg2,
                                       [[maybe_unused]] uint32_t arg3) {
#define DEF_IXXX(N) ::charly::make<IRInstruction_##N>();
#define DEF_IAXX(N) ::charly::make<IRInstruction_##N>(arg1);
#define DEF_IABX(N) ::charly::make<IRInstruction_##N>(arg1, arg2);
#define DEF_IABC(N) ::charly::make<IRInstruction_##N>(arg1, arg2, arg3);
#define DEF_IABB(N) ::charly::make<IRInstruction_##N>(arg1, arg2);
#define DEF_IAAX(N) ::charly::make<IRInstruction_##N>(arg1);
#define DEF_IAAA(N) ::charly::make<IRInstruction_##N>(arg1);

  switch (opcode) {
#define DEF(N, T, ...) \
  case Opcode::N: {    \
    return DEF_##T(N); \
  }
    FOREACH_OPCODE(DEF)
#undef DEF
    default: {
      FAIL("unexpected opcode");
    }
  }

#undef DEF_IXXX
#undef DEF_IAXX
#undef DEF_IABX
#undef DEF_IABC
#undef DEF_IABB
#undef DEF_IAAX
#undef DEF_IAAA
}

void IRInstructionIXXX::dump_arguments(std::ostream&) const {}

void IRInstructionIAXX::dump_arguments(std::ostream& out) const {
  utils::ColorWriter writer(out);
  out << " ";
  writer.fg(utils::Color::Red, (uint32_t)arg);
}

void IRInstructionIABX::dump_arguments(std::ostream& out) const {
  utils::ColorWriter writer(out);
  out << " ";
  writer.fg(utils::Color::Red, (uint32_t)arg1);
  out << ", ";
  writer.fg(utils::Color::Red, (uint32_t)arg2);
}

void IRInstructionIABC::dump_arguments(std::ostream& out) const {
  utils::ColorWriter writer(out);
  out << " ";
  writer.fg(utils::Color::Red, (uint32_t)arg1);
  out << ", ";
  writer.fg(utils::Color::Red, (uint32_t)arg2);
  out << ", ";
  writer.fg(utils::Color::Red, (uint32_t)arg3);
}

void IRInstructionIABB::dump_arguments(std::ostream& out) const {
  utils::ColorWriter writer(out);
  out << " ";

  if (opcode == Opcode::argcjmp) {
    writer.fg(utils::Color::Red, (uint32_t)arg1);
    out << ", ";
    writer.fg(utils::Color::Yellow, ".L", (uint32_t)arg2);
  } else {
    writer.fg(utils::Color::Red, (uint32_t)arg1);
    out << ", ";
    writer.fg(utils::Color::Red, (uint32_t)arg2);
  }
}

void IRInstructionIAAX::dump_arguments(std::ostream& out) const {
  utils::ColorWriter writer(out);
  out << " ";

  switch (opcode) {
    case Opcode::makefunc:
    case Opcode::jmp:
    case Opcode::jmpt:
    case Opcode::jmpf: {
      writer.fg(utils::Color::Yellow, ".L", (uint32_t)arg);
      break;
    }
    default: {
      writer.fg(utils::Color::Red, (uint32_t)arg);
      break;
    }
  }
}

void IRInstructionIAAA::dump_arguments(std::ostream& out) const {
  utils::ColorWriter writer(out);
  out << " ";

  if (opcode == Opcode::loadsmi) {
    writer.fg(utils::Color::Red, runtime::RawValue(arg));
  } else {
    writer.fg(utils::Color::Red, (uint32_t)arg);
  }
}

void IRBasicBlock::link(const ref<IRBasicBlock>& source, const ref<IRBasicBlock>& target) {
  source->outgoing_blocks.insert(target);
  target->incoming_blocks.insert(source);
}

void IRBasicBlock::unlink(const ref<IRBasicBlock>& block) {
  DCHECK(block->exception_handler_sources.empty());

  for (const ref<IRBasicBlock>& incoming : block->incoming_blocks) {
    incoming->outgoing_blocks.erase(block);
  }

  for (const ref<IRBasicBlock>& outgoing : block->outgoing_blocks) {
    outgoing->incoming_blocks.erase(block);
  }

  if (block->previous_block) {
    block->previous_block->next_block = block->next_block;
  }

  if (block->next_block) {
    block->next_block->previous_block = block->previous_block;
  }
}

void IRBasicBlock::unlink(const ref<IRBasicBlock>& source, const ref<IRBasicBlock>& target) {
  source->outgoing_blocks.erase(target);
  target->incoming_blocks.erase(source);
}

void IRBasicBlock::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Grey, "  |\n");
  writer.fg(Color::Grey, "  | #", this->id);

  // block labels
  if (!this->labels.empty()) {
    for (Label label : this->labels) {
      writer.fg(Color::Yellow, " .L", label);
    }
  }

  out << " ";

  // incoming / outgoing blocks
  writer.fg(Color::Blue, "[ ");
  for (const ref<IRBasicBlock>& block : this->incoming_blocks) {
    writer.fg(Color::Blue, "#", block->id);
    out << " ";
  }
  writer.fg(Color::Blue, "]");
  out << " ";
  writer.fg(Color::Green, "[ ");
  for (const ref<IRBasicBlock>& block : this->outgoing_blocks) {
    writer.fg(Color::Green, "#", block->id);
    out << " ";
  }
  writer.fg(Color::Green, "]");

  if (this->exception_handler.has_value()) {
    writer.fg(Color::Red, " exceptions -> ");
    writer.fg(Color::Yellow, ".L", this->exception_handler.value());
  }

  if (!this->exception_handler_sources.empty()) {
    out << " ";
    writer.fg(Color::Red, "[ ");
    for (const ref<IRBasicBlock>& block : this->exception_handler_sources) {
      writer.fg(Color::Red, "#", block->id);
      out << " ";
    }
    writer.fg(Color::Red, "]");
  }

  out << '\n';

  // instructions
  for (const auto& instruction : this->instructions) {
    writer.fg(Color::Grey, "  |   ");
    instruction->dump(out);
    out << '\n';
  }
}

std::optional<std::string> IRFunction::lookup_symbol(SYMBOL symbol) const {
  for (const IRStringTableEntry& entry : this->string_table) {
    if (entry.hash == symbol) {
      return entry.value;
    }
  }

  return {};
}

void IRFunction::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Yellow, "function .L", this->head, " ");
  if (this->ast->location().valid) {
    writer.fg(Color::Grey, " ; at ", this->ast->location());
    out << "\n";
  }
  writer.fg(Color::Grey, "  name = ");
  writer.fg(Color::Red, "'", this->ast->name->value, "'\n");
  writer.fg(Color::Grey, "  stacksize = ");
  writer.fg(Color::Green, (uint32_t)this->ast->ir_info.stacksize, "\n");
  writer.fg(Color::Grey, "  local variables = ");
  writer.fg(Color::Green, (uint32_t)this->ast->ir_info.local_variables, "\n");
  writer.fg(Color::Grey, "  heap variables = ");
  writer.fg(Color::Green, (uint32_t)this->ast->ir_info.heap_variables, "\n");
  writer.fg(Color::Grey, "  argc = ");
  writer.fg(Color::Green, (uint32_t)this->ast->ir_info.argc, "\n");
  writer.fg(Color::Grey, "  minargc = ");
  writer.fg(Color::Green, (uint32_t)this->ast->ir_info.minargc, "\n");
  writer.fg(Color::Grey, "  spread = ");
  writer.fg(Color::Green, this->ast->ir_info.spread_argument ? "true" : "false", "\n");
  writer.fg(Color::Grey, "  arrow = ");
  writer.fg(Color::Green, this->ast->ir_info.arrow_function ? "true" : "false", "\n");
  writer.fg(Color::Grey, "  constructor = ");
  writer.fg(Color::Green, this->ast->class_constructor ? "true" : "false", "\n");
  writer.fg(Color::Grey, "  static = ");
  writer.fg(Color::Green, this->ast->class_static_function ? "true" : "false", "\n");
  writer.fg(Color::Grey, "  member = ");
  writer.fg(Color::Green, this->ast->class_member_function ? "true" : "false", "\n");
  out << "\n";

  if (!exception_table.empty()) {
    writer.fg(Color::Yellow, "  exception table:", "\n");
    for (const auto& entry : this->exception_table) {
      writer.fg(Color::Grey, "  - (");
      writer.fg(Color::Yellow, ".L", entry.begin);
      out << " ";
      writer.fg(Color::Yellow, ".L", entry.end);
      out << " ";
      writer.fg(Color::Yellow, ".L", entry.handler);
      writer.fg(Color::Grey, ")");
      out << "\n";
    }
    out << "\n";
  }

  if (!string_table.empty()) {
    writer.fg(Color::Blue, "  string table:", "\n");
    int str_index = 0;
    for (const IRStringTableEntry& entry : this->string_table) {
      writer.fg(Color::Grey, "  - #", std::setw(2), std::left, str_index, std::setw(1));
      writer.fg(Color::Red, " \"", entry.value, "\"");
      writer.fg(Color::Grey, " length = ");
      writer.fg(Color::White, entry.value.size());
      writer.fg(Color::Grey, ", hash = ");
      writer.fg(Color::White, std::hex, entry.hash, std::dec);
      out << "\n";

      str_index++;
    }
    out << "\n";
  }

  if (!constant_table.empty()) {
    writer.fg(Color::Blue, "  constant table:", "\n");
    int constant_index = 0;
    for (const runtime::RawValue& value : this->constant_table) {
      writer.fg(Color::Grey, "  - #", std::setw(2), std::left, constant_index, std::setw(1), " ");

      if (value.isSymbol()) {
        auto symbol = runtime::RawSymbol::cast(value);
        if (auto symbol_string = lookup_symbol(symbol.value())) {
          writer.fg(Color::Red, ":'", symbol_string.value(), "'");
          out << " (" << std::hex << symbol.value() << std::dec << ")";
        } else {
          out << value;
        }
      } else {
        out << value;
      }

      out << "\n";

      constant_index++;
    }

    out << "\n";
  }

  writer.fg(Color::Yellow, "  body:", "\n");

  for (const ref<IRBasicBlock>& block : this->basic_blocks) {
    block->dump(out);
  }

  out << "\n";
}

void IRModule::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer.fg(Color::Grey, "; IR for file ");
  writer.fg(Color::Yellow, "'", this->filename, "'", "\n");
  out << '\n';

  writer.fg(Color::Grey, "; functions", '\n');
  for (const ref<IRFunction>& func : this->functions) {
    func->dump(out);
  }
  out << '\n';
}

}  // namespace charly::core::compiler::ir
