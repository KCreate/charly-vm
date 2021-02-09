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
    return nullptr;
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
  HANDLE_NODE(Program, Program, {
    APPLY_NODE(body);
  })

  HANDLE_NODE(Statement, Block, {
    APPLY_VECTOR(statements);
  })

  HANDLE_NODE(Statement, Return, {
    APPLY_NODE(expression);
  })

  HANDLE_NODE(Statement, Break, {})
  HANDLE_NODE(Statement, Continue, {})

  HANDLE_NODE(Statement, Defer, {
    APPLY_NODE(statement);
  })

  HANDLE_NODE(Statement, Throw, {
    APPLY_NODE(expression);
  })

  HANDLE_NODE(Statement, Export, {
    APPLY_NODE(expression);
  })

  HANDLE_NODE(Statement, Import, {
    APPLY_NODE(source);
  })

  HANDLE_NODE(Expression, Yield, {
    APPLY_NODE(expression);
  })

  HANDLE_NODE(Expression, Spawn, {
    APPLY_NODE(statement);
  })

  HANDLE_NODE(Expression, Await, {
    APPLY_NODE(expression);
  })

  HANDLE_NODE(Expression, Typeof, {
    APPLY_NODE(expression);
  })

  HANDLE_NODE(Expression, Id, {})
  HANDLE_NODE(Expression, Name, {})
  HANDLE_NODE(Expression, Int, {})
  HANDLE_NODE(Expression, Float, {})
  HANDLE_NODE(Expression, Bool, {})
  HANDLE_NODE(Expression, Char, {})
  HANDLE_NODE(Expression, String, {})

  HANDLE_NODE(Expression, FormatString, {
    APPLY_VECTOR(elements)
  })

  HANDLE_NODE(Expression, Null, {})
  HANDLE_NODE(Expression, Self, {})
  HANDLE_NODE(Expression, Super, {})

  HANDLE_NODE(Expression, Tuple, {
    APPLY_VECTOR(elements)
  })

  HANDLE_NODE(Expression, List, {
    APPLY_VECTOR(elements)
  })

  HANDLE_NODE(DictEntry, DictEntry, {
    APPLY_NODE(key);
    APPLY_NODE(value);
  })

  HANDLE_NODE(Expression, Dict, {
    APPLY_VECTOR(elements);
  })

  HANDLE_NODE(FunctionArgument, FunctionArgument, {
    APPLY_NODE(default_value);
  })

  HANDLE_NODE(Expression, Function, {
    APPLY_NODE(body);
    APPLY_VECTOR(arguments);
  })

  HANDLE_NODE(Expression, Class, {
    APPLY_NODE(parent);
    APPLY_NODE(constructor);
    APPLY_VECTOR(member_functions)
    APPLY_VECTOR(member_properties)
    APPLY_VECTOR(static_properties)
  })

  HANDLE_NODE(ClassProperty, ClassProperty, {
    APPLY_NODE(value);
  })

  HANDLE_NODE(Expression, Assignment, {
    APPLY_NODE(target);
    APPLY_NODE(source);
  })

  HANDLE_NODE(Expression, Ternary, {
    APPLY_NODE(condition);
    APPLY_NODE(then_exp);
    APPLY_NODE(else_exp);
  })

  HANDLE_NODE(Expression, BinaryOp, {
    APPLY_NODE(lhs);
    APPLY_NODE(rhs);
  })

  HANDLE_NODE(Expression, UnaryOp, {
    APPLY_NODE(expression);
  })

  HANDLE_NODE(Expression, Spread, {
    APPLY_NODE(expression);
  })

  HANDLE_NODE(Expression, CallOp, {
    APPLY_NODE(target);
    APPLY_VECTOR(arguments)
  })

  HANDLE_NODE(Expression, MemberOp, {
    APPLY_NODE(target);
  })

  HANDLE_NODE(Expression, IndexOp, {
    APPLY_NODE(target);
    APPLY_NODE(index);
  })

  HANDLE_NODE(Statement, Declaration, {
    APPLY_NODE(expression);
  })

  HANDLE_NODE(Statement, UnpackDeclaration, {
    APPLY_NODE(target);
    APPLY_NODE(expression);
  })

  HANDLE_NODE(Statement, If, {
    APPLY_NODE(condition);
    APPLY_NODE(then_stmt);
    APPLY_NODE(else_stmt);
  })

  HANDLE_NODE(Statement, While, {
    APPLY_NODE(condition);
    APPLY_NODE(then_stmt);
  })

  HANDLE_NODE(Statement, Try, {
    APPLY_NODE(try_stmt);
    APPLY_NODE(catch_stmt);
  })

  HANDLE_NODE(Statement, Switch, {
    APPLY_NODE(test);
    APPLY_NODE(default_stmt);
    APPLY_VECTOR(cases)
  })

  HANDLE_NODE(SwitchCase, SwitchCase, {
    APPLY_NODE(test);
    APPLY_NODE(stmt);
  })

  HANDLE_NODE(Statement, For, {
    APPLY_NODE(declaration);
    APPLY_NODE(stmt);
  })

  HANDLE_NODE(Expression, BuiltinOperation, {
    APPLY_VECTOR(arguments);
  })
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
#undef APPLY_NODE
#undef APPLY_VECTOR