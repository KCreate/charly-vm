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

#include "charly/utils/buffer.h"
#include "charly/utils/colorwriter.h"
#include "charly/core/compiler/ir/builder.h"

namespace charly::core::compiler::ir {

using namespace charly::core::compiler::ast;

using Color = utils::Color;

void Builder::register_symbol(const std::string& string) {
  m_module->symbol_table[SYM(string)] = string;
}

IRFunction& Builder::active_function() const {
  assert(m_active_function && "no active function");
  return *m_active_function;
}

IRFunction& Builder::begin_function(Label head, ref<ast::Function> ast) {
  m_module->functions.emplace_back(head, ast);
  m_active_function = &m_module->functions.back();
  return active_function();
}

Label Builder::reserve_label() {
  return m_label_counter++;
}

void Builder::place_label(Label label) {
  emit_label_definition(label);
}

void Builder::place_label_at_label(Label label, Label target_label) {

  // search for the target label
  IRFunction& func = active_function();
  for (auto it = func.statements.begin(); it != func.statements.end(); it++) {
    const ref<IRStatement>& ptr = *it;

    if (ptr->get_type() == IRStatement::Type::LabelDefinition) {
      const IRLabelDefinition* label_def = dynamic_cast<IRLabelDefinition*>(ptr.get());
      if (label_def->label == target_label) {
        func.statements.insert(it, make<IRLabelDefinition>(label));
        return;
      }
    }
  }

  assert(false && "unknown label");
}

ref<IRStatement> Builder::emit_string_data(const std::string& string) {
  IRFunction& func = active_function();
  return func.statements.emplace_back(make<IRStringData>(string));
}

ref<IRStatement> Builder::emit_label_definition(Label label) {
  IRFunction& func = active_function();
  return func.statements.emplace_back(make<IRLabelDefinition>(label));
}

// machine control
ref<IRStatement> Builder::emit_nop() {
  return emit(Opcode::nop);
}

ref<IRStatement> Builder::emit_panic() {
  return emit(Opcode::panic);
}

// misc. instructions
ref<IRStatement> Builder::emit_import() {
  return emit(Opcode::import);
}

ref<IRStatement> Builder::emit_stringconcat(OpCount8 count) {
  return emit(Opcode::stringconcat, IROperandCount8::make(count));
}

ref<IRStatement> Builder::emit_declareglobal(OpSymbol symbol) {
  return emit(Opcode::declareglobal, IROperandSymbol::make(symbol));
}

ref<IRStatement> Builder::emit_declareglobalconst(OpSymbol symbol) {
  return emit(Opcode::declareglobalconst, IROperandSymbol::make(symbol));
}

ref<IRStatement> Builder::emit_type() {
  return emit(Opcode::type);
}

// stack management
ref<IRStatement> Builder::emit_pop() {
  return emit(Opcode::pop);
}

ref<IRStatement> Builder::emit_dup() {
  return emit(Opcode::dup);
}

ref<IRStatement> Builder::emit_dup2() {
  return emit(Opcode::dup2);
}

// control flow
ref<IRStatement> Builder::emit_jmp(OpOffset label) {
  return emit(Opcode::jmp, IROperandOffset::make(label));
}

ref<IRStatement> Builder::emit_jmpf(OpOffset label) {
  return emit(Opcode::jmpf, IROperandOffset::make(label));
}

ref<IRStatement> Builder::emit_jmpt(OpOffset label) {
  return emit(Opcode::jmpt, IROperandOffset::make(label));
}

ref<IRStatement> Builder::emit_testjmp(OpImmediate value, OpOffset label) {
  return emit(Opcode::testjmp, IROperandImmediate::make(value), IROperandOffset::make(label));
}

ref<IRStatement> Builder::emit_testjmpstrict(OpImmediate value, OpOffset label) {
  return emit(Opcode::testjmpstrict, IROperandImmediate::make(value), IROperandOffset::make(label));
}

ref<IRStatement> Builder::emit_throwex() {
  return emit(Opcode::throwex);
}

ref<IRStatement> Builder::emit_getexception() {
  return emit(Opcode::getexception);
}

// function control flow
ref<IRStatement> Builder::emit_call(OpCount8 count) {
  return emit(Opcode::call, IROperandCount8::make(count));
}

ref<IRStatement> Builder::emit_callspread(OpCount8 count) {
  return emit(Opcode::callspread, IROperandCount8::make(count));
}

ref<IRStatement> Builder::emit_ret() {
  return emit(Opcode::ret);
}

// load operations
ref<IRStatement> Builder::emit_load(OpImmediate value) {
  return emit(Opcode::load, IROperandImmediate::make(value));
}

ref<IRStatement> Builder::emit_loadsymbol(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::loadsymbol, IROperandSymbol::make(symbol));
}

