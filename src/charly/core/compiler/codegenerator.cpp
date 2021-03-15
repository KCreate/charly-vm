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

#include "charly/core/compiler/codegenerator.h"

using namespace charly::core::compiler::ast;
using namespace charly::core::compiler::ir;

namespace charly::core::compiler {

std::shared_ptr<IRModule> CodeGenerator::compile(std::shared_ptr<CompilationUnit> unit) {
  CodeGenerator generator(unit);
  generator.compile();
  return generator.get_module();
}

void CodeGenerator::compile() {

  // register the module function
  assert(m_unit->ast->statements.size() == 1);
  assert(m_unit->ast->statements.front()->type() == Node::Type::Function);
  enqueue_function(cast<ast::Function>(m_unit->ast->statements.front()));

  while (m_function_queue.size()) {
    compile_function(m_function_queue.front());
    m_function_queue.pop();
  }
}

Label CodeGenerator::enqueue_function(const ref<Function>& ast) {
  Label begin_label = m_builder.reserve_label();
  m_function_queue.push({ .head = begin_label, .ast = ast });
  return begin_label;
}

Label CodeGenerator::register_string(const std::string& string) {
  Label l = m_builder.reserve_label();
  m_string_table.emplace_back(l, string);
  return l;
}

void CodeGenerator::compile_function(const QueuedFunction& queued_func) {
  m_builder.begin_function(queued_func.head, queued_func.ast);

  // function body
  Label return_label = m_builder.reserve_label();
  push_return_label(return_label);
  apply(queued_func.ast->body);
  pop_return_label();

  // function return block
  m_builder.place_label(return_label);
  m_builder.emit_ret();

  // emit string table
  std::unordered_map<SYMBOL, Label> emitted_strings;
  for (const auto& entry : m_string_table) {
    const Label& label = std::get<0>(entry);
    const std::string& string = std::get<1>(entry);
    SYMBOL string_hash = SYM(string);

    if (emitted_strings.count(string_hash)) {
      m_builder.place_label_at_label(label, emitted_strings.at(string_hash));
      continue;
    }

    m_builder.place_label(label);
    m_builder.emit_string_data(string);
    emitted_strings[string_hash] = label;
  }
  m_string_table.clear();
}

void CodeGenerator::generate_load(const ValueLocation& location) {
  switch (location.type) {
    case ValueLocation::Type::LocalFrame: {
      m_builder.emit_loadlocal(location.as.local_frame.offset);
      break;
    }
    case ValueLocation::Type::FarFrame: {
      m_builder.emit_loadfar(location.as.far_frame.depth, location.as.far_frame.offset);
      break;
    }
    case ValueLocation::Type::Global: {
      m_builder.emit_loadglobal(location.name);
      break;
    }
    default: {
      assert(false && "unexpected location type");
    }
  }
}

void CodeGenerator::generate_store(const ValueLocation& location) {
  switch (location.type) {
    case ValueLocation::Type::LocalFrame: {
      m_builder.emit_setlocal(location.as.local_frame.offset);
      break;
    }
    case ValueLocation::Type::FarFrame: {
      m_builder.emit_setfar(location.as.far_frame.depth, location.as.far_frame.offset);
      break;
    }
    case ValueLocation::Type::Global: {
      m_builder.emit_setglobal(location.name);
      break;
    }
    default: {
      assert(false && "unexpected location type");
    }
  }
}

Label CodeGenerator::active_return_label() const {
  assert(m_return_stack.size());
  return m_return_stack.top();
}

Label CodeGenerator::active_break_label() const {
  assert(m_break_stack.size());
  return m_break_stack.top();
}

Label CodeGenerator::active_continue_label() const {
  assert(m_continue_stack.size());
  return m_continue_stack.top();
}

void CodeGenerator::push_return_label(Label label) {
  m_return_stack.push(label);
}

void CodeGenerator::push_break_label(Label label) {
  m_break_stack.push(label);
}

void CodeGenerator::push_continue_label(Label label) {
  m_continue_stack.push(label);
}

void CodeGenerator::pop_return_label() {
  assert(m_return_stack.size());
  m_return_stack.pop();
}

void CodeGenerator::pop_break_label() {
  assert(m_break_stack.size());
  m_break_stack.pop();
}

void CodeGenerator::pop_continue_label() {
  assert(m_continue_stack.size());
  m_continue_stack.pop();
}

bool CodeGenerator::inspect_enter(const ref<Block>& node) {
  for (const ref<Statement>& stmt : node->statements) {
    apply(stmt);

    // pop toplevel expressions off the stack
    if (isa<Expression>(stmt)) {
      m_builder.emit_pop();
    }
  }

  return false;
}

void CodeGenerator::inspect_leave(const ref<Return>&) {
  m_builder.emit_jmp(active_return_label());
}

void CodeGenerator::inspect_leave(const ref<Break>&) {
  m_builder.emit_jmp(active_break_label());
}

void CodeGenerator::inspect_leave(const ref<Continue>&) {
  m_builder.emit_jmp(active_continue_label());
}

void CodeGenerator::inspect_leave(const ref<Throw>&) {
  m_builder.emit_throwex();
}

void CodeGenerator::inspect_leave(const ref<Id>& node) {
  generate_load(node->ir_location);
}

void CodeGenerator::inspect_leave(const ref<String>& node) {
  m_builder.emit_makestr(register_string(node->value));
}

void CodeGenerator::inspect_leave(const ref<FormatString>& node) {
  m_builder.emit_stringconcat(node->elements.size());
}

void CodeGenerator::inspect_leave(const ref<Int>& node) {
  m_builder.emit_load(VALUE::Int(node->value));
}

void CodeGenerator::inspect_leave(const ref<Float>& node) {
  m_builder.emit_load(VALUE::Float(node->value));
}

void CodeGenerator::inspect_leave(const ref<Bool>& node) {
  m_builder.emit_load(VALUE::Bool(node->value));
}

void CodeGenerator::inspect_leave(const ref<Char>& node) {
  m_builder.emit_load(VALUE::Char(node->value));
}

bool CodeGenerator::inspect_enter(const ref<Function>& node) {
  Label begin_label = enqueue_function(node);
  m_builder.emit_makefunc(begin_label);
  return false;
}

void CodeGenerator::inspect_leave(const ref<Null>&) {
  m_builder.emit_load(VALUE::Null());
}

void CodeGenerator::inspect_leave(const ast::ref<ast::MemberOp>& node) {
  m_builder.emit_loadattr(node->member->value);
}

void CodeGenerator::inspect_leave(const ast::ref<ast::IndexOp>&) {
  m_builder.emit_loadattrvalue();
}

bool CodeGenerator::inspect_enter(const ref<Assignment>& node) {
  apply(node->source);

  switch (node->operation) {
    case TokenType::Assignment: {
      generate_store(node->name->ir_location);
      break;
    }
    default: {
      assert(false && "operator assignment codegen not implemented");
    }
  }

  return false;
}

void CodeGenerator::inspect_leave(const ref<BinaryOp>& node) {
  switch (node->operation) {
    default: {
      assert(kBinopOpcodeMapping.count(node->operation));
      m_builder.emit(kBinopOpcodeMapping.at(node->operation));
      break;
    }
    case TokenType::And:
    case TokenType::Or: {
      assert(false && "not implemented");
      break;
    }
  }
}

void CodeGenerator::inspect_leave(const ref<UnaryOp>& node) {
  assert(kUnaryopOpcodeMapping.count(node->operation));
  m_builder.emit(kUnaryopOpcodeMapping.at(node->operation));
}

void CodeGenerator::inspect_leave(const ref<Declaration>& node) {
  generate_store(node->ir_location);
  m_builder.emit_pop();
}

bool CodeGenerator::inspect_enter(const ast::ref<ast::If>& node) {
  apply(node->condition);

  if (node->else_block) {

    // if (x) {} else {}
    Label else_label = m_builder.reserve_label();
    Label end_label = m_builder.reserve_label();
    m_builder.emit_jmpf(else_label);
    apply(node->then_block);
    m_builder.emit_jmp(end_label);
    m_builder.place_label(else_label);
    apply(node->else_block);
    m_builder.place_label(end_label);
  } else {

    // if (x) {}
    Label end_label = m_builder.reserve_label();
    m_builder.emit_jmpf(end_label);
    apply(node->then_block);
    m_builder.place_label(end_label);
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ast::ref<ast::While>& node) {

  // infinite loops
  bool infinite_loop = false;
  if (node->condition->is_constant_value() && node->condition->truthyness()) {
    infinite_loop = true;
  }

  Label body_label = m_builder.reserve_label();
  Label continue_label = m_builder.reserve_label();
  Label break_label = m_builder.reserve_label();

  push_break_label(break_label);
  push_continue_label(continue_label);

  m_builder.emit_jmp(continue_label);
  m_builder.place_label(body_label);

  if (infinite_loop) {
    m_builder.place_label(continue_label);
    apply(node->then_block);
    m_builder.emit_jmp(body_label);
  } else {
    apply(node->then_block);
    m_builder.place_label(continue_label);
    apply(node->condition);
    m_builder.emit_jmpt(body_label);
  }

  m_builder.place_label(break_label);

  pop_break_label();
  pop_continue_label();

  return false;
}

bool CodeGenerator::inspect_enter(const ast::ref<ast::BuiltinOperation>& node) {
  BuiltinId operation = node->operation;

  // emit arguments
  for (const ref<Expression>& exp : node->arguments) {
    apply(exp);
  }

  switch (operation) {
    case BuiltinId::stringconcat: {
      m_builder.emit_stringconcat(node->arguments.size());
      break;
    }
    default: {
      assert(kBuiltinOperationOpcodeMapping.count(operation));
      m_builder.emit(kBuiltinOperationOpcodeMapping.at(operation));
      break;
    }
  }

  return false;
}

}  // namespace charly::core::compiler
