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

#include "ast.h"

#pragma once

namespace Charly::Compiler {
class TreeWalker {
public:

  inline AST::AbstractNode* visit_node(AST::AbstractNode* node) {
    node->visit([&](AST::AbstractNode* child) {
      return this->visit_node(child);
    });

    return TreeWalker::redirect_to_correct_method(this, node);
  }

  // Subclasses override this method with different types as their argument
  virtual AST::AbstractNode* visit_abstract(AST::AbstractNode* node) { return node; }
  virtual AST::AbstractNode* visit_empty(AST::Empty* node) { return node; }
  virtual AST::AbstractNode* visit_nodelist(AST::NodeList* node) { return node; }
  virtual AST::AbstractNode* visit_block(AST::Block* node) { return node; }
  virtual AST::AbstractNode* visit_if(AST::If* node) { return node; }
  virtual AST::AbstractNode* visit_ifelse(AST::IfElse* node) { return node; }
  virtual AST::AbstractNode* visit_unless(AST::Unless* node) { return node; }
  virtual AST::AbstractNode* visit_unlesselse(AST::UnlessElse* node) { return node; }
  virtual AST::AbstractNode* visit_guard(AST::Guard* node) { return node; }
  virtual AST::AbstractNode* visit_while(AST::While* node) { return node; }
  virtual AST::AbstractNode* visit_until(AST::Until* node) { return node; }
  virtual AST::AbstractNode* visit_loop(AST::Loop* node) { return node; }
  virtual AST::AbstractNode* visit_unary(AST::Unary* node) { return node; }
  virtual AST::AbstractNode* visit_binary(AST::Binary* node) { return node; }
  virtual AST::AbstractNode* visit_switchnode(AST::SwitchNode* node) { return node; }
  virtual AST::AbstractNode* visit_switch(AST::Switch* node) { return node; }
  virtual AST::AbstractNode* visit_and(AST::And* node) { return node; }
  virtual AST::AbstractNode* visit_or(AST::Or* node) { return node; }
  virtual AST::AbstractNode* visit_typeof(AST::Typeof* node) { return node; }
  virtual AST::AbstractNode* visit_assignment(AST::Assignment* node) { return node; }
  virtual AST::AbstractNode* visit_memberassignment(AST::MemberAssignment* node) { return node; }
  virtual AST::AbstractNode* visit_indexassignment(AST::IndexAssignment* node) { return node; }
  virtual AST::AbstractNode* visit_call(AST::Call* node) { return node; }
  virtual AST::AbstractNode* visit_callmember(AST::CallMember* node) { return node; }
  virtual AST::AbstractNode* visit_callindex(AST::CallIndex* node) { return node; }
  virtual AST::AbstractNode* visit_identifier(AST::Identifier* node) { return node; }
  virtual AST::AbstractNode* visit_member(AST::Member* node) { return node; }
  virtual AST::AbstractNode* visit_index(AST::Index* node) { return node; }
  virtual AST::AbstractNode* visit_null(AST::Null* node) { return node; }
  virtual AST::AbstractNode* visit_nan(AST::Nan* node) { return node; }
  virtual AST::AbstractNode* visit_string(AST::String* node) { return node; }
  virtual AST::AbstractNode* visit_integer(AST::Integer* node) { return node; }
  virtual AST::AbstractNode* visit_float(AST::Float* node) { return node; }
  virtual AST::AbstractNode* visit_boolean(AST::Boolean* node) { return node; }
  virtual AST::AbstractNode* visit_array(AST::Array* node) { return node; }
  virtual AST::AbstractNode* visit_hash(AST::Hash* node) { return node; }
  virtual AST::AbstractNode* visit_function(AST::Function* node) { return node; }
  virtual AST::AbstractNode* visit_propertydeclaration(AST::PropertyDeclaration* node) { return node; }
  virtual AST::AbstractNode* visit_class(AST::Class* node) { return node; }
  virtual AST::AbstractNode* visit_localinitialisation(AST::LocalInitialisation* node) { return node; }
  virtual AST::AbstractNode* visit_return(AST::Return* node) { return node; }
  virtual AST::AbstractNode* visit_throw(AST::Throw* node) { return node; }
  virtual AST::AbstractNode* visit_break(AST::Break* node) { return node; }
  virtual AST::AbstractNode* visit_continue(AST::Continue* node) { return node; }
  virtual AST::AbstractNode* visit_trycatch(AST::TryCatch* node) { return node; }

