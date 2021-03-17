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
  if (queued_func.ast->class_constructor) {

    // class constructors must always return self
    m_builder.emit_loadlocal(0);
    m_builder.emit_setlocal(1);
    m_builder.emit_ret();
  } else {
    m_builder.emit_ret();
  }

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

uint8_t CodeGenerator::generate_spread_tuples(const std::vector<ast::ref<ast::Expression>>& vec) {
  uint8_t elements_in_last_segment = 0;
  uint8_t emitted_segments = 0;

  // adjacent expressions that are not wrapped inside the Spread node are
  // grouped together into tuples
  for (const ref<Expression>& exp : vec) {
    if (ref<Spread> spread = cast<Spread>(exp)) {
      if (elements_in_last_segment) {
        m_builder.emit_maketuple(elements_in_last_segment);
        emitted_segments++;
        elements_in_last_segment = 0;
      }

      apply(spread->expression);
      emitted_segments++;
    } else {
      apply(exp);
      elements_in_last_segment++;
    }
  }

  // wrap remaining elements
  if (elements_in_last_segment) {
    emitted_segments++;
    m_builder.emit_maketuple(elements_in_last_segment);
  }

  return emitted_segments;
}

void CodeGenerator::generate_unpack_assignment(const ref<UnpackTarget>& target) {
  bool spread_passed = false;
  uint8_t count_before = 0;
  uint8_t count_after = 0;

  // count elements before and after the spread
  for (const ref<UnpackTargetElement>& element : target->elements) {
    if (element->spread) {
      assert(!spread_passed);
      spread_passed = true;
      continue;
    }

    if (target->object_unpack) {
      m_builder.emit_loadsymbol(element->name->value);
    }

    if (spread_passed) {
      count_after++;
    } else {
      count_before++;
    }
  }

  // unpack the source expressions
  if (target->object_unpack) {
    if (spread_passed) {
      m_builder.emit_unpackobjectspread(target->elements.size() - 1);
    } else {
      m_builder.emit_unpackobject(target->elements.size());
    }
  } else {
    if (spread_passed) {
      m_builder.emit_unpacksequencespread(count_before, count_after);
    } else {
      m_builder.emit_unpacksequence(count_before);
    }
  }

  // store the unpacked values into their allocated locations
  for (auto begin = target->elements.rbegin();
      begin != target->elements.rend();
      begin++) {
    generate_store((*begin)->ir_location);
    m_builder.emit_pop();
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

  // store return value at the return value slot
  m_builder.emit_setlocal(1);
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

void CodeGenerator::inspect_leave(const ref<Symbol>& node) {
  m_builder.emit_loadsymbol(node->value);
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
  m_builder.register_symbol(node->name->value);
  m_builder.emit_makefunc(begin_label);
  return false;
}

bool CodeGenerator::inspect_enter(const ref<Class>& node) {
  m_builder.emit_loadsymbol(node->name->value);
  if (node->parent) {
    apply(node->parent);
  }
  apply(node->constructor);

  for (const auto& member_func : node->member_functions) {
    apply(member_func);
  }

  for (const auto& member_prop : node->member_properties) {
    m_builder.emit_loadsymbol(member_prop->name->value);
  }

  for (const auto& static_prop : node->static_properties) {
    m_builder.emit_loadsymbol(static_prop->name->value);
    apply(static_prop->value);
  }

  if (node->parent) {
    m_builder.emit_makesubclass(node->member_functions.size(), node->member_properties.size(),
                            node->static_properties.size());
  } else {
    m_builder.emit_makeclass(node->member_functions.size(), node->member_properties.size(),
                            node->static_properties.size());
  }

  return false;
}

void CodeGenerator::inspect_leave(const ref<Null>&) {
  m_builder.emit_load(VALUE::Null());
}

void CodeGenerator::inspect_leave(const ref<Self>&) {

  // arrow functions need to load their self value from the parent frame
  if (active_function()->ir_info.arrow_function) {
    m_builder.emit_loadcontextself();
  } else {
    m_builder.emit_loadlocal(0);
  }
}

bool CodeGenerator::inspect_enter(const ast::ref<ast::SuperCall>& node) {
  if (active_function()->class_constructor) {

    // load the class of the self value onto the stack
    m_builder.emit_loadlocal(0);
    m_builder.emit_dup();
    m_builder.emit_loadsuperconstructor();

    // call the constructor parent constructor
    if (node->has_spread_elements()) {
      uint8_t emitted_segments = generate_spread_tuples(node->arguments);
      m_builder.emit_callspread(emitted_segments);
    } else {
      for (const auto& exp : node->arguments) {
        apply(exp);
      }
      m_builder.emit_call(node->arguments.size());
    }
  } else {
    m_builder.emit_loadlocal(0);
    m_builder.emit_dup();
    m_builder.emit_loadsuperattr(active_function()->name->value);

    // call the parent function
    if (node->has_spread_elements()) {
      uint8_t emitted_segments = generate_spread_tuples(node->arguments);
      m_builder.emit_callspread(emitted_segments);
    } else {
      for (const auto& exp : node->arguments) {
        apply(exp);
      }
      m_builder.emit_call(node->arguments.size());
    }
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ast::ref<ast::SuperAttrCall>& node) {
  m_builder.emit_loadlocal(0);
  m_builder.emit_dup();
  m_builder.emit_loadsuperattr(node->member->value);

  // call the parent function
  if (node->has_spread_elements()) {
    uint8_t emitted_segments = generate_spread_tuples(node->arguments);
    m_builder.emit_callspread(emitted_segments);
  } else {
    for (const auto& exp : node->arguments) {
      apply(exp);
    }
    m_builder.emit_call(node->arguments.size());
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ast::ref<ast::Tuple>& node) {
  if (node->has_spread_elements()) {
    uint8_t emitted_segments = generate_spread_tuples(node->elements);
    m_builder.emit_maketuplespread(emitted_segments);
  } else {
    for (const auto& exp : node->elements) {
      apply(exp);
    }

    m_builder.emit_maketuple(node->elements.size());
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ast::ref<ast::List>& node) {
  if (node->has_spread_elements()) {
    uint8_t emitted_segments = generate_spread_tuples(node->elements);
    m_builder.emit_makelistspread(emitted_segments);
  } else {
    for (const auto& exp : node->elements) {
      apply(exp);
    }

    m_builder.emit_makelist(node->elements.size());
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ast::ref<ast::Dict>& node) {
  if (node->has_spread_elements()) {
    for (const ref<DictEntry>& exp : node->elements) {
      if (ref<Spread> spread = cast<Spread>(exp->key)) {
        m_builder.emit_load(VALUE::Null());
        apply(spread->expression);
      } else {
        apply(exp->key);
        apply(exp->value);
      }
    }

    m_builder.emit_makedictspread(node->elements.size());
  } else {
    for (const ref<DictEntry>& exp : node->elements) {
      apply(exp->key);
      apply(exp->value);
    }

    m_builder.emit_makedict(node->elements.size());
  }

  return false;
}

void CodeGenerator::inspect_leave(const ast::ref<ast::MemberOp>& node) {
  m_builder.emit_loadattr(node->member->value);
}

void CodeGenerator::inspect_leave(const ast::ref<ast::IndexOp>&) {
  m_builder.emit_loadattrvalue();
}

bool CodeGenerator::inspect_enter(const ref<Assignment>& node) {
  switch (node->operation) {
    case TokenType::Assignment: {
      apply(node->source);
      generate_store(node->name->ir_location);
      break;
    }
    default: {
      generate_load(node->name->ir_location);
      apply(node->source);
      assert(kBinopOpcodeMapping.count(node->operation));
      m_builder.emit(kBinopOpcodeMapping.at(node->operation));
      generate_store(node->name->ir_location);
      break;
    }
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ref<UnpackAssignment>& node) {
  apply(node->source);
  generate_unpack_assignment(node->target);
  return false;
}

bool CodeGenerator::inspect_enter(const ref<MemberAssignment>& node) {
  switch (node->operation) {
    case TokenType::Assignment: {
      apply(node->target);
      apply(node->source);
      m_builder.emit_setattr(node->member->value);
      break;
    }
    default: {
      apply(node->target);
      m_builder.emit_dup();
      m_builder.emit_loadattr(node->member->value);
      apply(node->source);
      assert(kBinopOpcodeMapping.count(node->operation));
      m_builder.emit(kBinopOpcodeMapping.at(node->operation));
      m_builder.emit_setattr(node->member->value);
      break;
    }
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ref<IndexAssignment>& node) {
  switch (node->operation) {
    case TokenType::Assignment: {
      apply(node->target);
      apply(node->index);
      apply(node->source);
      m_builder.emit_setattrvalue();
      break;
    }
    default: {
      apply(node->target);
      apply(node->index);
      m_builder.emit_dup2();
      m_builder.emit_loadattrvalue();
      apply(node->source);
      assert(kBinopOpcodeMapping.count(node->operation));
      m_builder.emit(kBinopOpcodeMapping.at(node->operation));
      m_builder.emit_setattrvalue();
      break;
    }
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ref<Ternary>& node) {
  Label else_label = m_builder.reserve_label();
  Label end_label = m_builder.reserve_label();

  apply(node->condition);
  m_builder.emit_jmpf(else_label);
  apply(node->then_exp);
  m_builder.emit_jmp(end_label);
  m_builder.place_label(else_label);
  apply(node->else_exp);
  m_builder.place_label(end_label);

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

bool CodeGenerator::inspect_enter(const ast::ref<ast::CallOp>& node) {
  m_builder.emit_load(VALUE::Null());
  apply(node->target);

  if (node->has_spread_elements()) {
    uint8_t emitted_segments = generate_spread_tuples(node->arguments);
    m_builder.emit_callspread(emitted_segments);
  } else {
    for (const auto& arg : node->arguments) {
      apply(arg);
    }

    m_builder.emit_call(node->arguments.size());
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ast::ref<ast::CallMemberOp>& node) {
  apply(node->target);
  m_builder.emit_dup();
  m_builder.emit_loadattr(node->member->value);

  if (node->has_spread_elements()) {
    uint8_t emitted_segments = generate_spread_tuples(node->arguments);
    m_builder.emit_callspread(emitted_segments);
  } else {
    for (const auto& arg : node->arguments) {
      apply(arg);
    }

    m_builder.emit_call(node->arguments.size());
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ast::ref<ast::CallIndexOp>& node) {
  apply(node->target);
  m_builder.emit_dup();
  apply(node->index);
  m_builder.emit_loadattrvalue();

  if (node->has_spread_elements()) {
    uint8_t emitted_segments = generate_spread_tuples(node->arguments);
    m_builder.emit_callspread(emitted_segments);
  } else {
    for (const auto& arg : node->arguments) {
      apply(arg);
    }

    m_builder.emit_call(node->arguments.size());
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ref<Declaration>& node) {

  // global declarations
  if (node->ir_location.type == ValueLocation::Type::Global) {
    m_builder.emit_declareglobal(node->name->value);
  }

  apply(node->expression);
  generate_store(node->ir_location);
  m_builder.emit_pop();

  return false;
}

bool CodeGenerator::inspect_enter(const ref<UnpackDeclaration>& node) {
  for (const ref<UnpackTargetElement>& element : node->target->elements) {
    // global declarations
    if (element->ir_location.type == ValueLocation::Type::Global) {
      m_builder.emit_declareglobal(element->name->value);
    }
  }

  apply(node->expression);
  generate_unpack_assignment(node->target);
  return false;
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

bool CodeGenerator::inspect_enter(const ast::ref<ast::Switch>& node) {
  // TODO: generate some kind of lookup table?
  //
  // this is kind of a temporary hack that i plan on removing later once
  // time and interest arises. for now switch statements are just generated as
  // sequential if statements...

  apply(node->test);
  m_builder.emit_dup();

  Label end_label = m_builder.reserve_label();
  push_break_label(end_label);
  for (const ref<SwitchCase>& case_node : node->cases) {
    Label next_label = m_builder.reserve_label();

    apply(case_node->test);
    m_builder.emit_eq();
    m_builder.emit_jmpf(next_label);
    apply(case_node->block);
    m_builder.emit_jmp(end_label);
    m_builder.place_label(next_label);
  }

  if (node->default_block) {
    apply(node->default_block);
  }

  m_builder.place_label(end_label);
  m_builder.emit_pop();
  pop_break_label();

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
