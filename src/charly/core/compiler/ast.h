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

#include <cmath>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "charly/charly.h"
#include "charly/core/compiler/ir/builtin.h"
#include "charly/core/compiler/ir/functioninfo.h"
#include "charly/core/compiler/ir/valuelocation.h"
#include "charly/core/compiler/location.h"
#include "charly/core/compiler/token.h"
#include "charly/utils/buffer.h"

#pragma once

namespace charly::core::compiler::ast {

class Pass;
class FunctionScope;
class BlockScope;

enum class Truthyness {
  False = 0,
  True = 1,
  Unknown = 3,
};

inline Truthyness truthyness_create(bool value) {
  return value ? Truthyness::True : Truthyness::False;
}

inline Truthyness truthyness_or(Truthyness a, Truthyness b) {
  if (a == Truthyness::True || b == Truthyness::True) {
    return Truthyness::True;
  }
  if (a == Truthyness::False && b == Truthyness::False) {
    return Truthyness::False;
  }
  return Truthyness::Unknown;
}

inline Truthyness truthyness_and(Truthyness a, Truthyness b) {
  if (a == Truthyness::True && b == Truthyness::True) {
    return Truthyness::True;
  }
  if (a == Truthyness::False || b == Truthyness::False) {
    return Truthyness::False;
  }
  return Truthyness::Unknown;
}

inline Truthyness truthyness_equal(Truthyness a, Truthyness b) {
  if (a == Truthyness::Unknown || b == Truthyness::Unknown) {
    return Truthyness::Unknown;
  }
  return truthyness_create(a == b);
}

inline Truthyness truthyness_not_equal(Truthyness a, Truthyness b) {
  if (a == Truthyness::Unknown || b == Truthyness::Unknown) {
    return Truthyness::Unknown;
  }
  return truthyness_create(a != b);
}

void dump_string_with_escape_sequences(std::ostream& out, const char* begin, size_t length);
inline void dump_string_with_escape_sequences(std::ostream& out, const std::string& str) {
  dump_string_with_escape_sequences(out, str.data(), str.size());
}

// base class of all ast nodes
class Node : public std::enable_shared_from_this<Node> {
  friend class Pass;

public:
  enum class Type : uint8_t {
    Unknown = 0,

    // Statements
    StatementList,
    Block,
    Return,
    Break,
    Continue,
    Throw,
    Assert,
    Export,
    Import,

    // Control Expressions
    Yield,
    Spawn,
    Await,
    Typeof,

    // Literals
    Id,
    Name,
    Int,
    Float,
    Bool,
    String,
    FormatString,
    Symbol,
    Null,
    Self,
    FarSelf,
    Super,
    Tuple,
    List,
    DictEntry,
    Dict,
    FunctionArgument,
    Function,
    ClassProperty,
    Class,

    // Expressions
    ExpressionWithSideEffects,
    MemberOp,
    IndexOp,
    UnpackTargetElement,
    UnpackTarget,
    Assignment,
    Ternary,
    BinaryOp,
    UnaryOp,
    Spread,
    CallOp,

    // Declaration
    Declaration,
    UnpackDeclaration,

    // Control structures
    If,
    While,
    Loop,
    Try,
    TryFinally,
    SwitchCase,
    Switch,
    For,

    // Intrinsic Operations
    BuiltinOperation
  };

  // finds the first node to match a compare function
  // using depth-first traversal of the ast
  //
  // a second skip function can be used to skip traversal
  // of certain node types
  static ref<Node> search(const ref<Node>& node,
                          std::function<bool(const ref<Node>&)> compare,
                          std::function<bool(const ref<Node>&)> skip);

  // finds all nodes that match a compare function
  // using depth-first traversal of the ast
  //
  // a second skip function can be used to skip traversal
  // of certain node types
  static std::vector<ref<Node>> search_all(const ref<Node>& node,
                                           std::function<bool(const ref<Node>&)> compare,
                                           std::function<bool(const ref<Node>&)> skip);

  explicit operator Location() const {
    return m_location;
  }

  const Location& location() const {
    return m_location;
  }

  void set_location(const Location& loc) {
    m_location = loc;
  }

  void set_location(const ref<Node>& node) {
    m_location = node->location();
  }

  void set_location(const Location& begin, const Location& end) {
    set_begin(begin);
    set_end(end);
  }

  void set_location(const ref<Node>& begin, const ref<Node>& end) {
    set_location(begin->location(), end->location());
  }

  void set_begin(const Location& loc) {
    m_location.set_begin(loc);
  }

