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

#include "charly/core/compiler/passes/local_allocator_pass.h"

namespace charly::core::compiler::ast {

void LocalAllocatorPass::push_function(const ref<Function>& node) {
  m_function = std::make_shared<FunctionScope>(node, m_function, m_block);
}

void LocalAllocatorPass::push_block(const ref<Block>& node) {
  m_block = std::make_shared<BlockScope>(node, m_function, m_block);
}

void LocalAllocatorPass::pop_function() {
  m_function = m_function->parent_function;
}

void LocalAllocatorPass::pop_block() {
  m_block = m_block->parent_block;
}

const LocalVariable* LocalAllocatorPass::declare_variable(const ref<Name>& name,
                                                          const ref<Node>& declaration,
                                                          bool constant,
                                                          bool force_local) {
  // check if this block already contains a declaration for this variable
  if (m_block->symbol_declared(name->value)) {
    const auto* variable = m_block->lookup_symbol(name->value);
    m_console.error(name, "duplicate declaration of '", name->value, "'");
    m_console.info(variable->ast_declaration, "originally declared here");
    return nullptr;
  }

  const auto* variable = m_block->alloc_slot(name, declaration, constant, force_local);
  if (variable) {
    name->ir_location = variable->value_location;
  }

  return variable;
}

bool LocalAllocatorPass::inspect_enter(const ref<Function>& node) {
  this->push_function(node);
  this->push_block(node->body);

  node->ir_info.valid = true;
  node->ir_info.argc = node->argc();
  node->ir_info.minargc = node->minimum_argc();
  node->ir_info.arrow_function = node->arrow_function;

  // declare local slots for self and return value
  declare_variable(make<Name>("self"), node, true, true);
  declare_variable(make<Name>("ret"), node, false, true);

  // register function node arguments as local variables
  for (const ref<FunctionArgument>& argument : node->arguments) {
    const auto& variable = declare_variable(argument->name, argument);
    if (variable) {
      argument->ir_location = variable->value_location;
    }

    if (argument->spread_initializer) {
      node->ir_info.spread_argument = true;
    }
  }

  return true;
}

void LocalAllocatorPass::inspect_leave(const ref<Function>& node) {
  node->ir_info.local_variables = m_function->slots.size();

  this->pop_block();
  this->pop_function();
}

bool LocalAllocatorPass::inspect_enter(const ref<Block>& node) {
  this->push_block(node);
  return true;
}

void LocalAllocatorPass::inspect_leave(const ref<Block>&) {
  this->pop_block();
}

bool LocalAllocatorPass::inspect_enter(const ref<Declaration>&) {
  return false;
}

ref<Statement> LocalAllocatorPass::transform(const ref<Declaration>& node) {

  // if this declaration declares a function or class, we declare the local
  // variable before processing the body
  //
  // this allows functions to access their own identifier and thus be recursive
  bool is_regular_function =
    isa<Function>(node->expression) ? !cast<Function>(node->expression)->arrow_function : false;
  bool is_klass = isa<Class>(node->expression);

  if (!(is_regular_function || is_klass)) {
    node->expression = apply(node->expression);
  }

  if (const auto* variable = declare_variable(node->name, node, node->constant)) {
    node->ir_location = variable->value_location;
  }

  if (is_regular_function || is_klass) {
    node->expression = apply(node->expression);
  }

  return node;
}

void LocalAllocatorPass::inspect_leave(const ref<UnpackDeclaration>& node) {
  for (const ref<UnpackTargetElement>& element : node->target->elements) {
    if (const auto* variable = declare_variable(element->name, element, node->constant)) {
      element->ir_location = variable->value_location;
    }
  }
}

void LocalAllocatorPass::inspect_leave(const ref<Assignment>& node) {
  if (node->name->ir_location.valid()) {
    const auto* variable = m_block->lookup_symbol(node->name->value);
    if (variable) {
      if (variable->constant) {
        m_console.error(node->name, "assignment to constant variable '", node->name->value, "'");
      }
    } else {
      m_console.error(node->name, "unknown variable '", node->name->value, "'");
    }
  }
}

void LocalAllocatorPass::inspect_leave(const ref<UnpackAssignment>& node) {
  for (const ref<UnpackTargetElement>& element : node->target->elements) {

    // lookup the symbol in the current block
    const auto* variable = m_block->lookup_symbol(element->name->value);
    if (variable) {
      if (variable->constant) {
        m_console.error(element->name, "assignment to constant variable '", element->name->value, "'");
      }
      element->ir_location = variable->value_location;
    } else {
      m_console.error(element->name, "unknown variable '", element->name->value, "'");
    }
  }
}

bool LocalAllocatorPass::inspect_enter(const ref<Try>&) {
  return false;
}

ref<Statement> LocalAllocatorPass::transform(const ref<Try>& node) {
  node->try_block = cast<Block>(apply(node->try_block));

  this->push_block(node->catch_block);
  if (const auto* variable = declare_variable(node->exception_name, node->exception_name, false)) {
    node->exception_name->ir_location = variable->value_location;
  }
  node->catch_block = cast<Block>(apply(node->catch_block));
  this->pop_block();

  return node;
}

bool LocalAllocatorPass::inspect_enter(const ref<TryFinally>&) {
  return false;
}

ref<Statement> LocalAllocatorPass::transform(const ref<TryFinally>& node) {
  node->try_block = cast<Block>(apply(node->try_block));

  this->push_block(node->finally_block);
  if (const auto* variable = declare_variable(make<Name>("error"), node, true)) {
    node->exception_value_location = variable->value_location;
  }
  node->finally_block = cast<Block>(apply(node->finally_block));
  this->pop_block();

  return node;
}

void LocalAllocatorPass::inspect_leave(const ref<Id>& node) {
  // lookup the symbol in the current block
  if (const auto* variable = m_block->lookup_symbol(node->value)) {
    node->ir_location = variable->value_location;

    if (ref<Declaration> declaration = cast<Declaration>(variable->ast_declaration)) {
      node->declaration_node = declaration;
    }
  } else {
    m_console.error(node, "unknown variable '", node->value, "'");
  }
}

}  // namespace charly::core::compiler::ast
