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
  virtual inline AST::AbstractNode* visit_ternaryif(AST::TernaryIf* node, VisitContinue cont) {
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
  virtual inline AST::AbstractNode* visit_andmemberassignment(AST::ANDMemberAssignment* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_indexassignment(AST::IndexAssignment* node, VisitContinue cont) {
    cont();
    return node;
  }
  virtual inline AST::AbstractNode* visit_andindexassignment(AST::ANDIndexAssignment* node, VisitContinue cont) {
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
  virtual inline AST::AbstractNode* visit_number(AST::Number* node, VisitContinue cont) {
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
      return walker->visit_empty(node->as<AST::Empty>(), cont);
    if (node->type() == AST::kTypeNodeList)
      return walker->visit_nodelist(node->as<AST::NodeList>(), cont);
    if (node->type() == AST::kTypeBlock)
      return walker->visit_block(node->as<AST::Block>(), cont);
    if (node->type() == AST::kTypeTernaryIf)
      return walker->visit_ternaryif(node->as<AST::TernaryIf>(), cont);
    if (node->type() == AST::kTypeIf)
      return walker->visit_if(node->as<AST::If>(), cont);
    if (node->type() == AST::kTypeIfElse)
      return walker->visit_ifelse(node->as<AST::IfElse>(), cont);
    if (node->type() == AST::kTypeUnless)
      return walker->visit_unless(node->as<AST::Unless>(), cont);
    if (node->type() == AST::kTypeUnlessElse)
      return walker->visit_unlesselse(node->as<AST::UnlessElse>(), cont);
    if (node->type() == AST::kTypeGuard)
      return walker->visit_guard(node->as<AST::Guard>(), cont);
    if (node->type() == AST::kTypeWhile)
      return walker->visit_while(node->as<AST::While>(), cont);
    if (node->type() == AST::kTypeUntil)
      return walker->visit_until(node->as<AST::Until>(), cont);
    if (node->type() == AST::kTypeLoop)
      return walker->visit_loop(node->as<AST::Loop>(), cont);
    if (node->type() == AST::kTypeUnary)
      return walker->visit_unary(node->as<AST::Unary>(), cont);
    if (node->type() == AST::kTypeBinary)
      return walker->visit_binary(node->as<AST::Binary>(), cont);
    if (node->type() == AST::kTypeSwitchNode)
      return walker->visit_switchnode(node->as<AST::SwitchNode>(), cont);
    if (node->type() == AST::kTypeSwitch)
      return walker->visit_switch(node->as<AST::Switch>(), cont);
    if (node->type() == AST::kTypeAnd)
      return walker->visit_and(node->as<AST::And>(), cont);
    if (node->type() == AST::kTypeOr)
      return walker->visit_or(node->as<AST::Or>(), cont);
    if (node->type() == AST::kTypeTypeof)
      return walker->visit_typeof(node->as<AST::Typeof>(), cont);
    if (node->type() == AST::kTypeAssignment)
      return walker->visit_assignment(node->as<AST::Assignment>(), cont);
    if (node->type() == AST::kTypeMemberAssignment)
      return walker->visit_memberassignment(node->as<AST::MemberAssignment>(), cont);
    if (node->type() == AST::kTypeANDMemberAssignment)
      return walker->visit_andmemberassignment(node->as<AST::ANDMemberAssignment>(), cont);
    if (node->type() == AST::kTypeIndexAssignment)
      return walker->visit_indexassignment(node->as<AST::IndexAssignment>(), cont);
    if (node->type() == AST::kTypeANDIndexAssignment)
      return walker->visit_andindexassignment(node->as<AST::ANDIndexAssignment>(), cont);
    if (node->type() == AST::kTypeCall)
      return walker->visit_call(node->as<AST::Call>(), cont);
    if (node->type() == AST::kTypeCallMember)
      return walker->visit_callmember(node->as<AST::CallMember>(), cont);
    if (node->type() == AST::kTypeCallIndex)
      return walker->visit_callindex(node->as<AST::CallIndex>(), cont);
    if (node->type() == AST::kTypeIdentifier)
      return walker->visit_identifier(node->as<AST::Identifier>(), cont);
    if (node->type() == AST::kTypeIndexIntoArguments)
      return walker->visit_indexintoarguments(node->as<AST::IndexIntoArguments>(), cont);
    if (node->type() == AST::kTypeSelf)
      return walker->visit_self(node->as<AST::Self>(), cont);
    if (node->type() == AST::kTypeMember)
      return walker->visit_member(node->as<AST::Member>(), cont);
    if (node->type() == AST::kTypeIndex)
      return walker->visit_index(node->as<AST::Index>(), cont);
    if (node->type() == AST::kTypeNull)
      return walker->visit_null(node->as<AST::Null>(), cont);
    if (node->type() == AST::kTypeNan)
      return walker->visit_nan(node->as<AST::Nan>(), cont);
    if (node->type() == AST::kTypeString)
      return walker->visit_string(node->as<AST::String>(), cont);
    if (node->type() == AST::kTypeNumber)
      return walker->visit_number(node->as<AST::Number>(), cont);
    if (node->type() == AST::kTypeBoolean)
      return walker->visit_boolean(node->as<AST::Boolean>(), cont);
    if (node->type() == AST::kTypeArray)
      return walker->visit_array(node->as<AST::Array>(), cont);
    if (node->type() == AST::kTypeHash)
      return walker->visit_hash(node->as<AST::Hash>(), cont);
    if (node->type() == AST::kTypeFunction)
      return walker->visit_function(node->as<AST::Function>(), cont);
    if (node->type() == AST::kTypePropertyDeclaration)
      return walker->visit_propertydeclaration(node->as<AST::PropertyDeclaration>(), cont);
    if (node->type() == AST::kTypeClass)
      return walker->visit_class(node->as<AST::Class>(), cont);
    if (node->type() == AST::kTypeLocalInitialisation)
      return walker->visit_localinitialisation(node->as<AST::LocalInitialisation>(), cont);
    if (node->type() == AST::kTypeReturn)
      return walker->visit_return(node->as<AST::Return>(), cont);
    if (node->type() == AST::kTypeThrow)
      return walker->visit_throw(node->as<AST::Throw>(), cont);
    if (node->type() == AST::kTypeBreak)
      return walker->visit_break(node->as<AST::Break>(), cont);
    if (node->type() == AST::kTypeContinue)
      return walker->visit_continue(node->as<AST::Continue>(), cont);
    if (node->type() == AST::kTypeTryCatch)
      return walker->visit_trycatch(node->as<AST::TryCatch>(), cont);
    return walker->visit_abstract(node, cont);
  }
};
};  // namespace Charly::Compilation
