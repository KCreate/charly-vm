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

#include <functional>

#include "ast.h"

#pragma once

namespace Charly::Compilation {

typedef std::function<void()> VisitContinue;

class TreeWalker {
public:
  inline AST::AbstractNode* visit_node(AST::AbstractNode* node) {
    return TreeWalker::redirect_to_correct_method(
        this, node, [&]() { node->visit([&](AST::AbstractNode* child) { return this->visit_node(child); }); });
  }

  // Subclasses override this method with different types as their argument
  virtual inline AST::AbstractNode* visit_abstract(AST::AbstractNode* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_empty(AST::Empty* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_nodelist(AST::NodeList* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_block(AST::Block* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_if(AST::If* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_ifelse(AST::IfElse* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_unless(AST::Unless* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_unlesselse(AST::UnlessElse* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_guard(AST::Guard* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_while(AST::While* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_until(AST::Until* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_loop(AST::Loop* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_unary(AST::Unary* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_binary(AST::Binary* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_switchnode(AST::SwitchNode* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_switch(AST::Switch* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_and(AST::And* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_or(AST::Or* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_typeof(AST::Typeof* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_assignment(AST::Assignment* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_memberassignment(AST::MemberAssignment* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_indexassignment(AST::IndexAssignment* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_call(AST::Call* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_callmember(AST::CallMember* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_callindex(AST::CallIndex* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_identifier(AST::Identifier* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_member(AST::Member* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_index(AST::Index* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_null(AST::Null* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_nan(AST::Nan* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_string(AST::String* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_integer(AST::Integer* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_float(AST::Float* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_boolean(AST::Boolean* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_array(AST::Array* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_hash(AST::Hash* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_function(AST::Function* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_propertydeclaration(AST::PropertyDeclaration* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_class(AST::Class* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_localinitialisation(AST::LocalInitialisation* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_return(AST::Return* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_throw(AST::Throw* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_break(AST::Break* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_continue(AST::Continue* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_trycatch(AST::TryCatch* node, VisitContinue cont) {
    cont();
    return node;
  }

  // Redirects to the correct handler methods
  static inline AST::AbstractNode* redirect_to_correct_method(TreeWalker* walker,
                                                              AST::AbstractNode* node,
                                                              VisitContinue cont) {
    if (node->type() == AST::kTypeEmpty)
      return walker->visit_empty(reinterpret_cast<AST::Empty*>(node), cont);
    if (node->type() == AST::kTypeNodeList)
      return walker->visit_nodelist(reinterpret_cast<AST::NodeList*>(node), cont);
    if (node->type() == AST::kTypeBlock)
      return walker->visit_block(reinterpret_cast<AST::Block*>(node), cont);
    if (node->type() == AST::kTypeIf)
      return walker->visit_if(reinterpret_cast<AST::If*>(node), cont);
    if (node->type() == AST::kTypeIfElse)
      return walker->visit_ifelse(reinterpret_cast<AST::IfElse*>(node), cont);
    if (node->type() == AST::kTypeUnless)
      return walker->visit_unless(reinterpret_cast<AST::Unless*>(node), cont);
    if (node->type() == AST::kTypeUnlessElse)
      return walker->visit_unlesselse(reinterpret_cast<AST::UnlessElse*>(node), cont);
    if (node->type() == AST::kTypeGuard)
      return walker->visit_guard(reinterpret_cast<AST::Guard*>(node), cont);
    if (node->type() == AST::kTypeWhile)
      return walker->visit_while(reinterpret_cast<AST::While*>(node), cont);
    if (node->type() == AST::kTypeUntil)
      return walker->visit_until(reinterpret_cast<AST::Until*>(node), cont);
    if (node->type() == AST::kTypeLoop)
      return walker->visit_loop(reinterpret_cast<AST::Loop*>(node), cont);
    if (node->type() == AST::kTypeUnary)
      return walker->visit_unary(reinterpret_cast<AST::Unary*>(node), cont);
    if (node->type() == AST::kTypeBinary)
      return walker->visit_binary(reinterpret_cast<AST::Binary*>(node), cont);
    if (node->type() == AST::kTypeSwitchNode)
      return walker->visit_switchnode(reinterpret_cast<AST::SwitchNode*>(node), cont);
    if (node->type() == AST::kTypeSwitch)
      return walker->visit_switch(reinterpret_cast<AST::Switch*>(node), cont);
    if (node->type() == AST::kTypeAnd)
      return walker->visit_and(reinterpret_cast<AST::And*>(node), cont);
    if (node->type() == AST::kTypeOr)
      return walker->visit_or(reinterpret_cast<AST::Or*>(node), cont);
    if (node->type() == AST::kTypeTypeof)
      return walker->visit_typeof(reinterpret_cast<AST::Typeof*>(node), cont);
    if (node->type() == AST::kTypeAssignment)
      return walker->visit_assignment(reinterpret_cast<AST::Assignment*>(node), cont);
    if (node->type() == AST::kTypeMemberAssignment)
      return walker->visit_memberassignment(reinterpret_cast<AST::MemberAssignment*>(node), cont);
    if (node->type() == AST::kTypeIndexAssignment)
      return walker->visit_indexassignment(reinterpret_cast<AST::IndexAssignment*>(node), cont);
    if (node->type() == AST::kTypeCall)
      return walker->visit_call(reinterpret_cast<AST::Call*>(node), cont);
    if (node->type() == AST::kTypeCallMember)
      return walker->visit_callmember(reinterpret_cast<AST::CallMember*>(node), cont);
    if (node->type() == AST::kTypeCallIndex)
      return walker->visit_callindex(reinterpret_cast<AST::CallIndex*>(node), cont);
    if (node->type() == AST::kTypeIdentifier)
      return walker->visit_identifier(reinterpret_cast<AST::Identifier*>(node), cont);
    if (node->type() == AST::kTypeMember)
      return walker->visit_member(reinterpret_cast<AST::Member*>(node), cont);
    if (node->type() == AST::kTypeIndex)
      return walker->visit_index(reinterpret_cast<AST::Index*>(node), cont);
    if (node->type() == AST::kTypeNull)
      return walker->visit_null(reinterpret_cast<AST::Null*>(node), cont);
    if (node->type() == AST::kTypeNan)
      return walker->visit_nan(reinterpret_cast<AST::Nan*>(node), cont);
    if (node->type() == AST::kTypeString)
      return walker->visit_string(reinterpret_cast<AST::String*>(node), cont);
    if (node->type() == AST::kTypeInteger)
      return walker->visit_integer(reinterpret_cast<AST::Integer*>(node), cont);
    if (node->type() == AST::kTypeFloat)
      return walker->visit_float(reinterpret_cast<AST::Float*>(node), cont);
    if (node->type() == AST::kTypeBoolean)
      return walker->visit_boolean(reinterpret_cast<AST::Boolean*>(node), cont);
    if (node->type() == AST::kTypeArray)
      return walker->visit_array(reinterpret_cast<AST::Array*>(node), cont);
    if (node->type() == AST::kTypeHash)
      return walker->visit_hash(reinterpret_cast<AST::Hash*>(node), cont);
    if (node->type() == AST::kTypeFunction)
      return walker->visit_function(reinterpret_cast<AST::Function*>(node), cont);
    if (node->type() == AST::kTypePropertyDeclaration)
      return walker->visit_propertydeclaration(reinterpret_cast<AST::PropertyDeclaration*>(node), cont);
    if (node->type() == AST::kTypeClass)
      return walker->visit_class(reinterpret_cast<AST::Class*>(node), cont);
    if (node->type() == AST::kTypeLocalInitialisation)
      return walker->visit_localinitialisation(reinterpret_cast<AST::LocalInitialisation*>(node), cont);
    if (node->type() == AST::kTypeReturn)
      return walker->visit_return(reinterpret_cast<AST::Return*>(node), cont);
    if (node->type() == AST::kTypeThrow)
      return walker->visit_throw(reinterpret_cast<AST::Throw*>(node), cont);
    if (node->type() == AST::kTypeBreak)
      return walker->visit_break(reinterpret_cast<AST::Break*>(node), cont);
    if (node->type() == AST::kTypeContinue)
      return walker->visit_continue(reinterpret_cast<AST::Continue*>(node), cont);
    if (node->type() == AST::kTypeTryCatch)
      return walker->visit_trycatch(reinterpret_cast<AST::TryCatch*>(node), cont);
    return walker->visit_abstract(node, cont);
  }
};
};  // namespace Charly::Compilation