  void set_end(const Location& loc) {
    m_location.set_end(loc);
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

  virtual void children(std::function<void(const ref<Node>&)>&&) const {}

  friend std::ostream& operator<<(std::ostream& out, const Node& node) {
    node.dump(out);
    return out;
  }

  // dump a textual representation of this node into the stream
  void dump(std::ostream& out, bool print_location = false) const;
  std::string dump(bool print_location = false) const {
    utils::Buffer buf;
    dump(buf, print_location);
    return buf.str();
  }

  // child classes may override and write additional info into the node output
  virtual void dump_info(std::ostream&) const {}

  virtual bool is_constant_value() const {
    return false;
  }

  virtual bool has_side_effects() const {
    return true;
  }

  virtual Truthyness truthyness() const {
    return Truthyness::Unknown;
  }

  virtual Truthyness compares_equal(const ref<Node>&) const;

private:
  static void search_all_impl(const ref<Node>& node,
                              std::function<bool(const ref<Node>&)> compare,
                              std::function<bool(const ref<Node>&)> skip,
                              std::vector<ref<Node>>& result);

protected:
  Location m_location = { .valid = false };
};

template <typename T>
bool isa(const ref<Node>& node) {
  return dynamic_cast<T*>(node.get()) != nullptr;
}

#define AST_NODE(T)                        \
public:                                    \
  Node::Type type() const override {       \
    return Node::Type::T;                  \
  }                                        \
                                           \
private:                                   \
  const char* node_name() const override { \
    return #T;                             \
  }

#define CHILD_NODE(N) \
  {                   \
    if (N)            \
      callback(N);    \
  }

#define CHILD_LIST(N)              \
  {                                \
    for (const auto& node : (N)) { \
      callback(node);              \
    }                              \
  }

#define CHILDREN() void children([[maybe_unused]] std::function<void(const ref<Node>&)>&& callback) const override

// {
//   <statement>
// }
class Statement : public Node {
public:
  virtual bool terminates_block() const {
    return false;
  }
};

// 1 + x, false, foo(bar)
class Expression : public Statement {};

class StatementList : public Statement {
  AST_NODE(StatementList)
public:
  template <typename... Args>
  explicit StatementList(Args&&... params) : statements({ std::forward<Args>(params)... }) {
    if (!statements.empty()) {
      this->set_begin(this->statements.front()->location());
      this->set_end(this->statements.back()->location());
    }
  }
  virtual ~StatementList() = default;

  std::list<ref<Statement>> statements;

  void append_statement(const ref<Statement>& stmt) {
    if (statements.empty()) {
      set_begin(stmt);
    }
    statements.push_back(stmt);
    set_end(stmt);
  }

  CHILDREN() {
    CHILD_LIST(statements)
  }

  bool has_side_effects() const override {
    for (auto& statement : statements) {
      if (statement->has_side_effects()) {
        return true;
      }
    }
    return false;
  }
};

// {
//   ...
// }
class Block final : public Statement {
  AST_NODE(Block)
public:
  template <typename... Args>
  explicit Block(Args&&... params) : statements({ std::forward<Args>(params)... }) {
    if (!statements.empty()) {
      this->set_begin(this->statements.front()->location());
      this->set_end(this->statements.back()->location());
    }
  }

  ref<BlockScope> variable_block_scope;

  // set to true for the top block of REPL input
  bool repl_toplevel_block = false;

  // set to true if break statements inside this block can jump to the end of the block
  bool allows_break = false;

  std::list<ref<Statement>> statements;

  CHILDREN() {
    CHILD_LIST(statements)
  }

  void dump_info(std::ostream& out) const override;

  void push_front(const ref<Statement>& stmt) {
    statements.insert(statements.begin(), stmt);
    set_begin(stmt);
  }

  void push_back(const ref<Statement>& stmt) {
    statements.push_back(stmt);
    set_end(stmt);
  }

  bool has_side_effects() const override {
    for (auto& statement : statements) {
      if (statement->has_side_effects()) {
        return true;
      }
    }
    return false;
  }
};

// return <exp>
class Return final : public Statement {
  AST_NODE(Return)
public:
  explicit Return() : value(nullptr) {}
  explicit Return(const ref<Expression>& value) : value(value) {
    set_location(value);
  }

  ref<Expression> value;

  CHILDREN() {
    CHILD_NODE(value)
  }

  bool terminates_block() const override {
    return true;
  }
};

// break
class Break final : public Statement {
  AST_NODE(Break)

  bool terminates_block() const override {
    return true;
  }
};

// continue
class Continue final : public Statement {
  AST_NODE(Continue)

  bool terminates_block() const override {
    return true;
  }
};

// throw <expression>
class Throw final : public Statement {
  AST_NODE(Throw)
public:
  explicit Throw(const ref<Expression>& expression) : expression(expression) {
    set_location(expression);
  }

  ref<Expression> expression;

  CHILDREN() {
    CHILD_NODE(expression)
  }

  bool terminates_block() const override {
    return true;
  }
};

// export <expression>
class Export final : public Statement {
  AST_NODE(Export)
public:
  explicit Export(const ref<Expression>& expression) : expression(expression) {
    set_location(expression);
  }

  ref<Expression> expression;

  CHILDREN() {
    CHILD_NODE(expression)
  }

  bool terminates_block() const override {
    return true;
  }
};

class ExpressionWithSideEffects final : public Expression {
  AST_NODE(ExpressionWithSideEffects)
public:
  explicit ExpressionWithSideEffects(const ref<Block>& block, const ref<Expression>& expression) :
    block(block), expression(expression) {
    set_location(block, expression);
  }

  ref<Block> block;
  ref<Expression> expression;

  CHILDREN(){ CHILD_NODE(block) CHILD_NODE(expression) }

  Truthyness truthyness() const override {
    return expression->truthyness();
  }

