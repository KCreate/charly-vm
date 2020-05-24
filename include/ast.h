/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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
#include <functional>
#include <iostream>
#include <list>
#include <optional>
#include <vector>

#include "lvar-location.h"
#include "location.h"
#include "token.h"

#pragma once

namespace Charly::Compilation::AST {
const std::string kPaddingCharacters = "  ";

struct AbstractNode;
typedef std::function<AbstractNode*(AbstractNode*)> VisitFunc;

// Abstract base class of all ASTNodes
struct AbstractNode {
public:
  std::optional<Location> location_start;
  std::optional<Location> location_end;

  ValueLocation* offset_info = nullptr;
  bool yielded_value_needed = true;

  virtual ~AbstractNode() {
    if (this->offset_info)
      delete this->offset_info;
  }

  inline AbstractNode* at(const Token& loc) {
    return this->at(loc.location);
  }

  inline AbstractNode* at(const Token& start, const Token& end) {
    return this->at(start.location, end.location);
  }

  inline AbstractNode* at(const Location& loc) {
    this->location_start = loc;
    this->location_end = loc;
    return this;
  }

  inline AbstractNode* at(const Location& start, const Location& end) {
    this->location_start = start;
    this->location_end = end;
    return this;
  }

  inline AbstractNode* at(const std::optional<Location>& loc) {
    this->location_start = loc;
    this->location_end = loc;
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

  virtual inline void visit(VisitFunc func) {
    (void)func;
  };

  template <typename T>
  T* as() {
    return reinterpret_cast<T*>(this);
  }
};

// A node represting the absence of another node
struct Empty : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Empty:" << '\n';
  }
};

// A list of AST nodes with no preconceived notion of what context
// they are used in
struct NodeList : public AbstractNode {
  std::list<AbstractNode*> children;

  NodeList() {
  }
  template <typename... Args>
  NodeList(Args... params) : children({params...}) {
  }

  inline void append_node(AbstractNode* node) {
    if (this->children.size() == 0)
      this->at(*node);
    this->children.push_back(node);
    this->location_end = node->location_end;
  }

  inline void prepend_node(AbstractNode* node) {
    if (this->children.size() == 0)
      this->at(*node);
    this->children.push_front(node);
    this->location_start = node->location_start;
  }

  inline ~NodeList() {
    for (auto& node : this->children)
      delete node;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- NodeList:" << '\n';
    for (auto& node : this->children)
      node->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    std::list<AbstractNode*> new_children;

    for (auto& node : this->children) {
      AST::AbstractNode* result = func(node);

      if (result != nullptr) {
        new_children.push_back(result);
      }
    }

    this->children = std::move(new_children);
  }
};

// A list of AST nodes meant to represent a scoped block
//
// {
//   <statements>
// }
struct Block : public AbstractNode {
  std::list<AbstractNode*> statements;
  bool ignore_const = false;

  Block() {
  }
  template <typename... Args>
  Block(Args... params) : statements({params...}) {
  }

  inline ~Block() {
    for (auto& node : this->statements) {
      delete node;
    }
  }

  inline void append_node(AbstractNode* node) {
    this->statements.push_back(node);
  }

  inline void prepend_node(AbstractNode* node) {
    this->statements.push_front(node);
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Block:" << '\n';
    for (auto& node : this->statements)
      node->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    std::list<AbstractNode*> new_statements;

    for (auto& node : this->statements) {
      AST::AbstractNode* result = func(node);

      if (result != nullptr) {
        new_statements.push_back(result);
      }
    }

    this->statements = std::move(new_statements);
  }
};

// Push an expression onto the stack without popping it off
//
// This is mostly used for interaction with machine internals
struct PushStack : public AbstractNode {
  AbstractNode* expression;

  PushStack(AbstractNode* e) : expression(e) {
  }

  inline ~PushStack() {
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- PushStack:" << '\n';
    this->expression->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->expression = func(this->expression);
  }
};

// <condition> ? <then_expression> : <else_expression>
struct TernaryIf : public AbstractNode {
  AbstractNode* condition;
  AbstractNode* then_expression;
  AbstractNode* else_expression;

  TernaryIf(AbstractNode* c, AbstractNode* t, AbstractNode* e) : condition(c), then_expression(t), else_expression(e) {
  }

