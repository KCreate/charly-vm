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

  inline AbstractNode* at(const Location& end) {
    this->location_end = end;
    return this;
  }

  inline AbstractNode* at(const Location& start, const Location& end) {
    this->location_start = start;
    this->location_end = end;
    return this;
  }

  inline AbstractNode* at(const std::optional<Location>& end) {
    this->location_end = end;
    return this;
  }

  inline AbstractNode* at(const std::optional<Location>& start, const std::optional<Location>& end) {
    this->location_start = start;
    this->location_end = end;
    return this;
  }

  inline AbstractNode* at(const AbstractNode& node) {
    this->location_start = node.location_start;
    this->location_end = node.location_end;
    return this;
  }

  inline AbstractNode* at(AbstractNode* node) {
    this->location_start = node->location_start;
    this->location_end = node->location_end;
    return this;
  }

  inline AbstractNode* at(const AbstractNode& start, const AbstractNode& end) {
    this->location_start = start.location_start;
    this->location_end = end.location_end;
    return this;
  }

  inline AbstractNode* at(AbstractNode* start, AbstractNode* end) {
    this->location_start = start->location_start;
    this->location_end = end->location_end;
    return this;
  }

  virtual inline size_t type() {
    return typeid(*this).hash_code();
  }

  virtual void dump(std::ostream& stream, size_t depth = 0) = 0;
};

// A node represting the absence of another node
struct Empty : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Empty:" << this << '\n';
  }
};

// A list of AST nodes with no preconceived notion of what context
// they are used in
struct NodeList : public AbstractNode {
  // TODO: Figure out a good name for this
  // Other alternatives are: nodes, items
  std::vector<AbstractNode*> children;

  NodeList() {
  }
  NodeList(std::initializer_list<AbstractNode*> list) : children(list) {
  }

  inline void append_node(AbstractNode* node) {
    if (this->children.size() == 0)
      this->at(*node);
    this->children.push_back(node);
    this->location_end = node->location_end;
  }

  inline ~NodeList() {
    for (auto& node : this->children)
      delete node;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- NodeList:" << this << '\n';
    for (auto& node : this->children)
      node->dump(stream, depth + 1);
  }
};

// A list of AST nodes meant to represent a scoped block
//
// {
//   <statements>
// }
struct Block : public AbstractNode {
  std::vector<AbstractNode*> statements;

  Block() {
  }
  Block(std::initializer_list<AbstractNode*> list) : statements(list) {
  }

  inline ~Block() {
    for (auto& node : this->statements) {
      delete node;
    }
  }

  inline void append_node(AbstractNode* node) {
    this->statements.push_back(node);
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Block:" << this << '\n';
    for (auto& node : this->statements)
      node->dump(stream, depth + 1);
  }
};

// if <condition> {
//   <then_block>
// }
struct If : public AbstractNode {
  AbstractNode* condition;
  AbstractNode* then_block;

  If(AbstractNode* c, AbstractNode* t) : condition(c), then_block(t) {
  }

  inline ~If() {
    delete condition;
    delete then_block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- If:" << this << '\n';
    this->condition->dump(stream, depth + 1);
    this->then_block->dump(stream, depth + 1);
  }
};

// if <condition> {
//   <then_block>
// } else {
//   <else_block>
// }
struct IfElse : public AbstractNode {
  AbstractNode* condition;
  AbstractNode* then_block;
  AbstractNode* else_block;

  IfElse(AbstractNode* c, AbstractNode* t, AbstractNode* e) : condition(c), then_block(t), else_block(e) {
  }

  inline ~IfElse() {
    delete condition;
    delete then_block;
    delete else_block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- IfElse:" << this << '\n';
    this->condition->dump(stream, depth + 1);
    this->then_block->dump(stream, depth + 1);
    this->else_block->dump(stream, depth + 1);
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
  AbstractNode* then_block;
  AbstractNode* else_block;

  Unless(AbstractNode* c, AbstractNode* t, AbstractNode* e) : condition(c), then_block(t), else_block(e) {
  }

  inline ~Unless() {
    delete condition;
    delete then_block;
    delete else_block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Unless:" << this << '\n';
    this->condition->dump(stream, depth + 1);
    this->then_block->dump(stream, depth + 1);
    this->else_block->dump(stream, depth + 1);
  }
};

// guard <condition> {
//   <block>
// }
struct Guard : public AbstractNode {
  AbstractNode* condition;
  AbstractNode* block;

  Guard(AbstractNode* c, AbstractNode* b) : condition(c), block(b) {
  }