  bool has_side_effects() const override {
    return block->has_side_effects() || expression->has_side_effects();
  }
};

// import <source>
class Import final : public Expression {
  AST_NODE(Import)
public:
  explicit Import(const ref<Expression>& source) : source(source) {
    set_location(source);
  }

  ref<Expression> source;

  CHILDREN() {
    CHILD_NODE(source)
  }
};

// yield <expression>
class Yield final : public Expression {
  AST_NODE(Yield)
public:
  explicit Yield(const ref<Expression>& expression) : expression(expression) {
    set_location(expression);
  }

  ref<Expression> expression;

  CHILDREN() {
    CHILD_NODE(expression)
  }
};

// spawn <statement>
class Spawn final : public Expression {
  AST_NODE(Spawn)
public:
  explicit Spawn(const ref<Statement>& statement) : statement(statement) {
    set_location(statement);
  }

  ref<Statement> statement;

  CHILDREN() {
    CHILD_NODE(statement)
  }
};

// await <expression>
class Await final : public Expression {
  AST_NODE(Await)
public:
  explicit Await(const ref<Expression>& expression) : expression(expression) {
    set_location(expression);
  }

  ref<Expression> expression;

  CHILDREN() {
    CHILD_NODE(expression)
  }
};

// typeof <expression>
class Typeof final : public Expression {
  AST_NODE(Typeof)
public:
  explicit Typeof(const ref<Expression>& expression) : expression(expression) {
    this->set_location(expression);
  }

  ref<Expression> expression;

  CHILDREN(){ CHILD_NODE(expression) }

  Truthyness truthyness() const override {
    return Truthyness::True;
  }

  bool has_side_effects() const override {
    return expression->has_side_effects();
  }
};

// self
class Self final : public Expression {
  AST_NODE(Self)
public:
  explicit Self() {}

  bool has_side_effects() const override {
    return false;
  }
};

// self of some parent function
class FarSelf final : public Expression {
  AST_NODE(FarSelf)
public:
  explicit FarSelf(uint8_t depth = 0) : depth(depth) {}
  uint8_t depth;

  void dump_info(std::ostream& out) const override;

  bool has_side_effects() const override {
    return false;
  }
};

template <typename T>
class Atom : public Expression {
public:
  explicit Atom(T value) : value(value) {}
  T value;
};

template <typename T>
class ConstantAtom : public Atom<T> {
public:
  using Atom<T>::Atom;

  bool is_constant_value() const override {
    return true;
  }

  bool has_side_effects() const override {
    return false;
  }
};

// foo, bar, $_baz42
class Name;  // forward declaration
class Declaration;
class Id final : public Atom<std::string> {
  AST_NODE(Id)
public:
  using Atom<std::string>::Atom;

  explicit Id(const ref<Name>& name);
  explicit Id(const std::string& name) : Id(make<Name>(name)) {}

  ir::ValueLocation ir_location;
  ref<Node> declaration_node;

  bool assignable() const override {
    return true;
  }

  void dump_info(std::ostream& out) const override;

  bool has_side_effects() const override {
    return ir_location.has_side_effects();
  }
};

// used to represent names that do not refer to a variable
class Name final : public Atom<std::string> {
  AST_NODE(Name)
public:
  using Atom<std::string>::Atom;

  explicit Name(const ref<Id>& id) : Atom<std::string>::Atom(id->value) {
    this->set_location(id);
  }

  explicit Name(const std::string& value) : Atom<std::string>::Atom(value) {}

  ir::ValueLocation ir_location;

  bool assignable() const override {
    return false;
  }

  void dump_info(std::ostream& out) const override;
};

inline Id::Id(const ref<Name>& name) : Atom<std::string>::Atom(name->value) {
  this->set_location(name);
}

// null
// defined as a ConstantAtom<uint8_t> but argument value is not used...
class Null final : public ConstantAtom<uint8_t> {
  AST_NODE(Null)
public:
  Null() : ConstantAtom<uint8_t>::ConstantAtom(0) {}

  Truthyness truthyness() const override {
    return Truthyness::False;
  }
};

// 1, 2, 42
class Int final : public ConstantAtom<int64_t> {
  AST_NODE(Int)
public:
  using ConstantAtom<int64_t>::ConstantAtom;

  void dump_info(std::ostream& out) const override;

  Truthyness truthyness() const override {
    return value == 0 ? Truthyness::False : Truthyness::True;
  }
};

// 0.5, 25.25, 5000.1234
class Float final : public ConstantAtom<double> {
  AST_NODE(Float)
public:
  using ConstantAtom<double>::ConstantAtom;

  explicit Float(const ref<Node>& node) : ConstantAtom<double>::ConstantAtom(0) {
    if (ref<Int> int_node = cast<Int>(node)) {
      this->value = (double)int_node->value;
    } else if (ref<Float> float_node = cast<Float>(node)) {
      this->value = float_node->value;
    } else {
      FAIL("unexpected node type");
    }
  }

  void dump_info(std::ostream& out) const override;

  Truthyness truthyness() const override {
    if (value == 0.0 || std::isnan(value)) {
      return Truthyness::False;
    }
    return Truthyness::True;
  }
};

// true, false
class Bool final : public ConstantAtom<bool> {
  AST_NODE(Bool)
public:
  using ConstantAtom<bool>::ConstantAtom;