  inline ~TernaryIf() {
    delete condition;
    delete then_expression;
    delete else_expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- TernaryIf:" << '\n';
    this->condition->dump(stream, depth + 1);
    this->then_expression->dump(stream, depth + 1);
    this->else_expression->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->condition = func(this->condition);
    this->then_expression = func(this->then_expression);
    this->else_expression = func(this->else_expression);
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
    stream << std::string(depth, ' ') << "- If:" << '\n';
    this->condition->dump(stream, depth + 1);
    this->then_block->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->condition = func(this->condition);
    this->then_block = func(this->then_block);
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
    stream << std::string(depth, ' ') << "- IfElse:" << '\n';
    this->condition->dump(stream, depth + 1);
    this->then_block->dump(stream, depth + 1);
    this->else_block->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->condition = func(this->condition);
    this->then_block = func(this->then_block);
    this->else_block = func(this->else_block);
  }
};

// unless <condition> {
//   <then_block>
// }
struct Unless : public AbstractNode {
  AbstractNode* condition;
  AbstractNode* then_block;

  Unless(AbstractNode* c, AbstractNode* t) : condition(c), then_block(t) {
  }

  inline ~Unless() {
    delete condition;
    delete then_block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Unless:" << '\n';
    this->condition->dump(stream, depth + 1);
    this->then_block->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->condition = func(this->condition);
    this->then_block = func(this->then_block);
  }
};

// unless <condition> {
//   <then_block>
// } else {
//   <else_block>
// }
struct UnlessElse : public AbstractNode {
  AbstractNode* condition;
  AbstractNode* then_block;
  AbstractNode* else_block;

  UnlessElse(AbstractNode* c, AbstractNode* t, AbstractNode* e) : condition(c), then_block(t), else_block(e) {
  }

  inline ~UnlessElse() {
    delete condition;
    delete then_block;
    delete else_block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- UnlessElse:" << '\n';
    this->condition->dump(stream, depth + 1);
    this->then_block->dump(stream, depth + 1);
    this->else_block->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->condition = func(this->condition);
    this->then_block = func(this->then_block);
    this->else_block = func(this->else_block);
  }
};

// do {
//   <block>
// } while <condition>
struct DoWhile : public AbstractNode {
  AbstractNode* condition;
  AbstractNode* block;

  DoWhile(AbstractNode* c, AbstractNode* b) : condition(c), block(b) {
  }

  inline ~DoWhile() {
    delete condition;
    delete block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- DoWhile: " << '\n';
    this->condition->dump(stream, depth + 1);
    this->block->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->condition = func(this->condition);
    this->block = func(this->block);
  }
};

// do {
//   <block>
// } until <condition>
struct DoUntil : public AbstractNode {
  AbstractNode* condition;
  AbstractNode* block;

  DoUntil(AbstractNode* c, AbstractNode* b) : condition(c), block(b) {
  }

