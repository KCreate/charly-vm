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

#include <list>

#include "normalizer.h"

namespace Charly::Compilation {
AST::AbstractNode* Normalizer::visit_block(AST::Block* node, VisitContinue cont) {
  (void)cont;

  std::list<AST::AbstractNode*> remaining_statements;

  // Remove all unneeded nodes
  bool append_to_block = true;
  for (auto statement : node->statements) {
    AST::AbstractNode* normalized_node = this->visit_node(statement);

    if (append_to_block) {
      // Generate local initialisations for function and class nodes
      if (normalized_node->type() == AST::kTypeFunction || normalized_node->type() == AST::kTypeClass) {
        // Read the name of the node
        std::string name;
        if (normalized_node->type() == AST::kTypeFunction) {
          name = normalized_node->as<AST::Function>()->name;
        }
        if (normalized_node->type() == AST::kTypeClass) {
          name = normalized_node->as<AST::Class>()->name;
        }

        // Wrap the node in a local initialisation if it has a name
        if (name.size() > 0) {
          normalized_node = (new AST::LocalInitialisation(name, normalized_node, true))->at(normalized_node);
        }
      }

      if (!AST::is_literal(normalized_node)) {
        remaining_statements.push_back(normalized_node);

        if (AST::is_control_statement(normalized_node)) {
          append_to_block = false;
        }
      } else {
        delete normalized_node;
      }
    } else {
      delete normalized_node;
    }
  }

  node->statements = remaining_statements;
  return node;
}

AST::AbstractNode* Normalizer::visit_if(AST::If* node, VisitContinue cont) {
  cont();
  node->then_block = this->wrap_in_block(node->then_block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_ifelse(AST::IfElse* node, VisitContinue cont) {
  cont();
  node->then_block = this->wrap_in_block(node->then_block, cont);
  node->else_block = this->wrap_in_block(node->else_block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_unless(AST::Unless* node, VisitContinue cont) {
  cont();
  node->then_block = this->wrap_in_block(node->then_block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_unlesselse(AST::UnlessElse* node, VisitContinue cont) {
  cont();
  node->then_block = this->wrap_in_block(node->then_block, cont);
  node->else_block = this->wrap_in_block(node->else_block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_guard(AST::Guard* node, VisitContinue cont) {
  cont();
  node->block = this->wrap_in_block(node->block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_while(AST::While* node, VisitContinue cont) {
  cont();
  node->block = this->wrap_in_block(node->block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_until(AST::Until* node, VisitContinue cont) {
  cont();
  node->block = this->wrap_in_block(node->block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_loop(AST::Loop* node, VisitContinue cont) {
  cont();
  node->block = this->wrap_in_block(node->block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_binary(AST::Binary* node, VisitContinue cont) {
  cont();

  switch (node->operator_type) {
    case TokenType::Plus: {
      // Concatenate arrays
      if (node->left->type() == AST::kTypeArray && node->right->type() == AST::kTypeArray) {
        AST::Array* cat_array = node->left->as<AST::Array>();
        cat_array->at(node);

        for (auto& right_item : node->right->as<AST::Array>()->expressions->children) {
          cat_array->expressions->append_node(right_item);
        }

        node->left = nullptr;
        node->right->as<AST::Array>()->expressions->children.clear();
        delete node;
        return cat_array;
      }

      if (node->left->type() == AST::kTypeNumber && node->right->type() == AST::kTypeNumber) {
        double left = node->left->as<AST::Number>()->value;
        double right = node->right->as<AST::Number>()->value;
        AST::Number* result = new AST::Number(left + right);
        result->at(node);
        delete node;
        return result;
      }

      if (node->left->type() == AST::kTypeString && node->right->type() == AST::kTypeString) {
        AST::String* cat_string = node->left->as<AST::String>();
        cat_string->at(node);
        cat_string->value.append(node->right->as<AST::String>()->value);

        node->left = nullptr;
        node->right = nullptr;
        delete node;
        return cat_string;
      }

      break;
    }
    case TokenType::Minus: {
      if (node->left->type() == AST::kTypeNumber && node->right->type() == AST::kTypeNumber) {
        double left = node->left->as<AST::Number>()->value;
        double right = node->right->as<AST::Number>()->value;
        AST::Number* result = new AST::Number(left - right);
        result->at(node);
        delete node;
        return result;
      }

      break;
    }
    case TokenType::Mul: {
      if (node->left->type() == AST::kTypeNumber && node->right->type() == AST::kTypeNumber) {
        double left = node->left->as<AST::Number>()->value;
        double right = node->right->as<AST::Number>()->value;
        AST::Number* result = new AST::Number(left * right);
        result->at(node);
        delete node;
        return result;
      }

      if ((node->left->type() == AST::kTypeString && node->right->type() == AST::kTypeNumber) ||
          (node->left->type() == AST::kTypeNumber && node->right->type() == AST::kTypeString)) {
        AST::String* new_string;
        std::string source_string;
        int64_t num;

        if (node->left->type() == AST::kTypeString) {
          new_string = node->left->as<AST::String>();
          new_string->at(node);
          num = node->right->as<AST::Number>()->value;
        } else {
          new_string = node->right->as<AST::String>();
          new_string->at(node);
          num = node->left->as<AST::Number>()->value;
        }
        source_string = new_string->value;

        // If we multiply a string with zero, we just return an empty string
        if (num <= 0) {
          new_string = (new AST::String(""))->at(node)->as<AST::String>();
          delete node;
          return new_string;
        }

        while (num--) {
          new_string->value.append(source_string);
        }

        node->left = nullptr;
        node->right = nullptr;
        delete node;
        return new_string;
      }

      break;
    }
    case TokenType::Div: {
      if (node->left->type() == AST::kTypeNumber && node->right->type() == AST::kTypeNumber) {
        double left = node->left->as<AST::Number>()->value;
        double right = node->right->as<AST::Number>()->value;
        AST::Number* result = new AST::Number(left / right);
        result->at(node);
        delete node;
        return result;
      }

      break;
    }
    case TokenType::Mod: {
      if (node->left->type() == AST::kTypeNumber && node->right->type() == AST::kTypeNumber) {
        int64_t left = node->left->as<AST::Number>()->value;
        int64_t right = node->right->as<AST::Number>()->value;
        AST::Number* result = new AST::Number(left % right);
        result->at(node);
        delete node;
        return result;
      }

      break;
    }
    case TokenType::Pow: {
      if (node->left->type() == AST::kTypeNumber && node->right->type() == AST::kTypeNumber) {
        double left = node->left->as<AST::Number>()->value;
        double right = node->right->as<AST::Number>()->value;
        AST::Number* result = new AST::Number(pow(left, right));
        result->at(node);
        delete node;
        return result;
      }

      break;
    }
    default: {
      // Do nothing
    }
  }

  return node;
}

AST::AbstractNode* Normalizer::visit_unary(AST::Unary* node, VisitContinue cont) {
  cont();

  if (node->expression->type() == AST::kTypeNumber) {
    switch (node->operator_type) {
      case TokenType::UMinus: {
        double num = node->expression->as<AST::Number>()->value;
        AST::Number* num_node = new AST::Number(-num);
        num_node->at(node);
        delete node;
        return num_node;
      }
      case TokenType::BitNOT: {
        int64_t num = node->expression->as<AST::Number>()->value;
        AST::Number* num_node = new AST::Number(~num);
        num_node->at(node);
        delete node;
        return num_node;
      }
      default: {
        return node;
      }
    }
  }

  return node;
}

AST::AbstractNode* Normalizer::visit_switch(AST::Switch* node, VisitContinue cont) {
  cont();

  if (node->default_block->type() != AST::kTypeEmpty) {
    node->default_block = this->wrap_in_block(node->default_block, cont);
  }

  for (auto& _child : node->cases->children) {
    AST::SwitchNode* switch_node = _child->as<AST::SwitchNode>();
    switch_node->block = this->wrap_in_block(switch_node->block, cont);
  }

  return node;
}

AST::AbstractNode* Normalizer::visit_function(AST::Function* node, VisitContinue cont) {
  node->body = this->wrap_in_block(node->body);

  AST::Block* body = node->body->as<AST::Block>();

  // Check if there are any statements inside the functions body
  if (body->statements.size() == 0) {
    body->append_node((new AST::Return(new AST::Null()))->at(body));
  } else {
    // Check that the last statement in the block would exit it,
    // either via a return, break, continue or throw
    //
    // If the last node yields a value, it is implicitly returned
    // If not, null is returned
    AST::AbstractNode* last_node = body->statements.back();
    if (AST::yields_value(last_node)) {
      body->statements.pop_back();
      body->append_node((new AST::Return(last_node))->at(last_node));
    } else {
      if (last_node->type() != AST::kTypeReturn && last_node->type() != AST::kTypeThrow) {
        body->append_node((new AST::Return(new AST::Null()))->at(body));
      }
    }
  }

  cont();
  return node;
}

AST::AbstractNode* Normalizer::visit_class(AST::Class* node, VisitContinue cont) {
  cont();

  IRKnownSelfVars* known_member_vars = new IRKnownSelfVars(node->member_properties);
  IRKnownSelfVars* known_static_vars = new IRKnownSelfVars(node->static_properties);

  for (auto& member_func : node->member_functions->children) {
    member_func->as<AST::Function>()->known_self_vars = known_member_vars;
  }

  for (auto& static_func : node->static_functions->children) {
    static_func->as<AST::Function>()->known_self_vars = known_static_vars;
  }

  if (node->constructor->type() == AST::kTypeEmpty) {
    node->constructor->as<AST::Function>()->known_self_vars = known_member_vars;
  }

  return node;
}

AST::AbstractNode* Normalizer::visit_localinitialisation(AST::LocalInitialisation* node, VisitContinue cont) {
  cont();

  // Copy the name of the variable a function or class is assigned to
  // into it's name field, unless it already has a name
  if (node->expression->type() == AST::kTypeFunction) {
    AST::Function* func = node->expression->as<AST::Function>();
    if (func->name.size() == 0) {
      func->name = node->name;
    }
  } else if (node->expression->type() == AST::kTypeClass) {
    AST::Class* klass = node->expression->as<AST::Class>();
    if (klass->name.size() == 0) {
      klass->name = node->name;
    }
  }

  return node;
}

AST::AbstractNode* Normalizer::wrap_in_block(AST::AbstractNode* node) {
  if (node->type() != AST::kTypeBlock) {
    node = (new AST::Block({node}))->at(node);
  }

  return node;
}

AST::AbstractNode* Normalizer::wrap_in_block(AST::AbstractNode* node, VisitContinue cont) {
  if (node->type() != AST::kTypeBlock) {
    node = (new AST::Block({node}))->at(node);
    node = this->visit_block(node->as<AST::Block>(), cont);
  }

  return node;
}
}  // namespace Charly::Compilation
