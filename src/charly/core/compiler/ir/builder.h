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
#include "charly/core/compiler/ir/ir.h"
#include "charly/value.h"

#pragma once

namespace charly::core::compiler::ir {

class Builder {
public:
  Builder() : m_label_counter(0), m_active_function(nullptr), m_module(std::make_shared<IRModule>()) {}

  // register a symbol in the module symbol table
  void register_symbol(SYMBOL symbol, const std::string& string);

  // function management
  IRFunction& active_function() const;
  IRFunction& begin_function(Label head, ast::ref<ast::Function> ast);
  void end_function();

  // register a string in the current function's string table
  Label register_string(const std::string& string);

  // label management
  Label reserve_label();
  Label label();
  void place_label(Label label);
  void place_label_at_label(Label label, Label target_label);

  // emit an IR statement into the current function
  template <typename... Args>
  void emit(Opcode opcode, Args&&... operands) {
    IRFunction& func = active_function();
    std::shared_ptr<IRInstruction> instruction =
      std::make_shared<IRInstruction>(opcode, std::forward<Args>(operands)...);
    func.statements.push_back(instruction);
  }
  void emit_string_data(const std::string& string);
  void emit_label_definition(Label label);
#define MAP(name, ...) void emit_##name(__VA_ARGS__);
  FOREACH_OPCODE(MAP)
#undef MAP

  std::shared_ptr<IRModule> get_module() const {
    return m_module;
  };

private:
  Label m_label_counter;
  IRFunction* m_active_function;
  std::shared_ptr<IRModule> m_module;
};

}  // namespace charly::core::compiler::ir