  void dump_info(std::ostream& out) const override;

  Truthyness truthyness() const override {
    return this->value ? Truthyness::True : Truthyness::False;
  }
};

// "hello world"
class String final : public ConstantAtom<std::string> {
  AST_NODE(String)
public:
  using ConstantAtom<std::string>::ConstantAtom;

  explicit String(const ref<Id>& id) : ConstantAtom<std::string>(id->value) {
    set_location(id);
  }

  void dump_info(std::ostream& out) const override;

  Truthyness truthyness() const override {
    return Truthyness::True;  // strings are always truthy
  }
};

// "name: {name} age: {age}"
class FormatString final : public Expression {
  AST_NODE(FormatString)
public:
  template <typename... Args>
  explicit FormatString(Args&&... params) : elements({ std::forward<Args>(params)... }) {}

  std::list<ref<Expression>> elements;

  CHILDREN(){ CHILD_LIST(elements) }

  Truthyness truthyness() const override {
    return Truthyness::True;
  }

  bool has_side_effects() const override {
    for (auto& element : elements) {
      if (element->has_side_effects()) {
        return true;
      }
    }
    return false;
  }
};

// :'hello'
class Symbol final : public ConstantAtom<std::string> {
  AST_NODE(Symbol)
public:
  using ConstantAtom<std::string>::ConstantAtom;
  explicit Symbol(const ref<Name>& name) : ConstantAtom<std::string>::ConstantAtom(name->value) {
    this->set_location(name);
  }
  explicit Symbol(const ref<Id>& name) : ConstantAtom<std::string>::ConstantAtom(name->value) {
    this->set_location(name);
  }
  explicit Symbol(const ref<String>& name) : ConstantAtom<std::string>::ConstantAtom(name->value) {
    this->set_location(name);
  }
  explicit Symbol(const std::string& value) : ConstantAtom<std::string>::ConstantAtom(value) {}

  void dump_info(std::ostream& out) const override;

  Truthyness truthyness() const override {
    return Truthyness::True;  // symbols are always truthy
  }
};

// (1, 2, 3)
class Tuple final : public Expression {
  AST_NODE(Tuple)
public:
  template <typename... Args>
  explicit Tuple(Args&&... params) : elements({ std::forward<Args>(params)... }) {}

  std::list<ref<Expression>> elements;

  bool assignable() const override;

  bool has_spread_elements() const;

  CHILDREN(){ CHILD_LIST(elements) }

  Truthyness truthyness() const override {
    return Truthyness::True;
  }

  bool has_side_effects() const override {
    for (auto& element : elements) {
      if (element->has_side_effects()) {
        return true;
      }
    }
    return false;
  }
};

// [1, 2, 3]
class List final : public Expression {
  AST_NODE(List)
public:
  template <typename... Args>
  explicit List(Args&&... params) : elements({ std::forward<Args>(params)... }) {}

  std::list<ref<Expression>> elements;

  bool has_spread_elements() const;

  CHILDREN(){ CHILD_LIST(elements) }

  Truthyness truthyness() const override {
    return Truthyness::True;
  }

  bool has_side_effects() const override {
    for (auto& element : elements) {
      if (element->has_side_effects()) {
        return true;
      }
    }
    return false;
  }
};

// { a: 1, b: false, c: foo }
class DictEntry final : public Node {
  AST_NODE(DictEntry)
public:
  explicit DictEntry(const ref<Expression>& key) : DictEntry(key, nullptr) {}
  DictEntry(const ref<Expression>& key, const ref<Expression>& value) : key(key), value(value) {
    if (value) {
      this->set_begin(key);
      this->set_end(value);
    } else {
      this->set_location(key);
    }
  }

  ref<Expression> key;
  ref<Expression> value;

  bool assignable() const override;

  CHILDREN() {
    CHILD_NODE(key)
    CHILD_NODE(value)
  }

  bool has_side_effects() const override {
    return key->has_side_effects() || value->has_side_effects();
  }
};

// { a: 1, b: false, c: foo }
class Dict final : public Expression {
  AST_NODE(Dict)
public:
  template <typename... Args>
  explicit Dict(Args&&... params) : elements({ std::forward<Args>(params)... }) {}

  std::list<ref<DictEntry>> elements;

  bool assignable() const override;

  bool has_spread_elements() const;

  CHILDREN(){ CHILD_LIST(elements) }

  Truthyness truthyness() const override {
    return Truthyness::True;
  }

  bool has_side_effects() const override {
    for (auto& element : elements) {
      if (element->has_side_effects()) {
        return true;
      }
    }
    return false;
  }
};

class FunctionArgument final : public Node {
  AST_NODE(FunctionArgument)
public:
  explicit FunctionArgument(const ref<Name>& name, const ref<Expression>& default_value = nullptr) :
    self_initializer(false), spread_initializer(false), name(name), default_value(default_value) {
    if (default_value) {
      this->set_location(name, default_value);
    } else {
      this->set_location(name);
    }
  }
  FunctionArgument(bool self_initializer,
                   bool spread_initializer,
                   const ref<Name>& name,
                   const ref<Expression>& default_value = nullptr) :
    self_initializer(self_initializer),
    spread_initializer(spread_initializer),
    name(name),
    default_value(default_value) {
    if (default_value) {
      this->set_location(name, default_value);
    } else {
      this->set_location(name);
    }
  }