  inline ~DoUntil() {
    delete condition;
    delete block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- DoUntil: " << '\n';
    this->condition->dump(stream, depth + 1);
    this->block->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->condition = func(this->condition);
    this->block = func(this->block);
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
    stream << std::string(depth, ' ') << "- While:" << '\n';
    this->condition->dump(stream, depth + 1);
    this->block->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->condition = func(this->condition);
    this->block = func(this->block);
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
    stream << std::string(depth, ' ') << "- Until:" << '\n';
    this->condition->dump(stream, depth + 1);
    this->block->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->condition = func(this->condition);
    this->block = func(this->block);
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
    stream << std::string(depth, ' ') << "- Loop:" << '\n';
    this->block->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->block = func(this->block);
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

  inline void visit(VisitFunc func) {
    this->expression = func(this->expression);
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

  inline void visit(VisitFunc func) {
    this->left = func(this->left);
    this->right = func(this->right);
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

  inline void visit(VisitFunc func) {
    this->conditions = reinterpret_cast<NodeList*>(func(this->conditions));
    this->block = func(this->block);
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

  inline void visit(VisitFunc func) {
    this->condition = func(this->condition);
    this->cases = reinterpret_cast<NodeList*>(func(this->cases));
    this->default_block = func(this->default_block);
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

  inline void visit(VisitFunc func) {
    this->left = func(this->left);
    this->right = func(this->right);
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

  inline void visit(VisitFunc func) {
    this->left = func(this->left);
    this->right = func(this->right);
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
    stream << std::string(depth, ' ') << "- Typeof:" << '\n';
    this->expression->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->expression = func(this->expression);
  }
};

// new klass([arguments])
struct New : public AbstractNode {
  AbstractNode* klass;
  NodeList* arguments;

  New(AbstractNode* k, NodeList* a) : klass(k), arguments(a) {
  }

  inline ~New() {
    delete klass;
    delete arguments;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- New:" << '\n';
    this->klass->dump(stream, depth + 1);
    this->arguments->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->klass = func(this->klass);
    this->arguments = reinterpret_cast<NodeList*>(func(this->arguments));
  }
};

// <target> = <expression>
struct Assignment : public AbstractNode {
  std::string target;
  AbstractNode* expression;
  bool no_codegen = false;

  Assignment(const std::string& t, AbstractNode* e) : target(t), expression(e) {
  }

  inline ~Assignment() {
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Assignment: " << this->target;
    stream << '\n';

    this->expression->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->expression = func(this->expression);
  }
};

// <target>.<member> = <expression>
struct MemberAssignment : public AbstractNode {
  AbstractNode* target;
  std::string member;
  AbstractNode* expression;

  MemberAssignment(AbstractNode* t, const std::string& m, AbstractNode* e) : target(t), member(m), expression(e) {
  }

  inline ~MemberAssignment() {
    delete target;
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- MemberAssignment: " << this->member << '\n';
    this->target->dump(stream, depth + 1);
    this->expression->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->target = func(this->target);
    this->expression = func(this->expression);
  }
};

// <target>.<member> <operator>= <expression>
struct ANDMemberAssignment : public AbstractNode {
  AbstractNode* target;
  std::string member;
  TokenType operator_type;
  AbstractNode* expression;

  ANDMemberAssignment(AbstractNode* t, const std::string& m, TokenType o, AbstractNode* e)
      : target(t), member(m), operator_type(o), expression(e) {
  }

  inline ~ANDMemberAssignment() {
    delete target;
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- ANDMemberAssignment: " << this->member;
    stream << ' ' << kTokenTypeStrings[this->operator_type] << '\n';
    this->target->dump(stream, depth + 1);
    this->expression->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->target = func(this->target);
    this->expression = func(this->expression);
  }
};

// <target>[<index>] = <expression>
struct IndexAssignment : public AbstractNode {
  AbstractNode* target;
  AbstractNode* index;
  AbstractNode* expression;

  IndexAssignment(AbstractNode* t, AbstractNode* i, AbstractNode* e) : target(t), index(i), expression(e) {
  }

  inline ~IndexAssignment() {
    delete target;
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- IndexAssignment: " << '\n';
    this->target->dump(stream, depth + 1);
    this->index->dump(stream, depth + 1);
    this->expression->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->target = func(this->target);
    this->index = func(this->index);
    this->expression = func(this->expression);
  }
};

// <target>[<index>] <operator>= <expression>
struct ANDIndexAssignment : public AbstractNode {
  AbstractNode* target;
  AbstractNode* index;
  TokenType operator_type;
  AbstractNode* expression;

  ANDIndexAssignment(AbstractNode* t, AbstractNode* i, TokenType o, AbstractNode* e)
      : target(t), index(i), operator_type(o), expression(e) {
  }

  inline ~ANDIndexAssignment() {
    delete target;
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- ANDIndexAssignment: " << this;
    stream << ' ' << kTokenTypeStrings[this->operator_type] << '\n';
    this->target->dump(stream, depth + 1);
    this->index->dump(stream, depth + 1);
    this->expression->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->target = func(this->target);
    this->index = func(this->index);
    this->expression = func(this->expression);
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
    stream << std::string(depth, ' ') << "- Call:" << '\n';
    this->target->dump(stream, depth + 1);
    this->arguments->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->target = func(this->target);
    this->arguments = reinterpret_cast<NodeList*>(func(this->arguments));
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
    stream << std::string(depth, ' ') << "- CallMember: " << this->symbol << '\n';
    this->context->dump(stream, depth + 1);
    this->arguments->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->context = func(this->context);
    this->arguments = reinterpret_cast<NodeList*>(func(this->arguments));
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
    stream << std::string(depth, ' ') << "- CallIndex:" << '\n';
    this->context->dump(stream, depth + 1);
    this->index->dump(stream, depth + 1);
    this->arguments->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->context = func(this->context);
    this->index = func(this->index);
    this->arguments = reinterpret_cast<NodeList*>(func(this->arguments));
  }
};

// reads a value from the stack
struct StackValue : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- StackValue:" << '\n';
  }
};

// <name>
struct Identifier : public AbstractNode {
  std::string name;

  Identifier(const std::string& str) : name(str) {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Identifier: " << this->name;
    stream << '\n';
  }
};

// self
struct Self : public AbstractNode {
  uint32_t ir_frame_level = 0;

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Self: ir_frame_level=" << this->ir_frame_level << '\n';
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
    stream << std::string(depth, ' ') << "- Member: " << this->symbol << '\n';
    this->target->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->target = func(this->target);
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
    stream << std::string(depth, ' ') << "- Index:" << '\n';
    this->target->dump(stream, depth + 1);
    this->argument->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->target = func(this->target);
    this->argument = func(this->argument);
  }
};

// null
struct Null : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Null: " << '\n';
  }
};

// NAN
struct Nan : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- NAN: " << '\n';
  }
};

