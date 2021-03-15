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

IRFunction& Builder::begin_function(Label head, ast::ref<ast::Function> ast) {
  m_module->functions.emplace_back(head, ast);
  m_active_function = &m_module->functions.back();
  return active_function();
}

Label Builder::reserve_label() {
  return m_label_counter++;
}

Label Builder::label() {
  Label l = reserve_label();
  place_label(l);
  return l;
}

void Builder::place_label(Label label) {
  emit_label_definition(label);
}

void Builder::place_label_at_label(Label label, Label target_label) {

  // search for the target label
  IRFunction& func = active_function();
  for (auto it = func.statements.begin(); it != func.statements.end(); it++) {
    const std::shared_ptr<IRStatement>& ptr = *it;

    if (ptr->get_type() == IRStatement::Type::LabelDefinition) {
      const IRLabelDefinition* label_def = dynamic_cast<IRLabelDefinition*>(ptr.get());
      if (label_def->label == target_label) {
        func.statements.insert(it, std::make_shared<IRLabelDefinition>(label));
        return;
      }
    }
  }

  assert(false && "unknown label");
}

void Builder::emit_string_data(const std::string& string) {
  IRFunction& func = active_function();
  func.statements.push_back(std::make_shared<IRStringData>(string));
}

void Builder::emit_label_definition(Label label) {
  IRFunction& func = active_function();
  func.statements.push_back(std::make_shared<IRLabelDefinition>(label));
}

// machine control
void Builder::emit_nop() {
  emit(Opcode::nop);
}

void Builder::emit_halt() {
  emit(Opcode::halt);
}

// misc. instructions
void Builder::emit_import() {
  emit(Opcode::import);
}

void Builder::emit_stringconcat(OpCount8 count) {
  emit(Opcode::stringconcat, IROperandCount8::make(count));
}

void Builder::emit_declareglobal(OpSymbol symbol) {
  emit(Opcode::declareglobal, IROperandSymbol::make(symbol));
}

void Builder::emit_type() {
  emit(Opcode::type);
}

// stack management
void Builder::emit_pop() {
  emit(Opcode::pop);
}

void Builder::emit_dup() {
  emit(Opcode::dup);
}

void Builder::emit_swap() {
  emit(Opcode::swap);
}

// control flow
void Builder::emit_jmp(OpOffset label) {
  emit(Opcode::jmp, IROperandOffset::make(label));
}

void Builder::emit_jmpf(OpOffset label) {
  emit(Opcode::jmpf, IROperandOffset::make(label));
}

void Builder::emit_jmpt(OpOffset label) {
  emit(Opcode::jmpt, IROperandOffset::make(label));
}

void Builder::emit_throwex() {
  emit(Opcode::throwex);
}

// function control flow
void Builder::emit_call(OpCount8 count) {
  emit(Opcode::call, IROperandCount8::make(count));
}

void Builder::emit_callspread(OpCount8 count) {
  emit(Opcode::callspread, IROperandCount8::make(count));
}

void Builder::emit_ret() {
  emit(Opcode::ret);
}

// load operations
void Builder::emit_load(OpImmediate value) {
  emit(Opcode::load, IROperandImmediate::make(value));
}

void Builder::emit_loadsymbol(OpSymbol symbol) {
  emit(Opcode::loadsymbol, IROperandSymbol::make(symbol));
}

void Builder::emit_loadcontextself() {
  emit(Opcode::loadcontextself);
}

void Builder::emit_loadglobal(OpSymbol symbol) {
  emit(Opcode::loadglobal, IROperandSymbol::make(symbol));
  register_symbol(symbol);
}

void Builder::emit_loadlocal(OpCount8 offset) {
  emit(Opcode::loadlocal, IROperandCount8::make(offset));
}

void Builder::emit_loadfar(OpCount8 depth, OpCount8 offset) {
  emit(Opcode::loadfar, IROperandCount8::make(depth), IROperandCount8::make(offset));
}

void Builder::emit_loadattr(OpSymbol symbol) {
  emit(Opcode::loadattr, IROperandSymbol::make(symbol));
  register_symbol(symbol);
}

void Builder::emit_loadattrvalue() {
  emit(Opcode::loadattrvalue);
}

void Builder::emit_loadsuper() {
  emit(Opcode::loadsuper);
}

void Builder::emit_loadsuperattr(OpSymbol symbol) {
  emit(Opcode::loadsuperattr, IROperandSymbol::make(symbol));
  register_symbol(symbol);
}

// write operations
void Builder::emit_setglobal(OpSymbol symbol) {
  emit(Opcode::setglobal, IROperandSymbol::make(symbol));
  register_symbol(symbol);
}

