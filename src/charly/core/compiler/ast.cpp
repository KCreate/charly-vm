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

#include <algorithm>

#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/astpass.h"

namespace charly::core::compiler::ast {

template <typename T>
ref<T> Node::visit(ASTPass* pass, const ref<T>& node) {
#define SWITCH_NODE(ASTType)                          \
  if (ref<ASTType> casted_node = cast<ASTType>(node)) \
    return cast<T>(pass->visit(casted_node));

  SWITCH_NODE(Program)

  SWITCH_NODE(Block)
  SWITCH_NODE(Return)
  SWITCH_NODE(Break)
  SWITCH_NODE(Continue)
  SWITCH_NODE(Defer)
  SWITCH_NODE(Throw)
  SWITCH_NODE(Export)

  SWITCH_NODE(Yield)
  SWITCH_NODE(Spawn)
  SWITCH_NODE(Import)
  SWITCH_NODE(Await)
  SWITCH_NODE(Typeof)
  SWITCH_NODE(As)

  SWITCH_NODE(Assignment)
  SWITCH_NODE(ANDAssignment)
  SWITCH_NODE(Ternary)
  SWITCH_NODE(BinaryOp)
  SWITCH_NODE(UnaryOp)
  SWITCH_NODE(CallOp)
  SWITCH_NODE(MemberOp)
  SWITCH_NODE(IndexOp)

  SWITCH_NODE(Id)
  SWITCH_NODE(Int)
  SWITCH_NODE(Float)
  SWITCH_NODE(Bool)
  SWITCH_NODE(Char)
  SWITCH_NODE(String)
  SWITCH_NODE(FormatString)
  SWITCH_NODE(Null)
  SWITCH_NODE(Self)
  SWITCH_NODE(Super)
  SWITCH_NODE(Tuple)
#undef SWITCH_NODE

  assert(false && "Unknown node type");
  return nullptr;
}

template ref<Node> Node::visit(ASTPass*, const ref<Node>&);
template ref<Statement> Node::visit(ASTPass*, const ref<Statement>&);
template ref<Expression> Node::visit(ASTPass*, const ref<Expression>&);

void Block::visit_children(ASTPass* pass) {
  for (ref<Statement>& node : this->statements) {
    node = cast<Statement>(pass->visit(node));
  }

  auto begin = this->statements.begin();
  auto end = this->statements.end();
  this->statements.erase(std::remove(begin, end, nullptr), end);
}

void Return::visit_children(ASTPass* pass) {
  this->expression = pass->visit(this->expression);
}

void Defer::visit_children(ASTPass* pass) {
  this->statement = pass->visit(this->statement);
}

void Throw::visit_children(ASTPass* pass) {
  this->expression = pass->visit(this->expression);
}

void Export::visit_children(ASTPass* pass) {
  this->expression = pass->visit(this->expression);
}

void Yield::visit_children(ASTPass* pass) {
  this->expression = pass->visit(this->expression);
}

void Spawn::visit_children(ASTPass* pass) {
  this->call = pass->visit(this->call);
}

void Import::visit_children(ASTPass* pass) {
  this->source = pass->visit(this->source);

  for (ref<Expression>& node : this->declarations) {
    node = cast<Expression>(pass->visit(node));
  }

  auto begin = this->declarations.begin();
  auto end = this->declarations.end();
  this->declarations.erase(std::remove(begin, end, nullptr), end);
}

void Await::visit_children(ASTPass* pass) {
  this->expression = pass->visit(this->expression);
}

void Typeof::visit_children(ASTPass* pass) {
  this->expression = pass->visit(this->expression);
}

void As::visit_children(ASTPass* pass) {
  this->expression = pass->visit(this->expression);
}

void Program::visit_children(ASTPass* pass) {
  this->body = pass->visit(this->body);
}

void Assignment::visit_children(ASTPass* pass) {
  this->target = pass->visit(this->target);
  this->source = pass->visit(this->source);
}

void ANDAssignment::visit_children(ASTPass* pass) {
  this->target = pass->visit(this->target);
  this->source = pass->visit(this->source);
}

void Ternary::visit_children(ASTPass* pass) {
  this->condition = pass->visit(this->condition);
  this->then_exp = pass->visit(this->then_exp);
  this->else_exp = pass->visit(this->else_exp);
}

void BinaryOp::visit_children(ASTPass* pass) {
  this->lhs = pass->visit(this->lhs);
  this->rhs = pass->visit(this->rhs);
}

void UnaryOp::visit_children(ASTPass* pass) {
  this->expression = pass->visit(this->expression);
}

void CallOp::visit_children(ASTPass* pass) {
  this->target = pass->visit(this->target);

  for (ref<Expression>& node : this->arguments) {
    node = cast<Expression>(pass->visit(node));
  }

  auto begin = this->arguments.begin();
  auto end = this->arguments.end();
  this->arguments.erase(std::remove(begin, end, nullptr), end);
}

void MemberOp::visit_children(ASTPass* pass) {
  this->target = pass->visit(this->target);
}

void IndexOp::visit_children(ASTPass* pass) {
  this->target = pass->visit(this->target);
  this->index = pass->visit(this->index);
}

void FormatString::visit_children(ASTPass* pass) {
  for (ref<Expression>& node : this->elements) {
    node = cast<Expression>(pass->visit(node));
  }

  auto begin = this->elements.begin();
  auto end = this->elements.end();
  this->elements.erase(std::remove(begin, end, nullptr), end);
}

void Tuple::visit_children(ASTPass* pass) {
  for (ref<Expression>& node : this->elements) {
    node = cast<Expression>(pass->visit(node));
  }

  auto begin = this->elements.begin();
  auto end = this->elements.end();
  this->elements.erase(std::remove(begin, end, nullptr), end);
}

}  // namespace charly::core::compiler::ast
