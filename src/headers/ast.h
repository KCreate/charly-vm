/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>

#include "irinfo.h"
#include "location.h"
#include "token.h"

#pragma once

namespace Charly::Compiler::AST {
static const std::string kPaddingCharacters = "  ";

// Abstract base class of all ASTNodes
//
// TODO: Location information
struct AbstractNode {
public:
  std::optional<Location> location_start;
  std::optional<Location> location_end;

  virtual ~AbstractNode() = default;

  inline AbstractNode& at(const Location& end) {
    this->location_end = end;
    return *this;
  }

  inline AbstractNode& at(const Location& start, const Location& end) {
    this->location_start = start;
    this->location_end = end;
    return *this;
  }

  inline AbstractNode& at(const AbstractNode& node) {
    this->location_start = node.location_start;
    this->location_end = node.location_end;
    return *this;
  }

  inline AbstractNode& at(const AbstractNode& start, const AbstractNode& end) {
    this->location_start = start.location_start;
    this->location_end = end.location_end;
    return *this;
  }

  inline size_t type() {
    return typeid(*this).hash_code();
  }
};

// A list of AST nodes with no preconceived notion of what context
// they are used in
struct NodeList : public AbstractNode {
  // TODO: Figure out a good name for this
  // Other alternatives are: nodes, items
  std::vector<AbstractNode*> children;

  NodeList() {}
  NodeList(std::initializer_list<AbstractNode*> list) : children(list) {}

  inline ~NodeList() {
    for (auto& node : this->children)
      delete node;
  }
};

// A list of AST nodes meant to represent a scoped block
//
// {
//   <statements>
// }
struct Block : public AbstractNode {
  std::vector<AbstractNode*> statements;

  Block() {}
  Block(std::initializer_list<AbstractNode*> list) : statements(list) {}

  inline ~Block() {
    for (auto& node : this->statements) {
      delete node;
    }
  }
};

// if <condition> {
//   <then_block>
// }
//
// if <condition> {
//   <then_block>
// } else {
//   <else_block>
// }
struct If : public AbstractNode {
  AbstractNode* condition;
  Block* then_block;
  Block* else_block;

  If(AbstractNode* c, Block* t, Block* e) : condition(c), then_block(t), else_block(e) {}

  inline ~If() {
    delete condition;
    delete then_block;
    delete else_block;
  }
};

// unless <condition> {
//   <then_block>
// }
//
// unless <condition> {
//   <then_block>
// } else {
//   <else_block>
// }
struct Unless : public AbstractNode {
  AbstractNode* condition;
  Block* then_block;
  Block* else_block;

  Unless(AbstractNode* c, Block* t, Block* e) : condition(c), then_block(t), else_block(e) {}

  inline ~Unless() {
    delete condition;
    delete then_block;
    delete else_block;
  }
};

// guard <condition> {
//   <block>
// }
struct Guard : public AbstractNode {
  AbstractNode* condition;
  Block* block;

  Guard(AbstractNode* c, Block* b) : condition(c), block(b) {}

  inline ~Guard() {
    delete condition;
    delete block;
  }
};

// while <condition> {
//   <block>
// }
struct While : public AbstractNode {
  AbstractNode* condition;
  Block* block;

  While(AbstractNode* c, Block* b) : condition(c), block(b) {}

  inline ~While() {
    delete condition;
    delete block;
  }
};

// until <condition> {
//   <block>
// }
struct Until : public AbstractNode {
  AbstractNode* condition;
  Block* block;

  Until(AbstractNode* c, Block* b) : condition(c), block(b) {}

  inline ~Until() {
    delete condition;
    delete block;
  }
};

// loop {
//   <block>
// }
struct Loop : public AbstractNode {
  Block* block;

  Loop(Block* b) : block(b) {}

  inline ~Loop() {
    delete block;
  }
};

// <operator_type> <expression>
struct Unary : public AbstractNode {
  TokenType operator_type;
  AbstractNode* expression;

  Unary(TokenType op, AbstractNode* e) : operator_type(op), expression(e) {}

  inline ~Unary() {
    delete expression;
  }
};

// <left> <operator_type> <right>
struct Binary : public AbstractNode {
  TokenType operator_type;
  AbstractNode* left;
  AbstractNode* right;

  Binary(TokenType op, AbstractNode* l, AbstractNode* r) : operator_type(op), left(l), right(r) {}

  inline ~Binary() {
    delete left;
    delete right;
  }
};

// case <conditions> {
//   <block>
// }
struct SwitchNode : public AbstractNode {
  NodeList* conditions;
  Block* block;

  SwitchNode(NodeList* c, Block* b) : conditions(c), block(b) {}

  inline ~SwitchNode() {
    delete conditions;
    delete block;
  }
};

// switch <condition> {
//   <cases>
//   default <default_block>
// }
struct Switch : public AbstractNode {
  AbstractNode* condition;
  std::vector<SwitchNode*> cases;
  Block* default_block;

