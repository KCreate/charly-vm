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

#include <cstdint>
#include <list>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "charly/utils/colorwriter.h"

#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/ir/bytecode.h"
#include "charly/core/compiler/location.h"

#pragma once

namespace charly::core::compiler::ir {

using Label = uint16_t;

#define DEF_FORWARD(T, _) struct IRInstruction##T;
FOREACH_OPCODE_TYPE(DEF_FORWARD)
#undef DEF_FORWARD
#define DEF_AS(N, ...) struct IRInstruction_##N;
FOREACH_OPCODE(DEF_AS)
#undef DEF_AS

struct IRInstruction : std::enable_shared_from_this<IRInstruction> {
  std::optional<Label> assembled_at_label;
  Opcode opcode;
  Location location;

  static ref<IRInstruction> make(Opcode opcode,
                                 [[maybe_unused]] uint32_t arg1 = 0,
                                 [[maybe_unused]] uint32_t arg2 = 0,
                                 [[maybe_unused]] uint32_t arg3 = 0);

  explicit IRInstruction(Opcode opcode) : opcode(opcode) {}
  virtual ~IRInstruction() = default;

  void at(const Location& location);
  void at(const ref<ast::Node>& node);

  virtual Instruction encode() const = 0;
  virtual uint32_t pushed_values() const = 0;
  virtual uint32_t popped_values() const = 0;

  void dump(std::ostream& out) const;
  virtual void dump_arguments(std::ostream& out) const = 0;

#define DEF_FORWARD(T, TS)            \
  IRInstruction##T* as_##TS() const { \
    return (IRInstruction##T*)this;   \
  }
  FOREACH_OPCODE_TYPE(DEF_FORWARD)
#undef DEF_FORWARD

#define DEF_CAST(N, T, ...)                                          \
  __attribute__((always_inline)) IRInstruction_##N* as_##N() const { \
    DCHECK(opcode == Opcode::N);                                     \
    return (IRInstruction_##N*)this;                                 \
  }
  FOREACH_OPCODE(DEF_CAST)
#undef DEF_CAST
};

struct IRInstructionIXXX : public IRInstruction {
  explicit IRInstructionIXXX(Opcode opcode) : IRInstruction(opcode) {}
  Instruction encode() const override {
    return encode_ixxx(opcode);
  }
  void dump_arguments(std::ostream&) const override {}
};

struct IRInstructionIAXX : public IRInstruction {
  uint8_t arg;
  explicit IRInstructionIAXX(Opcode opcode, uint8_t arg) : IRInstruction(opcode), arg(arg) {}
  Instruction encode() const override {
    return encode_iaxx(opcode, arg);
  }
  void dump_arguments(std::ostream& out) const override {
    utils::ColorWriter writer(out);
    writer.fg(utils::Color::Red, (uint32_t)arg);
  }
};

struct IRInstructionIABX : public IRInstruction {
  uint8_t arg1;
  uint8_t arg2;
  explicit IRInstructionIABX(Opcode opcode, uint8_t arg1, uint8_t arg2) :
    IRInstruction(opcode), arg1(arg1), arg2(arg2) {}
  Instruction encode() const override {
    return encode_iabx(opcode, arg1, arg2);
  }
  void dump_arguments(std::ostream& out) const override {
    utils::ColorWriter writer(out);
    writer.fg(utils::Color::Red, (uint32_t)arg1);
    out << ", ";
    writer.fg(utils::Color::Red, (uint32_t)arg2);
  }
};

struct IRInstructionIABC : public IRInstruction {
  uint8_t arg1;
  uint8_t arg2;
  uint8_t arg3;
  explicit IRInstructionIABC(Opcode opcode, uint8_t arg1, uint8_t arg2, uint8_t arg3) :
    IRInstruction(opcode), arg1(arg1), arg2(arg2), arg3(arg3) {}
  Instruction encode() const override {
    return encode_iabc(opcode, arg1, arg2, arg3);
  }
  void dump_arguments(std::ostream& out) const override {
    utils::ColorWriter writer(out);
    writer.fg(utils::Color::Red, (uint32_t)arg1);
    out << ", ";
    writer.fg(utils::Color::Red, (uint32_t)arg2);
    out << ", ";
    writer.fg(utils::Color::Red, (uint32_t)arg3);
  }
};

struct IRInstructionIABB : public IRInstruction {
  uint8_t arg1;
  uint16_t arg2;
  explicit IRInstructionIABB(Opcode opcode, uint8_t arg1, uint16_t arg2) :
    IRInstruction(opcode), arg1(arg1), arg2(arg2) {}
  Instruction encode() const override {
    return encode_iabb(opcode, arg1, arg2);
  }
  void dump_arguments(std::ostream& out) const override {
    utils::ColorWriter writer(out);
    writer.fg(utils::Color::Red, (uint32_t)arg1);
    out << ", ";
    writer.fg(utils::Color::Red, (uint32_t)arg2);
  }
};

struct IRInstructionIAAX : public IRInstruction {
  uint16_t arg;
  explicit IRInstructionIAAX(Opcode opcode, uint16_t arg) : IRInstruction(opcode), arg(arg) {}
  Instruction encode() const override {
    return encode_iaax(opcode, arg);
  }
  void dump_arguments(std::ostream& out) const override {
    utils::ColorWriter writer(out);
    writer.fg(utils::Color::Red, (uint32_t)arg);
  }
};

struct IRInstructionIAAA : public IRInstruction {
  uint32_t arg;
  explicit IRInstructionIAAA(Opcode opcode, uint64_t _arg) : IRInstruction(opcode), arg(_arg) {
    DCHECK((_arg & 0xffffffffff000000) == 0);
  }
  Instruction encode() const override {
    return encode_iaaa(opcode, arg);
  }
  void dump_arguments(std::ostream& out) const override {
    utils::ColorWriter writer(out);
    writer.fg(utils::Color::Red, (uint32_t)arg);
  }
};

#define DEF_IXXX(O, PO, PU)                                        \
  struct IRInstruction_##O : public IRInstructionIXXX {            \
    explicit IRInstruction_##O() : IRInstructionIXXX(Opcode::O) {} \
    uint32_t pushed_values() const override {                      \
      return PU;                                                   \
    }                                                              \
    uint32_t popped_values() const override {                      \
      return PO;                                                   \
    }                                                              \
  }

