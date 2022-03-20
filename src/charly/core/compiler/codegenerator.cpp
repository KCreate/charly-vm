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
#include "charly/value.h"

namespace charly::core::compiler {

using namespace charly::core::compiler::ast;
using namespace charly::core::compiler::ir;
using namespace charly::core::runtime;

ref<IRModule> CodeGenerator::compile() {
  // register the module function
  DCHECK(m_unit->ast->statements.size() == 1);
  DCHECK(m_unit->ast->statements.front()->type() == Node::Type::Function);
  enqueue_function(cast<ast::Function>(m_unit->ast->statements.front()));

  while (!m_function_queue.empty()) {
    compile_function(m_function_queue.front());
    m_function_queue.pop();
  }

  ref<IRModule> module = m_builder.get_module();
  module->next_label = m_builder.next_label_id();
  return module;
}

Label CodeGenerator::enqueue_function(const ref<Function>& ast) {
  Label begin_label = m_builder.reserve_label();
  m_function_queue.push({ .head = begin_label, .ast = ast });
  return begin_label;
}

void CodeGenerator::compile_function(const QueuedFunction& queued_func) {
  const ref<Function>& ast = queued_func.ast;
  m_builder.begin_function(queued_func.head, ast);

  // copy leaked variables into heap slots
  // initially the runtime copies all passed arguments into the local variable slots
  // we need to copy the ones that are actually stored in the heap slots to that area
  {
    uint8_t i = 0;
    for (const auto& argument : ast->arguments) {
      const auto& location = argument->ir_location;
      if (location.type == ValueLocation::Type::FarFrame) {
        m_builder.emit_loadlocal(i);
        m_builder.emit_setfar(0, location.as.far_frame.index);
        m_builder.emit_pop();
      }
      i++;
    }
  }

  // emit default argument initializers
  uint8_t argc = ast->ir_info.argc;
  uint8_t minargc = ast->ir_info.minargc;
  if (minargc < argc) {
    // emit initial jump table
    Label labels[argc];
    Label body_label = m_builder.reserve_label();
    m_builder.emit_loadargc();
    for (uint8_t i = minargc; i < argc; i++) {
      Label l = labels[i] = m_builder.reserve_label();
      m_builder.emit_testintjmp(i, l);
    }
    m_builder.emit_testintjmp(argc, body_label);
    m_builder.emit_pop();
    m_builder.emit_panic();

    // emit stores for each argument with a default value
    for (uint8_t i = minargc; i < argc; i++) {
      m_builder.place_label(labels[i]);

      // assign default value
      const ref<FunctionArgument>& arg = ast->arguments[i];
      if (!isa<Null>(arg->default_value)) {
        apply(arg->default_value);
        generate_store(arg->ir_location)->at(arg);
        m_builder.emit_pop();
      }
    }

    m_builder.place_label(body_label);
  }

  // class constructors must always return self
  if (ast->class_constructor) {
    m_builder.emit_loadself();
    m_builder.emit_setreturn();
  }

  // function body
  Label return_label = m_builder.reserve_label();
  push_return_label(return_label);
  apply(ast->body);
  pop_return_label();

  // function return block
  m_builder.place_label(return_label);
  m_builder.emit_ret();

  // stack size required for function
  m_builder.active_function()->ast->ir_info.stacksize = m_builder.maximum_stack_height();
  CHECK(m_builder.maximum_stack_height() < 0x0100 && "function exceeded maximum stack height of 256 values");

  m_builder.finish_function();
}

ref<ir::IRInstruction> CodeGenerator::generate_load(const ValueLocation& location) {
  switch (location.type) {
    case ValueLocation::Type::Invalid: {
      FAIL("expected valid value location");
    }
    case ValueLocation::Type::Id: {
      FAIL("expected id value location to be replaced with real location");
    }
    case ValueLocation::Type::LocalFrame: {
      return m_builder.emit_loadlocal(location.as.local_frame.index);
    }
    case ValueLocation::Type::FarFrame: {
      return m_builder.emit_loadfar(location.as.far_frame.depth, location.as.far_frame.index);
    }
    case ValueLocation::Type::Global: {
      return m_builder.emit_loadglobal(m_builder.register_string(location.name));
    }
  }
}

ref<ir::IRInstruction> CodeGenerator::generate_store(const ValueLocation& location) {
  switch (location.type) {
    case ValueLocation::Type::Invalid: {
      FAIL("expected valid value location");
    }
    case ValueLocation::Type::Id: {
      FAIL("expected id value location to be replaced with real location");
    }
    case ValueLocation::Type::LocalFrame: {
      return m_builder.emit_setlocal(location.as.local_frame.index);
    }
    case ValueLocation::Type::FarFrame: {
      return m_builder.emit_setfar(location.as.far_frame.depth, location.as.far_frame.index);
    }
    case ValueLocation::Type::Global: {
      return m_builder.emit_setglobal(m_builder.register_string(location.name));
    }
  }
}

uint8_t CodeGenerator::generate_spread_tuples(const std::vector<ref<ast::Expression>>& vec) {
  uint8_t elements_in_last_segment = 0;
  uint8_t emitted_segments = 0;

  // adjacent expressions that are not wrapped inside the Spread node are
  // grouped together into tuples
  for (const ref<Expression>& exp : vec) {
    if (ref<Spread> spread = cast<Spread>(exp)) {
      if (elements_in_last_segment) {
        m_builder.emit_maketuple(elements_in_last_segment)->at(exp);
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
    m_builder.emit_maketuple(elements_in_last_segment)->at(vec.back());
  }

  return emitted_segments;
}

void CodeGenerator::generate_unpack_assignment(const ref<UnpackTarget>& target) {
  // collect the elements before and after a potential spread element
  std::vector<ref<UnpackTargetElement>> before_elements;
  std::vector<ref<UnpackTargetElement>> after_elements;
  ref<UnpackTargetElement> spread_element = nullptr;
  for (const ref<UnpackTargetElement>& element : target->elements) {
    if (element->spread) {
      DCHECK(spread_element.get() == nullptr, "expected only a single spread target");
      spread_element = element;
      continue;
    }

    if (spread_element) {
      after_elements.push_back(element);
    } else {
      before_elements.push_back(element);
    }
  }

  // keep a copy of the source value around
  // it will remain on the stack after the assignment and serve
  // as the yielded value of the unpack assignment
  m_builder.emit_dup();

  if (target->object_unpack) {
    // emit the symbols to extract from the source value
    for (auto begin = target->elements.rbegin(); begin != target->elements.rend(); begin++) {
      const ref<UnpackTargetElement>& element = *begin;
      if (!element->spread) {
        m_builder.emit_loadsymbol(element->name->value)->at(element->name);
      }
    }

    if (spread_element) {
      m_builder.emit_unpackobjectspread(before_elements.size() + after_elements.size())->at(target);
      generate_store(spread_element->name->ir_location)->at(spread_element->name);
      m_builder.emit_pop();
    } else {
      m_builder.emit_unpackobject(before_elements.size())->at(target);
    }

    for (auto begin = target->elements.rbegin(); begin != target->elements.rend(); begin++) {
      const ref<UnpackTargetElement>& element = *begin;
      if (!element->spread) {
        generate_store(element->ir_location)->at(element);
        m_builder.emit_pop();
      }
    }
  } else {
    if (spread_element) {
      m_builder.emit_unpacksequencespread(before_elements.size(), after_elements.size())->at(target);
    } else {
      m_builder.emit_unpacksequence(before_elements.size())->at(target);
    }

    for (auto begin = target->elements.rbegin(); begin != target->elements.rend(); begin++) {
      const ref<UnpackTargetElement>& element = *begin;
      generate_store(element->ir_location)->at(element);
      m_builder.emit_pop();
    }
  }
}

Label CodeGenerator::active_return_label() const {
  DCHECK(!m_return_stack.empty());
  return m_return_stack.top();
}

Label CodeGenerator::active_break_label() const {
  DCHECK(!m_break_stack.empty());
  return m_break_stack.top();
}

Label CodeGenerator::active_continue_label() const {
  DCHECK(!m_continue_stack.empty());
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
  DCHECK(!m_return_stack.empty());
  m_return_stack.pop();
}

void CodeGenerator::pop_break_label() {
  DCHECK(!m_break_stack.empty());
  m_break_stack.pop();
}

void CodeGenerator::pop_continue_label() {
  DCHECK(!m_continue_stack.empty());
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

bool CodeGenerator::inspect_enter(const ref<Return>& node) {
  if (active_function()->class_constructor) {
    // class constructors always return the local self value
    if (!isa<Null>(node->expression)) {
      apply(node->expression);
      m_builder.emit_pop();
    }

    m_builder.emit_jmp(active_return_label());
  } else {
    // store return value at the return value slot
    apply(node->expression);
    m_builder.emit_setreturn();
    m_builder.emit_jmp(active_return_label());
  }

  return false;
}

void CodeGenerator::inspect_leave(const ref<Break>&) {
  m_builder.emit_jmp(active_break_label());
}

void CodeGenerator::inspect_leave(const ref<Continue>&) {
  m_builder.emit_jmp(active_continue_label());
}

void CodeGenerator::inspect_leave(const ref<Throw>& node) {
  m_builder.emit_throwex()->at(node);
}

void CodeGenerator::inspect_leave(const ref<ast::Export>&) {
  m_builder.emit_setreturn();
  m_builder.emit_jmp(active_return_label());
}

bool CodeGenerator::inspect_enter(const ref<ast::Import>& node) {
  apply(node->source);
  m_builder.emit_makestr(m_builder.register_string(m_unit->filepath))->at(node);
  m_builder.emit_import()->at(node);
  return false;
}

void CodeGenerator::inspect_leave(const ref<ast::Yield>& node) {
  m_console.fatal(node, "yield not yet implemented");
}

bool CodeGenerator::inspect_enter(const ref<ast::Spawn>& node) {
  switch (node->statement->type()) {
    case Node::Type::CallOp: {
      ref<CallOp> call = cast<CallOp>(node->statement);

      if (isa<Super>(call->target)) {
        m_builder.emit_loadself();
        apply(call->target);
      } else {
        m_builder.emit_load_value(kNull);
        apply(call->target);
      }

      if (call->has_spread_elements()) {
        uint8_t emitted_segments = generate_spread_tuples(call->arguments);
        m_builder.emit_maketuplespread(emitted_segments)->at(call);
      } else {
        for (const auto& arg : call->arguments) {
          apply(arg);
        }
        m_builder.emit_maketuple(call->arguments.size())->at(call);
      }

      break;
    }
    case Node::Type::CallMemberOp: {
      ref<CallMemberOp> call = cast<CallMemberOp>(node->statement);

      if (isa<Super>(call->target)) {
        m_builder.emit_loadself();
        ref<MemberOp> member = make<MemberOp>(call->target, call->member);
        member->set_location(call);
        apply(member);
      } else {
        apply(call->target);
        m_builder.emit_dup();
        m_builder.emit_loadattrsym(m_builder.register_string(call->member->value))->at(call->member);
      }

      if (call->has_spread_elements()) {
        uint8_t emitted_segments = generate_spread_tuples(call->arguments);
        m_builder.emit_maketuplespread(emitted_segments)->at(call->target);
      } else {
        for (const auto& arg : call->arguments) {
          apply(arg);
        }
        m_builder.emit_maketuple(call->arguments.size())->at(call->target);
      }

      break;
    }
    case Node::Type::CallIndexOp: {
      ref<CallIndexOp> call = cast<CallIndexOp>(node->statement);

      if (isa<Super>(call->target)) {
        m_builder.emit_loadself();
        ref<IndexOp> member = make<IndexOp>(call->target, call->index);
        member->set_location(call);
        apply(member);
      } else {
        apply(call->target);
        m_builder.emit_dup();
        apply(call->index);
        m_builder.emit_loadattr()->at(call->index);
      }

      if (call->has_spread_elements()) {
        uint8_t emitted_segments = generate_spread_tuples(call->arguments);
        m_builder.emit_maketuplespread(emitted_segments)->at(call->target);
      } else {
        for (const auto& arg : call->arguments) {
          apply(arg);
        }
        m_builder.emit_maketuple(call->arguments.size())->at(call->target);
      }

      break;
    }
    default: {
      UNIMPLEMENTED();
    }
  }

  m_builder.emit_makefiber()->at(node);

  return false;
}

void CodeGenerator::inspect_leave(const ref<ast::Await>& node) {
  m_builder.emit_fiberjoin()->at(node);
}

void CodeGenerator::inspect_leave(const ref<ast::Typeof>& node) {
  m_builder.emit_type()->at(node);
}

void CodeGenerator::inspect_leave(const ref<Id>& node) {
  generate_load(node->ir_location)->at(node);
}

void CodeGenerator::inspect_leave(const ref<String>& node) {
  uint16_t index = m_builder.register_string(node->value);

  if (node->value.size() <= RawSmallString::kMaxLength) {
    m_builder.emit_load_value(RawSmallString::make_from_str(node->value))->at(node);
  } else {
    m_builder.emit_makestr(index)->at(node);
  }
}

void CodeGenerator::inspect_leave(const ref<FormatString>& node) {
  m_builder.emit_stringconcat(node->elements.size())->at(node);
}

void CodeGenerator::inspect_leave(const ref<ast::Symbol>& node) {
  m_builder.emit_loadsymbol(node->value)->at(node);
}

void CodeGenerator::inspect_leave(const ref<Int>& node) {
  m_builder.emit_load_value(RawInt::make(node->value))->at(node);
}

void CodeGenerator::inspect_leave(const ref<Float>& node) {
  m_builder.emit_load_value(RawFloat::make(node->value))->at(node);
}

void CodeGenerator::inspect_leave(const ref<Bool>& node) {
  m_builder.emit_load_value(RawBool::make(node->value))->at(node);
}

void CodeGenerator::inspect_leave(const ref<Char>& node) {
  m_builder.emit_load_value(RawSmallString::make_from_cp(node->value))->at(node);
}

bool CodeGenerator::inspect_enter(const ref<Function>& node) {
  Label begin_label = enqueue_function(node);
  m_builder.emit_makefunc(begin_label)->at(node);
  return false;
}

bool CodeGenerator::inspect_enter(const ref<Class>& node) {
  m_builder.emit_loadsymbol(node->name->value);

  if (node->parent) {
    apply(node->parent);
  } else {
    m_builder.emit_load_value(kErrorNoBaseClass);
  }
  apply(node->constructor);

  for (const auto& member_func : node->member_functions) {
    apply(member_func);
  }

  for (const auto& member_prop : node->member_properties) {
    m_builder.emit_loadsymbol(member_prop->name->value)->at(member_prop->name);
  }

  for (const auto& static_prop : node->static_properties) {
    m_builder.emit_loadsymbol(static_prop->name->value)->at(static_prop->name);
    apply(static_prop->value);
  }

  m_builder
    .emit_makeclass(node->member_functions.size(), node->member_properties.size(), node->static_properties.size())
    ->at(node);

  return false;
}

void CodeGenerator::inspect_leave(const ref<Null>& node) {
  m_builder.emit_load_value(kNull)->at(node);
}

void CodeGenerator::inspect_leave(const ref<Self>&) {
  m_builder.emit_loadself();
}

void CodeGenerator::inspect_leave(const ref<ast::Super>& node) {
  m_builder.emit_loadself();
  m_builder.emit_type();

  if (active_function()->class_constructor) {
    m_builder.emit_loadsuperconstructor()->at(node);
  } else {
    m_builder.emit_loadsuperattr(m_builder.register_string(active_function()->name->value))->at(node);
  }
}

bool CodeGenerator::inspect_enter(const ref<ast::Tuple>& node) {
  if (node->has_spread_elements()) {
    uint8_t emitted_segments = generate_spread_tuples(node->elements);
    m_builder.emit_maketuplespread(emitted_segments)->at(node);
  } else {
    for (const auto& exp : node->elements) {
      apply(exp);
    }

    m_builder.emit_maketuple(node->elements.size())->at(node);
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ref<ast::List>& node) {
  if (node->has_spread_elements()) {
    uint8_t emitted_segments = generate_spread_tuples(node->elements);
    m_builder.emit_makelistspread(emitted_segments)->at(node);
  } else {
    for (const auto& exp : node->elements) {
      apply(exp);
    }

    m_builder.emit_makelist(node->elements.size())->at(node);
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ref<ast::Dict>& node) {
  if (node->has_spread_elements()) {
    for (const ref<DictEntry>& exp : node->elements) {
      if (ref<Spread> spread = cast<Spread>(exp->key)) {
        m_builder.emit_load_value(kNull)->at(exp->key);
        apply(spread->expression);
      } else {
        apply(exp->key);
        apply(exp->value);
      }
    }

    m_builder.emit_makedictspread(node->elements.size())->at(node);
  } else {
    for (const ref<DictEntry>& exp : node->elements) {
      apply(exp->key);
      apply(exp->value);
    }

    m_builder.emit_makedict(node->elements.size())->at(node);
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ref<ast::MemberOp>& node) {
  if (isa<Super>(node->target)) {
    m_builder.emit_loadself();
    m_builder.emit_type();
    m_builder.emit_loadsuperattr(m_builder.register_string(node->member->value))->at(node->member);
  } else {
    apply(node->target);
    m_builder.emit_loadattrsym(m_builder.register_string(node->member->value))->at(node->member);
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ref<ast::IndexOp>& node) {
  apply(node->target);
  apply(node->index);
  m_builder.emit_loadattr()->at(node->index);

  return false;
}

bool CodeGenerator::inspect_enter(const ref<Assignment>& node) {
  switch (node->operation) {
    case TokenType::Assignment: {
      apply(node->source);
      generate_store(node->name->ir_location)->at(node->name);
      break;
    }
    default: {
      generate_load(node->name->ir_location)->at(node->name);
      apply(node->source);
      DCHECK(kBinopOpcodeMapping.count(node->operation));
      m_builder.emit(IRInstruction::make(kBinopOpcodeMapping[node->operation]))->at(node);
      generate_store(node->name->ir_location)->at(node->name);
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
      m_builder.emit_setattrsym(m_builder.register_string(node->member->value))->at(node);
      break;
    }
    default: {
      apply(node->target);
      m_builder.emit_dup();
      m_builder.emit_loadattrsym(m_builder.register_string(node->member->value))->at(node->member);
      apply(node->source);
      DCHECK(kBinopOpcodeMapping.count(node->operation));
      m_builder.emit(IRInstruction::make(kBinopOpcodeMapping.at(node->operation)))->at(node);
      m_builder.emit_setattrsym(m_builder.register_string(node->member->value))->at(node->member);
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
      m_builder.emit_setattr()->at(node);
      break;
    }
    default: {
      apply(node->target);
      apply(node->index);
      m_builder.emit_dup2();
      m_builder.emit_loadattr()->at(node->index);
      apply(node->source);
      DCHECK(kBinopOpcodeMapping.count(node->operation));
      m_builder.emit(IRInstruction::make(kBinopOpcodeMapping.at(node->operation)))->at(node);
      m_builder.emit_setattr()->at(node);
      break;
    }

      // target[index] op= source
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

bool CodeGenerator::inspect_enter(const ref<BinaryOp>& node) {
  switch (node->operation) {
    default: {
      apply(node->lhs);
      apply(node->rhs);
      DCHECK(kBinopOpcodeMapping.count(node->operation));
      m_builder.emit(IRInstruction::make(kBinopOpcodeMapping.at(node->operation)))->at(node);
      break;
    }
    case TokenType::And: {
      Label true_label = m_builder.reserve_label();
      Label false_label = m_builder.reserve_label();
      Label end_label = m_builder.reserve_label();

      apply(node->lhs);
      m_builder.emit_jmpf(false_label);
      apply(node->rhs);
      m_builder.emit_jmpt(true_label);

      m_builder.place_label(false_label);
      m_builder.emit_load_value(kFalse);
      m_builder.emit_jmp(end_label);

      m_builder.place_label(true_label);
      m_builder.emit_load_value(kTrue);

      m_builder.place_label(end_label);
      break;
    }
    case TokenType::Or: {
      Label end_label = m_builder.reserve_label();

      apply(node->lhs);
      m_builder.emit_dup();
      m_builder.emit_jmpt(end_label);
      m_builder.emit_pop();
      apply(node->rhs);
      m_builder.place_label(end_label);

      break;
    }
  }

  return false;
}

void CodeGenerator::inspect_leave(const ref<UnaryOp>& node) {
  DCHECK(kUnaryopOpcodeMapping.count(node->operation));
  m_builder.emit(IRInstruction::make(kUnaryopOpcodeMapping.at(node->operation)))->at(node);
}

bool CodeGenerator::inspect_enter(const ref<ast::CallOp>& node) {
  if (isa<Super>(node->target)) {
    m_builder.emit_loadself();
    m_builder.emit_dup();
    m_builder.emit_type();

    if (active_function()->class_constructor) {
      m_builder.emit_loadsuperconstructor()->at(node);
    } else {
      m_builder.emit_loadsuperattr(m_builder.register_string(active_function()->name->value))->at(node);
    }
  } else {
    m_builder.emit_load_value(kNull);
    apply(node->target);
  }

  if (node->has_spread_elements()) {
    uint8_t emitted_segments = generate_spread_tuples(node->arguments);
    m_builder.emit_callspread(emitted_segments)->at(node);
  } else {
    for (const auto& arg : node->arguments) {
      apply(arg);
    }

    m_builder.emit_call(node->arguments.size())->at(node);
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ref<ast::CallMemberOp>& node) {
  apply(node->target);
  m_builder.emit_dup();
  m_builder.emit_loadattrsym(m_builder.register_string(node->member->value))->at(node->member);

  if (node->has_spread_elements()) {
    uint8_t emitted_segments = generate_spread_tuples(node->arguments);
    m_builder.emit_callspread(emitted_segments)->at(node);
  } else {
    for (const auto& arg : node->arguments) {
      apply(arg);
    }

    m_builder.emit_call(node->arguments.size())->at(node);
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ref<ast::CallIndexOp>& node) {
  apply(node->target);
  m_builder.emit_dup();
  apply(node->index);
  m_builder.emit_loadattr()->at(node->index);

  if (node->has_spread_elements()) {
    uint8_t emitted_segments = generate_spread_tuples(node->arguments);
    m_builder.emit_callspread(emitted_segments)->at(node);
  } else {
    for (const auto& arg : node->arguments) {
      apply(arg);
    }

    m_builder.emit_call(node->arguments.size())->at(node);
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ref<Declaration>& node) {
  // global declarations
  if (node->ir_location.type == ValueLocation::Type::Global) {
    if (node->constant) {
      m_builder.emit_declareglobalconst(m_builder.register_string(node->name->value))->at(node->name);
    } else {
      m_builder.emit_declareglobal(m_builder.register_string(node->name->value))->at(node->name);
    }
  }

  // all variables are initialized as null so the
  // initialization can be skipped in that case
  if (!isa<Null>(node->expression)) {
    apply(node->expression);
    generate_store(node->ir_location)->at(node->name);
    m_builder.emit_pop();
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ref<UnpackDeclaration>& node) {
  for (const ref<UnpackTargetElement>& element : node->target->elements) {
    // global declarations
    if (element->ir_location.type == ValueLocation::Type::Global) {
      if (node->constant) {
        m_builder.emit_declareglobalconst(m_builder.register_string(element->name->value))->at(element);
      } else {
        m_builder.emit_declareglobal(m_builder.register_string(element->name->value))->at(element);
      }
    }
  }

  apply(node->expression);
  generate_unpack_assignment(node->target);
  m_builder.emit_pop();
  return false;
}

bool CodeGenerator::inspect_enter(const ref<ast::If>& node) {
  if (node->else_block) {
    // if (x) {} else {}
    ref<BinaryOp> op = cast<BinaryOp>(node->condition);
    if (op && op->operation == TokenType::And) {
      // if (lhs && rhs) {} else {}
      Label else_label = m_builder.reserve_label();
      Label end_label = m_builder.reserve_label();

      apply(op->lhs);
      m_builder.emit_jmpf(else_label);
      apply(op->rhs);
      m_builder.emit_jmpf(else_label);
      apply(node->then_block);
      m_builder.emit_jmp(end_label);
      m_builder.place_label(else_label);
      apply(node->else_block);
      m_builder.place_label(end_label);
    } else if (op && op->operation == TokenType::Or) {
      // if (lhs || rhs) {} else {}
      Label then_label = m_builder.reserve_label();
      Label else_label = m_builder.reserve_label();
      Label end_label = m_builder.reserve_label();

      apply(op->lhs);
      m_builder.emit_jmpt(then_label);
      apply(op->rhs);
      m_builder.emit_jmpf(else_label);
      m_builder.place_label(then_label);
      apply(node->then_block);
      m_builder.emit_jmp(end_label);
      m_builder.place_label(else_label);
      apply(node->else_block);
      m_builder.place_label(end_label);
    } else {
      // if (x) {} else {}
      Label else_label = m_builder.reserve_label();
      Label end_label = m_builder.reserve_label();

      apply(node->condition);
      m_builder.emit_jmpf(else_label);
      apply(node->then_block);
      m_builder.emit_jmp(end_label);
      m_builder.place_label(else_label);
      apply(node->else_block);
      m_builder.place_label(end_label);
    }
  } else {
    // if (x) {}
    ref<BinaryOp> op = cast<BinaryOp>(node->condition);
    if (op && op->operation == TokenType::And) {
      // if (lhs && rhs) {}
      Label end_label = m_builder.reserve_label();
      apply(op->lhs);
      m_builder.emit_jmpf(end_label);
      apply(op->rhs);
      m_builder.emit_jmpf(end_label);
      apply(node->then_block);
      m_builder.place_label(end_label);
    } else if (op && op->operation == TokenType::Or) {
      // if (lhs || rhs) {}
      Label true_label = m_builder.reserve_label();
      Label end_label = m_builder.reserve_label();
      apply(op->lhs);
      m_builder.emit_jmpt(true_label);
      apply(op->rhs);
      m_builder.emit_jmpf(end_label);
      m_builder.place_label(true_label);
      apply(node->then_block);
      m_builder.place_label(end_label);
    } else {
      // if (x) {}
      Label end_label = m_builder.reserve_label();
      apply(node->condition);
      m_builder.emit_jmpf(end_label);
      apply(node->then_block);
      m_builder.place_label(end_label);
    }
  }

  return false;
}

bool CodeGenerator::inspect_enter(const ref<ast::While>& node) {
  Label continue_label = m_builder.reserve_label();
  Label break_label = m_builder.reserve_label();

  push_break_label(break_label);
  push_continue_label(continue_label);

  ref<BinaryOp> op = cast<BinaryOp>(node->condition);
  if (op && op->operation == TokenType::And) {
    // while (lhs && rhs) {}
    Label body_label = m_builder.reserve_label();
    m_builder.place_label(continue_label);
    apply(op->lhs);
    m_builder.emit_jmpf(break_label);
    apply(op->rhs);
    m_builder.emit_jmpf(break_label);
    m_builder.place_label(body_label);
    apply(node->then_block);
    m_builder.emit_jmp(continue_label);
    m_builder.place_label(break_label);
  } else if (op && op->operation == TokenType::Or) {
    // while (lhs || rhs) {}
    Label body_label = m_builder.reserve_label();
    m_builder.place_label(continue_label);
    apply(op->lhs);
    m_builder.emit_jmpt(body_label);
    apply(op->rhs);
    m_builder.emit_jmpf(break_label);
    m_builder.place_label(body_label);
    apply(node->then_block);
    m_builder.emit_jmp(continue_label);
    m_builder.place_label(break_label);
  } else {
    // while (x) {}
    m_builder.place_label(continue_label);
    apply(node->condition);
    m_builder.emit_jmpf(break_label);
    apply(node->then_block);
    m_builder.emit_jmp(continue_label);
    m_builder.place_label(break_label);
  }

  pop_break_label();
  pop_continue_label();

  return false;
}

bool CodeGenerator::inspect_enter(const ref<ast::Loop>& node) {
  Label continue_label = m_builder.reserve_label();
  Label break_label = m_builder.reserve_label();

  push_break_label(break_label);
  push_continue_label(continue_label);

  m_builder.place_label(continue_label);
  apply(node->then_block);
  m_builder.emit_jmp(continue_label);
  m_builder.place_label(break_label);

  pop_break_label();
  pop_continue_label();

  return false;
}

bool CodeGenerator::inspect_enter(const ref<ast::Try>& node) {
  Label try_begin = m_builder.reserve_label();
  Label try_end = m_builder.reserve_label();
  Label catch_begin = m_builder.reserve_label();
  Label catch_end = m_builder.reserve_label();

  // emit try block
  m_builder.push_exception_handler(catch_begin);
  m_builder.place_label(try_begin);
  apply(node->try_block);
  m_builder.emit_jmp(catch_end);
  m_builder.pop_exception_handler();
  m_builder.place_label(try_end);

  // emit catch block
  m_builder.place_label(catch_begin);
  m_builder.emit_getexception();
  generate_store(node->exception_name->ir_location)->at(node->exception_name);
  m_builder.emit_pop();

  apply(node->catch_block);
  m_builder.place_label(catch_end);

  return false;
}

bool CodeGenerator::inspect_enter(const ref<ast::TryFinally>& node) {
  /*
   * this method generates a lot of excess blocks in most cases
   * subsequent dead-code elimination passes will remove these blocks
   * */
  Label try_begin = m_builder.reserve_label();
  Label try_end = m_builder.reserve_label();
  Label normal_handler = m_builder.reserve_label();
  Label catch_handler = m_builder.reserve_label();

  // intercept control statements
  Label break_handler = m_builder.reserve_label();
  Label continue_handler = m_builder.reserve_label();
  Label return_handler = m_builder.reserve_label();
  push_break_label(break_handler);
  push_continue_label(continue_handler);
  push_return_label(return_handler);

  // emit try block
  m_builder.push_exception_handler(catch_handler);
  m_builder.place_label(try_begin);
  apply(node->try_block);
  m_builder.emit_jmp(normal_handler);
  m_builder.pop_exception_handler();
  m_builder.place_label(try_end);

  pop_break_label();
  pop_continue_label();
  pop_return_label();

  // emit catch handler
  m_builder.place_label(catch_handler);
  m_builder.emit_getexception();
  generate_store(node->exception_value_location)->at(node->finally_block);
  m_builder.emit_pop();
  apply(node->finally_block);
  generate_load(node->exception_value_location)->at(node->finally_block);
  m_builder.emit_throwex()->at(node->finally_block);

  // emit break, continue and return intercepts
  m_builder.place_label(break_handler);
  if (!m_break_stack.empty()) {
    apply(node->finally_block);
    m_builder.emit_jmp(active_break_label());
  } else {
    m_builder.emit_panic();
  }
  m_builder.place_label(continue_handler);
  if (!m_continue_stack.empty()) {
    apply(node->finally_block);
    m_builder.emit_jmp(active_continue_label());
  } else {
    m_builder.emit_panic();
  }
  m_builder.place_label(return_handler);
  if (!m_return_stack.empty()) {
    apply(node->finally_block);
    m_builder.emit_jmp(active_return_label());
  } else {
    m_builder.emit_panic();
  }

  m_builder.place_label(normal_handler);
  apply(node->finally_block);

  return false;
}

bool CodeGenerator::inspect_enter(const ref<ast::Switch>& node) {
  // TODO: generate some kind of lookup table?
  //
  // this is kind of a temporary hack that i plan on removing later once
  // time and interest arises. for now switch statements are just generated as
  // sequential if statements...
  Label end_label = m_builder.reserve_label();
  Label default_label = m_builder.reserve_label();
  push_break_label(end_label);

  apply(node->test);

  // emit conditions
  Label case_labels[node->cases.size()];
  uint32_t index = 0;
  for (const ref<SwitchCase>& case_node : node->cases) {
    Label block_label = case_labels[index] = m_builder.reserve_label();

    m_builder.emit_dup();
    apply(case_node->test);
    m_builder.emit_eq()->at(case_node->test);
    m_builder.emit_jmpt(block_label)->at(case_node->test);

    index++;
  }
  m_builder.emit_pop();
  m_builder.emit_jmp(default_label);

  // condition blocks
  index = 0;
  for (const ref<SwitchCase>& case_node : node->cases) {
    Label block_label = case_labels[index];
    m_builder.place_label(block_label);

    m_builder.update_stack(1);  // pop remaining condition value off stack
    m_builder.emit_pop();
    apply(case_node->block);
    m_builder.emit_jmp(end_label);

    index++;
  }

  // default block
  m_builder.place_label(default_label);
  if (node->default_block) {
    apply(node->default_block);
  }

  m_builder.place_label(end_label);
  pop_break_label();

  return false;
}

bool CodeGenerator::inspect_enter(const ref<ast::BuiltinOperation>& node) {
  BuiltinId operation = node->operation;

  // emit arguments
  for (const ref<Expression>& exp : node->arguments) {
    apply(exp);
  }

  DCHECK(kBuiltinOperationOpcodeMapping.count(operation));
  m_builder.emit(IRInstruction::make(kBuiltinOperationOpcodeMapping[operation]))->at(node);

  return false;
}

}  // namespace charly::core::compiler
