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

#include <cstdint>
#include <list>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/location.h"
#include "charly/core/compiler/ir/bytecode.h"
#include "charly/value.h"

#pragma once

namespace charly::core::compiler::ir {

struct IROperand {
  virtual ~IROperand() = default;
  virtual OperandType get_type() const = 0;
  virtual void dump(std::ostream& out) const = 0;
};

#define OPTYPE(name, type)                                            \
  struct IROperand##name : public IROperand {                         \
    virtual OperandType get_type() const override {                   \
      return OperandType::name;                                       \
    }                                                                 \
    IROperand##name(const type& value) : value(value) {}              \
    static ref<IROperand##name> make(const type& value) { \
      return charly::make<IROperand##name>(value);                \
    }                                                                 \
    type value;                                                       \
    virtual void dump(std::ostream& out) const override;                       \
  };
FOREACH_OPERANDTYPE(OPTYPE)
#undef OPTYPE

struct IRStatement {
  enum class Type : uint8_t { Instruction, LabelDefinition, StringData };

  Location location;
  void at(const Location& location);
  void at(const ref<ast::Node>& node);

  virtual ~IRStatement() = default;

  virtual Type get_type() const = 0;
  virtual void dump(std::ostream& out) const = 0;
};

struct IRInstruction : public IRStatement {
  template <typename... Args>
  IRInstruction(Opcode opcode, Args&&... operands) : opcode(opcode), operands({ std::forward<Args>(operands)... }) {}

  Opcode opcode;
  std::vector<ref<IROperand>> operands;

  virtual Type get_type() const override {
    return IRStatement::Type::Instruction;
  }

  uint32_t popped_values() const;
  uint32_t pushed_values() const;

  virtual void dump(std::ostream& out) const override;
};

struct IRLabelDefinition : public IRStatement {
  IRLabelDefinition(Label label) : label(label) {}

  Label label;

  virtual Type get_type() const override {
    return IRStatement::Type::LabelDefinition;
  }

  virtual void dump(std::ostream& out) const override;
};

struct IRStringData : public IRStatement {
  IRStringData(const std::string& data) : data(data) {}

  std::string data;

  virtual Type get_type() const override {
    return IRStatement::Type::StringData;
  }

  virtual void dump(std::ostream& out) const override;
};

struct IRFunction {
  IRFunction(Label head, ref<ast::Function> ast) : head(head), ast(ast) {}

  Label head;
  ref<ast::Function> ast;

  std::vector<std::tuple<Label, Label, Label>> exception_table;

  std::vector<ref<IRStatement>> statements;

  void dump(std::ostream& out) const;
};

struct IRModule {
  IRModule(const std::string& filename) : filename(filename) {}

  std::string filename;
  std::unordered_map<SYMBOL, std::string> symbol_table;
  std::vector<IRFunction> functions;

  void dump(std::ostream& out) const;
};

}  // namespace charly::core::compiler::ir
