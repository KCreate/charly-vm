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
#include <functional>
#include <iostream>
#include <list>
#include <optional>
#include <vector>

#include "irinfo.h"
#include "location.h"
#include "token.h"

#pragma once

namespace Charly::Compilation::AST {
static const std::string kPaddingCharacters = "  ";

struct AbstractNode;
typedef std::function<AbstractNode*(AbstractNode*)> VisitFunc;

template <class T>
static inline T* cast(AbstractNode* node) {
  return reinterpret_cast<T*>(node);
}

// Abstract base class of all ASTNodes
struct AbstractNode {
public:
  std::optional<Location> location_start;
  std::optional<Location> location_end;

  virtual ~AbstractNode() = default;

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

  virtual void visit(VisitFunc func) {
    (void)func;
  };

  virtual bool is_literal() {
    return false;
  }

  virtual bool yields_value() {
    return false;
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
  NodeList(std::initializer_list<AbstractNode*> list) : children(list) {
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

  void visit(VisitFunc func) {
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

  inline void prepend_node(AbstractNode* node) {
    this->statements.push_front(node);
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Block:" << '\n';
    for (auto& node : this->statements)
      node->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
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

  void visit(VisitFunc func) {
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

  void visit(VisitFunc func) {
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

  void visit(VisitFunc func) {
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

  void visit(VisitFunc func) {
    this->condition = func(this->condition);
    this->then_block = func(this->then_block);
    this->else_block = func(this->else_block);
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
    stream << std::string(depth, ' ') << "- Guard:" << '\n';
    this->condition->dump(stream, depth + 1);
    this->block->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
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

  void visit(VisitFunc func) {
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

  void visit(VisitFunc func) {
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

  void visit(VisitFunc func) {
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

  void visit(VisitFunc func) {
    this->expression = func(this->expression);
  }

  bool yields_value() {
    return true;
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

  void visit(VisitFunc func) {
    this->left = func(this->left);
    this->right = func(this->right);
  }

  bool yields_value() {
    return true;
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
    stream << std::string(depth, ' ') << "- SwitchNode: " << this << '\n';
    this->conditions->dump(stream, depth + 1);
    this->block->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
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
    stream << std::string(depth, ' ') << "- Switch: " << this << '\n';
    this->condition->dump(stream, depth + 1);
    this->cases->dump(stream, depth + 1);
    this->default_block->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
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
    stream << std::string(depth, ' ') << "- And: " << this << '\n';
    this->left->dump(stream, depth + 1);
    this->right->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
    this->left = func(this->left);
    this->right = func(this->right);
  }

  bool yields_value() {
    return true;
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
    stream << std::string(depth, ' ') << "- Or: " << this << '\n';
    this->left->dump(stream, depth + 1);
    this->right->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
    this->left = func(this->left);
    this->right = func(this->right);
  }

  bool yields_value() {
    return true;
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

  void visit(VisitFunc func) {
    this->expression = func(this->expression);
  }

  bool yields_value() {
    return true;
  }
};

// <target> = <expression>
struct Assignment : public AbstractNode {
  std::string target;
  AbstractNode* expression;

  IRVarOffsetInfo* offset_info = nullptr;

  Assignment(const std::string& t, AbstractNode* e) : target(t), expression(e) {
  }

  inline ~Assignment() {
    delete expression;
    if (offset_info != nullptr)
      delete offset_info;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Assignment: " << this << ' ' << this->target;
    if (this->offset_info != nullptr) {
      stream << ' ' << '[' << this->offset_info->level << ", " << this->offset_info->index << ']';
    }
    stream << '\n';

    this->expression->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
    this->expression = func(this->expression);
  }

  bool yields_value() {
    return true;
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
    stream << std::string(depth, ' ') << "- MemberAssignment: " << this << ' ' << this->member << '\n';
    this->target->dump(stream, depth + 1);
    this->expression->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
    this->target = func(this->target);
    this->expression = func(this->expression);
  }

  bool yields_value() {
    return true;
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
    stream << std::string(depth, ' ') << "- ANDMemberAssignment: " << this << ' ' << this->member;
    stream << ' ' << kTokenTypeStrings[this->operator_type] << '\n';
    this->target->dump(stream, depth + 1);
    this->expression->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
    this->target = func(this->target);
    this->expression = func(this->expression);
  }

  bool yields_value() {
    return true;
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
    stream << std::string(depth, ' ') << "- IndexAssignment: " << this << '\n';
    this->target->dump(stream, depth + 1);
    this->index->dump(stream, depth + 1);
    this->expression->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
    this->target = func(this->target);
    this->index = func(this->index);
    this->expression = func(this->expression);
  }

  bool yields_value() {
    return true;
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

  void visit(VisitFunc func) {
    this->target = func(this->target);
    this->index = func(this->index);
    this->expression = func(this->expression);
  }

  bool yields_value() {
    return true;
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

  void visit(VisitFunc func) {
    this->target = func(this->target);
    this->arguments = reinterpret_cast<NodeList*>(func(this->arguments));
  }

  bool yields_value() {
    return true;
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
    stream << std::string(depth, ' ') << "- CallMember:" << ' ' << this->symbol << '\n';
    this->context->dump(stream, depth + 1);
    this->arguments->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
    this->context = func(this->context);
    this->arguments = reinterpret_cast<NodeList*>(func(this->arguments));
  }

  bool yields_value() {
    return true;
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

  void visit(VisitFunc func) {
    this->context = func(this->context);
    this->index = func(this->index);
    this->arguments = reinterpret_cast<NodeList*>(func(this->arguments));
  }

  bool yields_value() {
    return true;
  }
};

// reads a value from the stack
struct StackValue : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- StackValue:" << '\n';
  }

  bool yields_value() {
    return true;
  }
};

// <name>
struct Identifier : public AbstractNode {
  std::string name;
  IRVarOffsetInfo* offset_info = nullptr;

  Identifier(const std::string& str) : name(str) {
  }

  inline ~Identifier() {
    if (offset_info != nullptr)
      delete offset_info;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Identifier:" << ' ' << this->name;
    if (this->offset_info != nullptr) {
      stream << ' ' << '[' << this->offset_info->level << ", " << this->offset_info->index << ']';
    }
    stream << '\n';
  }

  virtual bool is_literal() {
    return true;
  }

  bool yields_value() {
    return true;
  }
};

// $<index>
struct IndexIntoArguments : public AbstractNode {
  uint32_t index;

  IndexIntoArguments(uint32_t i) : index(i) {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- IndexIntoArguments:" << ' ' << this->index << '\n';
  }

  virtual bool is_literal() {
    return true;
  }

  bool yields_value() {
    return true;
  }
};

// self
struct Self : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Self: " << this << '\n';
  }

  virtual bool is_literal() {
    return true;
  }

  bool yields_value() {
    return true;
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
    stream << std::string(depth, ' ') << "- Member:" << ' ' << this->symbol << '\n';
    this->target->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
    this->target = func(this->target);
  }

  bool yields_value() {
    return true;
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

  void visit(VisitFunc func) {
    this->target = func(this->target);
    this->argument = func(this->argument);
  }

  bool yields_value() {
    return true;
  }
};

// null
struct Null : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Null: " << this << '\n';
  }

  virtual bool is_literal() {
    return true;
  }

  bool yields_value() {
    return true;
  }
};

// NAN
struct Nan : public AbstractNode {
  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- NAN: " << this << '\n';
  }

  virtual bool is_literal() {
    return true;
  }

  bool yields_value() {
    return true;
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

  virtual bool is_literal() {
    return true;
  }

  bool yields_value() {
    return true;
  }
};

// <value>
struct Integer : public AbstractNode {
  int64_t value;

  Integer(int64_t v) : value(v) {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Integer:";
    stream << ' ' << this->value << '\n';
  }

  virtual bool is_literal() {
    return true;
  }

  bool yields_value() {
    return true;
  }
};

// <value>
struct Float : public AbstractNode {
  double value;

  Float(double v) : value(v) {
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Float:";
    stream << ' ' << this->value << '\n';
  }

  virtual bool is_literal() {
    return true;
  }

  bool yields_value() {
    return true;
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

  virtual bool is_literal() {
    return true;
  }

  bool yields_value() {
    return true;
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
    stream << std::string(depth, ' ') << "- Array:" << '\n';
    this->expressions->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
    this->expressions = reinterpret_cast<NodeList*>(func(this->expressions));
  }

  bool yields_value() {
    return true;
  }
};

// {
//   <pairs>
// }
struct Hash : public AbstractNode {
  std::vector<std::pair<std::string, AbstractNode*>> pairs;

  Hash() {
  }
  Hash(std::initializer_list<std::pair<std::string, AbstractNode*>> p) : pairs(p) {
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

  void visit(VisitFunc func) {
    for (auto& pair : this->pairs) {
      pair.second = func(pair.second);
    }
  }

  bool yields_value() {
    return true;
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
  AbstractNode* body;
  bool anonymous;

  uint32_t lvar_count = 0;

  Function(const std::string& n, const std::vector<std::string>& p, AbstractNode* b, bool a)
      : name(n), parameters(p), body(b), anonymous(a) {
  }

  inline ~Function() {
    delete body;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Function:";
    if (this->name.size() > 0) {
      stream << ' ' << this->name;
    }
    stream << (this->anonymous ? " anonymous" : "");

    stream << ' ' << '(';
    for (auto& param : this->parameters) {
      stream << param;

      if (this->parameters.back() != param) {
        stream << ", ";
      }
    }
    stream << ')' << " lvar_count=" << this->lvar_count << '\n';

    this->body->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
    this->body = func(this->body);
  }

  bool yields_value() {
    return true;
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
    stream << std::string(depth, ' ') << "- PropertyDeclaration:" << ' ' << this->symbol << '\n';
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
  std::vector<std::string> member_properties;
  std::vector<std::string> static_properties;
  NodeList* member_functions;
  NodeList* static_functions;
  NodeList* parents;

  Class(const std::string& n,
        AbstractNode* c,
        std::vector<std::string> mp,
        NodeList* mf,
        std::vector<std::string> sp,
        NodeList* sf,
        NodeList* p)
      : name(n),
        constructor(c),
        member_properties(mp),
        static_properties(sp),
        member_functions(mf),
        static_functions(sf),
        parents(p) {
  }

  inline ~Class() {
    delete constructor;
    delete member_functions;
    delete static_functions;
    delete parents;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- Class:" << ' ' << this->name;

    stream << ' ' << '(';
    for (auto& param : this->member_properties) {
      stream << param;

      if (this->member_properties.back() != param) {
        stream << ", ";
      }
    }
    stream << ')';
    stream << ' ' << '(';
    for (auto& param : this->static_properties) {
      stream << param;

      if (this->static_properties.back() != param) {
        stream << ", ";
      }
    }
    stream << ')' << '\n';

    this->constructor->dump(stream, depth + 1);
    this->member_functions->dump(stream, depth + 1);
    this->static_functions->dump(stream, depth + 1);
    this->parents->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
    this->constructor = func(this->constructor);
    this->member_functions = reinterpret_cast<NodeList*>(func(this->member_functions));
    this->static_functions = reinterpret_cast<NodeList*>(func(this->static_functions));
    this->parents = reinterpret_cast<NodeList*>(func(this->parents));
  }

  bool yields_value() {
    return true;
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

  IRVarOffsetInfo* offset_info = nullptr;

  LocalInitialisation(const std::string& n, AbstractNode* e, bool c) : name(n), expression(e), constant(c) {
  }

  inline ~LocalInitialisation() {
    delete expression;
    if (offset_info != nullptr)
      delete offset_info;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- LocalInitialisation:";
    stream << ' ' << this->name;
    stream << ' ' << (this->constant ? "constant" : "");
    if (this->offset_info != nullptr) {
      stream << '[' << this->offset_info->level << ", " << this->offset_info->index << ']';
    }
    stream << '\n';
    this->expression->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
    this->expression = func(this->expression);
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

  void visit(VisitFunc func) {
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

  void visit(VisitFunc func) {
    this->expression = func(this->expression);
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

  IRVarOffsetInfo* offset_info = nullptr;

  TryCatch(AbstractNode* b, const std::string& e, AbstractNode* h, AbstractNode* f)
      : block(b), exception_name(e), handler_block(h), finally_block(f) {
  }

  inline ~TryCatch() {
    delete block;
    delete handler_block;
    delete finally_block;
  }

  inline void dump(std::ostream& stream, size_t depth = 0) {
    stream << std::string(depth, ' ') << "- TryCatch:" << ' ' << this->exception_name << '\n';
    this->block->dump(stream, depth + 1);
    this->handler_block->dump(stream, depth + 1);
    this->finally_block->dump(stream, depth + 1);
  }

  void visit(VisitFunc func) {
    this->block = func(this->block);
    this->handler_block = func(this->handler_block);
    this->finally_block = func(this->finally_block);
  }
};

// Precomputed typeid hashes for all AST nodes
static const size_t kTypeEmpty = typeid(Empty).hash_code();
static const size_t kTypeNodeList = typeid(NodeList).hash_code();
static const size_t kTypeBlock = typeid(Block).hash_code();
static const size_t kTypeIf = typeid(If).hash_code();
static const size_t kTypeIfElse = typeid(IfElse).hash_code();
static const size_t kTypeUnless = typeid(Unless).hash_code();
static const size_t kTypeUnlessElse = typeid(UnlessElse).hash_code();
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
static const size_t kTypeMemberAssignment = typeid(MemberAssignment).hash_code();
static const size_t kTypeANDMemberAssignment = typeid(ANDMemberAssignment).hash_code();
static const size_t kTypeIndexAssignment = typeid(IndexAssignment).hash_code();
static const size_t kTypeANDIndexAssignment = typeid(ANDIndexAssignment).hash_code();
static const size_t kTypeCall = typeid(Call).hash_code();
static const size_t kTypeCallMember = typeid(CallMember).hash_code();
static const size_t kTypeCallIndex = typeid(CallIndex).hash_code();
static const size_t kTypeStackValue = typeid(StackValue).hash_code();
static const size_t kTypeIdentifier = typeid(Identifier).hash_code();
static const size_t kTypeIndexIntoArguments = typeid(IndexIntoArguments).hash_code();
static const size_t kTypeSelf = typeid(Self).hash_code();
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
static const size_t kTypeClass = typeid(Class).hash_code();
static const size_t kTypeLocalInitialisation = typeid(LocalInitialisation).hash_code();
static const size_t kTypeReturn = typeid(Return).hash_code();
static const size_t kTypeThrow = typeid(Throw).hash_code();
static const size_t kTypeBreak = typeid(Break).hash_code();
static const size_t kTypeContinue = typeid(Continue).hash_code();
static const size_t kTypeTryCatch = typeid(TryCatch).hash_code();
}  // namespace Charly::Compilation::AST