// "<value>"
struct String : public AbstractNode {
  std::string value;

  String(const std::string& str) : value(str) {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- String:";
    stream << ' ' << this->value << '\n';
  }
};

// <value>
//
// TODO: Have two separate nodes for double and integer numbers as doubles can't store integers precisely
// which are bigger than 2^24
struct Number : public AbstractNode {
  double value;

  Number(double v) : value(v) {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Number:";
    stream << ' ' << this->value << '\n';
  }
};

// <value>
struct Boolean : public AbstractNode {
  bool value;

  Boolean(bool v) : value(v) {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Boolean:";
    stream << ' ' << (this->value ? "true" : "false") << '\n';
  }
};

// [<expressions>]
struct Array : public AbstractNode {
  NodeList* expressions;

  template <typename... Args>
  Array(Args... params) : expressions(params...) {
  }
  Array(NodeList* e) : expressions(e) {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Array:" << '\n';
    this->expressions->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->expressions = reinterpret_cast<NodeList*>(func(this->expressions));
  }
};

// {
//   <pairs>
// }
struct Hash : public AbstractNode {
  std::vector<std::pair<std::string, AbstractNode*>> pairs;

  Hash() {
  }

  inline void append_pair(const std::pair<std::string, AbstractNode*>& p) {
    this->pairs.push_back(p);
  }

  inline void append_pair(const std::string& k, AbstractNode* v) {
    this->pairs.push_back({k, v});
  }

  inline ~Hash() {
    for (auto& pair : this->pairs) {
      delete pair.second;
    }
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Hash:" << '\n';
    for (auto& note : this->pairs) {
      stream << std::string(depth + 1, ' ') << "- " << note.first << ':' << '\n';
      note.second->dump(stream, depth + 1);
    }
  }