ref<IRStatement> Builder::emit_loadself() {
  return emit(Opcode::loadself);
}

ref<IRStatement> Builder::emit_loadglobal(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::loadglobal, IROperandSymbol::make(symbol));
}

ref<IRStatement> Builder::emit_loadlocal(OpCount8 offset) {
  return emit(Opcode::loadlocal, IROperandCount8::make(offset));
}

ref<IRStatement> Builder::emit_loadfar(OpCount8 depth, OpCount8 offset) {
  return emit(Opcode::loadfar, IROperandCount8::make(depth), IROperandCount8::make(offset));
}

ref<IRStatement> Builder::emit_loadattr(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::loadattr, IROperandSymbol::make(symbol));
}

ref<IRStatement> Builder::emit_loadattrvalue() {
  return emit(Opcode::loadattrvalue);
}

ref<IRStatement> Builder::emit_loadsuperconstructor() {
  return emit(Opcode::loadsuperconstructor);
}

ref<IRStatement> Builder::emit_loadsuperattr(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::loadsuperattr, IROperandSymbol::make(symbol));
}

// write operations
ref<IRStatement> Builder::emit_setglobal(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::setglobal, IROperandSymbol::make(symbol));
}

ref<IRStatement> Builder::emit_setlocal(OpCount8 offset) {
  return emit(Opcode::setlocal, IROperandCount8::make(offset));
}

ref<IRStatement> Builder::emit_setreturn() {
  return emit(Opcode::setreturn);
}

ref<IRStatement> Builder::emit_setfar(OpCount8 depth, OpCount8 offset) {
  return emit(Opcode::setfar, IROperandCount8::make(depth), IROperandCount8::make(offset));
}

ref<IRStatement> Builder::emit_setattr(OpSymbol symbol) {
  register_symbol(symbol);
  return emit(Opcode::setattr, IROperandSymbol::make(symbol));
}

ref<IRStatement> Builder::emit_setattrvalue() {
  return emit(Opcode::setattrvalue);
}

// value destructuring operations
ref<IRStatement> Builder::emit_unpacksequence(OpCount8 count) {
  return emit(Opcode::unpacksequence, IROperandCount8::make(count));
}

ref<IRStatement> Builder::emit_unpacksequencespread(OpCount8 before, OpCount8 after) {
  return emit(Opcode::unpacksequencespread, IROperandCount8::make(before), IROperandCount8::make(after));
}

ref<IRStatement> Builder::emit_unpackobject(OpCount8 count) {
  return emit(Opcode::unpackobject, IROperandCount8::make(count));
}

ref<IRStatement> Builder::emit_unpackobjectspread(OpCount8 count) {
  return emit(Opcode::unpackobjectspread, IROperandCount8::make(count));
}

// value allocation
ref<IRStatement> Builder::emit_makefunc(OpOffset offset) {
  return emit(Opcode::makefunc, IROperandOffset::make(offset));
}

ref<IRStatement> Builder::emit_makeclass(OpSymbol name,
                                                     OpCount8 funccount,
                                                     OpCount8 propcount,
                                                     OpCount8 staticpropcount) {
  return emit(Opcode::makeclass, IROperandSymbol::make(name), IROperandCount8::make(funccount),
              IROperandCount8::make(propcount), IROperandCount8::make(staticpropcount));
}

ref<IRStatement> Builder::emit_makesubclass(OpSymbol name,
                                                        OpCount8 funccount,
                                                        OpCount8 propcount,
                                                        OpCount8 staticpropcount) {
  return emit(Opcode::makesubclass, IROperandSymbol::make(name), IROperandCount8::make(funccount),
              IROperandCount8::make(propcount), IROperandCount8::make(staticpropcount));
}

ref<IRStatement> Builder::emit_makestr(OpOffset offset) {
  return emit(Opcode::makestr, IROperandOffset::make(offset));
}

ref<IRStatement> Builder::emit_makelist(OpCount16 count) {
  return emit(Opcode::makelist, IROperandCount16::make(count));
}

