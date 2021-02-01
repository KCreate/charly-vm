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

  SWITCH_NODE(Nop)
  SWITCH_NODE(Block)
  SWITCH_NODE(Return)
  SWITCH_NODE(Break)
  SWITCH_NODE(Continue)
  SWITCH_NODE(Defer)
  SWITCH_NODE(Throw)
  SWITCH_NODE(Export)
  SWITCH_NODE(Import)

  SWITCH_NODE(ImportExpression)
  SWITCH_NODE(Yield)
  SWITCH_NODE(Spawn)
  SWITCH_NODE(Await)
  SWITCH_NODE(Typeof)
  SWITCH_NODE(As)

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
  SWITCH_NODE(List)
  SWITCH_NODE(DictEntry)
  SWITCH_NODE(Dict)
  SWITCH_NODE(Function)
  SWITCH_NODE(Class)
  SWITCH_NODE(ClassProperty)

  SWITCH_NODE(Assignment)
  SWITCH_NODE(Ternary)
  SWITCH_NODE(BinaryOp)
  SWITCH_NODE(UnaryOp)
  SWITCH_NODE(Spread)
  SWITCH_NODE(CallOp)
  SWITCH_NODE(MemberOp)
  SWITCH_NODE(IndexOp)

  SWITCH_NODE(Declaration)

  SWITCH_NODE(If)
  SWITCH_NODE(While)
  SWITCH_NODE(Try)
  SWITCH_NODE(Switch)
  SWITCH_NODE(SwitchCase)
  SWITCH_NODE(For)

#undef SWITCH_NODE

  assert(false && "Unknown node type");
  return nullptr;
}

template ref<Node> Node::visit(ASTPass*, const ref<Node>&);
template ref<Statement> Node::visit(ASTPass*, const ref<Statement>&);
template ref<Expression> Node::visit(ASTPass*, const ref<Expression>&);

#define VISIT_NODE_VECTOR(T, V)                           \
  {                                                       \
    for (ref<T> & node : this->V) {                       \
      node = cast<T>(pass->visit(node));                  \
    }                                                     \
    auto begin = this->V.begin();                         \
    auto end = this->V.end();                             \
    this->V.erase(std::remove(begin, end, nullptr), end); \
  }

void Block::visit_children(ASTPass* pass) {
  VISIT_NODE_VECTOR(Statement, statements)
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

void Import::visit_children(ASTPass* pass) {
  this->source = pass->visit(this->source);
}

void ImportExpression::visit_children(ASTPass* pass) {
  this->source = pass->visit(this->source);
}

void Yield::visit_children(ASTPass* pass) {
  this->expression = pass->visit(this->expression);
}

void Spawn::visit_children(ASTPass* pass) {
  this->statement = pass->visit(this->statement);
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

void Spread::visit_children(ASTPass* pass) {
  this->expression = pass->visit(this->expression);
}

void CallOp::visit_children(ASTPass* pass) {
  this->target = pass->visit(this->target);
  VISIT_NODE_VECTOR(Expression, arguments)
}

void MemberOp::visit_children(ASTPass* pass) {
  this->target = pass->visit(this->target);
}

void IndexOp::visit_children(ASTPass* pass) {
  this->target = pass->visit(this->target);
  this->index = pass->visit(this->index);
}

void Declaration::visit_children(ASTPass* pass) {
  this->target = pass->visit(this->target);
  this->expression = pass->visit(this->expression);
}

void FormatString::visit_children(ASTPass* pass) {
  VISIT_NODE_VECTOR(Expression, elements)
}

void Tuple::visit_children(ASTPass* pass) {
  VISIT_NODE_VECTOR(Expression, elements)
}

void List::visit_children(ASTPass* pass) {
  VISIT_NODE_VECTOR(Expression, elements)
}

void DictEntry::visit_children(ASTPass* pass) {
  if (this->value) {
    this->key = pass->visit(this->key);
    this->value = pass->visit(this->value);
  } else {
    this->key = pass->visit(this->key);
  }
}

void Dict::visit_children(ASTPass* pass) {
  VISIT_NODE_VECTOR(DictEntry, elements)
}

void Function::visit_children(ASTPass* pass) {
  VISIT_NODE_VECTOR(Expression, arguments)
  this->body = pass->visit(this->body);
}

void Class::visit_children(ASTPass* pass) {
  if (this->parent) {
    this->parent = pass->visit(this->parent);
  }

  if (this->constructor) {
    this->constructor = cast<Function>(pass->visit(this->constructor));
  }

  VISIT_NODE_VECTOR(Function, member_functions)
  VISIT_NODE_VECTOR(ClassProperty, member_properties)
  VISIT_NODE_VECTOR(ClassProperty, static_properties)
}

void ClassProperty::visit_children(ASTPass* pass) {
  if (this->value) {
    this->value = pass->visit(this->value);
  }
}

void If::visit_children(ASTPass* pass) {
  this->condition = pass->visit(this->condition);
  this->then_stmt = pass->visit(this->then_stmt);
  this->else_stmt = pass->visit(this->else_stmt);
}

void While::visit_children(ASTPass* pass) {
  this->condition = pass->visit(this->condition);
  this->then_stmt = pass->visit(this->then_stmt);
}

void Try::visit_children(ASTPass* pass) {
  this->try_stmt = pass->visit(this->try_stmt);
  if (this->catch_stmt)
    this->catch_stmt = pass->visit(this->catch_stmt);
}

void SwitchCase::visit_children(ASTPass* pass) {
  this->test = pass->visit(this->test);
  this->stmt = pass->visit(this->stmt);
}

void Switch::visit_children(ASTPass* pass) {
  this->test = pass->visit(this->test);
  this->default_stmt = pass->visit(this->default_stmt);
  VISIT_NODE_VECTOR(SwitchCase, cases)
}

void For::visit_children(ASTPass* pass) {
  this->target = pass->visit(this->target);
  this->source = pass->visit(this->source);
  this->stmt = pass->visit(this->stmt);
}

#undef VISIT_NODE_VECTOR

}  // namespace charly::core::compiler::ast
