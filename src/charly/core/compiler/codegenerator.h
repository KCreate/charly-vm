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

#include <queue>

#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/compiler.h"
#include "charly/core/compiler/ir/builder.h"
#include "charly/core/compiler/ir/ir.h"
#include "charly/core/compiler/ir/valuelocation.h"
#include "charly/core/compiler/pass.h"

#pragma once

namespace charly::core::compiler {

class CodeGenerator : public ast::DiagnosticPass {
public:
  explicit CodeGenerator(const ref<CompilationUnit>& unit) :
    DiagnosticPass(unit->console), m_unit(unit), m_builder(unit->filepath) {}

  ref<ir::IRModule> compile();

private:
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

  // compile a single function and enqueue child functions
  void compile_function(const QueuedFunction&);

  // generate load and stores to value locations
  ref<ir::IRInstruction> generate_load(const ir::ValueLocation&);
  ref<ir::IRInstruction> generate_store(const ir::ValueLocation&);

  // generate spread tuples for a list of expressions
  // returns the total amount of tuples pushed
  uint8_t generate_spread_tuples(const std::vector<ref<ast::Expression>>& vec);

  // generate an assignment to an unpack target
  void generate_unpack_assignment(const ref<ast::UnpackTarget>& target);

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
  bool inspect_enter(const ref<ast::ExpressionWithSideEffects>&) override;
  bool inspect_enter(const ref<ast::Block>&) override;
  bool inspect_enter(const ref<ast::Return>&) override;
  void inspect_leave(const ref<ast::Break>&) override;
  void inspect_leave(const ref<ast::Continue>&) override;
  void inspect_leave(const ref<ast::Throw>&) override;
  void inspect_leave(const ref<ast::Export>&) override;
  bool inspect_enter(const ref<ast::Import>&) override;

  void inspect_leave(const ref<ast::Yield>&) override;
  bool inspect_enter(const ref<ast::Spawn>&) override;
  void inspect_leave(const ref<ast::Await>&) override;
  void inspect_leave(const ref<ast::Typeof>&) override;

  void inspect_leave(const ref<ast::Id>&) override;
  void inspect_leave(const ref<ast::Int>&) override;
  void inspect_leave(const ref<ast::Float>&) override;
  void inspect_leave(const ref<ast::Bool>&) override;
  void inspect_leave(const ref<ast::Char>&) override;
  void inspect_leave(const ref<ast::String>&) override;
  void inspect_leave(const ref<ast::FormatString>&) override;
  void inspect_leave(const ref<ast::Symbol>&) override;
  void inspect_leave(const ref<ast::Null>&) override;
  void inspect_leave(const ref<ast::Self>&) override;
  void inspect_leave(const ref<ast::FarSelf>&) override;
  void inspect_leave(const ref<ast::Super>&) override;
  bool inspect_enter(const ref<ast::Tuple>&) override;
  bool inspect_enter(const ref<ast::List>&) override;
  bool inspect_enter(const ref<ast::Dict>&) override;
  bool inspect_enter(const ref<ast::Function>&) override;
  bool inspect_enter(const ref<ast::Class>&) override;

  bool inspect_enter(const ref<ast::MemberOp>&) override;
  bool inspect_enter(const ref<ast::IndexOp>&) override;
  bool inspect_enter(const ref<ast::Assignment>&) override;
  bool inspect_enter(const ref<ast::Ternary>&) override;
  bool inspect_enter(const ref<ast::BinaryOp>&) override;
  void inspect_leave(const ref<ast::UnaryOp>&) override;
  bool inspect_enter(const ref<ast::CallOp>&) override;

  bool inspect_enter(const ref<ast::Declaration>&) override;
  bool inspect_enter(const ref<ast::UnpackDeclaration>&) override;

  bool inspect_enter(const ref<ast::If>&) override;
  bool inspect_enter(const ref<ast::While>&) override;
  bool inspect_enter(const ref<ast::Loop>&) override;
  bool inspect_enter(const ref<ast::Try>&) override;
  bool inspect_enter(const ref<ast::TryFinally>&) override;
  bool inspect_enter(const ref<ast::Switch>&) override;

  bool inspect_enter(const ref<ast::BuiltinOperation>&) override;

  ref<CompilationUnit> m_unit;
  ir::Builder m_builder;

  std::queue<QueuedFunction> m_function_queue;
  std::stack<ir::Label> m_return_stack;
  std::stack<ir::Label> m_break_stack;
  std::stack<ir::Label> m_continue_stack;
};

}  // namespace charly::core::compiler