#define DEF_IAXX(O, PO, PU)                                                        \
  struct IRInstruction_##O : public IRInstructionIAXX {                            \
    explicit IRInstruction_##O(uint8_t arg) : IRInstructionIAXX(Opcode::O, arg) {} \
    uint32_t pushed_values() const override {                                      \
      return PU;                                                                   \
    }                                                                              \
    uint32_t popped_values() const override {                                      \
      return PO;                                                                   \
    }                                                                              \
  }

#define DEF_IABX(O, PO, PU)                                                                              \
  struct IRInstruction_##O : public IRInstructionIABX {                                                  \
    explicit IRInstruction_##O(uint8_t arg1, uint8_t arg2) : IRInstructionIABX(Opcode::O, arg1, arg2) {} \
    uint32_t pushed_values() const override {                                                            \
      return PU;                                                                                         \
    }                                                                                                    \
    uint32_t popped_values() const override {                                                            \
      return PO;                                                                                         \
    }                                                                                                    \
  }

#define DEF_IABC(O, PO, PU)                                                \
  struct IRInstruction_##O : public IRInstructionIABC {                    \
    explicit IRInstruction_##O(uint8_t arg1, uint8_t arg2, uint8_t arg3) : \
      IRInstructionIABC(Opcode::O, arg1, arg2, arg3) {}                    \
    uint32_t pushed_values() const override {                              \
      return PU;                                                           \
    }                                                                      \
    uint32_t popped_values() const override {                              \
      return PO;                                                           \
    }                                                                      \
  }