void Builder::emit_setlocal(OpCount8 offset) {
  emit(Opcode::setlocal, IROperandCount8::make(offset));
}

void Builder::emit_setfar(OpCount8 depth, OpCount8 offset) {
  emit(Opcode::setfar, IROperandCount8::make(depth), IROperandCount8::make(offset));
}

void Builder::emit_setattr(OpSymbol symbol) {
  emit(Opcode::setattr, IROperandSymbol::make(symbol));
  register_symbol(symbol);
}

void Builder::emit_setattrvalue() {
  emit(Opcode::setattrvalue);
}

// value destructuring operations
void Builder::emit_unpacksequence(OpCount8 count) {
  emit(Opcode::unpacksequence, IROperandCount8::make(count));
}

void Builder::emit_unpacksequencespread(OpCount8 before, OpCount8 after) {
  emit(Opcode::unpacksequencespread, IROperandCount8::make(before), IROperandCount8::make(after));
}

void Builder::emit_unpackobject(OpCount8 count) {
  emit(Opcode::unpackobject, IROperandCount8::make(count));
}

void Builder::emit_unpackobjectspread(OpCount8 count) {
  emit(Opcode::unpackobjectspread, IROperandCount8::make(count));
}

// value allocation
void Builder::emit_makefunc(OpOffset offset) {
  emit(Opcode::makefunc, IROperandOffset::make(offset));
}

void Builder::emit_makeclass(OpOffset offset) {
  emit(Opcode::makeclass, IROperandOffset::make(offset));
}

void Builder::emit_makestr(OpOffset offset) {
  emit(Opcode::makestr, IROperandOffset::make(offset));
}

void Builder::emit_makelist(OpCount16 count) {
  emit(Opcode::makelist, IROperandCount16::make(count));
}

void Builder::emit_makelistspread(OpCount16 count) {
  emit(Opcode::makelistspread, IROperandCount16::make(count));
}

void Builder::emit_makedict(OpCount16 count) {
  emit(Opcode::makedict, IROperandCount16::make(count));
}

void Builder::emit_makedictspread(OpCount16 count) {
  emit(Opcode::makedictspread, IROperandCount16::make(count));
}

void Builder::emit_maketuple(OpCount16 count) {
  emit(Opcode::maketuple, IROperandCount16::make(count));
}

void Builder::emit_maketuplespread(OpCount16 count) {
  emit(Opcode::maketuplespread, IROperandCount16::make(count));
}

// fiber management
void Builder::emit_fibercreate() {
  emit(Opcode::fibercreate);
}

void Builder::emit_fiberspawn() {
  emit(Opcode::fiberspawn);
}

void Builder::emit_fiberyield() {
  emit(Opcode::fiberyield);
}

void Builder::emit_fiberawait() {
  emit(Opcode::fiberawait);
}

// cast operations
void Builder::emit_caststring() {
  emit(Opcode::caststring);
}

// iterator operations
void Builder::emit_iteratornext() {
  emit(Opcode::iteratornext);
}

// arithmetic operations
void Builder::emit_add() {
  emit(Opcode::add);
}

void Builder::emit_sub() {
  emit(Opcode::sub);
}

void Builder::emit_mul() {
  emit(Opcode::mul);
}

void Builder::emit_div() {
  emit(Opcode::div);
}

void Builder::emit_mod() {
  emit(Opcode::mod);
}

void Builder::emit_pow() {
  emit(Opcode::pow);
}

void Builder::emit_usub() {
  emit(Opcode::usub);
}

// arithmetic operations
void Builder::emit_eq() {
  emit(Opcode::eq);
}

void Builder::emit_neq() {
  emit(Opcode::neq);
}

void Builder::emit_lt() {
  emit(Opcode::lt);
}

void Builder::emit_gt() {
  emit(Opcode::gt);
}

void Builder::emit_le() {
  emit(Opcode::le);
}

void Builder::emit_ge() {
  emit(Opcode::ge);
}

void Builder::emit_unot() {
  emit(Opcode::unot);
}

// bitwise operations
void Builder::emit_shl() {
  emit(Opcode::shl);
}

void Builder::emit_shr() {
  emit(Opcode::shr);
}

void Builder::emit_shru() {
  emit(Opcode::shru);
}

void Builder::emit_band() {
  emit(Opcode::band);
}

void Builder::emit_bor() {
  emit(Opcode::bor);
}

void Builder::emit_bxor() {
  emit(Opcode::bxor);
}

void Builder::emit_ubnot() {
  emit(Opcode::ubnot);
}

}  // namespace charly::core::compiler::ir