  inline ~Guard() {
    delete condition;
    delete block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Guard:" << this << '\n';
    this->condition->dump(stream, depth + 1);
    this->block->dump(stream, depth + 1);
  }
};

// while <condition> {
//   <block>
// }
struct While : public AbstractNode {
  AbstractNode* condition;
  AbstractNode* block;

  While(AbstractNode* c, AbstractNode* b) : condition(c), block(b) {
  }

  inline ~While() {
    delete condition;
    delete block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- While:" << this << '\n';
    this->condition->dump(stream, depth + 1);
    this->block->dump(stream, depth + 1);
  }
};

// until <condition> {
//   <block>
// }
struct Until : public AbstractNode {
  AbstractNode* condition;
  AbstractNode* block;

  Until(AbstractNode* c, AbstractNode* b) : condition(c), block(b) {
  }

  inline ~Until() {
    delete condition;
    delete block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Until:" << this << '\n';
    this->condition->dump(stream, depth + 1);
    this->block->dump(stream, depth + 1);
  }
};

// loop {
//   <block>
// }
struct Loop : public AbstractNode {
  AbstractNode* block;

  Loop(AbstractNode* b) : block(b) {
  }

  inline ~Loop() {
    delete block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Loop:" << this << '\n';
    this->block->dump(stream, depth + 1);
  }
};

// <operator_type> <expression>
struct Unary : public AbstractNode {
  TokenType operator_type;
  AbstractNode* expression;

  Unary(TokenType op, AbstractNode* e) : operator_type(op), expression(e) {
  }

  inline ~Unary() {
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Unary: " << kTokenTypeStrings[this->operator_type] << '\n';
    this->expression->dump(stream, depth + 1);
  }
};

// <left> <operator_type> <right>
struct Binary : public AbstractNode {
  TokenType operator_type;
  AbstractNode* left;
  AbstractNode* right;

  Binary(TokenType op, AbstractNode* l, AbstractNode* r) : operator_type(op), left(l), right(r) {
  }

  inline ~Binary() {
    delete left;
    delete right;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Binary: " << kTokenTypeStrings[this->operator_type] << '\n';
    this->left->dump(stream, depth + 1);
    this->right->dump(stream, depth + 1);
  }
};

// case <conditions> {
//   <block>
// }
struct SwitchNode : public AbstractNode {
  NodeList* conditions;
  AbstractNode* block;

  SwitchNode(NodeList* c, AbstractNode* b) : conditions(c), block(b) {
  }

  inline ~SwitchNode() {
    delete conditions;
    delete block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- SwitchNode: " << '\n';
    this->conditions->dump(stream, depth + 1);
    this->block->dump(stream, depth + 1);
  }
};

// switch <condition> {
//   <cases>
//   default <default_block>
// }
struct Switch : public AbstractNode {
  AbstractNode* condition;
  NodeList* cases;
  AbstractNode* default_block;

  Switch(AbstractNode* co, NodeList* c, AbstractNode* d) : condition(co), cases(c), default_block(d) {
  }

  inline ~Switch() {
    delete condition;
    delete cases;
    delete default_block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Switch: " << '\n';
    this->condition->dump(stream, depth + 1);
    this->cases->dump(stream, depth + 1);
    this->default_block->dump(stream, depth + 1);
  }
};

// <left> && <right>
struct And : public AbstractNode {
  AbstractNode* left;
  AbstractNode* right;

  And(AbstractNode* l, AbstractNode* r) : left(l), right(r) {
  }

  inline ~And() {
    delete left;
    delete right;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- And: " << '\n';
    this->left->dump(stream, depth + 1);
    this->right->dump(stream, depth + 1);
  }
};

// <left> || <right>
struct Or : public AbstractNode {
  AbstractNode* left;
  AbstractNode* right;

  Or(AbstractNode* l, AbstractNode* r) : left(l), right(r) {
  }

  inline ~Or() {
    delete left;
    delete right;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Or: " << '\n';
    this->left->dump(stream, depth + 1);
    this->right->dump(stream, depth + 1);
  }
};

// typeof <expression>
struct Typeof : public AbstractNode {
  AbstractNode* expression;

  Typeof(AbstractNode* e) : expression(e) {
  }

  inline ~Typeof() {
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Typeof:" << this << '\n';
    this->expression->dump(stream, depth + 1);
  }
};

// <target> = <expression>
struct Assignment : public AbstractNode {
  AbstractNode* target;
  AbstractNode* expression;

  Assignment(AbstractNode* t, AbstractNode* e) : target(t), expression(e) {
  }

