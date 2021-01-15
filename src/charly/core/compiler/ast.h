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

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "charly/core/compiler/location.h"
#include "charly/core/compiler/token.h"

#pragma once

namespace charly::core::compiler::ast {

template <typename T>
using ref = std::shared_ptr<T>;

template <typename T, typename... Args>
inline ref<T> make(Args... params) {
  return std::make_shared<T>(std::forward<Args>(params)...);
}

template <typename T, typename O>
inline ref<T> cast(ref<O> node) {
  return std::dynamic_pointer_cast<T>(node);
}

class ASTPass;  // forward declaration

// base class of all ast nodes
class Node : std::enable_shared_from_this<Node> {
  friend class ASTPass;

public:
  enum class Type : uint8_t {
    Unknown = 0,
    Program = 1,
    Block,
    Return,
    Break,
    Continue,
    Defer,
    Throw,
    Export,
    Id,
    Int,
    Float,
    Bool,
    Char,
    String,
    FormatString,
    Null,
    Self,
    Super,
    Tuple,
    Assignment,
    ANDAssignment,
    Ternary,
    BinaryOp,
    UnaryOp,
    TypeCount
  };

  static constexpr const char* TypeNames[] = {
    "Unknown", "Program", "Block",      "Return",        "Break",   "Continue", "Defer",        "Throw",      "Export",
    "Id",      "Int",     "Float",      "Bool",          "Char",    "String",   "FormatString", "Null",       "Self",
    "Super",   "Tuple",   "Assignment", "ANDAssignment", "Ternary", "BinaryOp", "UnaryOp",      "__SENTINEL",
  };

  const LocationRange& location() const {
    return m_location;
  }

  const Location& begin() const {
    return m_location.begin;
  }

  const Location& end() const {
    return m_location.end;
  }

  void set_location(const Location& loc) {
    set_begin(loc);
    set_end(loc);
  }

  void set_location(const LocationRange& range) {
    m_location = range;
  }

  void set_location(const ref<Node>& node) {
    m_location = node->location();
  }

  void set_begin(const Location& loc) {
    m_location.begin = loc;
  }

  void set_begin(const LocationRange& range) {
    m_location.begin = range.begin;
  }

  void set_begin(const ref<Node>& node) {
    set_begin(node->begin());
  }

  void set_end(const Location& loc) {
    m_location.end = loc;
  }

  void set_end(const LocationRange& range) {
    m_location.end = range.end;
  }

  void set_end(const ref<Node>& node) {
    set_end(node->end());
  }

  // get the name of this node based on its type
  const char* name() const {
    return TypeNames[static_cast<int>(this->type())];
  }

  virtual Type type() const = 0;

protected:
  virtual ~Node(){};
  template <typename T>
  ref<T> visit(ASTPass* pass, const ref<T>& node);
  virtual void visit_children(ASTPass*) {}

  LocationRange m_location;
};

template <typename T>
bool isa(const ref<Node>& node) {
  return dynamic_cast<T*>(node.get()) != nullptr;
}

#define AST_NODE(T)                          \
  virtual Node::Type type() const override { \
    return Node::Type::T;                    \
  }

// {
//   <statement>
// }
class Statement : public Node {};

// 1 + x, false, foo(bar)
class Expression : public Statement {};

// 1, 0.5, false, null, "hello", 0
class ConstantExpression : public Expression {};

// {
//   ...
// }
class Block final : public Statement {
  AST_NODE(Block)
public:
  template <typename... Args>
  Block(Args&&... params) : statements({ std::forward<Args>(params)... }) {}

  std::vector<ref<Statement>> statements;

  virtual void visit_children(ASTPass*) override;
};

// top level node of a compiled program
class Program final : public Node {
  AST_NODE(Program)
public:
  Program(const std::string& filename, ref<Statement> body = nullptr) : filename(filename), body(body) {
    this->set_location(body);
  }

  std::string filename;
  ref<Statement> body;

  virtual void visit_children(ASTPass*) override;
};

// return <exp>
class Return final : public Statement {
  AST_NODE(Return)
public:
  Return(ref<Expression> expression) : expression(expression) {
    this->set_location(expression);
  }

  ref<Expression> expression;

  virtual void visit_children(ASTPass*) override;
};

// break
class Break final : public Statement {
  AST_NODE(Break)
};

// continue
class Continue final : public Statement {
  AST_NODE(Continue)
};

// defer <statement>
class Defer final : public Statement {
  AST_NODE(Defer)
public:
  Defer(ref<Statement> statement) : statement(statement) {
    this->set_location(statement);
  }

  ref<Statement> statement;

