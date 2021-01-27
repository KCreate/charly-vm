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

    // Statements
    Nop,
    Block,
    Return,
    Break,
    Continue,
    Defer,
    Throw,
    Export,
    Import,

    // Control Expressions
    ImportExpression,
    Yield,
    Spawn,
    Await,
    Typeof,
    As,

    // Literals
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
    List,
    DictEntry,
    Dict,

    // Expressions
    Assignment,
    Ternary,
    BinaryOp,
    UnaryOp,
    CallOp,
    MemberOp,
    IndexOp,

    // Control Statements
    Declaration,
    If,
    While,

    // Meta
    TypeCount
  };

  const Location& location() const {
    return m_location;
  }

  void set_location(const Location& loc) {
    m_location = loc;
  }

  void set_location(const ref<Node>& node) {
    m_location = node->location();
  }

  void set_begin(const Location& loc) {
    m_location.valid = loc.valid;
    m_location.compound = true;
    m_location.offset = loc.offset;
    m_location.row = loc.row;
    m_location.column = loc.column;
  }

  void set_end(const Location& loc) {
    m_location.valid = loc.valid;
    m_location.compound = true;
    m_location.end_offset = loc.end_offset;
    m_location.end_row = loc.end_row;
    m_location.end_column = loc.end_column;
  }

  void set_begin(const ref<Node>& node) {
    set_begin(node->location());
  }

  void set_end(const ref<Node>& node) {
    set_end(node->location());
  }

  virtual Type type() const = 0;
  virtual const char* node_name() const = 0;

  virtual bool assignable() const {
    return false;
  }

protected:
  virtual ~Node(){};
  template <typename T>
  ref<T> visit(ASTPass* pass, const ref<T>& node);
  virtual void visit_children(ASTPass*) {}

  Location m_location = { .valid = false };
};

template <typename T>
bool isa(const ref<Node>& node) {
  return dynamic_cast<T*>(node.get()) != nullptr;
}

#define AST_NODE(T)                                \
public:                                            \
  virtual Node::Type type() const override {       \
    return Node::Type::T;                          \
  }                                                \
                                                   \
private:                                           \
  virtual const char* node_name() const override { \
    return #T;                                     \
  }

// {
//   <statement>
// }
class Statement : public Node {};

// no operation
class Nop final : public Statement {
  AST_NODE(Nop);
};

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
  Program(ref<Statement> body = nullptr) : body(body) {
    this->set_location(body);
  }

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

// import <identifier>
// import <identifier> as <identifier>
// import <string> as <identifier>
class Import final : public Statement {
  AST_NODE(Import)
public:
  Import(ref<Expression> source) : source(source) {
    this->set_location(source);
  }

  ref<Expression> source;

  virtual void visit_children(ASTPass*) override;
};

// (import <expression>)
class ImportExpression final : public Expression {
  AST_NODE(ImportExpression)
public:
  ImportExpression(ref<Expression> source) : source(source) {
    this->set_location(source);
  }

  ref<Expression> source;

  virtual void visit_children(ASTPass*) override;
};

// yield <expression>
class Yield final : public Expression {
  AST_NODE(Yield)
public:
  Yield(ref<Expression> expression) : expression(expression) {
    this->set_location(expression);
  }

  ref<Expression> expression;

  virtual void visit_children(ASTPass*) override;
};

// spawn <statement>
class Spawn final : public Expression {
  AST_NODE(Spawn)
public:
  Spawn(ref<Statement> statement) : statement(statement) {
    this->set_location(statement);
  }

  ref<Statement> statement;

  virtual void visit_children(ASTPass*) override;
};

// await <expression>
class Await final : public Expression {
  AST_NODE(Await)
public:
  Await(ref<Expression> expression) : expression(expression) {
    this->set_location(expression);
  }

  ref<Expression> expression;

  virtual void visit_children(ASTPass*) override;
};

// typeof <expression>
class Typeof final : public Expression {
  AST_NODE(Typeof)
public:
  Typeof(ref<Expression> expression) : expression(expression) {
    this->set_location(expression);
  }

  ref<Expression> expression;

  virtual void visit_children(ASTPass*) override;
};

// <target> <operation>= <source>
class Assignment final : public Expression {
  AST_NODE(Assignment)
public:
  Assignment(ref<Expression> target, ref<Expression> source) :
    operation(TokenType::Assignment), target(target), source(source) {
    this->set_begin(target);
    this->set_end(source);
  }
  Assignment(TokenType operation, ref<Expression> target, ref<Expression> source) :
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
class Ternary final : public Expression {
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
class BinaryOp final : public Expression {
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
class UnaryOp final : public Expression {
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

  virtual bool assignable() const override {
    return true;
  }
};

// <expression> as <name>
class As final : public Expression {
  AST_NODE(As)
public:
  As(ref<Expression> expression, std::string name) : expression(expression), name(name) {
    this->set_location(expression);
  }
  As(ref<Expression> expression, ref<Id> name) : expression(expression), name(name->value) {
    this->set_begin(expression);
    this->set_end(name);
  }

  ref<Expression> expression;
  std::string name;

  virtual void visit_children(ASTPass*) override;
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

