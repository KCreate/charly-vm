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

#include <memory>
#include <iostream>
#include <cassert>

#include "charly/core/compiler/ast.h"

#pragma once

namespace charly::core::compiler::ast {

class ASTPass {
#define AST_NODE(R, T)                    \
public:                                   \
  ref<R> visit(ref<T> node) {             \
    ref<R> replacement = on_enter(node);  \
    on_enter_any(replacement);            \
    if (replacement.get() != node.get()) \
      return replacement; \
    node->visit_children(this);           \
    replacement = on_leave(node);         \
    on_leave_any(replacement);            \
    return replacement;                   \
  }                                       \
                                          \
private:                                  \
  virtual ref<R> on_leave(ref<T>& node) { \
    return node;                          \
  }                                       \
  virtual ref<R> on_enter(ref<T>& node) { \
    return node;                          \
  }

public:
  ref<Node> visit(ref<Node> node) {
    if (ref<Block> n = cast<Block>(node)) {
      return this->visit(n);
    }
    if (ref<Program> n = cast<Program>(node)) {
      return this->visit(n);
    }
    if (ref<Id> n = cast<Id>(node)) {
      return this->visit(n);
    }
    if (ref<Int> n = cast<Int>(node)) {
      return this->visit(n);
    }
    if (ref<Float> n = cast<Float>(node)) {
      return this->visit(n);
    }
    if (ref<Bool> n = cast<Bool>(node)) {
      return this->visit(n);
    }
    if (ref<String> n = cast<String>(node)) {
      return this->visit(n);
    }
    if (ref<FormatString> n = cast<FormatString>(node)) {
      return this->visit(n);
    }
    if (ref<Null> n = cast<Null>(node)) {
      return this->visit(n);
    }
    if (ref<Self> n = cast<Self>(node)) {
      return this->visit(n);
    }
    if (ref<Super> n = cast<Super>(node)) {
      return this->visit(n);
    }
    if (ref<Tuple> n = cast<Tuple>(node)) {
      return this->visit(n);
    }

    assert(false && "Unsupported node type");
  }

private:
  virtual void on_enter_any(const ref<Node>&) {}
  virtual void on_leave_any(const ref<Node>&) {}

  AST_NODE(Statement,  Block)
  AST_NODE(Node,       Program)
  AST_NODE(Expression, Id)
  AST_NODE(Expression, Int)
  AST_NODE(Expression, Float)
  AST_NODE(Expression, Bool)
  AST_NODE(Expression, String)
  AST_NODE(Expression, FormatString)
  AST_NODE(Expression, Null)
  AST_NODE(Expression, Self)
  AST_NODE(Expression, Super)
  AST_NODE(Expression, Tuple)

#undef AST_NODE
};

}  // namespace charly::core::compiler::ast
