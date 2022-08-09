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

#include "charly/core/compiler/passes/variable_analyzer_pass.h"

namespace charly::core::compiler::ast {

using namespace ir;

VariableId VariableAnalyzer::get_variable_id() {
  return m_id_counter++;
}

const ref<FunctionScope> VariableAnalyzer::current_function_scope() const {
  return m_function;
}

const ref<BlockScope> VariableAnalyzer::current_block_scope() const {
  return m_block;
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
  m_function = m_function->parent_function();
}

void VariableAnalyzer::pop_block() {
  m_block = m_block->parent_block();
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
  declaration.global = m_block->block_ast()->repl_toplevel_block;
  declaration.class_property = false;
  m_function->locals().insert({ id, declaration });

  return id;
}

VariableId VariableAnalyzer::declare_class_property(const std::string& name) {
  DCHECK(m_function);

  VariableId id = get_variable_id();
  VariableDeclaration declaration;
  declaration.id = id;
  declaration.name = name;
  declaration.argument = false;
  declaration.constant = false;
  declaration.leaked = false;
  declaration.global = false;
  declaration.class_property = true;
  m_function->class_properties().insert({ id, declaration });
  m_block->locals().insert({ name, id });

  return id;
}

VariableId VariableAnalyzer::declare_variable(const std::string& name, bool constant) {
  DCHECK(m_function);
  DCHECK(m_block);

  bool global_variable = m_block->block_ast()->repl_toplevel_block;

  VariableId id = get_variable_id();
  VariableDeclaration declaration;
  declaration.id = id;
  declaration.name = name;
  declaration.argument = false;
  declaration.constant = constant;
  declaration.leaked = false;
  declaration.global = global_variable;
  declaration.class_property = false;

  if (global_variable) {
    m_function->globals().insert({ id, declaration });
  } else {
    m_function->locals().insert({ id, declaration });
  }
  m_block->locals().insert({ name, id });

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
  declaration.global = m_block->block_ast()->repl_toplevel_block;
  declaration.class_property = false;

  m_function->locals().insert({ id, declaration });
  m_block->locals().insert({ name, id });

  return id;
}

VariableId VariableAnalyzer::lookup_variable(const std::string& name) {
  ref<BlockScope> search_block = m_block;

  while (search_block) {
    ref<FunctionScope> search_function = search_block->parent_function();

    // locally declared variable
    if (search_block->locals().count(name)) {
      VariableId id = search_block->locals().at(name);
      VariableDeclaration& declaration = search_function->lookup_variable(id);

      // variables accessed from child functions are marked as leaked
      if (search_function != m_function && !declaration.global) {
        declaration.leaked = true;
      }

      return id;
    }

    search_block = search_block->parent_block();
  }

  VariableId id = get_variable_id();
  m_global_variables.insert({ id, name });
  return id;
}

bool VariableAnalyzer::is_constant(VariableId id) const {
  ref<FunctionScope> search_function = m_function;

  while (search_function) {
    if (search_function->contains_variable(id)) {
      VariableDeclaration& declaration = search_function->lookup_variable(id);
      return declaration.constant;
    }

    search_function = search_function->parent_function();
  }

  if (m_global_variables.count(id)) {
    return false;
  }

  FAIL("could not find variable with id %", id);
}

bool VariableAnalyzer::name_already_taken(const std::string& name) const {
  DCHECK(m_block);
  return m_block->locals().count(name) > 0;
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
    if (declaration.leaked && !declaration.class_property) {
      count++;
    }
  }

  return count;
}

bool FunctionScope::has_frame_context() const {
  if (get_heap_variable_count()) {
    return true;
  }

  for (const auto& entry : m_class_properties) {
    if (entry.second.leaked) {
      return true;
    }
  }

  return false;
}

bool FunctionScope::contains_variable(ir::VariableId id) {
  return m_globals.count(id) || m_locals.count(id) || m_class_properties.count(id);
}

VariableDeclaration& FunctionScope::lookup_variable(ir::VariableId id) {
  if (m_globals.count(id)) {
    return m_globals.at(id);
  }
  if (m_locals.count(id)) {
    return m_locals.at(id);
  }
  if (m_class_properties.count(id)) {
    return m_class_properties.at(id);
  }
  FAIL("Expected variable id to be declared in at least one place");
}

