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
#include "charly/core/compiler/ir/builder.h"
#include "charly/core/compiler/pass.h"

#pragma once

namespace charly::core::compiler {

class CodeGenerator : public ast::DiagnosticPass {
public:
  CodeGenerator(std::shared_ptr<CompilationUnit> unit) :
    DiagnosticPass(unit->console), m_unit(unit), m_builder(unit->ir_builder) {}

  void compile();

private:
  struct QueuedFunction {
    ir::Label head_label;
    ast::ref<ast::Function> ast;
  };

  void compile_function(const QueuedFunction&);

  virtual bool inspect_enter(const ast::ref<ast::Function>&) override;
  virtual void inspect_leave(const ast::ref<ast::Int>&) override;
  virtual void inspect_leave(const ast::ref<ast::BinaryOp>&) override;
  virtual void inspect_leave(const ast::ref<ast::UnaryOp>&) override;

  std::shared_ptr<CompilationUnit> m_unit;
  ir::Builder& m_builder;
  std::queue<QueuedFunction> m_function_queue;
};

}  // namespace charly::core::compiler