  bool self_initializer;
  bool spread_initializer;
  ref<Name> name;
  ref<Expression> default_value;

  ir::ValueLocation ir_location;

  CHILDREN() {
    CHILD_NODE(default_value)
  }

  void dump_info(std::ostream& out) const override;
};

// func foo(a, b = 1, ...rest) {}
// ->(a, b) a + b
class Class;
class Function final : public Expression {
  AST_NODE(Function)
public:
  template <typename... Args>
  Function(bool arrow_function, const ref<Name>& name, const ref<Block>& body, Args&&... params) :
    class_constructor(false),
    class_member_function(false),
    class_static_function(false),
    class_private_function(false),
    variable_function_scope(nullptr),
    arrow_function(arrow_function),
    name(name),
    body(body),
    arguments({ std::forward<Args>(params)... }) {
    this->set_location(name, body);
  }

  // wether the function is a constructor
  bool class_constructor;

  // wether the function is a class member function
  bool class_member_function;

  // wether this is a static function of a class
  bool class_static_function;

  // wether this is a private function of a class
  bool class_private_function;

  ref<FunctionScope> variable_function_scope;
  weak_ref<Class> host_class;

  bool arrow_function;
  ref<Name> name;
  ref<Block> body;
  std::list<ref<FunctionArgument>> arguments;

  ir::FunctionInfo ir_info;

  CHILDREN(){ CHILD_LIST(arguments) CHILD_NODE(body) }

  uint8_t argc() const {
    if (!arguments.empty() && arguments.back()->spread_initializer) {
      return arguments.size() - 1;
    }

    return arguments.size();
  }

  uint8_t minimum_argc() const {
    uint8_t index = 0;
    for (auto& arg : arguments) {
      if (arg->default_value) {
        return index;
      }
      index++;
    }

    if (!arguments.empty() && arguments.back()->spread_initializer) {
      return arguments.size() - 1;
    }

    return arguments.size();
  }

  bool has_spread() const {
    return !arguments.empty() && arguments.back()->spread_initializer;
  }

  void dump_info(std::ostream& out) const override;

  bool has_side_effects() const override {
    return false;
  }

  Truthyness truthyness() const override {
    return Truthyness::True;
  }
};

// property foo
// static property bar = 42
class ClassProperty final : public Node {
  AST_NODE(ClassProperty)
public:
  ClassProperty(bool is_static, bool is_private, const ref<Name>& name, const ref<Expression>& value) :
    is_static(is_static), is_private(is_private), name(name), value(value) {
    this->set_location(name, value);
  }
  ClassProperty(bool is_static, bool is_private, const std::string& name, const ref<Expression>& value) :
    is_static(is_static), is_private(is_private), name(make<Name>(name)), value(value) {
    this->set_location(value);
  }

  bool is_static;
  bool is_private;
  ref<Name> name;
  ref<Expression> value;

  CHILDREN() {
    CHILD_NODE(value)
  }

  void dump_info(std::ostream& out) const override;
};

// class <name> [extends <parent>] { ... }
class Class final : public Expression {
  AST_NODE(Class)
public:
  Class(ref<Name> name, ref<Expression> parent) : is_final(false), name(name), parent(parent), constructor(nullptr) {}
  Class(const std::string& name, ref<Expression> parent) :
    is_final(false), name(make<Name>(name)), parent(parent), constructor(nullptr) {}

  bool is_final;
  ref<Name> name;
  ref<Expression> parent;
  ref<Function> constructor;
  std::list<ref<Function>> member_functions;
  std::list<ref<ClassProperty>> member_properties;
  std::list<ref<Function>> static_functions;
  std::list<ref<ClassProperty>> static_properties;

  std::unordered_map<SYMBOL, std::list<ref<Function>>> member_function_overloads;
  std::unordered_map<SYMBOL, std::list<ref<Function>>> static_function_overloads;

  CHILDREN() {
    CHILD_NODE(parent)
    CHILD_NODE(constructor)
    CHILD_LIST(member_functions)
    CHILD_LIST(member_properties)
    CHILD_LIST(static_functions)
    CHILD_LIST(static_properties)

    // *_function_overloads contains the same references as the *_functions lists
    // no need to visit again
  }

  void dump_info(std::ostream& out) const override;

  Truthyness truthyness() const override {
    return Truthyness::True;
  }
};

// super
class Super final : public Expression {
  AST_NODE(Super)
};

// <target>.<member>
class MemberOp final : public Expression {
  AST_NODE(MemberOp)
public:
  MemberOp(const ref<Expression>& target, const ref<Name>& member) : target(target), member(member) {
    this->set_begin(target);
    this->set_end(member);
  }
  MemberOp(const ref<Expression>& target, const std::string& member) : target(target), member(make<Name>(member)) {
    this->set_location(target);
  }

  ref<Expression> target;
  ref<Name> member;

  bool assignable() const override {
    return true;
  }

  CHILDREN() {
    CHILD_NODE(target)
  }

  void dump_info(std::ostream& out) const override;
};

// <target>[<index>]
class IndexOp final : public Expression {
  AST_NODE(IndexOp)
public:
  IndexOp(const ref<Expression>& target, const ref<Expression>& index) : target(target), index(index) {
    this->set_begin(target);
    this->set_end(index);
  }

