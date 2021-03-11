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

#include "charly/core/compiler/pass.h"
#include "charly/core/compiler/localvars.h"

#pragma once

namespace charly::core::compiler::ast {

class LocalAllocatorPass : public DiagnosticPass {
public:
  using DiagnosticPass::DiagnosticPass;

private:
  std::shared_ptr<FunctionScope> m_function;
  std::shared_ptr<BlockScope> m_block;
  // bool m_toplevel = false;

  void push_function(const ref<Function>&);
  void push_block(const ref<Block>&);

  void pop_function();
  void pop_block();

  // declare a new variable in the current block
  const LocalVariable* declare_variable(const ref<Name>&, const ref<Node>&, bool constant = false);

  virtual bool inspect_enter(const ref<Function>&) override;
  virtual void inspect_leave(const ref<Function>&) override;

  virtual bool inspect_enter(const ref<Block>&) override;
  virtual void inspect_leave(const ref<Block>&) override;

  virtual bool inspect_enter(const ref<Declaration>&) override;
  virtual ref<Statement> transform(const ref<Declaration>&) override;

  virtual void inspect_leave(const ref<UnpackDeclaration>&) override;

  virtual void inspect_leave(const ref<Assignment>&) override;
  virtual void inspect_leave(const ref<UnpackAssignment>&) override;

  virtual bool inspect_enter(const ref<Try>&) override;
  virtual ref<Statement> transform(const ref<Try>&) override;

  virtual void inspect_leave(const ref<Id>&) override;
};

}  // namespace charly::core::compiler::ast
