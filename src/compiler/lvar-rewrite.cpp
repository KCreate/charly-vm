/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2018 Leonard Schütz
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

#include "lvar-rewrite.h"

namespace Charly::Compilation {
AST::AbstractNode* LVarRewriter::visit_function(AST::Function* node, VisitContinue descend) {
  this->scope = new IRScope(this->scope, node);

  // Visit all child nodes of this function
  uint64_t backup_blockid = this->blockid;
  this->blockid = reinterpret_cast<uint64_t>(node->body);

  // Register the function's parameters
  this->scope->declare(this->context.symtable("__CHARLY_FUNCTION_ARGUMENTS"), this->depth + 1, this->blockid, true);
  for (const std::string& param : node->parameters) {
    this->scope->declare(this->context.symtable(param), this->depth + 1, this->blockid, false);
  }
  descend();
  this->blockid = backup_blockid;

  node->lvarcount = this->scope->next_frame_index;

  // Restore the old scope
  IRScope* current_scope = this->scope;
  this->scope = this->scope->parent;
  delete current_scope;

  return node;
}

AST::AbstractNode* LVarRewriter::visit_block(AST::Block* node, VisitContinue descend) {
  uint64_t backup_blockid = this->blockid;
  this->blockid = reinterpret_cast<uint64_t>(node);
  this->depth += 1;
  bool backup_ignore_const_assignment = this->ignore_const_assignment;
  this->ignore_const_assignment = this->ignore_const_assignment || node->ignore_const;
  descend();
  this->scope->pop_blockid(this->blockid);
  this->depth -= 1;
  this->blockid = backup_blockid;
  this->ignore_const_assignment = backup_ignore_const_assignment;
  return node;
}

AST::AbstractNode* LVarRewriter::visit_localinitialisation(AST::LocalInitialisation* node, VisitContinue descend) {
  // Only descend if this is not a function or class initialisation
  auto exp_type = node->expression->type();
  if (exp_type != AST::kTypeFunction && exp_type != AST::kTypeClass) {
    descend();
  }

  // Check if this is a duplicate declaration
  auto result = this->scope->resolve(this->context.symtable(node->name), this->depth, this->blockid, true);
  if (result.has_value()) {
    if (node->name == "arguments") {
      this->push_error(node, "Duplicate declaration of " + node->name +
                                 ". 'arguments' is a reserved identifier automatically inserted into every function.");
    } else {
      this->push_error(node, "Duplicate declaration of " + node->name);
    }

    return node;
  }

  // Register this variable in the current scope
  IRVarRecord record =
      this->scope->declare(this->context.symtable(node->name), this->depth, this->blockid, node->constant);

  // If the expression is a function or class, descend into it now
  // This allows a function or class to reference itself inside its body
  if (exp_type == AST::kTypeFunction || exp_type == AST::kTypeClass) {
    descend();
  }

  // If we have no expression to assign (empty declaration) remove this node from the AST
  if (node->expression->type() == AST::kTypeEmpty) {
    delete node;
    return nullptr;
  }

  // Create the initialisation node for this declaration
  AST::Assignment* initialisation = new AST::Assignment(node->name, node->expression);
  initialisation->at(node);
  initialisation->offset_info = new IRVarOffsetInfo({0, record.frame_index});

  // Deallocate the old localinitialisation node
  node->expression = nullptr;
  delete node;

  return initialisation;
}

AST::AbstractNode* LVarRewriter::visit_identifier(AST::Identifier* node, VisitContinue) {

  // Skip this node if it already has an offset_info field
  if (node->offset_info != nullptr) {
    return node;
  }

  // Check if this is a special argument index
  // e.g: $0, $1, ..., $
  if (node->name[0] == '$') {
    // Check if the remaining characters are only numbers
    if (std::all_of(node->name.begin() + 1, node->name.end(), ::isdigit)) {
      uint32_t index = std::atoi(node->name.substr(1, std::string::npos).c_str());

      if (this->scope->function_node == nullptr || index >= this->scope->function_node->parameters.size()) {
        AST::IndexIntoArguments* new_node = new AST::IndexIntoArguments(index);
        new_node->at(node);
        delete node;
        return new_node;
      } else {
        node->offset_info = new IRVarOffsetInfo({0, index + 1});
      }

      return node;
    }
  }

  // Check if this symbol exists
  auto result = this->scope->resolve(this->context.symtable(node->name), this->depth, this->blockid, false);
  if (!result.has_value()) {
    // If we reference `arguments`, we redirect to __CHARLY_FUNCTION_ARGUMENTS
    if (node->name == "arguments") {
      node->offset_info = new IRVarOffsetInfo({0, 0});
      return node;
    }

    // Search each function in the current scope stack if it has it as a known self var
    IRScope* search_scope = this->scope;
    uint32_t ir_frame_level = 0;
    while (search_scope) {
      if (search_scope->function_node != nullptr) {
        if (search_scope->function_node->known_self_vars != nullptr) {
          if (search_scope->function_node->known_self_vars->names.count(node->name) != 0) {
            AST::Self* self_val = new AST::Self();
            self_val->at(node);
            self_val->ir_frame_level = ir_frame_level;
            AST::AbstractNode* var = new AST::Member(self_val, node->name);
            var->at(node);
            delete node;
            return var;
          }
        }

        if (!search_scope->function_node->anonymous) {
          ir_frame_level++;
        }
      }

      search_scope = search_scope->parent;
    }

    this->push_error(node, "Could not resolve symbol: " + node->name);
    return node;
  }

  // Assign the offset info to the node
  node->offset_info = new IRVarOffsetInfo({result->depth, result->frame_index});
  return node;
}

AST::AbstractNode* LVarRewriter::visit_assignment(AST::Assignment* node, VisitContinue cont) {
  auto result = this->scope->resolve(this->context.symtable(node->target), this->depth, this->blockid, false);
  if (!result.has_value()) {
    // If we reference `arguments`, we redirect to __CHARLY_FUNCTION_ARGUMENTS
    if (node->target == "arguments") {
      this->push_error(node, "Can't assign to funtion arguments. Redeclare to avoid conflicts.");
      return node;
    }

    this->push_error(node, "Could not resolve symbol: " + node->target);
    return node;
  }

  if (result->is_constant && !this->ignore_const_assignment) {
    this->push_error(node, "Assignment to constant variable: " + node->target);
    return node;
  }

  node->offset_info = new IRVarOffsetInfo({result->depth, result->frame_index});

  cont();

  return node;
}

AST::AbstractNode* LVarRewriter::visit_trycatch(AST::TryCatch* node, VisitContinue) {

  // This block should always exist
  this->visit_node(node->block);

  // Register the exception name in the scope of the handler block
  this->scope->declare(this->context.symtable(node->exception_name->name), this->depth + 1,
                       reinterpret_cast<uint64_t>(node->handler_block), false);

  // Create the offset info for the exception name
  auto result = this->scope->resolve(this->context.symtable(node->exception_name->name), this->depth + 1,
                                     reinterpret_cast<uint64_t>(node->handler_block), false);
  node->exception_name->offset_info = new IRVarOffsetInfo({result->depth, result->frame_index});

  // Check if we have a handler block
  if (node->handler_block->type() != AST::kTypeEmpty) {
    this->visit_node(node->handler_block);
  }

  // Check if we have a finally block
  if (node->finally_block->type() != AST::kTypeEmpty) {
    this->visit_node(node->finally_block);
  }

  return node;
}
}  // namespace Charly::Compilation