  ref<Expression> target;
  ref<Expression> index;

  bool assignable() const override {
    return true;
  }

  CHILDREN() {
    CHILD_NODE(target)
    CHILD_NODE(index)
  }
};

class UnpackTargetElement final : public Node {
  AST_NODE(UnpackTargetElement)
public:
  explicit UnpackTargetElement(const ref<Expression>& target, bool spread = false) : spread(spread), target(target) {
    this->set_location(target);
  }

  bool spread;
  ref<Expression> target;

  CHILDREN() {
    CHILD_NODE(target)
  }

  void dump_info(std::ostream& out) const override;
};

class UnpackTarget final : public Expression {
  AST_NODE(UnpackTarget)
public:
  template <typename... Args>
  explicit UnpackTarget(bool object_unpack, Args&&... params) :
    object_unpack(object_unpack), elements({ std::forward<Args>(params)... }) {
    if (!elements.empty()) {
      this->set_location(elements.front(), elements.back());
    }
  }

  bool object_unpack;
  std::list<ref<UnpackTargetElement>> elements;

  CHILDREN() {
    CHILD_LIST(elements)
  }

  void dump_info(std::ostream& out) const override;

  bool has_side_effects() const override {
    for (auto& element : elements) {
      if (element->has_side_effects()) {
        return true;
      }
    }
    return false;
  }
};

// <target> <operation>= <source>
class Assignment final : public Expression {
  AST_NODE(Assignment)
public:
  Assignment(const ref<Expression>& target, const ref<Expression>& source) :
    operation(TokenType::Assignment), target(target), source(source) {
    this->set_begin(target);
    this->set_end(source);
  }
  Assignment(TokenType operation, const ref<Expression>& target, const ref<Expression>& source) :
    operation(operation), target(target), source(source) {
    this->set_begin(target);
    this->set_end(source);
  }

  TokenType operation;
  ref<Expression> target;
  ref<Expression> source;

  CHILDREN() {
    CHILD_NODE(target)
    CHILD_NODE(source)
  }

  void dump_info(std::ostream& out) const override;
};

// <condition> ? <then_exp> : <else_exp>
class Ternary final : public Expression {
  AST_NODE(Ternary)
public:
  Ternary(const ref<Expression>& condition, const ref<Expression>& then_exp, const ref<Expression>& else_exp) :
    condition(condition), then_exp(then_exp), else_exp(else_exp) {
    this->set_begin(condition);
    this->set_end(else_exp);
  }

  ref<Expression> condition;
  ref<Expression> then_exp;
  ref<Expression> else_exp;

  CHILDREN(){ CHILD_NODE(condition) CHILD_NODE(then_exp) CHILD_NODE(else_exp) }

  Truthyness truthyness() const override {
    auto condition_truthyness = condition->truthyness();
    if (condition_truthyness == Truthyness::True) {
      return then_exp->truthyness();
    } else if (condition_truthyness == Truthyness::False) {
      return else_exp->truthyness();
    } else {
      // if the condition truthyness isn't known, but both cases have the
      // same known truthyness, the whole expression will always evaluate to
      // that shared truthyness
      auto then_truth = then_exp->truthyness();
      auto else_truth = else_exp->truthyness();
      if (then_truth == else_truth) {
        return then_truth;
      }
    }

    return Truthyness::Unknown;
  }

  bool has_side_effects() const override {
    return condition->has_side_effects() || then_exp->has_side_effects() || else_exp->has_side_effects();
  }
};

// <lhs> <operation> <rhs>
class BinaryOp final : public Expression {
  AST_NODE(BinaryOp)
public:
  BinaryOp(TokenType operation, const ref<Expression>& lhs, const ref<Expression>& rhs) :
    operation(operation), lhs(lhs), rhs(rhs) {
    this->set_begin(lhs);
    this->set_end(rhs);
  }

  TokenType operation;
  ref<Expression> lhs;
  ref<Expression> rhs;

  CHILDREN() {
    CHILD_NODE(lhs)
    CHILD_NODE(rhs)
  }

  void dump_info(std::ostream& out) const override;

  Truthyness truthyness() const override {
    auto lhst = lhs->truthyness();
    auto rhst = rhs->truthyness();

    switch (operation) {
      case TokenType::And: {
        return truthyness_and(lhst, rhst);
      }
      case TokenType::Or: {
        return truthyness_or(lhst, rhst);
      }
      case TokenType::Equal:
      case TokenType::NotEqual: {
        bool invert_result = operation == TokenType::NotEqual;
        Truthyness comparison = lhs->compares_equal(rhs);
        if (comparison != Truthyness::Unknown) {
          bool result = comparison == Truthyness::True;
          if (invert_result) {
            result = !result;
          }

          return truthyness_create(result);
        }

        return Truthyness::Unknown;
      }
      default: return Truthyness::Unknown;
    }
  }

  bool has_side_effects() const override {
    return lhs->has_side_effects() || rhs->has_side_effects();
  }
};

// <operation> <expression>
class UnaryOp final : public Expression {
  AST_NODE(UnaryOp)
public:
  UnaryOp(TokenType operation, const ref<Expression>& expression) : operation(operation), expression(expression) {
    this->set_location(expression);
  }

