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

#include <queue>

#include "charly/core/compiler.h"
#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/ir/ir.h"
#include "charly/core/compiler/ir/builder.h"
#include "charly/core/compiler/ir/valuelocation.h"
#include "charly/core/compiler/pass.h"

#pragma once

namespace charly::core::compiler {

class CodeGenerator : public ast::DiagnosticPass {
public:
  static std::shared_ptr<ir::IRModule> compile(std::shared_ptr<CompilationUnit> unit);

private:
  void compile();

  std::shared_ptr<ir::IRModule> get_module() const {
    return m_builder.get_module();
  };

  CodeGenerator(std::shared_ptr<CompilationUnit> unit) :
    DiagnosticPass(unit->console), m_unit(unit) {}

  struct QueuedFunction {
    ir::Label head;
    ast::ref<ast::Function> ast;
  };

  // enqueue a function to be compiled
  // returns the label the function will be placed at
  ir::Label enqueue_function(const ast::ref<ast::Function>& ast);

  // store a string in the current function's string table
  ir::Label register_string(const std::string& string);

  // compile a single function and enqueue child functions
  void compile_function(const QueuedFunction&);

  // generate load and stores to value locations
  void generate_load(const ir::ValueLocation&);
  void generate_store(const ir::ValueLocation&);

  // label stacks
  ir::Label active_return_label() const;
  ir::Label active_break_label() const;
  ir::Label active_continue_label() const;
  void push_return_label(ir::Label label);
  void push_break_label(ir::Label label);
  void push_continue_label(ir::Label label);
  void pop_return_label();
  void pop_break_label();
  void pop_continue_label();

  // pass visitor event hooks
  virtual bool inspect_enter(const ast::ref<ast::Block>&) override;
  virtual void inspect_leave(const ast::ref<ast::Return>&) override;
  virtual void inspect_leave(const ast::ref<ast::Break>&) override;
  virtual void inspect_leave(const ast::ref<ast::Continue>&) override;
  virtual void inspect_leave(const ast::ref<ast::Throw>&) override;

  virtual void inspect_leave(const ast::ref<ast::Id>&) override;
  virtual void inspect_leave(const ast::ref<ast::Int>&) override;
  virtual void inspect_leave(const ast::ref<ast::Float>&) override;
  virtual void inspect_leave(const ast::ref<ast::Bool>&) override;
  virtual void inspect_leave(const ast::ref<ast::Char>&) override;
  virtual void inspect_leave(const ast::ref<ast::String>&) override;
  virtual void inspect_leave(const ast::ref<ast::FormatString>&) override;
  virtual void inspect_leave(const ast::ref<ast::Null>&) override;
  virtual bool inspect_enter(const ast::ref<ast::Function>&) override;

  virtual void inspect_leave(const ast::ref<ast::MemberOp>&) override;
  virtual void inspect_leave(const ast::ref<ast::IndexOp>&) override;
  virtual bool inspect_enter(const ast::ref<ast::Assignment>&) override;
  virtual bool inspect_enter(const ast::ref<ast::Ternary>&) override;
  virtual void inspect_leave(const ast::ref<ast::BinaryOp>&) override;
  virtual void inspect_leave(const ast::ref<ast::UnaryOp>&) override;

  virtual void inspect_leave(const ast::ref<ast::Declaration>&) override;

  virtual bool inspect_enter(const ast::ref<ast::If>&) override;
  virtual bool inspect_enter(const ast::ref<ast::While>&) override;

  virtual bool inspect_enter(const ast::ref<ast::BuiltinOperation>&) override;

  std::shared_ptr<CompilationUnit> m_unit;
  ir::Builder m_builder;

  std::queue<QueuedFunction> m_function_queue;

  std::vector<std::tuple<ir::Label, const std::string&>> m_string_table;

  std::stack<ir::Label> m_return_stack;
  std::stack<ir::Label> m_break_stack;
  std::stack<ir::Label> m_continue_stack;
};

}  // namespace charly::core::compiler
