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

#include <cassert>
#include <iostream>
#include <memory>

#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/diagnostic.h"

#pragma once

namespace charly::core::compiler::ast {

#define AST_NODE(R, T)                       \
public:                                      \
  ref<R> visit(const ref<T>& node) {         \
    before_enter_any(node);                  \
    bool visit_children = enter(node);       \
    after_enter_any(node);                   \
    if (visit_children) {                    \
      node->visit_children(this);            \
    }                                        \
    before_leave_any(node);                  \
    ref<R> replacement = leave(node);        \
    after_leave_any(replacement);            \
    if (replacement.get() != node.get())     \
      m_modified_nodes++;                    \
    return replacement;                      \
  }                                          \
                                             \
private:                                     \
  virtual bool enter(const ref<T>& node) {   \
    return enter_any(node);                  \
  }                                          \
  virtual ref<R> leave(const ref<T>& node) { \
    return cast<R>(leave_any(node));         \
  }

// base class of all ast passes.
//
// subclasses may override the virtual begin and leave methods
class ASTPass {
  friend class Node;  // for overload switch handling

  uint32_t m_modified_nodes = 0;

  virtual void before_enter_any(const ref<Node>&) {}
  virtual void after_enter_any(const ref<Node>&) {}
  virtual void before_leave_any(const ref<Node>&) {}
  virtual void after_leave_any(const ref<Node>&) {}

  virtual bool enter_any(const ref<Node>&) {
    return true;
  }
  virtual ref<Node> leave_any(const ref<Node>& node) {
    return node;
  }

  AST_NODE(Node, Program)

  AST_NODE(Statement, Nop)
  AST_NODE(Statement, Block)
  AST_NODE(Statement, Return)
  AST_NODE(Statement, Break)
  AST_NODE(Statement, Continue)
  AST_NODE(Statement, Defer)
  AST_NODE(Statement, Throw)
  AST_NODE(Statement, Export)
  AST_NODE(Statement, Import)

  AST_NODE(Expression, ImportExpression)
  AST_NODE(Expression, Yield)
  AST_NODE(Expression, Spawn)
  AST_NODE(Expression, Await)
  AST_NODE(Expression, Typeof)
  AST_NODE(Expression, As)

  AST_NODE(Expression, Id)
  AST_NODE(Expression, Int)
  AST_NODE(Expression, Float)
  AST_NODE(Expression, Bool)
  AST_NODE(Expression, Char)
  AST_NODE(Expression, String)
  AST_NODE(Expression, FormatString)
  AST_NODE(Expression, Null)
  AST_NODE(Expression, Self)
  AST_NODE(Expression, Super)
  AST_NODE(Expression, Tuple)
  AST_NODE(Expression, List)
  AST_NODE(DictEntry, DictEntry)
  AST_NODE(Expression, Dict)
  AST_NODE(Expression, Function)
  AST_NODE(Expression, Class)
  AST_NODE(ClassProperty, ClassProperty)

  AST_NODE(Expression, Assignment)
  AST_NODE(Expression, Ternary)
  AST_NODE(Expression, BinaryOp)
  AST_NODE(Expression, UnaryOp)
  AST_NODE(Expression, Spread)
  AST_NODE(Expression, CallOp)
  AST_NODE(Expression, MemberOp)
  AST_NODE(Expression, IndexOp)

  AST_NODE(Statement, Declaration)

  AST_NODE(Statement, If)
  AST_NODE(Statement, While)
  AST_NODE(Statement, Try)

public:
  ref<Node> visit(const ref<Node>& node) {
    if (node.get() == nullptr)
      return node;
    return node->visit(this, node);
  }

  ref<Statement> visit(const ref<Statement>& node) {
    if (node.get() == nullptr)
      return node;
    return node->visit(this, node);
  }

  ref<Expression> visit(const ref<Expression>& node) {
    if (node.get() == nullptr)
      return node;
    return node->visit(this, node);
  }

  uint32_t modified_count() {
    return m_modified_nodes;
  }
};

#undef AST_NODE

}  // namespace charly::core::compiler::ast
