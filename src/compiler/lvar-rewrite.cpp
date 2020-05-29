/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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
  FunctionScope* func_scope = new FunctionScope(node, this->scope->parent_function);
  LocalScope* local_scope = new LocalScope(func_scope, this->scope);
  this->scope = local_scope;

  // Register the function arguments
  if (node->needs_arguments) {
    this->scope->alloc_slot(this->context.symtable("arguments"), true);
    this->scope->register_symbol(
      this->context.symtable("arguments"),
      LocalOffsetInfo(ValueLocation::frame(0, 0))
    );
  }

  for (const std::string& param : node->parameters) {
    VALUE symbol = this->context.symtable(param);

    LocalOffsetInfo tmp = this->scope->access_symbol(symbol);
    if (tmp.valid && tmp.shadowing) {
      this->push_error(node, "Illegal redeclaration of '" + param + "'");
      this->scope = local_scope->parent_scope;
      delete local_scope;
      delete func_scope;
      return node;
    }

    this->scope->alloc_slot(symbol);
  }

  descend();

  // Reset the old scope
  this->scope = local_scope->parent_scope;
  delete local_scope;
  delete func_scope;

  return node;
}

AST::AbstractNode* LVarRewriter::visit_class(AST::Class* node, VisitContinue) {
  node->parent_class = this->visit_node(node->parent_class);

  this->push_local_scope();
  node->member_functions = reinterpret_cast<AST::NodeList*>(this->visit_node(node->member_functions));
  node->constructor = this->visit_node(node->constructor);
  this->pop_scope();

  this->push_local_scope();
  node->static_functions = reinterpret_cast<AST::NodeList*>(this->visit_node(node->static_functions));
  node->static_properties = reinterpret_cast<AST::NodeList*>(this->visit_node(node->static_properties));
  this->pop_scope();

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
    this->push_error(node, "Illegal redeclaration of '" + node->name + "'");
    return node;
  }

  LocalOffsetInfo tmp = this->scope->access_symbol(name_symbol);
  if (tmp.valid && tmp.shadowing) {
    this->push_error(node, "Illegal redeclaration of '" + node->name + "'");
    return node;
  }

  LocalOffsetInfo result = this->scope->alloc_slot(name_symbol, node->constant);

  // If the expression is a function or class, descend into it now
  // This allows a function or class to reference itself inside its body
  if (exp_type == AST::kTypeFunction || exp_type == AST::kTypeClass) {
    descend();
  }

  // If we have no expression to assign (empty declaration) remove this node from the AST
  if (node->expression->type() == AST::kTypeEmpty) {
    delete node->expression;
    node->expression = new AST::Null();
  }

  // Create the initialisation node for this declaration
  AST::Assignment* initialisation = new AST::Assignment(node->name, node->expression);
  initialisation->at(node);
  initialisation->offset_info = new ValueLocation(result.location);
  initialisation->yielded_value_needed = false;

  // Deallocate the old localinitialisation node
  node->expression = nullptr;
  delete node;

  return initialisation;
}

AST::AbstractNode* LVarRewriter::visit_match(AST::Match* node, VisitContinue) {
  // TODO: Implement match visitor
  //       This depends on the rewrite of the lvar allocator and
  //       can't be finished without it.
  //
  //       This just stubs this method out, any missing fields are catched
  //       in the code generator and thrown as fatal errors
  return node;
}

AST::AbstractNode* LVarRewriter::visit_identifier(AST::Identifier* node, VisitContinue) {
  // Skip this node if it already has an offset_info field
  if (node->offset_info != nullptr) {
    return node;
  }

  // Check if this symbol exists
  LocalOffsetInfo result = this->scope->access_symbol(node->name);
  if (!result.valid) {
    this->push_error(node, "Could not resolve symbol: " + node->name);
    return node;
  }

  node->offset_info = new ValueLocation(result.location);
  return node;
}

AST::AbstractNode* LVarRewriter::visit_assignment(AST::Assignment* node, VisitContinue cont) {
  if (!node->no_codegen) {
    LocalOffsetInfo result = this->scope->access_symbol(node->target);
    if (!result.valid) {
      this->push_error(node, "Could not resolve symbol: " + node->target);
      return node;
    }

    if (result.constant && !this->allow_const_assignment) {
      this->push_error(node, "Assignment to constant variable: " + node->target);
      return node;
    }

    node->offset_info = new ValueLocation(result.location);
  }

  cont();

  return node;
}

AST::AbstractNode* LVarRewriter::visit_trycatch(AST::TryCatch* node, VisitContinue) {
  // This block should always exist
  this->visit_node(node->block);

  this->push_local_scope();

  VALUE ex_name_symbol = this->context.symtable(node->exception_name->name);

  LocalOffsetInfo tmp = this->scope->access_symbol(ex_name_symbol);
  if (tmp.valid && tmp.shadowing) {
    this->push_error(node, "Illegal redeclaration of '" + node->exception_name->name + "'");
    return node;
  }

  // Register the exception name in the scope of the handler block
  LocalOffsetInfo result = this->scope->alloc_slot(ex_name_symbol);
  node->exception_name->offset_info = new ValueLocation(result.location);

  // Check if we have a handler block
  if (node->handler_block->type() != AST::kTypeEmpty) {
    this->visit_node(node->handler_block);
  }

  // Let the finally block have its own scope
  this->pop_scope();
  this->push_local_scope();

  // Check if we have a finally block
  if (node->finally_block->type() != AST::kTypeEmpty) {
    this->visit_node(node->finally_block);
  }

  this->pop_scope();

  return node;
}
}  // namespace Charly::Compilation