  TokenType operation;
  ref<Expression> expression;

  CHILDREN() {
    CHILD_NODE(expression)
  }

  void dump_info(std::ostream& out) const override;

  Truthyness truthyness() const override {
    if (operation == TokenType::UnaryNot) {
      switch (expression->truthyness()) {
        case Truthyness::True: return Truthyness::False;
        case Truthyness::False: return Truthyness::True;
        default: break;
      }
    }
    return Truthyness::Unknown;
  }

  bool has_side_effects() const override {
    return expression->has_side_effects();
  }
};

// <target>(<arguments>)
class CallOp final : public Expression {
  AST_NODE(CallOp)
public:
  template <typename... Args>
  explicit CallOp(const ref<Expression>& target, Args&&... params) :
    target(target), arguments({ std::forward<Args>(params)... }) {
    this->set_begin(target);
    if (!arguments.empty() && arguments.back().get())
      this->set_end(arguments.back()->location());
  }

  ref<Expression> target;
  std::list<ref<Expression>> arguments;

  bool has_spread_elements() const;

  CHILDREN() {
    CHILD_NODE(target)
    CHILD_LIST(arguments)
  }
};

// ...<exp>
class Spread final : public Expression {
  AST_NODE(Spread)
public:
  explicit Spread(const ref<Expression>& expression) : expression(expression) {
    this->set_location(expression);
  }

  ref<Expression> expression;

  CHILDREN() {
    CHILD_NODE(expression)
  }
};

// let a
// let a = 2
// const b = 3
class Declaration final : public Statement {
  AST_NODE(Declaration)
public:
  Declaration(const ref<Name>& name, const ref<Expression>& expression, bool constant = false, bool implicit = false) :
    constant(constant), implicit(implicit), name(name), expression(expression) {
    this->set_begin(name);
    this->set_end(expression);
  }
  Declaration(const std::string& name,
              const ref<Expression>& expression,
              bool constant = false,
              bool implicit = false) :
    constant(constant), implicit(implicit), name(make<Name>(name)), expression(expression) {
    this->set_location(expression);
  }

  bool constant;
  bool implicit;  // func and class literal declaration are marked as implicit
  ref<Name> name;
  ref<Expression> expression;

  ir::ValueLocation ir_location;

  CHILDREN() {
    CHILD_NODE(expression)
  }

  void dump_info(std::ostream& out) const override;
};

// let (a, ...b, c) = 1
// const (a, ...b, c) = x
class UnpackDeclaration final : public Statement {
  AST_NODE(UnpackDeclaration)
public:
  UnpackDeclaration(const ref<UnpackTarget>& target, const ref<Expression>& expression, bool constant = false) :
    constant(constant), target(target), expression(expression) {
    this->set_begin(target);
    this->set_end(expression);
  }

  bool constant;
  ref<UnpackTarget> target;
  ref<Expression> expression;

  CHILDREN() {
    CHILD_NODE(target)
    CHILD_NODE(expression)
  }

  void dump_info(std::ostream& out) const override;
};

// if <condition>
//   <then_block>
// else
//   <else_block>
class If final : public Statement {
  AST_NODE(If)
public:
  If(const ref<Expression>& condition, const ref<Block>& then_block, const ref<Block>& else_block = nullptr) :
    condition(condition), then_block(then_block), else_block(else_block) {
    if (else_block) {
      this->set_location(condition, else_block);
    } else {
      this->set_location(condition);
    }
  }

  ref<Expression> condition;
  ref<Block> then_block;
  ref<Block> else_block;

  CHILDREN() {
    CHILD_NODE(condition)
    CHILD_NODE(then_block)
    CHILD_NODE(else_block)
  }

  bool has_side_effects() const override {
    return condition->has_side_effects() || then_block->has_side_effects() || else_block->has_side_effects();
  }
};

// while <condition> <then_block>
class While final : public Statement {
  AST_NODE(While)
public:
  While(const ref<Expression>& condition, const ref<Block>& then_block) : condition(condition), then_block(then_block) {
    this->set_begin(condition);
    this->set_end(then_block);
  }

  ref<Expression> condition;
  ref<Block> then_block;

  CHILDREN() {
    CHILD_NODE(condition)
    CHILD_NODE(then_block)
  }
};

// loop <then_block>
class Loop final : public Statement {
  AST_NODE(Loop)
public:
  explicit Loop(const ref<Block>& then_block) : then_block(then_block) {
    this->set_location(then_block);
  }

  ref<Block> then_block;

  CHILDREN() {
    CHILD_NODE(then_block)
  }
};

// assert <expression>
// assert <expression> : <message>
class Assert final : public Statement {
  AST_NODE(Assert)
public:
  explicit Assert(const ref<Expression>& expression, const ref<Expression>& message) :
    expression(expression), message(message) {
    set_begin(expression);
    set_end(message);
  }

  ref<Expression> expression;
  ref<Expression> message;

