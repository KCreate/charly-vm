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
  static ref<ir::IRModule> compile(ref<CompilationUnit> unit);

private:
  void compile();

  ref<ir::IRModule> get_module() const {
    return m_builder.get_module();
  };

  CodeGenerator(ref<CompilationUnit> unit) :
    DiagnosticPass(unit->console), m_unit(unit), m_builder(unit->filepath) {}

  struct QueuedFunction {
    ir::Label head;
    ref<ast::Function> ast;
  };

  const ref<ast::Function>& active_function() const {
    return m_function_queue.front().ast;
  }

  // enqueue a function to be compiled
  // returns the label the function will be placed at
  ir::Label enqueue_function(const ref<ast::Function>& ast);

  // store a string in the current function's string table
  ir::Label register_string(const std::string& string);

  // add a new exception table entry
  // overlapping segments are untangled by the codegenerator internally
  void add_exception_table_entry(ir::Label begin, ir::Label end, ir::Label handler);

  // compile a single function and enqueue child functions
  void compile_function(const QueuedFunction&);

  // generate load and stores to value locations
  ref<ir::IRStatement> generate_load(const ir::ValueLocation&);
  ref<ir::IRStatement> generate_store(const ir::ValueLocation&);

  // generate spread tuples for a list of expressions
  // returns the total amount of tuples pushed
  uint8_t generate_spread_tuples(const std::vector<ref<ast::Expression>>& vec);

  // generate an assignment to an unpack target
  void generate_unpack_assignment(const ref<ast::UnpackTarget>& target);

  // generate the string table for the current function
  void generate_string_table();

  // generate the exception table for the current function
  void generate_exception_table();

  // detects instructions that cannot get executed and removes them
  void delete_dead_instructions();

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
  virtual bool inspect_enter(const ref<ast::Block>&) override;
  virtual bool inspect_enter(const ref<ast::Return>&) override;
  virtual void inspect_leave(const ref<ast::Break>&) override;
  virtual void inspect_leave(const ref<ast::Continue>&) override;
  virtual void inspect_leave(const ref<ast::Throw>&) override;
  virtual void inspect_leave(const ref<ast::Export>&) override;
  virtual bool inspect_enter(const ref<ast::Import>&) override;

  virtual void inspect_leave(const ref<ast::Yield>&) override;
  virtual bool inspect_enter(const ref<ast::Spawn>&) override;
  virtual void inspect_leave(const ref<ast::Await>&) override;
  virtual void inspect_leave(const ref<ast::Typeof>&) override;

  virtual void inspect_leave(const ref<ast::Id>&) override;
  virtual void inspect_leave(const ref<ast::Int>&) override;
  virtual void inspect_leave(const ref<ast::Float>&) override;
  virtual void inspect_leave(const ref<ast::Bool>&) override;
  virtual void inspect_leave(const ref<ast::Char>&) override;
  virtual void inspect_leave(const ref<ast::String>&) override;
  virtual void inspect_leave(const ref<ast::FormatString>&) override;
  virtual void inspect_leave(const ref<ast::Symbol>&) override;
  virtual void inspect_leave(const ref<ast::Null>&) override;
  virtual void inspect_leave(const ref<ast::Self>&) override;
  virtual void inspect_leave(const ref<ast::Super>&) override;
  virtual bool inspect_enter(const ref<ast::Tuple>&) override;
  virtual bool inspect_enter(const ref<ast::List>&) override;
  virtual bool inspect_enter(const ref<ast::Dict>&) override;
  virtual bool inspect_enter(const ref<ast::Function>&) override;
  virtual bool inspect_enter(const ref<ast::Class>&) override;

  virtual bool inspect_enter(const ref<ast::MemberOp>&) override;
  virtual void inspect_leave(const ref<ast::IndexOp>&) override;
  virtual bool inspect_enter(const ref<ast::Assignment>&) override;
  virtual bool inspect_enter(const ref<ast::UnpackAssignment>&) override;
  virtual bool inspect_enter(const ref<ast::MemberAssignment>&) override;
  virtual bool inspect_enter(const ref<ast::IndexAssignment>&) override;
  virtual bool inspect_enter(const ref<ast::Ternary>&) override;
  virtual bool inspect_enter(const ref<ast::BinaryOp>&) override;
  virtual void inspect_leave(const ref<ast::UnaryOp>&) override;
  virtual bool inspect_enter(const ref<ast::CallOp>&) override;
  virtual bool inspect_enter(const ref<ast::CallMemberOp>&) override;
  virtual bool inspect_enter(const ref<ast::CallIndexOp>&) override;

  virtual bool inspect_enter(const ref<ast::Declaration>&) override;
  virtual bool inspect_enter(const ref<ast::UnpackDeclaration>&) override;

  virtual bool inspect_enter(const ref<ast::If>&) override;
  virtual bool inspect_enter(const ref<ast::While>&) override;
  virtual bool inspect_enter(const ref<ast::Loop>&) override;
  virtual bool inspect_enter(const ref<ast::Try>&) override;
  virtual bool inspect_enter(const ref<ast::TryFinally>&) override;
  virtual bool inspect_enter(const ref<ast::Switch>&) override;

  virtual bool inspect_enter(const ref<ast::BuiltinOperation>&) override;

  ref<CompilationUnit> m_unit;
  ir::Builder m_builder;

  std::queue<QueuedFunction> m_function_queue;

  // strings defined in the current function
  // flushed at the end of the function and cleared for the next function
  std::vector<std::tuple<ir::Label, const std::string&>> m_string_table;

  // exception table containing the region -> handler mapping
  std::vector<std::tuple<ir::Label, ir::Label, ir::Label>> m_exception_table;

  std::stack<ir::Label> m_return_stack;
  std::stack<ir::Label> m_break_stack;
  std::stack<ir::Label> m_continue_stack;
};

}  // namespace charly::core::compiler
