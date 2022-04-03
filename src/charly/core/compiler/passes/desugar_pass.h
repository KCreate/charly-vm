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

#include "charly/core/compiler/pass.h"

#pragma once

namespace charly::core::compiler::ast {

class DesugarPass : public DiagnosticPass {
public:
  using DiagnosticPass::DiagnosticPass;

private:
  ref<Statement> transform(const ref<Block>&) override;
  void inspect_leave(const ref<Import>&) override;
  bool inspect_enter(const ref<Spawn>&) override;
  ref<Expression> transform(const ref<MemberOp>&) override;
  ref<Expression> transform(const ref<FormatString>&) override;
  void inspect_leave(const ref<Function>&) override;
  void inspect_leave(const ref<Class>&) override;
  ref<Statement> transform(const ref<For>&) override;
};

}  // namespace charly::core::compiler::ast
