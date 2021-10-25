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

#include "charly/core/compiler/passes/variable_analyzer_pass.h"

namespace charly::core::compiler::ast {

using namespace ir;

VariableId VariableAnalyzer::get_variable_id() {
  return m_id_counter++;
}

void VariableAnalyzer::push_function(const ref<Function>& node) {
  ref<FunctionScope> new_scope = make<FunctionScope>(m_function, m_block, node);
  node->variable_function_scope = new_scope;
  m_function = new_scope;
}

void VariableAnalyzer::push_block(const ref<Block>& node) {
  m_block = make<BlockScope>(m_function, m_block, node);
  node->variable_block_scope = m_block;
}

void VariableAnalyzer::enter_function_scope(const ref<FunctionScope>& scope) {
  m_function = scope;
}
void VariableAnalyzer::enter_block_scope(const ref<BlockScope>& scope) {
  m_block = scope;
}

void VariableAnalyzer::pop_function() {
  m_function = m_function->m_parent_function;
}

void VariableAnalyzer::pop_block() {
  m_block = m_block->m_parent_block;
}

VariableId VariableAnalyzer::declare_anonymous_variable(bool constant) {
  DCHECK(m_function);

  VariableId id = get_variable_id();
  VariableDeclaration declaration;
  declaration.id = id;
  declaration.name = "";
  declaration.argument = false;
  declaration.constant = constant;
  declaration.leaked = false;
  declaration.global = m_block->m_block_ast->repl_toplevel_block;
  m_function->m_locals.insert({ id, declaration });

  return id;
}

VariableId VariableAnalyzer::declare_variable(const std::string& name, bool constant) {
  DCHECK(m_function);
  DCHECK(m_block);

  VariableId id = get_variable_id();
  VariableDeclaration declaration;
  declaration.id = id;
  declaration.name = name;
  declaration.argument = false;
  declaration.constant = constant;
  declaration.leaked = false;
  declaration.global = m_block->m_block_ast->repl_toplevel_block;
  m_function->m_locals.insert({ id, declaration });

  m_block->m_locals.insert({ name, id });

  return id;
}

VariableId VariableAnalyzer::declare_argument(const std::string& name) {
  DCHECK(m_function);
  DCHECK(m_block);

  VariableId id = get_variable_id();
  VariableDeclaration declaration;
  declaration.id = id;
  declaration.name = name;
  declaration.argument = true;
  declaration.constant = false;
  declaration.leaked = false;
  declaration.global = m_block->m_block_ast->repl_toplevel_block;
  m_function->m_locals.insert({ id, declaration });

  m_block->m_locals.insert({ name, id });

  return id;
}

VariableId VariableAnalyzer::lookup_variable(const std::string& name) {
  ref<BlockScope> search_block = m_block;

  while (search_block) {
    ref<FunctionScope> search_function = search_block->m_parent_function;

    // locally declared variable
    if (search_block->m_locals.count(name)) {
      VariableId id = search_block->m_locals.at(name);
      VariableDeclaration& declaration = search_function->m_locals.at(id);

      // variables accessed from child functions are marked as leaked
      if (search_function != m_function && !declaration.global) {
        declaration.leaked = true;
      }

      return id;
    }

    search_block = search_block->m_parent_block;
  }

  VariableId id = get_variable_id();
  m_global_variables.insert({ id, name });
  return id;
}

bool VariableAnalyzer::is_constant(VariableId id) const {
  ref<FunctionScope> search_function = m_function;

  while (search_function) {
    if (search_function->m_locals.count(id)) {
      VariableDeclaration& declaration = search_function->m_locals.at(id);
      return declaration.constant;
    }

    search_function = search_function->m_parent_function;
  }

  if (m_global_variables.count(id)) {
    return false;
  }

  FAIL("could not find variable with id %", id);
}

bool VariableAnalyzer::name_already_taken(const std::string& name) const {
  DCHECK(m_block);
  return m_block->m_locals.count(name) > 0;
}

uint8_t FunctionScope::get_local_variable_count() const {
  uint8_t count = 0;

  for (const auto& entry : m_locals) {
    const VariableDeclaration& declaration = entry.second;
    if (declaration.argument || !(declaration.leaked || declaration.global)) {
      count++;
    }
  }

  return count;
}

uint8_t FunctionScope::get_heap_variable_count() const {
  uint8_t count = 0;

  for (const auto& entry : m_locals) {
    const VariableDeclaration& declaration = entry.second;
    if (declaration.leaked) {
      count++;
    }
  }

  return count;
}

void VariableAnalyzer::patch_value_location(ValueLocation& location) const {
  DCHECK(location.type == ValueLocation::Type::Id);

  VariableId id = location.as.id.id;

  ref<FunctionScope> search_function = m_function;
  uint8_t leaked_functions_counter = 0;
  while (search_function) {
    uint8_t local_index = 0;
    uint8_t heap_index = 0;
    for (const auto& entry : search_function->m_locals) {
      VariableId entry_id = entry.first;
      const VariableDeclaration& declaration = entry.second;

      if (id == entry_id) {
        if (declaration.global) {
          location = ValueLocation::Global(declaration.name);
        } else if (declaration.leaked) {
          location = ValueLocation::FarFrame(leaked_functions_counter, heap_index);
        } else {
          location = ValueLocation::LocalFrame(local_index);
        }

        return;
      }

      if (!declaration.global) {
        if (!declaration.leaked) {
          local_index++;
          continue;
        }

        if (declaration.leaked) {
          heap_index++;

          if (declaration.argument) {
            local_index++;
          }

          continue;
        }
      }
    }

    if (search_function->get_heap_variable_count()) {
      leaked_functions_counter++;
    }

    search_function = search_function->m_parent_function;
  }

  if (m_global_variables.count(id)) {
    location = ValueLocation::Global(m_global_variables.at(id));
    return;
  }

  FAIL("could not find variable with id %", id);
}

VariableId VariableAnalyzerPass::declare_variable(const ref<Name>& name, const ref<Node>& declaration, bool constant) {
  // check if this block already contains a declaration for this variable
  if (m_analyzer.name_already_taken(name->value)) {
    VariableId id = m_analyzer.lookup_variable(name->value);
    m_console.error(name, "duplicate declaration of '", name->value, "'");

    if (m_variable_declarations.count(id)) {
      ref<Node> original_declaration = m_variable_declarations.at(id);
      m_console.info(original_declaration, "originally declared here");
    }

    return kInvalidVariableId;
  }

  VariableId variable = m_analyzer.declare_variable(name->value, constant);
  if (variable) {
    name->ir_location = ValueLocation::Id(variable);
  }
  m_variable_declarations.insert({ variable, declaration });

  return variable;
}

VariableId VariableAnalyzerPass::declare_argument(const ref<Name>& name, const ref<Node>& declaration) {
  VariableId variable = m_analyzer.declare_argument(name->value);
  DCHECK(variable, "could not declare variable %", name->value);
  name->ir_location = ValueLocation::Id(variable);
  m_variable_declarations.insert({ variable, declaration });

  return variable;
}

VariableId VariableAnalyzerPass::declare_anonymous_variable(const ref<Node>& declaration, bool constant) {
  VariableId variable = m_analyzer.declare_anonymous_variable(constant);
  m_variable_declarations.insert({ variable, declaration });
  return variable;
}

bool VariableAnalyzerPass::inspect_enter(const ref<Function>& node) {
  m_analyzer.push_function(node);
  m_analyzer.push_block(node->body);

  node->ir_info.valid = true;
  node->ir_info.argc = node->argc();
  node->ir_info.minargc = node->minimum_argc();
  node->ir_info.arrow_function = node->arrow_function;

  for (const ref<FunctionArgument>& argument : node->arguments) {
    if (argument->spread_initializer) {
      node->ir_info.spread_argument = true;
    }

    if (VariableId id = declare_argument(argument->name, argument)) {
      argument->ir_location = ValueLocation::Id(id);
    }
  }

  return true;
}

void VariableAnalyzerPass::inspect_leave(const ref<Function>&) {
  m_analyzer.pop_block();
  m_analyzer.pop_function();
}

bool VariableAnalyzerPass::inspect_enter(const ref<Block>& node) {
  m_analyzer.push_block(node);
  return true;
}

void VariableAnalyzerPass::inspect_leave(const ref<Block>&) {
  m_analyzer.pop_block();
}

bool VariableAnalyzerPass::inspect_enter(const ref<Declaration>&) {
  return false;
}

ref<Statement> VariableAnalyzerPass::transform(const ref<Declaration>& node) {
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

  if (VariableId id = declare_variable(node->name, node, node->constant)) {
    node->ir_location = ValueLocation::Id(id);
  }

  if (is_regular_function || is_klass) {
    node->expression = apply(node->expression);
  }

  return node;
}

void VariableAnalyzerPass::inspect_leave(const ref<UnpackDeclaration>& node) {
  for (const ref<UnpackTargetElement>& element : node->target->elements) {
    if (VariableId id = declare_variable(element->name, element, node->constant)) {
      element->ir_location = ValueLocation::Id(id);
    }
  }
}

void VariableAnalyzerPass::inspect_leave(const ref<Assignment>& node) {
  if (node->name->ir_location.valid()) {
    VariableId variable = m_analyzer.lookup_variable(node->name->value);
    if (variable) {
      if (m_analyzer.is_constant(variable)) {
        m_console.error(node->name, "assignment to constant variable '", node->name->value, "'");
      }
    } else {
      m_console.error(node->name, "unknown variable '", node->name->value, "'");
    }
  }
}

void VariableAnalyzerPass::inspect_leave(const ref<UnpackAssignment>& node) {
  for (const ref<UnpackTargetElement>& element : node->target->elements) {
    // lookup the symbol in the current block
    VariableId variable = m_analyzer.lookup_variable(element->name->value);
    if (variable) {
      if (m_analyzer.is_constant(variable)) {
        m_console.error(element->name, "assignment to constant variable '", element->name->value, "'");
      }
      element->ir_location = ValueLocation::Id(variable);
    } else {
      m_console.error(element->name, "unknown variable '", element->name->value, "'");
    }
  }
}

bool VariableAnalyzerPass::inspect_enter(const ref<Try>&) {
  return false;
}

ref<Statement> VariableAnalyzerPass::transform(const ref<Try>& node) {
  node->try_block = cast<Block>(apply(node->try_block));

  m_analyzer.push_block(node->catch_block);
  if (VariableId id = declare_variable(node->exception_name, node->exception_name, false)) {
    node->exception_name->ir_location = ValueLocation::Id(id);
  }
  node->catch_block = cast<Block>(apply(node->catch_block));
  m_analyzer.pop_block();

  return node;
}

bool VariableAnalyzerPass::inspect_enter(const ref<TryFinally>&) {
  return false;
}

ref<Statement> VariableAnalyzerPass::transform(const ref<TryFinally>& node) {
  node->try_block = cast<Block>(apply(node->try_block));

  m_analyzer.push_block(node->finally_block);
  if (VariableId id = declare_anonymous_variable(node, true)) {
    node->exception_value_location = ValueLocation::Id(id);
  }
  node->finally_block = cast<Block>(apply(node->finally_block));
  m_analyzer.pop_block();

  return node;
}

void VariableAnalyzerPass::inspect_leave(const ref<Id>& node) {
  // lookup the symbol in the current block
  if (VariableId variable = m_analyzer.lookup_variable(node->value)) {
    node->ir_location = ValueLocation::Id(variable);

    if (m_variable_declarations.count(variable)) {
      ref<Declaration> declaration = cast<Declaration>(m_variable_declarations.at(variable));
      node->declaration_node = declaration;
    }
  } else {
    m_console.error(node, "unknown variable '", node->value, "'");
  }
}

bool VariableLocationPatchPass::inspect_enter(const ref<Function>& node) {
  m_analyzer.enter_function_scope(node->variable_function_scope);
  m_analyzer.enter_block_scope(node->body->variable_block_scope);

  for (const ref<FunctionArgument>& argument : node->arguments) {
    m_analyzer.patch_value_location(argument->ir_location);
  }

  node->ir_info.local_variables = m_analyzer.m_function->get_local_variable_count();
  node->ir_info.heap_variables = m_analyzer.m_function->get_heap_variable_count();

  return true;
}

void VariableLocationPatchPass::inspect_leave(const ref<Function>&) {
  m_analyzer.pop_block();
  m_analyzer.pop_function();
}

bool VariableLocationPatchPass::inspect_enter(const ref<Block>& node) {
  m_analyzer.enter_block_scope(node->variable_block_scope);
  return true;
}

void VariableLocationPatchPass::inspect_leave(const ref<Block>&) {
  m_analyzer.pop_block();
}

bool VariableLocationPatchPass::inspect_enter(const ref<Declaration>&) {
  return false;
}

ref<Statement> VariableLocationPatchPass::transform(const ref<Declaration>& node) {
  apply(node->expression);
  m_analyzer.patch_value_location(node->ir_location);
  return node;
}

void VariableLocationPatchPass::inspect_leave(const ref<UnpackDeclaration>& node) {
  for (const ref<UnpackTargetElement>& element : node->target->elements) {
    m_analyzer.patch_value_location(element->ir_location);
  }
}

void VariableLocationPatchPass::inspect_leave(const ref<UnpackAssignment>& node) {
  for (const ref<UnpackTargetElement>& element : node->target->elements) {
    m_analyzer.patch_value_location(element->ir_location);
  }
}

bool VariableLocationPatchPass::inspect_enter(const ref<Try>&) {
  return false;
}

ref<Statement> VariableLocationPatchPass::transform(const ref<Try>& node) {
  node->try_block = cast<Block>(apply(node->try_block));

  m_analyzer.enter_block_scope(node->catch_block->variable_block_scope);
  m_analyzer.patch_value_location(node->exception_name->ir_location);
  node->catch_block = cast<Block>(apply(node->catch_block));
  m_analyzer.pop_block();

  return node;
}

bool VariableLocationPatchPass::inspect_enter(const ref<TryFinally>&) {
  return false;
}

ref<Statement> VariableLocationPatchPass::transform(const ref<TryFinally>& node) {
  node->try_block = cast<Block>(apply(node->try_block));

  m_analyzer.enter_block_scope(node->finally_block->variable_block_scope);
  m_analyzer.patch_value_location(node->exception_value_location);
  node->finally_block = cast<Block>(apply(node->finally_block));
  m_analyzer.pop_block();

  return node;
}

void VariableLocationPatchPass::inspect_leave(const ref<Id>& node) {
  m_analyzer.patch_value_location(node->ir_location);
}

}  // namespace charly::core::compiler::ast
