/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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

#include "ast.h"
#include "compiler-pass.h"
#include "lvar-alloc.h"

#pragma once

namespace Charly::Compilation {
class LVarRewriter : public CompilerPass {
  using CompilerPass::CompilerPass;

public:
  AST::AbstractNode* visit_function(AST::Function* node, VisitContinue cont);
  AST::AbstractNode* visit_block(AST::Block* node, VisitContinue cont);
  AST::AbstractNode* visit_localinitialisation(AST::LocalInitialisation* node, VisitContinue cont);
  AST::AbstractNode* visit_match(AST::Match* node, VisitContinue cont);
  AST::AbstractNode* visit_identifier(AST::Identifier* node, VisitContinue cont);
  AST::AbstractNode* visit_assignment(AST::Assignment* node, VisitContinue cont);
  AST::AbstractNode* visit_trycatch(AST::TryCatch* node, VisitContinue cont);

  void push_local_scope() {
    LocalScope* current_scope = this->scope;
    FunctionScope* parent_func_scope = current_scope ? current_scope->contained_function : nullptr;
    this->scope = new LocalScope(parent_func_scope, current_scope);
  }

  void push_func_scope(AST::Function* node) {
    FunctionScope* func_scope = new FunctionScope(node, this->scope->contained_function);
    LocalScope* local_scope = new LocalScope(func_scope, this->scope);
    this->scope = local_scope;
  }

  void pop_scope() {
    LocalScope* scope = this->scope;
    this->scope = scope->parent_scope;

    if (scope->contained_function != this->scope->contained_function) {
      delete scope->contained_function;
    }

    delete scope;
  }

  LocalOffsetInfo resolve_symbol(const std::string& id) {

  }

  LocalScope* scope = nullptr;
  bool allow_const_assignment = false;
};
}  // namespace Charly::Compilation