ref<IRStatement> Builder::emit_makelistspread(OpCount16 count) {
  return emit(Opcode::makelistspread, IROperandCount16::make(count));
}

ref<IRStatement> Builder::emit_makedict(OpCount16 count) {
  return emit(Opcode::makedict, IROperandCount16::make(count));
}

ref<IRStatement> Builder::emit_makedictspread(OpCount16 count) {
  return emit(Opcode::makedictspread, IROperandCount16::make(count));
}

ref<IRStatement> Builder::emit_maketuple(OpCount16 count) {
  return emit(Opcode::maketuple, IROperandCount16::make(count));
}

ref<IRStatement> Builder::emit_maketuplespread(OpCount16 count) {
  return emit(Opcode::maketuplespread, IROperandCount16::make(count));
}

// fiber management
ref<IRStatement> Builder::emit_fiberspawn() {
  return emit(Opcode::fiberspawn);
}

ref<IRStatement> Builder::emit_fiberyield() {
  return emit(Opcode::fiberyield);
}

ref<IRStatement> Builder::emit_fibercall() {
  return emit(Opcode::fibercall);
}

ref<IRStatement> Builder::emit_fiberpause() {
  return emit(Opcode::fiberpause);
}

ref<IRStatement> Builder::emit_fiberresume() {
  return emit(Opcode::fiberresume);
}

ref<IRStatement> Builder::emit_fiberawait() {
  return emit(Opcode::fiberawait);
}

// cast operations
ref<IRStatement> Builder::emit_caststring() {
  return emit(Opcode::caststring);
}

// iterator operations
ref<IRStatement> Builder::emit_iteratornext() {
  return emit(Opcode::iteratornext);
}

// arithmetic operations
ref<IRStatement> Builder::emit_add() {
  return emit(Opcode::add);
}

ref<IRStatement> Builder::emit_sub() {
  return emit(Opcode::sub);
}

ref<IRStatement> Builder::emit_mul() {
  return emit(Opcode::mul);
}

ref<IRStatement> Builder::emit_div() {
  return emit(Opcode::div);
}

ref<IRStatement> Builder::emit_mod() {
  return emit(Opcode::mod);
}

ref<IRStatement> Builder::emit_pow() {
  return emit(Opcode::pow);
}

ref<IRStatement> Builder::emit_usub() {
  return emit(Opcode::usub);
}

// arithmetic operations
ref<IRStatement> Builder::emit_eq() {
  return emit(Opcode::eq);
}

ref<IRStatement> Builder::emit_neq() {
  return emit(Opcode::neq);
}

ref<IRStatement> Builder::emit_lt() {
  return emit(Opcode::lt);
}

ref<IRStatement> Builder::emit_gt() {
  return emit(Opcode::gt);
}

ref<IRStatement> Builder::emit_le() {
  return emit(Opcode::le);
}

ref<IRStatement> Builder::emit_ge() {
  return emit(Opcode::ge);
}

ref<IRStatement> Builder::emit_unot() {
  return emit(Opcode::unot);
}

// bitwise operations
ref<IRStatement> Builder::emit_shl() {
  return emit(Opcode::shl);
}

ref<IRStatement> Builder::emit_shr() {
  return emit(Opcode::shr);
}

ref<IRStatement> Builder::emit_shru() {
  return emit(Opcode::shru);
}

ref<IRStatement> Builder::emit_band() {
  return emit(Opcode::band);
}

ref<IRStatement> Builder::emit_bor() {
  return emit(Opcode::bor);
}

ref<IRStatement> Builder::emit_bxor() {
  return emit(Opcode::bxor);
}

ref<IRStatement> Builder::emit_ubnot() {
  return emit(Opcode::ubnot);
}

uint32_t Builder::maximum_stack_height() const {
  return m_maximum_stack_height;
};

void Builder::reset_stack_height() {
  m_maximum_stack_height = 0;
  assert(m_current_stack_height == 0);
}

void Builder::update_stack(int32_t amount) {
  if (amount < 0) {
    assert((uint32_t)-amount <= m_current_stack_height);
  }

  // std::cout << " stack " << (int)m_current_stack_height << " += " << amount << std::endl;
  m_current_stack_height += amount;

  if (m_current_stack_height > m_maximum_stack_height) {
    m_maximum_stack_height = m_current_stack_height;
  }
}

}  // namespace charly::core::compiler::ir
