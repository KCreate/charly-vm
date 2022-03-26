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

#include <map>
#include <utility>

#include "charly/core/compiler/ir/valuelocation.h"
#include "charly/core/compiler/pass.h"

#pragma once

namespace charly::core::compiler::ast {

struct VariableDeclaration {
  ir::VariableId id;
  std::string name;
  bool argument;
  bool constant;
  bool leaked;
  bool global;
  bool class_property;
};

class BlockScope;
class VariableAnalyzer;
class FunctionScope {
  friend class BlockScope;

public:
  FunctionScope(ref<FunctionScope> parent_function, ref<BlockScope> parent_block, const ref<Function>& ast) :
    m_parent_function(parent_function), m_parent_block(parent_block), m_function_ast(ast) {}

  uint8_t get_local_variable_count() const;
  uint8_t get_heap_variable_count() const;
  bool has_frame_context() const;

  bool contains_variable(ir::VariableId id);
  VariableDeclaration& lookup_variable(ir::VariableId id);

  const ref<FunctionScope> parent_function() const;
  const ref<BlockScope> parent_block() const;
  const ref<Function> function_ast() const;

  std::map<ir::VariableId, VariableDeclaration>& globals();
  std::map<ir::VariableId, VariableDeclaration>& locals();
  std::map<ir::VariableId, VariableDeclaration>& class_properties();

private:
  std::map<ir::VariableId, VariableDeclaration> m_globals;
  std::map<ir::VariableId, VariableDeclaration> m_locals;
  std::map<ir::VariableId, VariableDeclaration> m_class_properties;
  ref<FunctionScope> m_parent_function;
  ref<BlockScope> m_parent_block;
  ref<Function> m_function_ast;
};

class BlockScope {
  friend class FunctionScope;

public:
  BlockScope(ref<FunctionScope> parent_function, ref<BlockScope> parent_block, const ref<Block>& ast) :
    m_parent_function(parent_function), m_parent_block(parent_block), m_block_ast(ast) {}

  std::unordered_map<std::string, ir::VariableId>& locals();
  const ref<FunctionScope> parent_function() const;
  const ref<BlockScope> parent_block() const;
  const ref<Block> block_ast() const;

private:
  std::unordered_map<std::string, ir::VariableId> m_locals;
  ref<FunctionScope> m_parent_function;
  ref<BlockScope> m_parent_block;
  ref<Block> m_block_ast;
};

static const ir::VariableId kInvalidVariableId = 0;

class VariableAnalyzer {
public:
  VariableAnalyzer() = default;

  ir::VariableId get_variable_id();

  const ref<FunctionScope> current_function_scope() const;
  const ref<BlockScope> current_block_scope() const;

  void push_function(const ref<Function>& node);
  void push_block(const ref<Block>& node);
  void enter_function_scope(const ref<FunctionScope>& scope);
  void enter_block_scope(const ref<BlockScope>& scope);
  void pop_function();
  void pop_block();

  ir::VariableId declare_anonymous_variable(bool constant);
  ir::VariableId declare_class_property(const std::string& name);
  ir::VariableId declare_variable(const std::string& name, bool constant);
  ir::VariableId declare_argument(const std::string& name);

  ir::VariableId lookup_variable(const std::string& name);

  bool is_constant(ir::VariableId id) const;

  // checks wether a given name is already declared in the currently active block
  bool name_already_taken(const std::string& name) const;

  // replaces the placeholder id with the actual relative value location
  void patch_value_location(ir::ValueLocation& location) const;

private:
  std::unordered_map<ir::VariableId, std::string> m_global_variables;
  ir::VariableId m_id_counter = 1;
  ref<FunctionScope> m_function = nullptr;
  ref<BlockScope> m_block = nullptr;
};

class VariableAnalyzerPass : public DiagnosticPass {
public:
  VariableAnalyzerPass(DiagnosticConsole& console, VariableAnalyzer& analyzer) :
    DiagnosticPass(console), m_analyzer(analyzer) {}

private:
  VariableAnalyzer& m_analyzer;
  std::unordered_map<ir::VariableId, ref<Node>> m_variable_declarations;

  // declare a new variable in the current block
  ir::VariableId declare_anonymous_variable(const ref<Node>& declaration, bool constant);
  ir::VariableId declare_class_property(const ref<Name>& name, const ref<Node>& declaration);
  ir::VariableId declare_variable(const ref<Name>& name, const ref<Node>& declaration, bool constant);
  ir::VariableId declare_argument(const ref<Name>& name, const ref<Node>& declaration);

  bool inspect_enter(const ref<Function>&) override;
  void inspect_leave(const ref<Function>&) override;

  bool inspect_enter(const ref<Block>&) override;
  void inspect_leave(const ref<Block>&) override;

  bool inspect_enter(const ref<Declaration>&) override;
  ref<Statement> transform(const ref<Declaration>&) override;

  void inspect_leave(const ref<UnpackDeclaration>&) override;

  void inspect_leave(const ref<Assignment>&) override;
  void inspect_leave(const ref<UnpackAssignment>&) override;

  bool inspect_enter(const ref<Try>&) override;
  ref<Statement> transform(const ref<Try>&) override;

  bool inspect_enter(const ref<TryFinally>&) override;
  ref<Statement> transform(const ref<TryFinally>&) override;

  void inspect_leave(const ref<Id>&) override;
};

class VariableLocationPatchPass : public DiagnosticPass {
public:
  VariableLocationPatchPass(DiagnosticConsole& console, VariableAnalyzer& analyzer) :
    DiagnosticPass(console), m_analyzer(analyzer) {}

private:
  VariableAnalyzer& m_analyzer;

  bool inspect_enter(const ref<Function>&) override;
  void inspect_leave(const ref<Function>&) override;

  bool inspect_enter(const ref<Block>&) override;
  void inspect_leave(const ref<Block>&) override;

  ref<Expression> transform(const ref<CallOp>&) override;

  bool inspect_enter(const ref<Declaration>&) override;
  ref<Statement> transform(const ref<Declaration>&) override;

  void inspect_leave(const ref<UnpackDeclaration>&) override;

  void inspect_leave(const ref<UnpackAssignment>&) override;

  bool inspect_enter(const ref<Try>&) override;
  ref<Statement> transform(const ref<Try>&) override;

  bool inspect_enter(const ref<TryFinally>&) override;
  ref<Statement> transform(const ref<TryFinally>&) override;

  void inspect_leave(const ref<Id>&) override;
};

}  // namespace charly::core::compiler::ast
