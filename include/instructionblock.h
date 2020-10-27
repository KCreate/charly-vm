/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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

#include <cstdint>

#include "memoryblock.h"
#include "opcode.h"

#pragma once

namespace Charly {
class InstructionBlock : public MemoryBlock {
public:
  inline void write_nop() {
    this->write(Opcode::Nop);
  }

  inline void write_readlocal(uint32_t index, uint32_t level) {
    this->write(Opcode::ReadLocal);
    this->write(index);
    this->write(level);
  }

  inline void write_readmembersymbol(VALUE symbol) {
    this->write(Opcode::ReadMemberSymbol);
    this->write(symbol);
  }

  inline void write_readmembervalue() {
    this->write(Opcode::ReadMemberValue);
  }

  inline void write_readarrayindex(uint32_t index) {
    this->write(Opcode::ReadArrayIndex);
    this->write(index);
  }

  inline void write_readglobal(VALUE symbol) {
    this->write(Opcode::ReadGlobal);
    this->write(symbol);
  }

  inline void write_setlocal(uint32_t index, uint32_t level) {
    this->write(Opcode::SetLocal);
    this->write(index);
    this->write(level);
  }

  inline void write_setmembersymbol(VALUE symbol) {
    this->write(Opcode::SetMemberSymbol);
    this->write(symbol);
  }

  inline void write_setmembervalue() {
    this->write(Opcode::SetMemberValue);
  }

  inline void write_setglobal(VALUE symbol) {
    this->write(Opcode::SetGlobal);
    this->write(symbol);
  }

  inline void write_setglobalpush(VALUE symbol) {
    this->write(Opcode::SetGlobalPush);
    this->write(symbol);
  }

  inline void write_setarrayindex(uint32_t index) {
    this->write(Opcode::SetArrayIndex);
    this->write(index);
  }

  inline void write_setlocalpush(uint32_t index, uint32_t level) {
    this->write(Opcode::SetLocalPush);
    this->write(index);
    this->write(level);
  }

  inline void write_setmembersymbolpush(VALUE symbol) {
    this->write(Opcode::SetMemberSymbolPush);
    this->write(symbol);
  }

  inline void write_setmembervaluepush() {
    this->write(Opcode::SetMemberValuePush);
  }

  inline void write_setarrayindexpush(uint32_t index) {
    this->write(Opcode::SetArrayIndexPush);
    this->write(index);
  }

  inline void write_putself() {
    this->write(Opcode::PutSelf);
  }

  inline void write_putvalue(VALUE value) {
    this->write(Opcode::PutValue);
    this->write(value);
  }

  inline void write_putstring(uint32_t offset, uint32_t length) {
    this->write(Opcode::PutString);
    this->write(offset);
    this->write(length);
  }

  inline void write_putfunction(VALUE symbol,
                                int32_t body_offset,
                                bool anonymous,
                                bool needs_arguments,
                                uint32_t argc,
                                uint32_t lvarcount) {
    this->write(Opcode::PutFunction);
    this->write(symbol);
    this->write(body_offset);
    this->write(anonymous);
    this->write(needs_arguments);
    this->write(argc);
    this->write(lvarcount);
  }

  inline void write_putarray(uint32_t count) {
    this->write(Opcode::PutArray);
    this->write(count);
  }

  inline void write_puthash(uint32_t count) {
    this->write(Opcode::PutHash);
    this->write(count);
  }

  inline void write_putclass(VALUE symbol,
                             uint32_t propertycount,
                             uint32_t staticpropertycount,
                             uint32_t methodcount,
                             uint32_t staticmethodcount,
                             bool has_parent_class,
                             bool has_constructor) {
    this->write(Opcode::PutClass);
    this->write(symbol);
    this->write(propertycount);
    this->write(staticpropertycount);
    this->write(methodcount);
    this->write(staticmethodcount);
    this->write(has_parent_class);
    this->write(has_constructor);
  }

  inline void write_pop() {
    this->write(Opcode::Pop);
  }

  inline void write_dup() {
    this->write(Opcode::Dup);
  }

  inline void write_dupn(uint32_t count) {
    this->write(Opcode::Dupn);
    this->write(count);
  }

  inline void write_swap() {
    this->write(Opcode::Swap);
  }

  inline void write_call(uint32_t argc) {
    this->write(Opcode::Call);
    this->write(argc);
  }

  inline void write_callmember(uint32_t argc) {
    this->write(Opcode::CallMember);
    this->write(argc);
  }

  inline void write_putsuper() {
    this->write(Opcode::PutSuper);
  }

  inline void write_putsupermember(VALUE symbol) {
    this->write(Opcode::PutSuperMember);
    this->write(symbol);
  }

  inline void write_new(uint32_t argc) {
    this->write(Opcode::New);
    this->write(argc);
  }

  inline void write_return() {
    this->write(Opcode::Return);
  }

  inline void write_yield() {
    this->write(Opcode::Yield);
  }

  inline void write_throw() {
    this->write(Opcode::Throw);
  }

  inline void write_registercatchtable(int32_t offset) {
    this->write(Opcode::RegisterCatchTable);
    this->write(offset);
  }

  inline void write_popcatchtable() {
    this->write(Opcode::PopCatchTable);
  }

  inline void write_branch(int32_t offset) {
    this->write(Opcode::Branch);
    this->write(offset);
  }

  inline void write_branchif(int32_t offset) {
    this->write(Opcode::BranchIf);
    this->write(offset);
  }

  inline void write_branchunless(int32_t offset) {
    this->write(Opcode::BranchUnless);
    this->write(offset);
  }

  inline void write_branchlt(int32_t offset) {
    this->write(Opcode::BranchLt);
    this->write(offset);
  }

  inline void write_branchgt(int32_t offset) {
    this->write(Opcode::BranchGt);
    this->write(offset);
  }

  inline void write_branchle(int32_t offset) {
    this->write(Opcode::BranchLe);
    this->write(offset);
  }

  inline void write_branchge(int32_t offset) {
    this->write(Opcode::BranchGe);
    this->write(offset);
  }

  inline void write_brancheq(int32_t offset) {
    this->write(Opcode::BranchEq);
    this->write(offset);
  }

  inline void write_branchneq(int32_t offset) {
    this->write(Opcode::BranchNeq);
    this->write(offset);
  }

  inline void write_operator(uint8_t opcode) {
    this->write(opcode);
  }

  inline void write_halt() {
    this->write(Opcode::Halt);
  }

  inline void write_typeof() {
    this->write(Opcode::Typeof);
  }
};
}  // namespace Charly
