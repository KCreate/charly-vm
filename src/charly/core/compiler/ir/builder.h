/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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

#include <cstdint>
#include <list>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/ir/bytecode.h"
#include "charly/value.h"

#pragma once

namespace charly::core::compiler::ir {

using Label = uint32_t;

struct Operand {
  OperandType type;
#define OPTYPE(name, optype)                                                           \
  static Operand name(optype value) {                                                  \
    return Operand{ .type = OperandType::name, .as = { .name = { .value = value } } }; \
  }
  FOREACH_OPERANDTYPE(OPTYPE)
#undef OPTYPE

  union {
#define OPTYPE(name, type) \
  struct {                 \
    type value;            \
  } name;
    FOREACH_OPERANDTYPE(OPTYPE)
#undef OPTYPE
  } as;
};

struct Instruction {
  Opcode opcode;
  std::vector<Operand> operands;

  template <typename... Args>
  Instruction(Opcode opcode, Args&&... operands) : opcode(opcode), operands({ std::forward<Args>(operands)... }) {}
};

using IRStatement = std::variant<Instruction, Label>;

class Builder {
public:
  Builder() : m_label_counter(0) {}

  // labels
  Label reserve_label();
  Label label();
  void place_label(Label label);

  // register functions
  void begin_function(Label head_label, const ast::ref<ast::Function>&);
  void end_function(Label head_label);

  // instructions
  void emit(Opcode opcode);
  void emit_nop();
  void emit_load(VALUE value);
  void emit_jmp(Label label);

  void dump(std::ostream& out) const;

  bool has_pending_labels() const {
    return m_pending_labels.size();
  }

private:
  struct FunctionData {
    Label head_label;
    Label body_label;
    Label end_label;
    ast::ref<ast::Function> function_ast;
  };

  Label m_label_counter;
  std::unordered_set<Label> m_pending_labels;
  std::list<IRStatement> m_statements;
  std::unordered_map<Label, FunctionData> m_functions;
};

}  // namespace charly::core::compiler::ir