  inline void visit(VisitFunc func) {
    for (auto& pair : this->pairs) {
      pair.second = func(pair.second);
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
// func (a, b) = a + b
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
  std::vector<std::string> parameters;
  std::vector<std::string> self_initialisations;
  std::unordered_map<std::string, AbstractNode*> default_values;
  AbstractNode* body;
  bool anonymous;
  bool generator = false;
  bool needs_arguments = false;

  uint32_t lvarcount = 0;
  uint32_t required_arguments;

  Function(const std::string& n,
           const std::vector<std::string>& p,
           const std::vector<std::string>& s,
           AbstractNode* b,
           bool a)
      : name(n), parameters(p), self_initialisations(s), body(b), anonymous(a), required_arguments(p.size()) {
  }
  Function(const std::string& n,
           std::vector<std::string>&& p,
           const std::vector<std::string>& s,
           AbstractNode* b,
           bool a)
      : name(n),
        parameters(std::move(p)),
        self_initialisations(std::move(s)),
        body(b),
        anonymous(a),
        required_arguments(p.size()) {
  }

  inline ~Function() {
    delete body;

    for (auto& entry : this->default_values) {
      delete entry.second;
    }

    this->default_values.clear();
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Function:";
    if (this->name.size() > 0) {
      stream << ' ' << this->name;
    }
    stream << (this->anonymous ? " anonymous" : "");
    stream << (this->generator ? " generator" : "");
    stream << (this->needs_arguments ? " needs_arguments" : "");

    stream << ' ' << '(';

    int param_count = this->parameters.size();
    int i = 0;
    for (auto& param : this->parameters) {
      stream << param;

      if (i != param_count - 1) {
        stream << ", ";
      }

      i++;
    }
    stream << ')' << " lvarcount=" << this->lvarcount << ' ';
    stream << " minimum_argc=" << this->required_arguments;
    stream << '\n';

    for (auto& entry : this->default_values) {
      stream << std::string(depth + 1, ' ') << "- " << entry.first << ": ";
      entry.second->dump(stream, depth + 2);
    }

    this->body->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->body = func(this->body);
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
    stream << std::string(depth, ' ') << "- PropertyDeclaration: " << this->symbol << '\n';
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
  AbstractNode* constructor;
  NodeList* member_properties;
  NodeList* member_functions;
  NodeList* static_properties;
  NodeList* static_functions;
  AbstractNode* parent_class;

  Class(const std::string& n, AbstractNode* c, NodeList* mp, NodeList* mf, NodeList* sp, NodeList* sf, AbstractNode* p)
      : name(n),
        constructor(c),
        member_properties(mp),
        member_functions(mf),
        static_properties(sp),
        static_functions(sf),
        parent_class(p) {
  }

  inline ~Class() {
    delete constructor;
    delete member_properties;
    delete member_functions;
    delete static_properties;
    delete static_functions;
    delete parent_class;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Class: " << this->name << '\n';
    this->constructor->dump(stream, depth + 1);
    this->member_properties->dump(stream, depth + 1);
    this->member_functions->dump(stream, depth + 1);
    this->static_properties->dump(stream, depth + 1);
    this->static_functions->dump(stream, depth + 1);
    this->parent_class->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {

    // Visit calls to member_properties and static_properties have been removed in a previous
    // commit. I forgot why i did this but i assume i had a good reason at the time
    //
    // I added them again but if there is ever anything funny going on with this area
    // of the code then this is probably the reason it broke
    //
    // See commit: cf75a8

    this->constructor = func(this->constructor);
    this->member_functions = reinterpret_cast<NodeList*>(func(this->member_functions));
    this->member_properties = reinterpret_cast<NodeList*>(func(this->member_properties));
    this->static_functions = reinterpret_cast<NodeList*>(func(this->static_functions));
    this->static_properties = reinterpret_cast<NodeList*>(func(this->static_properties));
    this->parent_class = reinterpret_cast<AbstractNode*>(func(this->parent_class));
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

  LocalInitialisation(const std::string& n, AbstractNode* e, bool c) : name(n), expression(e), constant(c) {
  }

  inline ~LocalInitialisation() {
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- LocalInitialisation:";
    stream << ' ' << this->name;
    stream << ' ' << (this->constant ? "constant" : "");
    stream << '\n';
    this->expression->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->expression = func(this->expression);
  }
};

// <condition> => <expression>
struct MatchArm : public AbstractNode {
  AbstractNode* condition;
  AbstractNode* expression;

  MatchArm(AbstractNode* c, AbstractNode* e) : condition(c), expression(e) {
  }

  inline ~MatchArm() {
    delete condition;
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- MatchArm: " << '\n';
    this->condition->dump(stream, depth + 1);
    this->expression->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->condition = func(this->condition);
    this->expression = func(this->expression);
  }

  inline bool yields_value() {
    return this->expression->type() != typeid(Block).hash_code();
  }
};

// match <condition> [-> <condition_ident>] {
//   <arms>
//   <default_arm>
// }
struct Match : public AbstractNode {
  AbstractNode* condition;
  std::optional<std::string> condition_ident;
  NodeList* arms;
  AbstractNode* default_arm;

  Match(AbstractNode* c, const std::optional<std::string>& ci, NodeList* a, AbstractNode* d)
      : condition(c), condition_ident(ci), arms(a), default_arm(d) {
  }

  inline ~Match() {
    delete condition;
    delete arms;
    delete default_arm;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Match: " << this->condition_ident.value_or("<no condition name>") << '\n';
    this->condition->dump(stream, depth + 1);
    this->arms->dump(stream, depth + 1);
    this->default_arm->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->condition = func(this->condition);
    this->arms = reinterpret_cast<NodeList*>(func(this->arms));
    this->default_arm = func(this->default_arm);
  }

  inline bool yields_value() {
    for (auto& node : this->arms->children) {
      if (node->as<AST::MatchArm>()->yields_value())
        return true;
    }
    return false;
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
    stream << std::string(depth, ' ') << "- Return:" << '\n';
    this->expression->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->expression = func(this->expression);
  }
};


// import <name>
struct Import : public AbstractNode {
  AbstractNode* source;

  Import(AbstractNode* s) : source(s) {
  }

  inline ~Import() {
    delete source;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Import: " << '\n';
    this->source->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->source = func(this->source);
  }
};

// yield <expression>
struct Yield : public AbstractNode {
  AbstractNode* expression;

  Yield(AbstractNode* e) : expression(e) {
  }

  inline ~Yield() {
    delete expression;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Yield:" << '\n';
    this->expression->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->expression = func(this->expression);
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
    stream << std::string(depth, ' ') << "- Throw:" << '\n';
    this->expression->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->expression = func(this->expression);
  }
};

// break
struct Break : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Break: " << '\n';
  }
};

// continue
struct Continue : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Continue: " << '\n';
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
  Identifier* exception_name;
  AbstractNode* handler_block;
  AbstractNode* finally_block;

  TryCatch(AbstractNode* b, Identifier* e, AbstractNode* h, AbstractNode* f)
      : block(b), exception_name(e), handler_block(h), finally_block(f) {
  }

  inline ~TryCatch() {
    delete block;
    delete exception_name;
    delete handler_block;
    delete finally_block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- TryCatch: " << '\n';
    this->exception_name->dump(stream, depth + 1);
    this->block->dump(stream, depth + 1);
    this->handler_block->dump(stream, depth + 1);
    this->finally_block->dump(stream, depth + 1);
  }

  inline void visit(VisitFunc func) {
    this->block = func(this->block);
    this->exception_name = func(this->exception_name)->as<Identifier>();
    this->handler_block = func(this->handler_block);
    this->finally_block = func(this->finally_block);
  }
};

// Precomputed typeid hashes for all AST nodes
const size_t kTypeEmpty = typeid(Empty).hash_code();
const size_t kTypeNodeList = typeid(NodeList).hash_code();
const size_t kTypeBlock = typeid(Block).hash_code();
const size_t kTypePushStack = typeid(PushStack).hash_code();
const size_t kTypeTernaryIf = typeid(TernaryIf).hash_code();
const size_t kTypeIf = typeid(If).hash_code();
const size_t kTypeIfElse = typeid(IfElse).hash_code();
const size_t kTypeUnless = typeid(Unless).hash_code();
const size_t kTypeUnlessElse = typeid(UnlessElse).hash_code();
const size_t kTypeDoWhile = typeid(DoWhile).hash_code();
const size_t kTypeDoUntil = typeid(DoUntil).hash_code();
const size_t kTypeWhile = typeid(While).hash_code();
const size_t kTypeUntil = typeid(Until).hash_code();
const size_t kTypeLoop = typeid(Loop).hash_code();
const size_t kTypeUnary = typeid(Unary).hash_code();
const size_t kTypeBinary = typeid(Binary).hash_code();
const size_t kTypeSwitchNode = typeid(SwitchNode).hash_code();
const size_t kTypeSwitch = typeid(Switch).hash_code();
const size_t kTypeAnd = typeid(And).hash_code();
const size_t kTypeOr = typeid(Or).hash_code();
const size_t kTypeTypeof = typeid(Typeof).hash_code();
const size_t kTypeNew = typeid(New).hash_code();
const size_t kTypeAssignment = typeid(Assignment).hash_code();
const size_t kTypeMemberAssignment = typeid(MemberAssignment).hash_code();
const size_t kTypeANDMemberAssignment = typeid(ANDMemberAssignment).hash_code();
const size_t kTypeIndexAssignment = typeid(IndexAssignment).hash_code();
const size_t kTypeANDIndexAssignment = typeid(ANDIndexAssignment).hash_code();
const size_t kTypeCall = typeid(Call).hash_code();
const size_t kTypeCallMember = typeid(CallMember).hash_code();
const size_t kTypeCallIndex = typeid(CallIndex).hash_code();
const size_t kTypeStackValue = typeid(StackValue).hash_code();
const size_t kTypeIdentifier = typeid(Identifier).hash_code();
const size_t kTypeSelf = typeid(Self).hash_code();
const size_t kTypeMember = typeid(Member).hash_code();
const size_t kTypeIndex = typeid(Index).hash_code();
const size_t kTypeNull = typeid(Null).hash_code();
const size_t kTypeNan = typeid(Nan).hash_code();
const size_t kTypeString = typeid(String).hash_code();
const size_t kTypeNumber = typeid(Number).hash_code();
const size_t kTypeBoolean = typeid(Boolean).hash_code();
const size_t kTypeArray = typeid(Array).hash_code();
const size_t kTypeHash = typeid(Hash).hash_code();
const size_t kTypeFunction = typeid(Function).hash_code();
const size_t kTypePropertyDeclaration = typeid(PropertyDeclaration).hash_code();
const size_t kTypeClass = typeid(Class).hash_code();
const size_t kTypeLocalInitialisation = typeid(LocalInitialisation).hash_code();
const size_t kTypeMatch = typeid(Match).hash_code();
const size_t kTypeMatchArm = typeid(MatchArm).hash_code();
const size_t kTypeReturn = typeid(Return).hash_code();
const size_t kTypeYield = typeid(Yield).hash_code();
const size_t kTypeThrow = typeid(Throw).hash_code();
const size_t kTypeBreak = typeid(Break).hash_code();
const size_t kTypeContinue = typeid(Continue).hash_code();
const size_t kTypeTryCatch = typeid(TryCatch).hash_code();
const size_t kTypeImport = typeid(Import).hash_code();

// Casts node to a given type without checking for errors
template <class T>
T* cast(AbstractNode* node) {
  return reinterpret_cast<T*>(node);
}

// Checks wether a given node is a control statement
inline bool is_control_statement(AbstractNode* node) {
  return (node->type() == kTypeReturn || node->type() == kTypeBreak || node->type() == kTypeContinue ||
          node->type() == kTypeThrow || node->type() == kTypeYield);
}

inline bool terminates_block(AbstractNode* node) {
  return (node->type() == kTypeReturn || node->type() == kTypeBreak || node->type() == kTypeContinue ||
          node->type() == kTypeThrow);
}

// Checks wether a given node is a literal that can be safely removed from a block
inline bool is_literal(AbstractNode* node) {
  return (node->type() == kTypeIdentifier || node->type() == kTypeSelf ||
          node->type() == kTypeNull || node->type() == kTypeNan || node->type() == kTypeString ||
          node->type() == kTypeNumber || node->type() == kTypeBoolean || node->type() == kTypeFunction);
}

// Checks wether a given node yields a value
inline bool yields_value(AbstractNode* node) {
  if (node->type() == kTypeMatch)
    return node->as<AST::Match>()->yields_value();
  return node->yielded_value_needed &&
         (node->type() == kTypeTernaryIf || node->type() == kTypeUnary || node->type() == kTypeBinary ||
          node->type() == kTypeAnd || node->type() == kTypeOr || node->type() == kTypeTypeof ||
          node->type() == kTypeNew || node->type() == kTypeAssignment || node->type() == kTypeMemberAssignment ||
          node->type() == kTypeANDMemberAssignment || node->type() == kTypeIndexAssignment ||
          node->type() == kTypeANDIndexAssignment || node->type() == kTypeCall || node->type() == kTypeCallMember ||
          node->type() == kTypeCallIndex || node->type() == kTypeStackValue || node->type() == kTypeIdentifier ||
          node->type() == kTypeSelf || node->type() == kTypeMember || node->type() == kTypeYield ||
          node->type() == kTypeIndex || node->type() == kTypeNull || node->type() == kTypeNan ||
          node->type() == kTypeString || node->type() == kTypeNumber || node->type() == kTypeBoolean ||
          node->type() == kTypeArray || node->type() == kTypeHash || node->type() == kTypeFunction ||
          node->type() == kTypeClass || node->type() == kTypeImport);
}

// Checks wether a given node is an assignment
inline bool is_assignment(AbstractNode* node) {
  return (node->type() == kTypeAssignment || node->type() == kTypeMemberAssignment ||
          node->type() == kTypeANDMemberAssignment || node->type() == kTypeIndexAssignment ||
          node->type() == kTypeANDIndexAssignment);
}

inline bool is_comparison(AbstractNode* node) {
  if (node->type() != kTypeBinary)
    return false;
  Binary* binexp = node->as<Binary>();

  return (binexp->operator_type == TokenType::Equal || binexp->operator_type == TokenType::Not ||
          binexp->operator_type == TokenType::Less || binexp->operator_type == TokenType::Greater ||
          binexp->operator_type == TokenType::LessEqual || binexp->operator_type == TokenType::GreaterEqual);
}
}  // namespace Charly::Compilation::AST