  inline ~Assignment() {
    delete target;
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Assignment: " << '\n';
    this->target->dump(stream, depth + 1);
    this->expression->dump(stream, depth + 1);
  }
};

// <target>(<arguments>)
struct Call : public AbstractNode {
  AbstractNode* target;
  NodeList* arguments;

  Call(AbstractNode* t, NodeList* a) : target(t), arguments(a) {
  }

  inline ~Call() {
    delete target;
    delete arguments;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Call:" << this << '\n';
    this->target->dump(stream, depth + 1);
    this->arguments->dump(stream, depth + 1);
  }
};

// <context>.<target>(<arguments>)
struct CallMember : public AbstractNode {
  AbstractNode* context;
  std::string symbol;
  NodeList* arguments;

  CallMember(AbstractNode* c, const std::string& s, NodeList* a) : context(c), symbol(s), arguments(a) {
  }

  inline ~CallMember() {
    delete context;
    delete arguments;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- CallMember:" << this << ' ' << this->symbol << '\n';
    this->context->dump(stream, depth + 1);
    this->arguments->dump(stream, depth + 1);
  }
};

// <context>[<index>](<arguments>)
struct CallIndex : public AbstractNode {
  AbstractNode* context;
  AbstractNode* index;
  NodeList* arguments;

  CallIndex(AbstractNode* c, AbstractNode* i, NodeList* a) : context(c), index(i), arguments(a) {
  }

  inline ~CallIndex() {
    delete context;
    delete index;
    delete arguments;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- CallIndex:" << this << '\n';
    this->context->dump(stream, depth + 1);
    this->index->dump(stream, depth + 1);
    this->arguments->dump(stream, depth + 1);
  }
};

// <name>
struct Identifier : public AbstractNode {
  std::string name;
  IRVarOffsetInfo* offset_info;

  Identifier(const std::string& str) : name(str) {
  }

  inline ~Identifier() {
    if (offset_info != nullptr)
      delete offset_info;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Identifier:" << this;
    stream << ' ' << this->name << '\n';
  }
};

// <target>.<symbol>
struct Member : public AbstractNode {
  AbstractNode* target;
  std::string symbol;

  Member(AbstractNode* t, const std::string& s) : target(t), symbol(s) {
  }

  inline ~Member() {
    delete target;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Member:" << this << ' ' << this->symbol << '\n';
    this->target->dump(stream, depth + 1);
  }
};

// <target>[<argument>]
struct Index : public AbstractNode {
  AbstractNode* target;
  AbstractNode* argument;

  Index(AbstractNode* t, AbstractNode* a) : target(t), argument(a) {
  }

  inline ~Index() {
    delete target;
    delete argument;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Index:" << this << '\n';
    this->target->dump(stream, depth + 1);
    this->argument->dump(stream, depth + 1);
  }
};

// null
struct Null : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Null: " << this << '\n';
  }
};

// NAN
struct Nan : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- NAN: " << this << '\n';
  }
};

// "<value>"
//
// value is optional because we don't want to allocate any memory for an empty string
struct String : public AbstractNode {
  std::optional<std::string> value;

  String(const std::string& str) : value(str) {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- String:" << this;
    stream << ' ' << this->value.value_or("") << '\n';
  }
};

// <value>
struct Integer : public AbstractNode {
  int64_t value;

  Integer(int64_t v) : value(v) {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Integer:" << this;
    stream << ' ' << this->value << '\n';
  }
};

// <value>
struct Float : public AbstractNode {
  double value;

  Float(double v) : value(v) {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Float:" << this;
    stream << ' ' << this->value << '\n';
  }
};

// <value>
struct Boolean : public AbstractNode {
  bool value;

  Boolean(bool v) : value(v) {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Boolean:" << this;
    stream << ' ' << (this->value ? "true" : "false") << '\n';
  }
};

// [<expressions>]
struct Array : public AbstractNode {
  NodeList* expressions;

  Array(std::initializer_list<AbstractNode*> e)
      : expressions(new NodeList(std::forward<std::initializer_list<AbstractNode*>>(e))) {
  }
  Array(NodeList* e) : expressions(e) {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Array:" << this << '\n';
    this->expressions->dump(stream, depth + 1);
  }
};

// {
//   <pairs>
// }
struct Hash : public AbstractNode {
  std::vector<std::pair<AbstractNode*, AbstractNode*>> pairs;

  Hash(std::initializer_list<std::pair<AbstractNode*, AbstractNode*>> p) : pairs(p) {
  }