  virtual void visit_children(ASTPass*) override;
};

// throw <expression>
class Throw final : public Statement {
  AST_NODE(Throw)
public:
  Throw(ref<Expression> expression) : expression(expression) {
    this->set_location(expression);
  }

  ref<Expression> expression;

  virtual void visit_children(ASTPass*) override;
};

// export <expression>
class Export final : public Statement {
  AST_NODE(Export)
public:
  Export(ref<Expression> expression) : expression(expression) {
    this->set_location(expression);
  }

  ref<Expression> expression;

  virtual void visit_children(ASTPass*) override;
};

// <target> = <source>
class Assignment : public Expression {
  AST_NODE(Assignment)
public:
  Assignment(ref<Expression> target, ref<Expression> source) : target(target), source(source) {
    this->set_begin(target);
    this->set_end(source);
  }

  ref<Expression> target;
  ref<Expression> source;

  virtual void visit_children(ASTPass*) override;
};

// <target> <operation>= <source>
class ANDAssignment : public Expression {
  AST_NODE(ANDAssignment)
public:
  ANDAssignment(TokenType operation, ref<Expression> target, ref<Expression> source) :
    operation(operation), target(target), source(source) {
    this->set_begin(target);
    this->set_end(source);
  }

  TokenType operation;
  ref<Expression> target;
  ref<Expression> source;

  virtual void visit_children(ASTPass*) override;
};

// <condition> ? <then_exp> : <else_exp>
class Ternary : public Expression {
  AST_NODE(Ternary)
public:
  Ternary(ref<Expression> condition, ref<Expression> then_exp, ref<Expression> else_exp) :
    condition(condition), then_exp(then_exp), else_exp(else_exp) {
    this->set_begin(condition);
    this->set_end(else_exp);
  }

  ref<Expression> condition;
  ref<Expression> then_exp;
  ref<Expression> else_exp;

  virtual void visit_children(ASTPass*) override;
};

// <lhs> <operation> <rhs>
class BinaryOp : public Expression {
  AST_NODE(BinaryOp)
public:
  BinaryOp(TokenType operation, ref<Expression> lhs, ref<Expression> rhs) : operation(operation), lhs(lhs), rhs(rhs) {
    this->set_begin(lhs);
    this->set_end(rhs);
  }

  TokenType operation;
  ref<Expression> lhs;
  ref<Expression> rhs;

  virtual void visit_children(ASTPass*) override;
};

// <operation> <expression>
class UnaryOp : public Expression {
  AST_NODE(UnaryOp)
public:
  UnaryOp(TokenType operation, ref<Expression> expression) : operation(operation), expression(expression) {
    this->set_location(expression);
  }

  TokenType operation;
  ref<Expression> expression;

  virtual void visit_children(ASTPass*) override;
};

// null
class Null final : public ConstantExpression {
  AST_NODE(Null)
};

// self
class Self final : public ConstantExpression {
  AST_NODE(Self)
};

// super
class Super final : public ConstantExpression {
  AST_NODE(Super)
};

template <typename T>
class Atom : public ConstantExpression {
public:
  Atom(T value) : value(value) {}
  T value;
};

// foo, bar, $_baz42
class Id final : public Atom<std::string> {
  AST_NODE(Id)
public:
  using Atom<std::string>::Atom;
};

// 1, 2, 42
class Int final : public Atom<int64_t> {
  AST_NODE(Int)
public:
  using Atom<int64_t>::Atom;
};

// 0.5, 25.25, 5000.1234
class Float final : public Atom<double> {
  AST_NODE(Float)
public:
  using Atom<double>::Atom;
};

// true, false
class Bool final : public Atom<bool> {
  AST_NODE(Bool)
public:
  using Atom<bool>::Atom;
};

// 'a', '\n', 'ä', 'π'
class Char final : public Atom<uint32_t> {
  AST_NODE(Char)
public:
  using Atom<uint32_t>::Atom;
};

// "hello world"
class String final : public Atom<std::string> {
  AST_NODE(String)
public:
  using Atom<std::string>::Atom;
};

// "name: {name} age: {age}"
class FormatString final : public Expression {
  AST_NODE(FormatString)
public:
  template <typename... Args>
  FormatString(Args&&... params) : elements({ std::forward<Args>(params)... }) {}

  std::vector<ref<Expression>> elements;

  virtual void visit_children(ASTPass*) override;
};

// (1, 2, 3)
class Tuple final : public Expression {
  AST_NODE(Tuple)
public:
  template <typename... Args>
  Tuple(Args&&... params) : elements({ std::forward<Args>(params)... }) {}

  std::vector<ref<Expression>> elements;

  virtual void visit_children(ASTPass*) override;
};

#undef AST_NODE

}  // namespace charly::core::compiler::ast
