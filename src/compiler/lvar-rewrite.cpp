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
  FunctionScope* func_scope = new FunctionScope(node, this->scope->contained_function);
  LocalScope* local_scope = new LocalScope(func_scope, this->scope);
  this->scope = local_scope;

  // Register the function arguments
  this->scope->alloc_slot(this->context.symtable("__CHARLY_FUNCTION_ARGUMENTS"), true);
  for (const std::string& param : node->parameters) {
    this->scope->alloc_slot(this->context.symtable(param), false);
  }

  descend();

  // Reset the old scope
  this->scope = local_scope->parent_scope;
  delete local_scope;
  delete func_scope;

  return node;
}

AST::AbstractNode* LVarRewriter::visit_block(AST::Block* node, VisitContinue descend) {
  this->push_local_scope();

  bool backup_allow_const_assignment = this->allow_const_assignment;
  this->allow_const_assignment = this->allow_const_assignment || node->ignore_const;

  descend();

  this->allow_const_assignment = backup_allow_const_assignment;

  this->pop_scope();
  return node;
}

AST::AbstractNode* LVarRewriter::visit_localinitialisation(AST::LocalInitialisation* node, VisitContinue descend) {
  // Only descend if this is not a function or class initialisation
  auto exp_type = node->expression->type();
  if (exp_type != AST::kTypeFunction && exp_type != AST::kTypeClass) {
    descend();
  }

  VALUE name_symbol = this->context.symtable(node->name);

  // Check if this is a duplicate declaration
  if (this->scope->scope_contains_symbol(name_symbol)) {
    this->push_error(node, "Duplicate declaration of " + node->name);
    return node;
  }

  LocalOffsetInfo offset_info = this->scope->alloc_slot(name_symbol, node->constant);

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
  initialisation->offset_info = new IRVarOffsetInfo(offset_info.to_offset_info());
  initialisation->assignment_info = new IRAssignmentInfo(false);

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

      AST::Function* function_node = this->scope->contained_function->function_node;

      if (function_node == nullptr || index >= function_node->parameters.size()) {
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
  LocalOffsetInfo result = this->scope->resolve_symbol(this->context.symtable(node->name));
  if (!result.valid) {
    // If we reference `arguments`, we redirect to __CHARLY_FUNCTION_ARGUMENTS
    if (node->name == "arguments") {
      node->offset_info = new IRVarOffsetInfo({0, 0});
      return node;
    }

    // Search each function in the current scope stack if it has it as a known self var
    FunctionScope* search_scope = this->scope->contained_function;
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

      search_scope = search_scope->parent_scope;
    }

    this->push_error(node, "Could not resolve symbol: " + node->name);
    return node;
  }

  // Assign the offset info to the node
  node->offset_info = new IRVarOffsetInfo({result.level, result.offset});
  return node;
}

AST::AbstractNode* LVarRewriter::visit_assignment(AST::Assignment* node, VisitContinue cont) {
  LocalOffsetInfo result = this->scope->resolve_symbol(this->context.symtable(node->target));
  if (!result.valid) {
    if (node->target == "arguments") {
      this->push_error(node, "Can't assign to function arguments. Redeclare to avoid conflicts.");
      return node;
    }

    this->push_error(node, "Could not resolve symbol: " + node->target);
    return node;
  }

  if (result.constant && !this->allow_const_assignment) {
    this->push_error(node, "Assignment to constant variable: " + node->target);
    return node;
  }

  node->offset_info = new IRVarOffsetInfo({result.level, result.offset});

  cont();

  return node;
}

AST::AbstractNode* LVarRewriter::visit_trycatch(AST::TryCatch* node, VisitContinue) {

  // This block should always exist
  this->visit_node(node->block);

  // Register the exception name in the scope of the handler block
  LocalOffsetInfo result = this->scope->alloc_slot(this->context.symtable(node->exception_name->name));
  node->exception_name->offset_info = new IRVarOffsetInfo(result.to_offset_info());

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
