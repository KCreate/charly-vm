/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

#include <cmath>

#include "charly/core/compiler/passes/constant_folding_pass.h"
#include "charly/utils/cast.h"

namespace charly::core::compiler::ast {

ref<Expression> ConstantFoldingPass::transform(const ref<Ternary>& node) {
  switch (node->condition->truthyness()) {
    case Truthyness::True: {
      if (node->condition->has_side_effects()) {
        auto tmp = make<ExpressionWithSideEffects>(make<Block>(node->condition), node->then_exp);
        tmp->set_location(node->then_exp);
        return tmp;
      } else {
        return node->then_exp;
      }
    }
    case Truthyness::False: {
      if (node->condition->has_side_effects()) {
        auto tmp = make<ExpressionWithSideEffects>(make<Block>(node->condition), node->else_exp);
        tmp->set_location(node->else_exp);
        return tmp;
      } else {
        return node->else_exp;
      }
    }
    default: return node;
  }
}

ref<Expression> ConstantFoldingPass::transform(const ref<BinaryOp>& node) {
  ref<Expression> replacement = node;

  // int <-> int
  if (isa<Int>(node->lhs) && isa<Int>(node->rhs)) {
    ref<Int> lhs = cast<Int>(node->lhs);
    ref<Int> rhs = cast<Int>(node->rhs);

    switch (node->operation) {
      case TokenType::Plus: {
        replacement = make<Int>(lhs->value + rhs->value);
        break;
      }
      case TokenType::Minus: {
        replacement = make<Int>(lhs->value - rhs->value);
        break;
      }
      case TokenType::Mul: {
        replacement = make<Int>(lhs->value * rhs->value);
        break;
      }
      case TokenType::Div: {
        if (rhs->value == 0) {
          if (lhs->value == 0) {
            replacement = make<Float>(NAN);
          } else if (lhs->value < 0) {
            replacement = make<Float>(-INFINITY);
          } else {
            replacement = make<Float>(INFINITY);
          }
        } else {
          replacement = make<Float>(lhs->value / rhs->value);
        }
        break;
      }
      case TokenType::Mod: {
        replacement = make<Int>(lhs->value % rhs->value);
        break;
      }
      case TokenType::Pow: {
        replacement = make<Int>(std::pow(lhs->value, rhs->value));
        break;
      }
      default: {
        break;
      }
    }
  }

  // float <-> float
  if (isa<Float>(node->lhs) && isa<Float>(node->rhs)) {
    ref<Float> lhs = cast<Float>(node->lhs);
    ref<Float> rhs = cast<Float>(node->rhs);

    switch (node->operation) {
      case TokenType::Plus: {
        replacement = make<Float>(lhs->value + rhs->value);
        break;
      }
      case TokenType::Minus: {
        replacement = make<Float>(lhs->value - rhs->value);
        break;
      }
      case TokenType::Mul: {
        replacement = make<Float>(lhs->value * rhs->value);
        break;
      }
      case TokenType::Div: {
        replacement = make<Float>(lhs->value / rhs->value);
        break;
      }
      case TokenType::Mod: {
        replacement = make<Float>(std::fmod(lhs->value, rhs->value));
        break;
      }
      case TokenType::Pow: {
        replacement = make<Float>(std::pow(lhs->value, rhs->value));
        break;
      }
      default: {
        break;
      }
    }
  }

  // int <-> float
  // float <-> int
  // rewrite to float <-> float
  if ((isa<Int>(node->lhs) && isa<Float>(node->rhs)) || (isa<Float>(node->lhs) && isa<Int>(node->rhs))) {
    switch (node->operation) {
      // only perform type conversion for arithmetic operators
      case TokenType::Plus:
      case TokenType::Minus:
      case TokenType::Mul:
      case TokenType::Div:
      case TokenType::Mod:
      case TokenType::Pow: {
        ref<Float> flhs = make<Float>(node->lhs);
        ref<Float> frhs = make<Float>(node->rhs);
        replacement = make<BinaryOp>(node->operation, flhs, frhs);
        break;
      }
      default: {
        break;
      }
    }
  }

  // <exp> && <exp>
  if (node->operation == TokenType::And) {
    auto lhs_truthy = node->lhs->truthyness();
    auto rhs_truthy = node->lhs->truthyness();
    auto lhs_sideeffects = node->lhs->has_side_effects();
    auto rhs_sideeffects = node->rhs->has_side_effects();

    if (lhs_truthy == Truthyness::True) {
      if (lhs_sideeffects) {
        replacement = make<ExpressionWithSideEffects>(make<Block>(node->lhs),
                                                      make<BuiltinOperation>(ir::BuiltinId::castbool, node->rhs));
      } else {
        replacement = make<BuiltinOperation>(ir::BuiltinId::castbool, node->rhs);
      }
    } else if (lhs_truthy == Truthyness::False) {
      if (lhs_sideeffects) {
        replacement = make<ExpressionWithSideEffects>(make<Block>(node->lhs), make<Bool>(false));
      } else {
        replacement = make<Bool>(false);
      }
    } else if (!rhs_sideeffects) {
      if (rhs_truthy == Truthyness::True) {
        replacement = make<BuiltinOperation>(ir::BuiltinId::castbool, node->lhs);
      } else if (rhs_truthy == Truthyness::False) {
        replacement = make<ExpressionWithSideEffects>(make<Block>(node->lhs), make<Bool>(false));
      }
    }
  }

  // <exp> || <exp>
  if (node->operation == TokenType::Or) {
    auto lhs_truthy = node->lhs->truthyness();
    auto lhs_sideeffects = node->lhs->has_side_effects();

    if (lhs_truthy == Truthyness::True) {
      replacement = node->lhs;
    } else if (lhs_truthy == Truthyness::False) {
      if (lhs_sideeffects) {
        replacement = make<ExpressionWithSideEffects>(make<Block>(node->lhs), node->rhs);
      } else {
        replacement = node->rhs;
      }
    }
  }

  if (node->operation == TokenType::Equal || node->operation == TokenType::NotEqual) {
    bool invert_result = node->operation == TokenType::NotEqual;

    Truthyness comparison = node->lhs->compares_equal(node->rhs);
    if (comparison != Truthyness::Unknown) {
      bool result = comparison == Truthyness::True;
      if (invert_result) {
        result = !result;
      }

      if (node->lhs->has_side_effects() && node->rhs->has_side_effects()) {
        replacement = make<ExpressionWithSideEffects>(make<Block>(node->lhs, node->rhs), make<Bool>(result));
      } else if (node->lhs->has_side_effects()) {
        replacement = make<ExpressionWithSideEffects>(make<Block>(node->lhs), make<Bool>(result));
      } else if (node->rhs->has_side_effects()) {
        replacement = make<ExpressionWithSideEffects>(make<Block>(node->rhs), make<Bool>(result));
      } else {
        replacement = make<Bool>(result);
      }
    }
  }

  replacement->set_location(node);
  return replacement;
}

ref<Expression> ConstantFoldingPass::transform(const ref<UnaryOp>& node) {
  ref<Expression> replacement = node;

  if (ref<Int> expression = cast<Int>(node->expression)) {
    switch (node->operation) {
      case TokenType::Plus: {
        replacement = expression;
        break;
      }
      case TokenType::Minus: {
        replacement = make<Int>(-expression->value);
        break;
      }
      default: {
        break;
      }
    }
  }

  if (ref<Float> expression = cast<Float>(node->expression)) {
    switch (node->operation) {
      case TokenType::Plus: {
        replacement = expression;
        break;
      }
      case TokenType::Minus: {
        replacement = make<Float>(-expression->value);
        break;
      }
      default: {
        break;
      }
    }
  }

  if (ref<Bool> expression = cast<Bool>(node->expression)) {
    switch (node->operation) {
      case TokenType::Plus: {
        replacement = expression;
        break;
      }
      case TokenType::Minus: {
        DCHECK(expression->truthyness() != Truthyness::Unknown);
        replacement = make<Bool>(!expression->value);
        break;
      }
      default: {
        break;
      }
    }
  }

  if (node->operation == TokenType::UnaryNot) {
    auto exp_truthy = node->expression->truthyness();
    if (exp_truthy != Truthyness::Unknown) {
      bool truthyness = exp_truthy == Truthyness::True;

      if (node->expression->has_side_effects()) {
        replacement = make<ExpressionWithSideEffects>(make<Block>(node->expression), make<Bool>(!truthyness));
      } else {
        replacement = make<Bool>(!truthyness);
      }
    }
  }

  replacement->set_location(node);
  return replacement;
}

ref<Expression> ConstantFoldingPass::transform(const ref<Id>& node) {
  if (ref<Declaration> declaration = cast<Declaration>(node->declaration_node)) {
    if (declaration->constant && declaration->expression->is_constant_value()) {
      return declaration->expression;
    }
  }

  return node;
}

ref<Statement> ConstantFoldingPass::transform(const ref<If>& node) {
  // if !x {A} else {B}   ->    if x {B} else {A}
  if (ref<UnaryOp> op = cast<UnaryOp>(node->condition)) {
    if (op->operation == TokenType::UnaryNot) {
      if (node->else_block) {
        ref<Block> else_block = node->else_block;
        node->else_block = node->then_block;
        node->then_block = else_block;
        node->condition = op->expression;
      }
    }
  }

  switch (node->condition->truthyness()) {
    case Truthyness::True: {
      if (node->then_block) {
        return make<Block>(node->condition, node->then_block);
      } else {
        return node->condition;
      }
    }
    case Truthyness::False: {
      if (node->else_block) {
        return make<Block>(node->condition, node->else_block);
      } else {
        return node->condition;
      }
    }
    default: return node;
  }
}

ref<Statement> ConstantFoldingPass::transform(const ref<While>& node) {
  switch (node->condition->truthyness()) {
    case Truthyness::True: return make<Loop>(make<Block>(node->condition, node->then_block));
    case Truthyness::False: return make<Block>(node->condition);
    default: return node;
  }
}

ref<Expression> ConstantFoldingPass::transform(const ref<BuiltinOperation>& node) {
  ref<Expression> replacement = node;

  switch (node->operation) {
    case ir::BuiltinId::castbool: {
      DCHECK(node->arguments.size() == 1);
      auto exp = node->arguments.front();
      auto truthyness = exp->truthyness();
      if (truthyness != Truthyness::Unknown) {
        bool truthyness_bool = truthyness == Truthyness::True;
        if (exp->has_side_effects()) {
          replacement = make<ExpressionWithSideEffects>(make<Block>(exp), make<Bool>(truthyness_bool));
        } else {
          replacement = make<Bool>(truthyness_bool);
        }
      }
      break;
    }
    case ir::BuiltinId::caststring: {
      DCHECK(node->arguments.size() == 1);
      const ref<Expression>& expression = node->arguments.front();

      switch (expression->type()) {
        case Node::Type::String: {
          replacement = expression;
          break;
        }

        case Node::Type::Int: {
          replacement = make<String>(std::to_string(cast<Int>(expression)->value));
          break;
        }

        case Node::Type::Float: {
          replacement = make<String>(std::to_string(cast<Float>(expression)->value));
          break;
        }

        case Node::Type::Bool: {
          replacement = make<String>(cast<Bool>(expression)->value ? "true" : "false");
          break;
        }

        case Node::Type::Null: {
          replacement = make<String>("null");
          break;
        }

        default: {
          break;
        }
      }

      break;
    }
    case ir::BuiltinId::castsymbol: {
      DCHECK(node->arguments.size() == 1);
      const ref<Expression>& expression = node->arguments.front();

      switch (expression->type()) {
        case Node::Type::String: {
          replacement = make<Symbol>(cast<String>(expression));
          break;
        }

        case Node::Type::Int: {
          replacement = make<Symbol>(std::to_string(cast<Int>(expression)->value));
          break;
        }

        case Node::Type::Float: {
          replacement = make<Symbol>(std::to_string(cast<Float>(expression)->value));
          break;
        }

        case Node::Type::Bool: {
          replacement = make<Symbol>(cast<Bool>(expression)->value ? "true" : "false");
          break;
        }

        case Node::Type::Null: {
          replacement = make<Symbol>("null");
          break;
        }

        default: {
          break;
        }
      }

      break;
    }
    default: {
      break;
    }
  }

  replacement->set_location(node);
  return replacement;
}

}  // namespace charly::core::compiler::ast