  CHILDREN() {
    CHILD_NODE(expression)
    CHILD_NODE(message)
  }
};

// try <try_block>
// [catch (<exception_name>) <catch_block>]
class Try final : public Statement {
  AST_NODE(Try)
public:
  Try(const ref<Block>& try_block, const ref<Name>& exception_name, const ref<Block>& catch_block) :
    try_block(try_block), exception_name(exception_name), catch_block(catch_block) {
    this->set_begin(try_block);
    this->set_end(catch_block);
  }
  Try(const ref<Block>& try_block, const std::string& exception_name, const ref<Block>& catch_block) :
    try_block(try_block), exception_name(make<Name>(exception_name)), catch_block(catch_block) {
    this->set_begin(try_block);
    this->set_end(catch_block);
  }

  ref<Block> try_block;
  ref<Name> exception_name;
  ref<Block> catch_block;
  ir::ValueLocation original_pending_exception;

  CHILDREN() {
    CHILD_NODE(try_block)
    CHILD_NODE(catch_block)
  }

  void dump_info(std::ostream& out) const override;

  bool has_side_effects() const override {
    return try_block->has_side_effects() || catch_block->has_side_effects();
  }
};

// try <try_block> finally <finally_block>
class TryFinally final : public Statement {
  AST_NODE(TryFinally)
public:
  TryFinally(const ref<Block>& try_block, const ref<Block>& finally_block) :
    try_block(try_block), finally_block(finally_block) {
    this->set_begin(finally_block);
    this->set_end(try_block);
  }

  ref<Block> try_block;
  ref<Block> finally_block;

  ir::ValueLocation exception_value_location;

  CHILDREN() {
    CHILD_NODE(try_block)
    CHILD_NODE(finally_block)
  }

  bool has_side_effects() const override {
    return try_block->has_side_effects() || finally_block->has_side_effects();
  }
};

class SwitchCase final : public Node {
  AST_NODE(SwitchCase)
public:
  SwitchCase(const ref<Expression>& test, const ref<Block>& block) : test(test), block(block) {
    this->set_begin(test);
    this->set_end(block);
  }

  ref<Expression> test;
  ref<Block> block;

  CHILDREN() {
    CHILD_NODE(test)
    CHILD_NODE(block)
  }

  bool has_side_effects() const override {
    return test->has_side_effects() || block->has_side_effects();
  }
};

// switch (<test>) { case <test> <stmt> default <default_block> }
class Switch final : public Statement {
  AST_NODE(Switch)
public:
  explicit Switch(const ref<Expression>& test) : test(test), default_block(nullptr) {
    this->set_location(test);
  }
  template <typename... Args>
  Switch(const ref<Expression>& test, const ref<Block>& default_block, Args&&... params) :
    test(test), default_block(default_block), cases({ std::forward<Args>(params)... }) {
    this->set_begin(test);
    if (default_block) {
      this->set_end(default_block);
    } else {
      this->set_end(test);
    }
  }

  ref<Expression> test;
  ref<Block> default_block;
  std::list<ref<SwitchCase>> cases;

  CHILDREN() {
    CHILD_NODE(test)
    CHILD_NODE(default_block)
    CHILD_LIST(cases)
  }

  bool has_side_effects() const override {
    if (test->has_side_effects() || default_block->has_side_effects()) {
      return true;
    }

    for (auto& element : cases) {
      if (element->has_side_effects()) {
        return true;
      }
    }

    return false;
  }
};

// for <target> in <source> <stmt>
class For final : public Statement {
  AST_NODE(For)
public:
  For(const ref<Statement>& declaration, const ref<Statement>& stmt) : declaration(declaration), stmt(stmt) {
    this->set_begin(declaration);
    this->set_end(stmt);
  }

  ref<Statement> declaration;
  ref<Statement> stmt;

  CHILDREN() {
    CHILD_NODE(declaration)
    CHILD_NODE(stmt)
  }

  bool has_side_effects() const override {
    return declaration->has_side_effects() || stmt->has_side_effects();
  }
};

// builtin(builtin_id, <arguments>...)
class BuiltinOperation final : public Expression {
  AST_NODE(BuiltinOperation)
public:
  explicit BuiltinOperation(const ref<Name>& name) {
    CHECK(ir::kBuiltinNameMapping.count(name->value));
    this->operation = ir::kBuiltinNameMapping.at(name->value);
    this->set_location(name);
  }

  template <typename... Args>
  explicit BuiltinOperation(ir::BuiltinId operation, Args&&... params) :
    operation(operation), arguments({ std::forward<Args>(params)... }) {
    if (!arguments.empty()) {
      this->set_location(arguments.front()->location(), arguments.back()->location());
    }
  }

  ir::BuiltinId operation;
  std::list<ref<Expression>> arguments;

  CHILDREN() {
    CHILD_LIST(arguments)
  }

  void dump_info(std::ostream& out) const override;

  Truthyness truthyness() const override {
    switch (operation) {
      case ir::BuiltinId::castbool: {
        DCHECK(arguments.size() == 1);
        return arguments.front()->truthyness();
      }
      case ir::BuiltinId::caststring:
      case ir::BuiltinId::castsymbol: {
        return Truthyness::True;
      }
      default: {
        return Truthyness::Unknown;
      }
    }
  }
};

#undef AST_NODE
#undef CHILD_NODE
#undef CHILD_LIST
#undef CHILDREN

}  // namespace charly::core::compiler::ast
