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

#include "charly/core/compiler/pass.h"
#include "charly/core/compiler/ir/valuelocation.h"

#pragma once

namespace charly::core::compiler::ast {

struct VariableDeclaration {
  ir::VariableId id;
  std::string name;
  bool argument;
  bool constant;
  bool leaked;
  bool global;
};

class BlockScope;
class VariableAnalyzer;
class FunctionScope {
  friend class BlockScope;
  friend class VariableAnalyzer;
public:
  FunctionScope(ref<FunctionScope> parent_function, ref<BlockScope> parent_block, const ref<Function>& ast) :
    m_parent_function(parent_function), m_parent_block(parent_block), m_function_ast(ast) {}

  uint8_t get_local_variable_count() const;
  uint8_t get_heap_variable_count() const;

private:
  std::map<ir::VariableId, VariableDeclaration> m_locals;
  ref<FunctionScope> m_parent_function;
  ref<BlockScope> m_parent_block;
  ref<Function> m_function_ast;
};

class BlockScope {
  friend class FunctionScope;
  friend class VariableAnalyzer;
public:
  BlockScope(ref<FunctionScope> parent_function, ref<BlockScope> parent_block, const ref<Block>& ast) :
    m_parent_function(parent_function), m_parent_block(parent_block), m_block_ast(ast) {}

private:
  std::unordered_map<std::string, ir::VariableId> m_locals;
  ref<FunctionScope> m_parent_function;
  ref<BlockScope> m_parent_block;
  ref<Block> m_block_ast;
};

static const ir::VariableId kInvalidVariableId = 0;

class VariableAnalyzer {
  friend class VariableLocationPatchPass;
public:
  VariableAnalyzer() {}

  ir::VariableId get_variable_id();

  void push_function(const ref<Function>& node);
  void push_block(const ref<Block>& node);
  void enter_function_scope(const ref<FunctionScope>& scope);
  void enter_block_scope(const ref<BlockScope>& scope);
  void pop_function();
  void pop_block();

  ir::VariableId declare_anonymous_variable(bool constant);
  ir::VariableId declare_variable(const std::string& name, bool constant);
  ir::VariableId declare_argument(const std::string& name);

  ir::VariableId lookup_variable(const std::string& name);

  void access_variable(ir::VariableId id);

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
  ir::VariableId declare_variable(const ref<Name>& name, const ref<Node>& declaration, bool constant);
  ir::VariableId declare_argument(const ref<Name>& name, const ref<Node>& declaration);

  virtual bool inspect_enter(const ref<Function>&) override;
  virtual void inspect_leave(const ref<Function>&) override;

  virtual bool inspect_enter(const ref<Block>&) override;
  virtual void inspect_leave(const ref<Block>&) override;

  virtual bool inspect_enter(const ref<Declaration>&) override;
  virtual ref<Statement> transform(const ref<Declaration>&) override;

  virtual void inspect_leave(const ref<UnpackDeclaration>&) override;

  virtual void inspect_leave(const ref<Assignment>&) override;
  virtual void inspect_leave(const ref<UnpackAssignment>&) override;

  virtual bool inspect_enter(const ref<Try>&) override;
  virtual ref<Statement> transform(const ref<Try>&) override;

  virtual bool inspect_enter(const ref<TryFinally>&) override;
  virtual ref<Statement> transform(const ref<TryFinally>&) override;

  virtual void inspect_leave(const ref<Id>&) override;
};

class VariableLocationPatchPass : public DiagnosticPass {
public:
  VariableLocationPatchPass(DiagnosticConsole& console, VariableAnalyzer& analyzer) :
    DiagnosticPass(console), m_analyzer(analyzer) {}

private:
  VariableAnalyzer& m_analyzer;

  virtual bool inspect_enter(const ref<Function>&) override;
  virtual void inspect_leave(const ref<Function>&) override;

  virtual bool inspect_enter(const ref<Block>&) override;
  virtual void inspect_leave(const ref<Block>&) override;

  virtual bool inspect_enter(const ref<Declaration>&) override;
  virtual ref<Statement> transform(const ref<Declaration>&) override;

  virtual void inspect_leave(const ref<UnpackDeclaration>&) override;

  virtual void inspect_leave(const ref<UnpackAssignment>&) override;

  virtual bool inspect_enter(const ref<Try>&) override;
  virtual ref<Statement> transform(const ref<Try>&) override;

  virtual bool inspect_enter(const ref<TryFinally>&) override;
  virtual ref<Statement> transform(const ref<TryFinally>&) override;

  virtual void inspect_leave(const ref<Id>&) override;
};

}  // namespace charly::core::compiler::ast