  Switch(AbstractNode* co, const std::vector<SwitchNode*>& vec, Block* d)
    : condition(co), cases(vec), default_block(d) {}
  Switch(AbstractNode* co, std::initializer_list<SwitchNode*> c, Block* d)
    : condition(co), cases(c), default_block(d) {}

  inline ~Switch() {
    delete condition;
    for (auto& node : this->cases)
      delete node;
    delete default_block;
  }
};

// <left> && <right>
struct And : public AbstractNode {
  AbstractNode* left;
  AbstractNode* right;

  And(AbstractNode* l, AbstractNode* r) : left(l), right(r) {}

  inline ~And() {
    delete left;
    delete right;
  }
};

// <left> || <right>
struct Or : public AbstractNode {
  AbstractNode* left;
  AbstractNode* right;

  Or(AbstractNode* l, AbstractNode* r) : left(l), right(r) {}

  inline ~Or() {
    delete left;
    delete right;
  }
};

// typeof <expression>
struct Typeof : public AbstractNode {
  AbstractNode* expression;

  Typeof(AbstractNode* e) : expression(e) {}

  inline ~Typeof() {
    delete expression;
  }
};

// <target> = <expression>
struct Assignment : public AbstractNode {
  AbstractNode* target;
  AbstractNode* expression;

  Assignment(AbstractNode* t, AbstractNode* e) : target(t), expression(e) {}

  inline ~Assignment() {
    delete target;
    delete expression;
  }
};

// <target>(<arguments>)
struct Call : public AbstractNode {
  AbstractNode* target;
  NodeList* arguments;

  Call(AbstractNode* t, NodeList* a) : target(t), arguments(a) {}

  inline ~Call() {
    delete target;
    delete arguments;
  }
};

// <name>
struct Identifier : public AbstractNode {
  std::string name;
  IRVarOffsetInfo* offset_info;

  Identifier(const std::string& str) : name(str) {}
};

// <name>
struct Symbol : public AbstractNode {
  std::string name;

  Symbol(const std::string& str) : name(str) {}
};

// <target>.<identifier>
struct Member : public AbstractNode {
  AbstractNode* target;
  Symbol* symbol;

  Member(AbstractNode* t, Symbol* s) : target(t), symbol(s) {}

  inline ~Member() {
    delete target;
    delete symbol;
  }
};

// <target>[<argument>]
struct Index : public AbstractNode {
  AbstractNode* target;
  AbstractNode* argument;

  Index(AbstractNode* t, AbstractNode* a) : target(t), argument(a) {}

  inline ~Index() {
    delete target;
    delete argument;
  }
};

// null
struct Null : public AbstractNode {};

// NAN
struct Nan : public AbstractNode {};

// "<value>"
//
// value is optional because we don't want to allocate any memory for an empty string
struct String : public AbstractNode {
  std::optional<std::string> value;

  String(const std::string& str) : value(str) {}
};

// <value>
struct Integer : public AbstractNode {
  int64_t value;

  Integer(int64_t v) : value(v) {}
};

// <value>
struct Float : public AbstractNode {
  double value;

  Float(double v) : value(v) {}
};

// <value>
struct Boolean : public AbstractNode {
  bool value;

  Boolean(bool v) : value(v) {}
};

// [<expressions>]
struct Array : public AbstractNode {
  NodeList* expressions;

  Array(std::initializer_list<AbstractNode*> e)
    : expressions(new NodeList(std::forward<std::initializer_list<AbstractNode*>>(e))) {}
  Array(NodeList* e) : expressions(e) {}
};

// {
//   <pairs>
// }
struct Hash : public AbstractNode {
  std::vector<std::pair<AbstractNode*, AbstractNode*>> pairs;

  Hash(std::initializer_list<std::pair<AbstractNode*, AbstractNode*>> p)
    : pairs(p) {}

  inline ~Hash() {
    for (auto& pair : this->pairs) {
      delete pair.first;
      delete pair.second;
    }
  }
};

// func <name> (<parameters>) {
//   <body>
// }
//
// func (<parameters>) {
//   <body>
// }
//
// func {
//   <body>
// }
//
// ->(<parameters>) {
//   <body>
// }
//
// ->{
//   <body>
// }
//
// -><body>
struct Function : public AbstractNode {
  Symbol* name;
  NodeList* parameters;
  Block* body;
  bool anonymous;

  IRLVarInfo* lvar_info;

  Function(Symbol* n, NodeList* p, Block* b, bool a) : name(n), parameters(p), body(b), anonymous(a) {}

  inline ~Function() {
    delete name;
    delete parameters;
    delete body;
  }
};

// property <identifier>;
struct PropertyDeclaration : public AbstractNode {
  Symbol* symbol;

  PropertyDeclaration(Symbol* s) : symbol(s) {}