#define DEF_IABB(O, PO, PU)                                                                               \
  struct IRInstruction_##O : public IRInstructionIABB {                                                   \
    explicit IRInstruction_##O(uint8_t arg1, uint16_t arg2) : IRInstructionIABB(Opcode::O, arg1, arg2) {} \
    uint32_t pushed_values() const override {                                                             \
      return PU;                                                                                          \
    }                                                                                                     \
    uint32_t popped_values() const override {                                                             \
      return PO;                                                                                          \
    }                                                                                                     \
  }

#define DEF_IAAX(O, PO, PU)                                                         \
  struct IRInstruction_##O : public IRInstructionIAAX {                             \
    explicit IRInstruction_##O(uint16_t arg) : IRInstructionIAAX(Opcode::O, arg) {} \
    uint32_t pushed_values() const override {                                       \
      return PU;                                                                    \
    }                                                                               \
    uint32_t popped_values() const override {                                       \
      return PO;                                                                    \
    }                                                                               \
  }

#define DEF_IAAA(O, PO, PU)                                                         \
  struct IRInstruction_##O : public IRInstructionIAAA {                             \
    explicit IRInstruction_##O(uint32_t arg) : IRInstructionIAAA(Opcode::O, arg) {} \
    uint32_t pushed_values() const override {                                       \
      return PU;                                                                    \
    }                                                                               \
    uint32_t popped_values() const override {                                       \
      return PO;                                                                    \
    }                                                                               \
  }

#define DEF_OPCODES(N, T, X, Y) DEF_##T(N, X, Y);
FOREACH_OPCODE(DEF_OPCODES)
#undef DEF_OPCODES
#undef DEF_IXXX
#undef DEF_IAXX
#undef DEF_IABX
#undef DEF_IABC
#undef DEF_IABB
#undef DEF_IAAX
#undef DEF_IAAA

struct IRBasicBlock {
  explicit IRBasicBlock(uint32_t id) : id(id) {}
  uint32_t id;

  // set during dead-code analysis phases
  bool reachable = false;

  // optional exception handler for this block
  std::optional<Label> exception_handler = std::nullopt;

  // labels that refer to this block
  std::unordered_set<Label> labels;
  std::list<ref<IRInstruction>> instructions;

  // control-flow-graph data
  ref<IRBasicBlock> next_block;
  ref<IRBasicBlock> previous_block;
  std::unordered_set<ref<IRBasicBlock>> outgoing_blocks;
  std::unordered_set<ref<IRBasicBlock>> incoming_blocks;

  static void link(const ref<IRBasicBlock>& source, const ref<IRBasicBlock>& target);
  static void unlink(const ref<IRBasicBlock>& block);
  static void unlink(const ref<IRBasicBlock>& source, const ref<IRBasicBlock>& target);

  void dump(std::ostream& out) const;
};

struct IRExceptionTableEntry {
  Label begin;
  Label end;
  Label handler;

  IRExceptionTableEntry(Label begin, Label end, Label handler) : begin(begin), end(end), handler(handler) {}
};

struct IRStringTableEntry {
  SYMBOL hash;
  std::string value;

  explicit IRStringTableEntry(const std::string& string) : hash(crc32::hash_string(string)), value(string) {}
};

struct IRFunction {
  explicit IRFunction(Label head, const ref<ast::Function>& ast) : head(head), ast(ast) {}

  Label head;
  ref<ast::Function> ast;

  std::vector<IRStringTableEntry> string_table;
  std::vector<IRExceptionTableEntry> exception_table;
  std::list<ref<IRBasicBlock>> basic_blocks;

  std::vector<runtime::RawValue> constant_table;

  void dump(std::ostream& out) const;
};

struct IRModule {
  explicit IRModule(const std::string& filename) : next_label(0), filename(filename) {}

  Label next_label;

  std::string filename;
  std::vector<ref<IRFunction>> functions;

  void dump(std::ostream& out) const;
};

}  // namespace charly::core::compiler::ir
