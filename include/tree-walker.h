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
  virtual inline AST::AbstractNode* visit_indexintoarguments(AST::IndexIntoArguments* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_self(AST::Self* node, VisitContinue cont) {
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
      return walker->visit_empty(AST::cast<AST::Empty>(node), cont);
    if (node->type() == AST::kTypeNodeList)
      return walker->visit_nodelist(AST::cast<AST::NodeList>(node), cont);
    if (node->type() == AST::kTypeBlock)
      return walker->visit_block(AST::cast<AST::Block>(node), cont);
    if (node->type() == AST::kTypeIf)
      return walker->visit_if(AST::cast<AST::If>(node), cont);
    if (node->type() == AST::kTypeIfElse)
      return walker->visit_ifelse(AST::cast<AST::IfElse>(node), cont);
    if (node->type() == AST::kTypeUnless)
      return walker->visit_unless(AST::cast<AST::Unless>(node), cont);
    if (node->type() == AST::kTypeUnlessElse)
      return walker->visit_unlesselse(AST::cast<AST::UnlessElse>(node), cont);
    if (node->type() == AST::kTypeGuard)
      return walker->visit_guard(AST::cast<AST::Guard>(node), cont);
    if (node->type() == AST::kTypeWhile)
      return walker->visit_while(AST::cast<AST::While>(node), cont);
    if (node->type() == AST::kTypeUntil)
      return walker->visit_until(AST::cast<AST::Until>(node), cont);
    if (node->type() == AST::kTypeLoop)
      return walker->visit_loop(AST::cast<AST::Loop>(node), cont);
    if (node->type() == AST::kTypeUnary)
      return walker->visit_unary(AST::cast<AST::Unary>(node), cont);
    if (node->type() == AST::kTypeBinary)
      return walker->visit_binary(AST::cast<AST::Binary>(node), cont);
    if (node->type() == AST::kTypeSwitchNode)
      return walker->visit_switchnode(AST::cast<AST::SwitchNode>(node), cont);
    if (node->type() == AST::kTypeSwitch)
      return walker->visit_switch(AST::cast<AST::Switch>(node), cont);
    if (node->type() == AST::kTypeAnd)
      return walker->visit_and(AST::cast<AST::And>(node), cont);
    if (node->type() == AST::kTypeOr)
      return walker->visit_or(AST::cast<AST::Or>(node), cont);
    if (node->type() == AST::kTypeTypeof)
      return walker->visit_typeof(AST::cast<AST::Typeof>(node), cont);
    if (node->type() == AST::kTypeAssignment)
      return walker->visit_assignment(AST::cast<AST::Assignment>(node), cont);
    if (node->type() == AST::kTypeMemberAssignment)
      return walker->visit_memberassignment(AST::cast<AST::MemberAssignment>(node), cont);
    if (node->type() == AST::kTypeIndexAssignment)
      return walker->visit_indexassignment(AST::cast<AST::IndexAssignment>(node), cont);
    if (node->type() == AST::kTypeCall)
      return walker->visit_call(AST::cast<AST::Call>(node), cont);
    if (node->type() == AST::kTypeCallMember)
      return walker->visit_callmember(AST::cast<AST::CallMember>(node), cont);
    if (node->type() == AST::kTypeCallIndex)
      return walker->visit_callindex(AST::cast<AST::CallIndex>(node), cont);
    if (node->type() == AST::kTypeIdentifier)
      return walker->visit_identifier(AST::cast<AST::Identifier>(node), cont);
    if (node->type() == AST::kTypeIndexIntoArguments)
      return walker->visit_indexintoarguments(AST::cast<AST::IndexIntoArguments>(node), cont);
    if (node->type() == AST::kTypeSelf)
      return walker->visit_self(AST::cast<AST::Self>(node), cont);
    if (node->type() == AST::kTypeMember)
      return walker->visit_member(AST::cast<AST::Member>(node), cont);
    if (node->type() == AST::kTypeIndex)
      return walker->visit_index(AST::cast<AST::Index>(node), cont);
    if (node->type() == AST::kTypeNull)
      return walker->visit_null(AST::cast<AST::Null>(node), cont);
    if (node->type() == AST::kTypeNan)
      return walker->visit_nan(AST::cast<AST::Nan>(node), cont);
    if (node->type() == AST::kTypeString)
      return walker->visit_string(AST::cast<AST::String>(node), cont);
    if (node->type() == AST::kTypeInteger)
      return walker->visit_integer(AST::cast<AST::Integer>(node), cont);
    if (node->type() == AST::kTypeFloat)
      return walker->visit_float(AST::cast<AST::Float>(node), cont);
    if (node->type() == AST::kTypeBoolean)
      return walker->visit_boolean(AST::cast<AST::Boolean>(node), cont);
    if (node->type() == AST::kTypeArray)
      return walker->visit_array(AST::cast<AST::Array>(node), cont);
    if (node->type() == AST::kTypeHash)
      return walker->visit_hash(AST::cast<AST::Hash>(node), cont);
    if (node->type() == AST::kTypeFunction)
      return walker->visit_function(AST::cast<AST::Function>(node), cont);
    if (node->type() == AST::kTypePropertyDeclaration)
      return walker->visit_propertydeclaration(AST::cast<AST::PropertyDeclaration>(node), cont);
    if (node->type() == AST::kTypeClass)
      return walker->visit_class(AST::cast<AST::Class>(node), cont);
    if (node->type() == AST::kTypeLocalInitialisation)
      return walker->visit_localinitialisation(AST::cast<AST::LocalInitialisation>(node), cont);
    if (node->type() == AST::kTypeReturn)
      return walker->visit_return(AST::cast<AST::Return>(node), cont);
    if (node->type() == AST::kTypeThrow)
      return walker->visit_throw(AST::cast<AST::Throw>(node), cont);
    if (node->type() == AST::kTypeBreak)
      return walker->visit_break(AST::cast<AST::Break>(node), cont);
    if (node->type() == AST::kTypeContinue)
      return walker->visit_continue(AST::cast<AST::Continue>(node), cont);
    if (node->type() == AST::kTypeTryCatch)
      return walker->visit_trycatch(AST::cast<AST::TryCatch>(node), cont);
    return walker->visit_abstract(node, cont);
  }
};
};  // namespace Charly::Compilation
