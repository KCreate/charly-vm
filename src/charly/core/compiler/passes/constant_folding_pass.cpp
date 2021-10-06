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

#include <list>
#include <cmath>

#include "charly/core/compiler/passes/constant_folding_pass.h"
#include "charly/utils/cast.h"

namespace charly::core::compiler::ast {

ref<Expression> ConstantFoldingPass::transform(const ref<Ternary>& node) {

  // remove dead code
  if (node->condition->is_constant_value()) {
    if (node->condition->truthyness()) {
      return node->then_exp;
    } else {
      return node->else_exp;
    }
  }

  return node;
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
        replacement = make<Float>(lhs->value / rhs->value);
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

  replacement->set_location(node);
  return replacement;
}

ref<Expression> ConstantFoldingPass::transform(const ref<UnaryOp>& node) {
  ref<Expression> replacement = node;

  // int
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
      case TokenType::UnaryNot: {
        replacement = make<Bool>(!expression->truthyness());
        break;
      }
      default: {
        break;
      }
    }
  }

  // float
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
      case TokenType::UnaryNot: {
        replacement = make<Bool>(!expression->truthyness());
        break;
      }
      default: {
        break;
      }
    }
  }

  // bool
  if (ref<Bool> expression = cast<Bool>(node->expression)) {
    switch (node->operation) {
      case TokenType::Plus: {
        replacement = expression;
        break;
      }
      case TokenType::Minus:
      case TokenType::UnaryNot: {
        replacement = make<Bool>(!expression->truthyness());
        break;
      }
      default: {
        break;
      }
    }
  }

  replacement->set_location(node);
  return replacement;
}

ref<Expression> ConstantFoldingPass::transform(const ref<Id>& node) {

  // constant value propagation
  if (ref<Declaration> declaration = cast<Declaration>(node->declaration_node)) {
    if (declaration->constant && declaration->expression->is_constant_value()) {
      return declaration->expression;
    }
  }

  return node;
}

ref<Statement> ConstantFoldingPass::transform(const ref<If>& node) {

  // remove dead code
  if (node->condition->is_constant_value()) {
    if (node->condition->truthyness()) {
      return node->then_block;
    } else {
      return node->else_block;
    }
  }

  if (node->condition->type() == Node::Type::UnaryOp) {
    ref<UnaryOp> op = cast<UnaryOp>(node->condition);

    // if !x {A} else {B}   ->    if x {B} else {A}
    if (op->operation == TokenType::UnaryNot) {
      if (node->else_block) {
        ref<Block> else_block = node->else_block;
        node->else_block = node->then_block;
        node->then_block = else_block;
      }
    }
  }

  return node;
}

ref<Statement> ConstantFoldingPass::transform(const ref<While>& node) {

  // detect infinite loops and remove dead code
  if (node->condition->is_constant_value()) {
    if (node->condition->truthyness()) {
      return make<Loop>(node->then_block);
    } else {
      return nullptr;
    }
  }

  return node;
}

ref<Expression> ConstantFoldingPass::transform(const ref<BuiltinOperation>& node) {
  ref<Expression> replacement = node;

  switch (node->operation) {
    case ir::BuiltinId::caststring: {
      DCHECK(node->arguments.size() == 1);
      const ref<Expression>& expression = node->arguments.at(0);

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

        case Node::Type::Char: {
          utils::Buffer tmpbuf(4);
          tmpbuf.emit_utf8_cp(cast<Char>(expression)->value);
          replacement = make<String>(tmpbuf.buffer_string());
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
      const ref<Expression>& expression = node->arguments.at(0);

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

        case Node::Type::Char: {
          utils::Buffer tmpbuf(4);
          tmpbuf.emit_utf8_cp(cast<Char>(expression)->value);
          replacement = make<Symbol>(tmpbuf.buffer_string());
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
