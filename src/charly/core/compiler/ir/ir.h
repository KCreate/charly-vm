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
#include <unordered_set>
#include <stack>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <optional>

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

#define OPTYPE(name, type, _)                                \
  struct IROperand##name : public IROperand {             \
    virtual OperandType get_type() const override {       \
      return OperandType::name;                           \
    }                                                     \
    IROperand##name(const type& value) : value(value) {}  \
    static ref<IROperand##name> make(const type& value) { \
      return charly::make<IROperand##name>(value);        \
    }                                                     \
    type value;                                           \
    virtual void dump(std::ostream& out) const override;  \
  };
FOREACH_OPERANDTYPE(OPTYPE)
#undef OPTYPE

struct IRInstruction {
  template <typename... Args>
  IRInstruction(Opcode opcode, Args&&... operands) : opcode(opcode), operands({ std::forward<Args>(operands)... }) {}

  std::optional<Label> assembled_at_label;

  Opcode opcode;
  std::vector<ref<IROperand>> operands;

  std::optional<uint16_t> inline_cache_index;

  Location location;
  void at(const Location& location);
  void at(const ref<ast::Node>& node);

  uint32_t popped_values() const;
  uint32_t pushed_values() const;

  void dump(std::ostream& out) const;
};

struct IRBasicBlock {
  IRBasicBlock(uint32_t id) : id(id) {}
  uint32_t id;

  // set during dead-code analysis phases
  bool reachable = false;

  // optional exception handler for this block
  std::optional<Label> exception_handler = std::nullopt;

  // labels that refer to this block
  std::set<Label> labels;
  std::list<ref<IRInstruction>> instructions;

  // control-flow-graph data
  ref<IRBasicBlock> next_block;
  ref<IRBasicBlock> previous_block;
  std::unordered_set<ref<IRBasicBlock>> outgoing_blocks;
  std::unordered_set<ref<IRBasicBlock>> incoming_blocks;

  static void link(ref<IRBasicBlock> source, ref<IRBasicBlock> target);
  static void unlink(ref<IRBasicBlock> block);
  static void unlink(ref<IRBasicBlock> source, ref<IRBasicBlock> target);

  void dump(std::ostream& out) const;
};

struct IRExceptionTableEntry {
  Label begin;
  Label end;
  Label handler;

  IRExceptionTableEntry(Label begin, Label end, Label handler) : begin(begin), end(end), handler(handler) {}
};

struct IRInlineCacheTableEntry {
  ICType type;

  IRInlineCacheTableEntry(ICType type) : type(type) {}
};

struct IRStringTableEntry {
  SYMBOL hash;
  std::string value;

  IRStringTableEntry(const std::string& string) : hash(SYM(string)), value(string) {}
};

struct IRFunction {
  IRFunction(Label head, ref<ast::Function> ast) : head(head), ast(ast) {}

  Label head;
  ref<ast::Function> ast;

  std::vector<IRStringTableEntry> string_table;
  std::vector<IRExceptionTableEntry> exception_table;
  std::vector<IRInlineCacheTableEntry> inline_cache_table;
  std::list<ref<IRBasicBlock>> basic_blocks;

  void dump(std::ostream& out) const;
};

struct IRModule {
  IRModule(const std::string& filename) : filename(filename) {}

  Label next_label;

  std::string filename;
  std::vector<std::string> symbol_table;
  std::vector<ref<IRFunction>> functions;

  void dump(std::ostream& out) const;
};

}  // namespace charly::core::compiler::ir