  virtual bool assignable() const override {
    if (elements.size() == 0)
      return false;

    bool spread_passed = false;
    for (const ref<Expression>& node : elements) {
      if (isa<Id>(node)) {
        continue;
      }

      if (ref<UnaryOp> unaryop = cast<UnaryOp>(node)) {
        if (spread_passed)
          return false;

        spread_passed = true;

        if (isa<Id>(unaryop->expression)) {
          continue;
        }
      }

      return false;
    }

    return true;
  }

  virtual void visit_children(ASTPass*) override;
};

// [1, 2, 3]
class List final : public Expression {
  AST_NODE(List)
public:
  template <typename... Args>
  List(Args&&... params) : elements({ std::forward<Args>(params)... }) {}

  std::vector<ref<Expression>> elements;

  virtual void visit_children(ASTPass*) override;
};

// { a: 1, b: false, c: foo }
class DictEntry final : public Node {
  AST_NODE(DictEntry)
public:
  DictEntry(ref<Expression> key) : DictEntry(key, nullptr) {}
  DictEntry(ref<Expression> key, ref<Expression> value) : key(key), value(value) {
    if (value) {
      this->set_begin(key);
      this->set_end(value);
    } else {
      this->set_location(key);
    }
  }

  ref<Expression> key;
  ref<Expression> value;

  virtual bool assignable() const override {
    if (value.get())
      return false;

    if (isa<Id>(key))
      return true;

    if (ref<UnaryOp> unaryop = cast<UnaryOp>(key)) {
      return isa<Id>(unaryop->expression);
    }

    return false;
  }

  virtual void visit_children(ASTPass*) override;
};

// { a: 1, b: false, c: foo }
class Dict final : public Expression {
  AST_NODE(Dict)
public:
  template <typename... Args>
  Dict(Args&&... params) : elements({ std::forward<Args>(params)... }) {}

  std::vector<ref<DictEntry>> elements;

  virtual bool assignable() const override {
    if (elements.size() == 0)
      return false;

    bool spread_passed = false;
    for (const ref<DictEntry>& node : elements) {
      if (!node->assignable())
        return false;

      if (isa<UnaryOp>(node->key)) {
        if (spread_passed)
          return false;
        spread_passed = true;
      }
    }

    return true;
  }

  virtual void visit_children(ASTPass*) override;
};

// <target>(<arguments>)
class CallOp final : public Expression {
  AST_NODE(CallOp)
public:
  template <typename... Args>
  CallOp(ref<Expression> target, Args&&... params) : target(target), arguments({ std::forward<Args>(params)... }) {
    this->set_begin(target);
    if (arguments.size() && arguments.back().get())
      this->set_end(arguments.back()->location());
  }

  ref<Expression> target;
  std::vector<ref<Expression>> arguments;

  virtual void visit_children(ASTPass*) override;
};

// <target>.<member>
class MemberOp final : public Expression {
  AST_NODE(MemberOp)
public:
  MemberOp(ref<Expression> target, const std::string& member) : target(target), member(member) {
    this->set_location(target);
  }
  MemberOp(ref<Expression> target, ref<Id> member) : target(target), member(member->value) {
    this->set_begin(target);
    this->set_end(member);
  }

  ref<Expression> target;
  std::string member;

  virtual bool assignable() const override {
    return true;
  }

  virtual void visit_children(ASTPass*) override;
};

// <target>[<index>]
class IndexOp final : public Expression {
  AST_NODE(IndexOp)
public:
  IndexOp(ref<Expression> target, ref<Expression> index) : target(target), index(index) {
    this->set_begin(target);
    this->set_end(index);
  }

  ref<Expression> target;
  ref<Expression> index;

  virtual bool assignable() const override {
    return true;
  }

  virtual void visit_children(ASTPass*) override;
};

// let a
// let a = 2
// const b = 3
// let (a, ...b, c) = 1
// const (a, ...b, c) = x
class Declaration final : public Expression {
  AST_NODE(Declaration)
public:
  Declaration(ref<Expression> target, ref<Expression> expression, bool constant = false) :
    constant(constant), target(target), expression(expression) {
    this->set_begin(target);
    this->set_end(expression);
  }

  bool constant;
  ref<Expression> target;
  ref<Expression> expression;

  virtual void visit_children(ASTPass*) override;
};

// if <condition>
//   <then_stmt>
// else
//   <else_stmt>
class If final : public Expression {
  AST_NODE(If)
public:
  If(ref<Expression> condition, ref<Statement> then_stmt, ref<Statement> else_stmt) :
    condition(condition), then_stmt(then_stmt), else_stmt(else_stmt) {
    this->set_begin(condition);
    this->set_end(else_stmt);
  }

  ref<Expression> condition;
  ref<Statement> then_stmt;
  ref<Statement> else_stmt;

  virtual void visit_children(ASTPass*) override;
};

// while <condition>
//   <then_stmt>
class While final : public Expression {
  AST_NODE(While)
public:
  While(ref<Expression> condition, ref<Statement> then_stmt) : condition(condition), then_stmt(then_stmt) {
    this->set_begin(condition);
    this->set_end(then_stmt);
  }

  ref<Expression> condition;
  ref<Statement> then_stmt;

  virtual void visit_children(ASTPass*) override;
};

#undef AST_NODE

}  // namespace charly::core::compiler::ast