  inline ~PropertyDeclaration() {
    delete symbol;
  }
};

// static <declaration>
struct StaticDeclaration : public AbstractNode {
  AbstractNode* declaration;

  StaticDeclaration(AbstractNode* d) : declaration(d) {}

  inline ~StaticDeclaration() {
    delete declaration;
  }
};

// class {
//   <body>
// }
//
// class <name> {
//   <body>
// }
//
// class <name> extends <parents> {
//   <body>
// }
struct Class : public AbstractNode {
  Symbol* name;
  NodeList* body;
  NodeList* parents;

  Class(Symbol* n, NodeList* b, NodeList* p) : name(n), body(b), parents(p) {}

  inline ~Class() {
    delete name;
    delete body;
    delete parents;
  }
};

// let <name>
//
// let <name> = <expression>
//
// const <name> = <expression>
struct LocalInitialisation : public AbstractNode {
  Symbol* name;
  AbstractNode* expression;
  bool constant;

  LocalInitialisation(Symbol* n, AbstractNode* e, bool c) : name(n), expression(e), constant(c) {}

  inline ~LocalInitialisation() {
    delete name;
    delete expression;
  }
};

// return
//
// return <expression>
struct Return : public AbstractNode {
  AbstractNode* expression;

  Return(AbstractNode* e) : expression(e) {}

  inline ~Return() {
    delete expression;
  }
};

// throw <expression>
struct Throw : public AbstractNode {
  AbstractNode* expression;

  Throw(AbstractNode* e) : expression(e) {}

  inline ~Throw() {
    delete expression;
  }
};

// break
struct Break : public AbstractNode {};

// continue
struct Continue : public AbstractNode {};

// try {
//   <block>
// } catch (<exception_name>) {
//   <handler_block>
// }
struct TryCatch : public AbstractNode {
  Block* block;
  Symbol* exception_name;
  Block* handler_block;

  TryCatch(Block* b, Symbol* e, Block* h) : block(b), exception_name(e), handler_block(h) {}

  inline ~TryCatch() {
    delete block;
    delete exception_name;
    delete handler_block;
  }
};

// Precomputed typeid hashes for all AST nodes
static const size_t kTypeNodeList = typeid(NodeList).hash_code();
static const size_t kTypeBlock = typeid(Block).hash_code();
static const size_t kTypeIf = typeid(If).hash_code();
static const size_t kTypeUnless = typeid(Unless).hash_code();
static const size_t kTypeGuard = typeid(Guard).hash_code();
static const size_t kTypeWhile = typeid(While).hash_code();
static const size_t kTypeUntil = typeid(Until).hash_code();
static const size_t kTypeLoop = typeid(Loop).hash_code();
static const size_t kTypeUnary = typeid(Unary).hash_code();
static const size_t kTypeBinary = typeid(Binary).hash_code();
static const size_t kTypeSwitchNode = typeid(SwitchNode).hash_code();
static const size_t kTypeSwitch = typeid(Switch).hash_code();
static const size_t kTypeAnd = typeid(And).hash_code();
static const size_t kTypeOr = typeid(Or).hash_code();
static const size_t kTypeTypeof = typeid(Typeof).hash_code();
static const size_t kTypeAssignment = typeid(Assignment).hash_code();
static const size_t kTypeCall = typeid(Call).hash_code();
static const size_t kTypeIdentifier = typeid(Identifier).hash_code();
static const size_t kTypeSymbol = typeid(Symbol).hash_code();
static const size_t kTypeMember = typeid(Member).hash_code();
static const size_t kTypeIndex = typeid(Index).hash_code();
static const size_t kTypeNull = typeid(Null).hash_code();
static const size_t kTypeNan = typeid(Nan).hash_code();
static const size_t kTypeString = typeid(String).hash_code();
static const size_t kTypeInteger = typeid(Integer).hash_code();
static const size_t kTypeFloat = typeid(Float).hash_code();
static const size_t kTypeBoolean = typeid(Boolean).hash_code();
static const size_t kTypeArray = typeid(Array).hash_code();
static const size_t kTypeHash = typeid(Hash).hash_code();
static const size_t kTypeFunction = typeid(Function).hash_code();
static const size_t kTypePropertyDeclaration = typeid(PropertyDeclaration).hash_code();
static const size_t kTypeStaticDeclaration = typeid(StaticDeclaration).hash_code();
static const size_t kTypeClass = typeid(Class).hash_code();
static const size_t kTypeLocalInitialisation = typeid(LocalInitialisation).hash_code();
static const size_t kTypeReturn = typeid(Return).hash_code();
static const size_t kTypeThrow = typeid(Throw).hash_code();
static const size_t kTypeBreak = typeid(Break).hash_code();
static const size_t kTypeContinue = typeid(Continue).hash_code();
static const size_t kTypeTryCatch = typeid(TryCatch).hash_code();
}  // namespace Charly::Compiler::AST