  // Redirects to the correct handler methods
  static inline AST::AbstractNode* redirect_to_correct_method(TreeWalker* walker, AST::AbstractNode* node) {
    if (node->type() == AST::kTypeEmpty) return walker->visit_empty(reinterpret_cast<AST::Empty*>(node));
    if (node->type() == AST::kTypeNodeList) return walker->visit_nodelist(reinterpret_cast<AST::NodeList*>(node));
    if (node->type() == AST::kTypeBlock) return walker->visit_block(reinterpret_cast<AST::Block*>(node));
    if (node->type() == AST::kTypeIf) return walker->visit_if(reinterpret_cast<AST::If*>(node));
    if (node->type() == AST::kTypeIfElse) return walker->visit_ifelse(reinterpret_cast<AST::IfElse*>(node));
    if (node->type() == AST::kTypeUnless) return walker->visit_unless(reinterpret_cast<AST::Unless*>(node));
    if (node->type() == AST::kTypeUnlessElse) return walker->visit_unlesselse(reinterpret_cast<AST::UnlessElse*>(node));
    if (node->type() == AST::kTypeGuard) return walker->visit_guard(reinterpret_cast<AST::Guard*>(node));
    if (node->type() == AST::kTypeWhile) return walker->visit_while(reinterpret_cast<AST::While*>(node));
    if (node->type() == AST::kTypeUntil) return walker->visit_until(reinterpret_cast<AST::Until*>(node));
    if (node->type() == AST::kTypeLoop) return walker->visit_loop(reinterpret_cast<AST::Loop*>(node));
    if (node->type() == AST::kTypeUnary) return walker->visit_unary(reinterpret_cast<AST::Unary*>(node));
    if (node->type() == AST::kTypeBinary) return walker->visit_binary(reinterpret_cast<AST::Binary*>(node));
    if (node->type() == AST::kTypeSwitchNode) return walker->visit_switchnode(reinterpret_cast<AST::SwitchNode*>(node));
    if (node->type() == AST::kTypeSwitch) return walker->visit_switch(reinterpret_cast<AST::Switch*>(node));
    if (node->type() == AST::kTypeAnd) return walker->visit_and(reinterpret_cast<AST::And*>(node));
    if (node->type() == AST::kTypeOr) return walker->visit_or(reinterpret_cast<AST::Or*>(node));
    if (node->type() == AST::kTypeTypeof) return walker->visit_typeof(reinterpret_cast<AST::Typeof*>(node));
    if (node->type() == AST::kTypeAssignment) return walker->visit_assignment(reinterpret_cast<AST::Assignment*>(node));
    if (node->type() == AST::kTypeMemberAssignment) return walker->visit_memberassignment(reinterpret_cast<AST::MemberAssignment*>(node));
    if (node->type() == AST::kTypeIndexAssignment) return walker->visit_indexassignment(reinterpret_cast<AST::IndexAssignment*>(node));
    if (node->type() == AST::kTypeCall) return walker->visit_call(reinterpret_cast<AST::Call*>(node));
    if (node->type() == AST::kTypeCallMember) return walker->visit_callmember(reinterpret_cast<AST::CallMember*>(node));
    if (node->type() == AST::kTypeCallIndex) return walker->visit_callindex(reinterpret_cast<AST::CallIndex*>(node));
    if (node->type() == AST::kTypeIdentifier) return walker->visit_identifier(reinterpret_cast<AST::Identifier*>(node));
    if (node->type() == AST::kTypeMember) return walker->visit_member(reinterpret_cast<AST::Member*>(node));
    if (node->type() == AST::kTypeIndex) return walker->visit_index(reinterpret_cast<AST::Index*>(node));
    if (node->type() == AST::kTypeNull) return walker->visit_null(reinterpret_cast<AST::Null*>(node));
    if (node->type() == AST::kTypeNan) return walker->visit_nan(reinterpret_cast<AST::Nan*>(node));
    if (node->type() == AST::kTypeString) return walker->visit_string(reinterpret_cast<AST::String*>(node));
    if (node->type() == AST::kTypeInteger) return walker->visit_integer(reinterpret_cast<AST::Integer*>(node));
    if (node->type() == AST::kTypeFloat) return walker->visit_float(reinterpret_cast<AST::Float*>(node));
    if (node->type() == AST::kTypeBoolean) return walker->visit_boolean(reinterpret_cast<AST::Boolean*>(node));
    if (node->type() == AST::kTypeArray) return walker->visit_array(reinterpret_cast<AST::Array*>(node));
    if (node->type() == AST::kTypeHash) return walker->visit_hash(reinterpret_cast<AST::Hash*>(node));
    if (node->type() == AST::kTypeFunction) return walker->visit_function(reinterpret_cast<AST::Function*>(node));
    if (node->type() == AST::kTypePropertyDeclaration) return walker->visit_propertydeclaration(reinterpret_cast<AST::PropertyDeclaration*>(node));
    if (node->type() == AST::kTypeClass) return walker->visit_class(reinterpret_cast<AST::Class*>(node));
    if (node->type() == AST::kTypeLocalInitialisation) return walker->visit_localinitialisation(reinterpret_cast<AST::LocalInitialisation*>(node));
    if (node->type() == AST::kTypeReturn) return walker->visit_return(reinterpret_cast<AST::Return*>(node));
    if (node->type() == AST::kTypeThrow) return walker->visit_throw(reinterpret_cast<AST::Throw*>(node));
    if (node->type() == AST::kTypeBreak) return walker->visit_break(reinterpret_cast<AST::Break*>(node));
    if (node->type() == AST::kTypeContinue) return walker->visit_continue(reinterpret_cast<AST::Continue*>(node));
    if (node->type() == AST::kTypeTryCatch) return walker->visit_trycatch(reinterpret_cast<AST::TryCatch*>(node));
    return walker->visit_abstract(node);
  }
};
};