const ref<FunctionScope> FunctionScope::parent_function() const {
  return m_parent_function;
}

const ref<BlockScope> FunctionScope::parent_block() const {
  return m_parent_block;
}

const ref<Function> FunctionScope::function_ast() const {
  return m_function_ast;
}

std::map<ir::VariableId, VariableDeclaration>& FunctionScope::globals() {
  return m_globals;
}

std::map<ir::VariableId, VariableDeclaration>& FunctionScope::locals() {
  return m_locals;
}

std::map<ir::VariableId, VariableDeclaration>& FunctionScope::class_properties() {
  return m_class_properties;
}

std::unordered_map<std::string, ir::VariableId>& BlockScope::locals() {
  return m_locals;
}

const ref<FunctionScope> BlockScope::parent_function() const {
  return m_parent_function;
}

const ref<BlockScope> BlockScope::parent_block() const {
  return m_parent_block;
}

const ref<Block> BlockScope::block_ast() const {
  return m_block_ast;
}

void VariableAnalyzer::patch_value_location(ValueLocation& location) const {
  // variable has already been visited
  if (location.type != ValueLocation::Type::Id) {
    return;
  }

  VariableId id = location.as.id.id;

  ref<FunctionScope> search_function = m_function;
  uint8_t leaked_functions_counter = 0;
  bool passed_non_arrow_function = false;
  while (search_function) {
    if (search_function->globals().count(id)) {
      auto& decl = search_function->globals().at(id);
      CHECK(decl.global);
      location = ValueLocation::Global(decl.name);
      return;
    }

    if (search_function->class_properties().count(id)) {
      auto& decl = search_function->class_properties().at(id);
      CHECK(decl.class_property);

      if (search_function == m_function || !passed_non_arrow_function) {
        location = ValueLocation::Self(decl.name);
      } else {
        location = ValueLocation::FarSelf(leaked_functions_counter, decl.name);
      }

      return;
    }

    uint8_t local_index = 0;
    uint8_t heap_index = 0;
    for (const auto& entry : search_function->locals()) {
      VariableId entry_id = entry.first;
      const VariableDeclaration& declaration = entry.second;

      if (id == entry_id) {
        if (declaration.leaked) {
          location = ValueLocation::FarFrame(leaked_functions_counter, heap_index);
        } else {
          location = ValueLocation::LocalFrame(local_index);
        }
        return;
      }

      if (declaration.leaked) {
        heap_index++;
      }

      if (declaration.argument || !declaration.leaked) {
        local_index++;
      }

      continue;
    }

    if (search_function->has_frame_context()) {
      leaked_functions_counter++;
    }

    if (!search_function->function_ast()->arrow_function) {
      passed_non_arrow_function = true;
    }

    search_function = search_function->parent_function();
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
      m_console.info(original_declaration, "previously declared here");
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

VariableId VariableAnalyzerPass::declare_class_property(const ref<Name>& name, const ref<Node>& declaration) {
  VariableId variable = m_analyzer.declare_class_property(name->value);
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
  node->ir_info.private_function = node->class_private_function;
  node->ir_info.is_constructor = node->class_constructor;

  if (ref<Class> host_class = node->host_class.lock()) {
    for (const auto& prop : host_class->member_properties) {
      CHECK(declare_class_property(prop->name, prop));
    }

    for (const auto& func : host_class->member_functions) {
      CHECK(declare_class_property(func->name, func));
    }
  }

  m_analyzer.push_block(node->body);

  bool spread_passed = false;
  for (const ref<FunctionArgument>& argument : node->arguments) {
    CHECK(!spread_passed);

    if (argument->spread_initializer) {
      node->ir_info.spread_argument = true;
      spread_passed = true;
    }

    if (VariableId id = declare_argument(argument->name, argument)) {
      argument->ir_location = ValueLocation::Id(id);
    }
  }

  return true;
}

void VariableAnalyzerPass::inspect_leave(const ref<Function>&) {
  m_analyzer.pop_block();
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
  //
  // the same behaviour would not be desirable for regular declarations as that
  // would not allow new declarations to reference previously declared variables
  // with the same name
  bool is_regular_function = false;
  if (auto function = cast<Function>(node->expression)) {
    if (!function->arrow_function) {
      is_regular_function = true;
    }
  }
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

bool VariableAnalyzerPass::inspect_enter(const ref<UnpackDeclaration>&) {
  return false;
}

ref<Statement> VariableAnalyzerPass::transform(const ref<UnpackDeclaration>& node) {
  for (const ref<UnpackTargetElement>& element : node->target->elements) {
    CHECK(isa<Id>(element->target));
    auto id_node = cast<Id>(element->target);
    if (VariableId id = declare_variable(make<Name>(id_node), element, node->constant)) {
      id_node->ir_location = ValueLocation::Id(id);
    }
  }

  apply(node->expression);

  return node;
}

void VariableAnalyzerPass::inspect_leave(const ref<Assignment>& node) {
  if (auto id = cast<Id>(node->target)) {
    if (id->ir_location.valid()) {
      VariableId variable = m_analyzer.lookup_variable(id->value);
      if (variable) {
        if (m_analyzer.is_constant(variable)) {
          m_console.error(id, "assignment to constant variable '", id->value, "'");
        }
      } else {
        m_console.error(id, "unknown variable '", id->value, "'");
      }
    }
  } else if (auto unpack_target = cast<UnpackTarget>(node->target)) {
    for (const ref<UnpackTargetElement>& element : unpack_target->elements) {
      switch (element->target->type()) {
        case Node::Type::Id: {
          auto target_id = cast<Id>(element->target);
          VariableId variable = m_analyzer.lookup_variable(target_id->value);
          if (variable) {
            if (m_analyzer.is_constant(variable)) {
              m_console.error(target_id, "assignment to constant variable '", target_id->value, "'");
            }
          } else {
            m_console.error(target_id, "unknown variable '", target_id->value, "'");
          }
        }
        default: {
          break;
        }
      }
    }
  }
}

bool VariableAnalyzerPass::inspect_enter(const ref<Try>&) {
  return false;
}

ref<Statement> VariableAnalyzerPass::transform(const ref<Try>& node) {
  node->try_block = cast<Block>(apply(node->try_block));

  m_analyzer.push_block(node->catch_block);
  node->original_pending_exception = ValueLocation::Id(declare_anonymous_variable(node, true));
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
  node->exception_value_location = ValueLocation::Id(declare_anonymous_variable(node, true));
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

  node->ir_info.name = crc32::hash_string(node->name->value);
  node->ir_info.local_variables = m_analyzer.current_function_scope()->get_local_variable_count();
  node->ir_info.heap_variables = m_analyzer.current_function_scope()->get_heap_variable_count();
  node->ir_info.has_frame_context = m_analyzer.current_function_scope()->has_frame_context();

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

bool VariableLocationPatchPass::inspect_enter(const ref<UnpackTarget>&) {
  return false;
}

ref<UnpackTarget> VariableLocationPatchPass::transform(const ref<UnpackTarget>& node) {
  for (auto& element : node->elements) {
    element->target = apply(element->target);
  }

  return node;
}

bool VariableLocationPatchPass::inspect_enter(const ref<Try>&) {
  return false;
}

ref<Statement> VariableLocationPatchPass::transform(const ref<Try>& node) {
  node->try_block = cast<Block>(apply(node->try_block));

  m_analyzer.enter_block_scope(node->catch_block->variable_block_scope);
  m_analyzer.patch_value_location(node->original_pending_exception);
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

ref<Expression> VariableLocationPatchPass::transform(const ref<Id>& node) {
  m_analyzer.patch_value_location(node->ir_location);

  if (node->ir_location.type == ValueLocation::Type::Self) {
    return make<MemberOp>(make<Self>(), make<Name>(node));
  } else if (node->ir_location.type == ValueLocation::Type::FarSelf) {
    return make<MemberOp>(make<FarSelf>(node->ir_location.as.far_self.depth), make<Name>(node));
  }

  return node;
}

}  // namespace charly::core::compiler::ast
