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
#include "charly/core/compiler/passmacros.h"

#pragma once

namespace charly::core::compiler::ast {

class Pass {
public:
  template <typename N>
  ref<N> apply(const ref<N>& node) {
    AST_TYPESWITCH(node, { return cast<N>(apply(node)); });
  }

protected:
  uint32_t m_depth = 0;

  // override these methods to inspect every node
  // upon entering and leaving
  //
  // because the node may be transformed during the pass
  // the node that is being entered might not be the node that is being left
  virtual void enter(const ref<Node>&) {}
  virtual void leave(const ref<Node>&) {}

  // clang-format off
  HANDLE_NODE(Program, Program) {
    VISIT_NODE(body);
  }

  HANDLE_NODE(Statement, Block) {
    VISIT_NODE_VECTOR(statements);
  }

  HANDLE_NODE(Statement, Return) {
    VISIT_NODE(expression);
  }

  HANDLE_NODE(Statement, Break) {}
  HANDLE_NODE(Statement, Continue) {}

  HANDLE_NODE(Statement, Defer) {
    VISIT_NODE(statement);
  }

  HANDLE_NODE(Statement, Throw) {
    VISIT_NODE(expression);
  }

  HANDLE_NODE(Statement, Export) {
    VISIT_NODE(expression);
  }

  HANDLE_NODE(Statement, Import) {
    VISIT_NODE(source);
  }

  HANDLE_NODE(Expression, ImportExpression) {
    VISIT_NODE(source);
  }

  HANDLE_NODE(Expression, Yield) {
    VISIT_NODE(expression);
  }

  HANDLE_NODE(Expression, Spawn) {
    VISIT_NODE(statement);
  }

  HANDLE_NODE(Expression, Await) {
    VISIT_NODE(expression);
  }

  HANDLE_NODE(Expression, Typeof) {
    VISIT_NODE(expression);
  }

  HANDLE_NODE(Expression, As) {
    VISIT_NODE(expression);
  }

  HANDLE_NODE(Expression, Id) {}
  HANDLE_NODE(Expression, Int) {}
  HANDLE_NODE(Expression, Float) {}
  HANDLE_NODE(Expression, Bool) {}
  HANDLE_NODE(Expression, Char) {}
  HANDLE_NODE(Expression, String) {}

  HANDLE_NODE(Expression, FormatString) {
    VISIT_NODE_VECTOR(elements)
  }

  HANDLE_NODE(Expression, Null) {}
  HANDLE_NODE(Expression, Self) {}
  HANDLE_NODE(Expression, Super) {}

  HANDLE_NODE(Expression, Tuple) {
    VISIT_NODE_VECTOR(elements)
  }

  HANDLE_NODE(Expression, List) {
    VISIT_NODE_VECTOR(elements)
  }

  HANDLE_NODE(DictEntry, DictEntry) {
    VISIT_NODE(key);
    VISIT_NODE(value);
  }

  HANDLE_NODE(Expression, Dict) {
    VISIT_NODE_VECTOR(elements);
  }

  HANDLE_NODE(Expression, Function) {
    VISIT_NODE_VECTOR(arguments);
    VISIT_NODE(body);
  }

  HANDLE_NODE(Expression, Class) {
    VISIT_NODE(parent);
    VISIT_NODE(constructor);
    VISIT_NODE_VECTOR(member_functions)
    VISIT_NODE_VECTOR(member_properties)
    VISIT_NODE_VECTOR(static_properties)
  }

  HANDLE_NODE(ClassProperty, ClassProperty) {
    VISIT_NODE(value);
  }

  HANDLE_NODE(Expression, Assignment) {
    VISIT_NODE(target);
    VISIT_NODE(source);
  }

  HANDLE_NODE(Expression, Ternary) {
    VISIT_NODE(condition);
    VISIT_NODE(then_exp);
    VISIT_NODE(else_exp);
  }

  HANDLE_NODE(Expression, BinaryOp) {
    VISIT_NODE(lhs);
    VISIT_NODE(rhs);
  }

  HANDLE_NODE(Expression, UnaryOp) {
    VISIT_NODE(expression);
  }

  HANDLE_NODE(Expression, Spread) {
    VISIT_NODE(expression);
  }

  HANDLE_NODE(Expression, CallOp) {
    VISIT_NODE(target);
    VISIT_NODE_VECTOR(arguments)
  }

  HANDLE_NODE(Expression, MemberOp) {
    VISIT_NODE(target);
  }

  HANDLE_NODE(Expression, IndexOp) {
    VISIT_NODE(target);
    VISIT_NODE(index);
  }

  HANDLE_NODE(Statement, Declaration) {
    VISIT_NODE(target);
    VISIT_NODE(expression);
  }

  HANDLE_NODE(Statement, If) {
    VISIT_NODE(condition);
    VISIT_NODE(then_stmt);
    VISIT_NODE(else_stmt);
  }

  HANDLE_NODE(Statement, While) {
    VISIT_NODE(condition);
    VISIT_NODE(then_stmt);
  }

  HANDLE_NODE(Statement, Try) {
    VISIT_NODE(try_stmt);
    VISIT_NODE(catch_stmt);
  }

  HANDLE_NODE(Statement, Switch) {
    VISIT_NODE(test);
    VISIT_NODE(default_stmt);
    VISIT_NODE_VECTOR(cases)
  }

  HANDLE_NODE(SwitchCase, SwitchCase) {
    VISIT_NODE(test);
    VISIT_NODE(stmt);
  }

  HANDLE_NODE(Statement, For) {
    VISIT_NODE(target);
    VISIT_NODE(source);
    VISIT_NODE(stmt);
  }
  // clang-format on
};

class DiagnosticPass : public Pass {
public:
  DiagnosticPass(DiagnosticConsole& console) : m_console(console) {}

protected:
  DiagnosticConsole& m_console;
};

}  // namespace charly::core::compiler::ast

#undef HANDLE_NODE
#undef VISIT_NODE
#undef VISIT_NODE_VECTOR