  inline ~Hash() {
    for (auto& pair : this->pairs) {
      delete pair.first;
      delete pair.second;
    }
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Hash:" << this;
    for (auto& note : this->pairs) {
      note.first->dump(stream, depth + 1);
      note.second->dump(stream, depth + 1);
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
  std::string name;
  NodeList* parameters;
  AbstractNode* body;
  bool anonymous;

  IRLVarInfo* lvar_info;

  Function(const std::string& n, NodeList* p, AbstractNode* b, bool a) : name(n), parameters(p), body(b), anonymous(a) {
  }

  inline ~Function() {
    delete parameters;
    delete body;
    if (lvar_info != nullptr)
      delete lvar_info;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Function:" << this;
    stream << ' ' << this->name;
    stream << ' ' << (this->anonymous ? "anonymous" : "") << '\n';
    this->parameters->dump(stream, depth + 1);
    this->body->dump(stream, depth + 1);
  }
};

// property <symbol>;
struct PropertyDeclaration : public AbstractNode {
  std::string symbol;

  PropertyDeclaration(const std::string& s) : symbol(s) {
  }

  inline ~PropertyDeclaration() {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- PropertyDeclaration:" << this << ' ' << this->symbol << '\n';
  }
};

// static <declaration>
struct StaticDeclaration : public AbstractNode {
  AbstractNode* declaration;

  StaticDeclaration(AbstractNode* d) : declaration(d) {
  }

  inline ~StaticDeclaration() {
    delete declaration;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- StaticDeclaration:" << this << '\n';
    this->declaration->dump(stream, depth + 1);
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
  std::string name;
  NodeList* body;
  NodeList* parents;

  Class(const std::string& n, NodeList* b, NodeList* p) : name(n), body(b), parents(p) {
  }

  inline ~Class() {
    delete body;
    delete parents;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Class:" << this << ' ' << this->name << '\n';
    this->body->dump(stream, depth + 1);
    this->parents->dump(stream, depth + 1);
  }
};

// let <name>
//
// let <name> = <expression>
//
// const <name> = <expression>
struct LocalInitialisation : public AbstractNode {
  std::string name;
  AbstractNode* expression;
  bool constant;

  IRVarOffsetInfo* offset_info;

  LocalInitialisation(const std::string& n, AbstractNode* e, bool c) : name(n), expression(e), constant(c) {
  }

  inline ~LocalInitialisation() {
    delete expression;
    if (offset_info != nullptr)
      delete offset_info;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- LocalInitialisation:" << this;
    stream << ' ' << this->name;
    stream << ' ' << (this->constant ? "constant" : "") << '\n';
    this->expression->dump(stream, depth + 1);
  }
};

// return
//
// return <expression>
struct Return : public AbstractNode {
  AbstractNode* expression;

  Return(AbstractNode* e) : expression(e) {
  }

  inline ~Return() {
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Return:" << this << '\n';
    this->expression->dump(stream, depth + 1);
  }
};

// throw <expression>
struct Throw : public AbstractNode {
  AbstractNode* expression;

  Throw(AbstractNode* e) : expression(e) {
  }

  inline ~Throw() {
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Throw:" << this << '\n';
    this->expression->dump(stream, depth + 1);
  }
};

// break
struct Break : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Break: " << this << '\n';
  }
};

// continue
struct Continue : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Continue: " << this << '\n';
  }
};

// try {
//   <block>
// } catch (<exception_name>) {
//   <handler_block>
// } finally {
//   <finally_block>
// }
struct TryCatch : public AbstractNode {
  AbstractNode* block;
  std::string exception_name;
  AbstractNode* handler_block;
  AbstractNode* finally_block;

  TryCatch(AbstractNode* b, const std::string& e, AbstractNode* h, AbstractNode* f)
      : block(b), exception_name(e), handler_block(h), finally_block(f) {
  }

  inline ~TryCatch() {
    delete block;
    delete handler_block;
    delete finally_block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- TryCatch:" << this << ' ' << this->exception_name << '\n';
    this->block->dump(stream, depth + 1);
    this->handler_block->dump(stream, depth + 1);
    this->finally_block->dump(stream, depth + 1);
  }
};

// Precomputed typeid hashes for all AST nodes
static const size_t kTypeEmpty = typeid(Empty).hash_code();
static const size_t kTypeNodeList = typeid(NodeList).hash_code();
static const size_t kTypeBlock = typeid(Block).hash_code();
static const size_t kTypeIf = typeid(If).hash_code();
static const size_t kTypeIfElse = typeid(IfElse).hash_code();
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
static const size_t kTypeCallMember = typeid(CallMember).hash_code();
static const size_t kTypeCallIndex = typeid(CallIndex).hash_code();
static const size_t kTypeIdentifier = typeid(Identifier).hash_code();
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
