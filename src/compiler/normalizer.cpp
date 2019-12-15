/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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
#include <list>
#include <unordered_map>
#include <vector>

#include "normalizer.h"

namespace Charly::Compilation {
AST::AbstractNode* Normalizer::visit_block(AST::Block* node, VisitContinue) {
  std::list<AST::AbstractNode*> remaining_statements;

  // Remove all unneeded nodes
  for (auto statement : node->statements) {
    AST::AbstractNode* normalized_node = this->visit_node(statement);

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

      if (AST::terminates_block(normalized_node)) {
        break;
      }

      if (AST::is_assignment(normalized_node)) {
        normalized_node->yielded_value_needed = false;
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

  // If the condition is a unary not expression, we can change the type of
  // this node to Unless as it has the same effect, but without the additional
  // unot operation
  if (node->condition->type() == AST::kTypeUnary) {
    AST::Unary* cond = node->condition->as<AST::Unary>();
    if (cond->operator_type == TokenType::UNot) {
      AST::AbstractNode* expr = cond->expression;

      // Create the replacement node
      AST::Unless* unless = new AST::Unless(expr, node->then_block);
      unless->at(node);

      // Delete the old node
      node->then_block = nullptr;
      cond->expression = nullptr;
      delete node;
      return unless;
    }
  }

  return node;
}

AST::AbstractNode* Normalizer::visit_ifelse(AST::IfElse* node, VisitContinue cont) {
  cont();
  node->then_block = this->wrap_in_block(node->then_block, cont);
  node->else_block = this->wrap_in_block(node->else_block, cont);

  // If the condition is a unary not expression, we can change the type of
  // this node to Unless as it has the same effect, but without the additional
  // unot operation
  if (node->condition->type() == AST::kTypeUnary) {
    AST::Unary* cond = node->condition->as<AST::Unary>();
    if (cond->operator_type == TokenType::UNot) {
      AST::AbstractNode* expr = cond->expression;

      // Create the new node
      AST::UnlessElse* unless = new AST::UnlessElse(expr, node->then_block, node->else_block);
      unless->at(node);

      // Cleanup
      node->then_block = nullptr;
      node->else_block = nullptr;
      cond->expression = nullptr;
      delete node;
      return unless;
    }
  }

  return node;
}

AST::AbstractNode* Normalizer::visit_unless(AST::Unless* node, VisitContinue cont) {
  cont();
  node->then_block = this->wrap_in_block(node->then_block, cont);

  // If the condition is a unary not expression, we can change the type of
  // this node to Unless as it has the same effect, but without the additional
  // unot operation
  if (node->condition->type() == AST::kTypeUnary) {
    AST::Unary* cond = node->condition->as<AST::Unary>();
    if (cond->operator_type == TokenType::UNot) {
      AST::AbstractNode* expr = cond->expression;

      // Create the replacement node
      AST::If* ifnode = new AST::If(expr, node->then_block);
      ifnode->at(node);

      // Delete the old node
      node->then_block = nullptr;
      cond->expression = nullptr;
      delete node;
      return ifnode;
    }
  }

  return node;
}

AST::AbstractNode* Normalizer::visit_unlesselse(AST::UnlessElse* node, VisitContinue cont) {
  cont();
  node->then_block = this->wrap_in_block(node->then_block, cont);
  node->else_block = this->wrap_in_block(node->else_block, cont);

  // If the condition is a unary not expression, we can change the type of
  // this node to Unless as it has the same effect, but without the additional
  // unot operation
  if (node->condition->type() == AST::kTypeUnary) {
    AST::Unary* cond = node->condition->as<AST::Unary>();
    if (cond->operator_type == TokenType::UNot) {
      AST::AbstractNode* expr = cond->expression;

      // Create the new node
      AST::IfElse* ifnode = new AST::IfElse(expr, node->then_block, node->else_block);
      ifnode->at(node);

      // Cleanup
      node->then_block = nullptr;
      node->else_block = nullptr;
      cond->expression = nullptr;
      delete node;
      return ifnode;
    }
  }

  return node;
}

AST::AbstractNode* Normalizer::visit_do_while(AST::DoWhile* node, VisitContinue cont) {
  cont();
  node->block = this->wrap_in_block(node->block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_do_until(AST::DoUntil* node, VisitContinue cont) {
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

        num--;  // subtract one from num because we already have a copy inside new_string
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

        // Division by 0 would cause a SIGFPE
        if (right == 0) return node;

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
      default: { return node; }
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
  AST::Function* current_backup = this->current_function_node;
  this->current_function_node = node;

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
      // If the last statement in a function is a yield statement, it is not returned implicitly
      // We rather insert a return null after it
      if (last_node->type() == AST::kTypeYield) {
        body->append_node((new AST::Return(new AST::Null()))->at(last_node));
      } else {
        body->statements.pop_back();
        body->append_node((new AST::Return(last_node))->at(last_node));
      }
    } else {
      if (last_node->type() != AST::kTypeReturn && last_node->type() != AST::kTypeThrow) {
        body->append_node((new AST::Return(new AST::Null()))->at(body));
      }
    }
  }

  // Initialize member properties
  for (auto& member_init : node->self_initialisations) {
    AST::MemberAssignment* assignment = new AST::MemberAssignment(new AST::Self(), member_init, new AST::Identifier(member_init));
    assignment->yielded_value_needed = false;
    body->prepend_node(assignment);
  }

  bool mark_func_as_generator_backup = this->mark_func_as_generator;
  bool mark_func_needs_arguments_backup = this->mark_func_needs_arguments;
  this->mark_func_needs_arguments = false;

  cont();

  if (this->mark_func_as_generator)
    node->generator = true;
  if (this->mark_func_needs_arguments)
    node->needs_arguments = true;

  this->current_function_node = current_backup;
  this->mark_func_needs_arguments = mark_func_needs_arguments_backup;
  this->mark_func_as_generator = mark_func_as_generator_backup;

  return node;
}

AST::AbstractNode* Normalizer::visit_class(AST::Class* node, VisitContinue cont) {
  cont();

  // Collect all the member and static symbols
  std::unordered_map<VALUE, AST::AbstractNode*> member_symbols;
  std::unordered_map<VALUE, AST::AbstractNode*> static_symbols;

  // Check for duplicate declarations or declarations that shadow other declarations
  for (auto member_func : node->member_functions->children) {
    AST::Function* as_func = member_func->as<AST::Function>();
    VALUE symbol = this->context.symtable(as_func->name);

    // Check if this symbol is already taken
    if (member_symbols.count(symbol) > 0) {
      this->push_error(member_func, "Duplicate declaration of func " + as_func->name);
      this->push_info(member_symbols[symbol], "First declaration appeared here");
    }
    member_symbols.emplace(symbol, member_func);
  }

  for (auto member_property : node->member_properties->children) {
    AST::Identifier* as_ident = member_property->as<AST::Identifier>();
    VALUE symbol = this->context.symtable(as_ident->name);

    // Check if this symbol is already taken
    if (member_symbols.count(symbol) > 0) {
      if (member_symbols[symbol]->type() == AST::kTypeFunction) {
        this->push_error(member_property, "Declaration of property " + as_ident->name + " shadows function");
        this->push_info(member_symbols[symbol], "Function declaration appeared here");
      } else {
        this->push_error(member_property, "Duplicate declaration of property " + as_ident->name);
        this->push_info(member_symbols[symbol], "First declaration appeared here");
      }
    }
    member_symbols.emplace(symbol, member_property);
  }

  for (auto static_func : node->static_functions->children) {
    AST::Function* as_func = static_func->as<AST::Function>();
    VALUE symbol = this->context.symtable(as_func->name);

    // Check if this symbol is already taken
    if (static_symbols.count(symbol) > 0) {
      this->push_error(static_func, "Duplicate declaration of " + as_func->name);
      this->push_info(static_symbols[symbol], "First declaration appeared here");
    }
    static_symbols.emplace(symbol, static_func);
  }

  for (auto static_property : node->static_properties->children) {
    AST::Identifier* as_ident = static_property->as<AST::Identifier>();
    VALUE symbol = this->context.symtable(as_ident->name);

    // Check if this symbol is already taken
    if (static_symbols.count(symbol) > 0) {
      this->push_error(static_property, "Duplicate declaration of " + as_ident->name);
      this->push_info(static_symbols[symbol], "First declaration appeared here");
    }
    static_symbols.emplace(symbol, static_property);
  }

  // if there is no user defined constructor we inject a default constructor
  // that initializes all the member variables
  if (node->constructor->type() == AST::kTypeEmpty) {

    std::vector<std::string> member_names;

    for (auto member_property : node->member_properties->children) {
      AST::Identifier* as_ident = member_property->as<AST::Identifier>();
      member_names.push_back(as_ident->name);
    }

    AST::Function* constructor = new AST::Function(
      "constructor",
      member_names,
      member_names,
      new AST::Block({

      }),
      false
    );

    constructor = this->visit_function(constructor, cont)->as<AST::Function>();

    delete node->constructor;
    node->constructor = constructor;
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

AST::AbstractNode* Normalizer::visit_yield(AST::Yield* node, VisitContinue cont) {
  cont();

  if (node->expression->type() == AST::kTypeEmpty) {
    delete node->expression;
    node->expression = new AST::Null();
  }

  this->mark_func_as_generator = true;
  return node;
}

AST::AbstractNode* Normalizer::visit_identifier(AST::Identifier* node, VisitContinue) {
  if (node->name == "arguments") {
    this->mark_func_needs_arguments = true;
  }

  if (node->name[0] == '$') {
    // Check if the remaining characters are only numbers
    if (std::all_of(node->name.begin() + 1, node->name.end(), ::isdigit)) {
      uint32_t index = std::atoi(node->name.substr(1, std::string::npos).c_str());
      AST::Function* function_node = this->current_function_node;
      if (function_node == nullptr || index >= function_node->parameters.size()) {
        this->mark_func_needs_arguments = true;
      } else {
        AST::Identifier* new_node = new AST::Identifier(function_node->parameters[index]);
        new_node->at(node);
        delete node;
        return new_node;
      }
    }
  }

  return node;
}

AST::AbstractNode* Normalizer::visit_import(AST::Import* node, VisitContinue) {

  // Get the location of this call
  std::optional<Location> loc = node->location_start;
  std::string source_filename = "(in buffer)";

  if (loc) source_filename = loc.value().filename;

  AST::AbstractNode* new_node = new AST::Call(
    new AST::Identifier("__charly_internal_import"),
    new AST::NodeList(node->source, new AST::String(source_filename))
  );

  node->source = nullptr;

  delete node;

  return new_node;
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
