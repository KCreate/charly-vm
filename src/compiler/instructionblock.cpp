/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include "instructionblock.h"

namespace Charly {
void InstructionBlock::clean() {
  free(this->data);
  free(this->staticdata);
}

void InstructionBlock::check_needs_resize() {
  if (this->writeoffset >= this->data_size - sizeof(uint64_t)) {
    this->grow();
  }
}

void InstructionBlock::check_text_needs_resize(size_t size) {
  if (this->staticdata_writeoffset + size >= this->staticdata_size) {
    size_t newsize = this->staticdata_size * kBlockTextDataGrowthFactor;
    while (newsize < this->staticdata_writeoffset + size)
      newsize *= kBlockTextDataGrowthFactor;

    this->staticdata = static_cast<char*>(realloc(this->staticdata, newsize));
    this->staticdata_size = newsize;
  }
}

void InstructionBlock::grow() {
  this->data = static_cast<char*>(realloc(this->data, this->data_size * kBlockSizeGrowthFactor));
  this->data_size *= kBlockSizeGrowthFactor;
}

void InstructionBlock::write_byte(char val) {
  this->check_needs_resize();
  *reinterpret_cast<char*>(this->data + this->writeoffset) = val;
  this->writeoffset += sizeof(char);
}

void InstructionBlock::write_bool(bool val) {
  this->check_needs_resize();
  *reinterpret_cast<bool*>(this->data + this->writeoffset) = val;
  this->writeoffset += sizeof(bool);
}

void InstructionBlock::write_short(uint16_t val) {
  this->check_needs_resize();
  *reinterpret_cast<uint16_t*>(this->data + this->writeoffset) = val;
  this->writeoffset += sizeof(uint16_t);
}

void InstructionBlock::write_int(uint32_t val) {
  this->check_needs_resize();
  *reinterpret_cast<uint32_t*>(this->data + this->writeoffset) = val;
  this->writeoffset += sizeof(uint32_t);
}

void InstructionBlock::write_long(uint64_t val) {
  this->check_needs_resize();
  *reinterpret_cast<uint64_t*>(this->data + this->writeoffset) = val;
  this->writeoffset += sizeof(uint64_t);
}

void InstructionBlock::write_pointer(void* val) {
  this->check_needs_resize();
  *reinterpret_cast<void**>(this->data + this->writeoffset) = val;
  this->writeoffset += sizeof(void*);
}

void InstructionBlock::write_double(double val) {
  this->check_needs_resize();
  *reinterpret_cast<double*>(this->data + this->writeoffset) = val;
  this->writeoffset += sizeof(double);
}

uint32_t InstructionBlock::write_string(const std::string& data) {
  this->check_text_needs_resize(data.size() + 1);
  memcpy(this->staticdata + this->staticdata_writeoffset, data.c_str(), data.size() + 1);

  uint32_t old_offset = this->staticdata_writeoffset;
  this->staticdata_writeoffset += data.size() + 1;
  return old_offset;
}

void InstructionBlock::write_nop() {
  this->write_byte(Opcode::Nop);
}

void InstructionBlock::write_readlocal(uint32_t index, uint32_t level) {
  this->write_byte(Opcode::ReadLocal);
  this->write_int(index);
  this->write_int(level);
}

void InstructionBlock::write_readmembersymbol(VALUE symbol) {
  this->write_byte(Opcode::ReadMemberSymbol);
  this->write_long(symbol);
}

void InstructionBlock::write_readmembervalue() {
  this->write_byte(Opcode::ReadMemberValue);
}

void InstructionBlock::write_readarrayindex(uint32_t index) {
  this->write_byte(Opcode::ReadArrayIndex);
  this->write_int(index);
}

void InstructionBlock::write_setlocal(uint32_t index, uint32_t level) {
  this->write_byte(Opcode::SetLocal);
  this->write_int(index);
  this->write_int(level);
}

void InstructionBlock::write_setmembersymbol(VALUE symbol) {
  this->write_byte(Opcode::SetMemberSymbol);
  this->write_long(symbol);
}

void InstructionBlock::write_setmembervalue() {
  this->write_byte(Opcode::SetMemberValue);
}

void InstructionBlock::write_setarrayindex(uint32_t index) {
  this->write_byte(Opcode::SetArrayIndex);
  this->write_int(index);
}

void InstructionBlock::write_putself(uint32_t level) {
  this->write_byte(Opcode::PutSelf);
  this->write_int(level);
}

void InstructionBlock::write_putvalue(VALUE value) {
  this->write_byte(Opcode::PutValue);
  this->write_long(value);
}

void InstructionBlock::write_putfloat(double value) {
  this->write_byte(Opcode::PutFloat);
  this->write_double(value);
}

void InstructionBlock::write_putstring(const std::string& data) {
  this->write_byte(Opcode::PutString);
  uint32_t offset = this->write_string(data);
  this->write_int(offset);
  this->write_int(data.size());
}

void InstructionBlock::write_putstring(uint32_t offset, uint32_t length) {
  this->write_byte(Opcode::PutString);
  this->write_int(offset);
  this->write_int(length);
}

void InstructionBlock::write_putfunction(VALUE symbol,
                                         int32_t body_offset,
                                         bool anonymous,
                                         uint32_t argc,
                                         uint32_t lvarcount) {
  this->write_byte(Opcode::PutFunction);
  this->write_long(symbol);
  this->write_int(body_offset);
  this->write_byte(anonymous);
  this->write_int(argc);
  this->write_int(lvarcount);
}

void InstructionBlock::write_putcfunction(VALUE symbol, FPOINTER funcptr, uint32_t argc) {
  this->write_byte(Opcode::PutCFunction);
  this->write_long(symbol);
  this->write_pointer(funcptr);
  this->write_int(argc);
}

void InstructionBlock::write_putarray(uint32_t count) {
  this->write_byte(Opcode::PutArray);
  this->write_int(count);
}

void InstructionBlock::write_puthash(uint32_t count) {
  this->write_byte(Opcode::PutHash);
  this->write_int(count);
}

void InstructionBlock::write_putclass(VALUE symbol,
                                      uint32_t propertycount,
                                      uint32_t staticpropertycount,
                                      uint32_t methodcount,
                                      uint32_t staticmethodcount,
                                      uint32_t parentclasscount,
                                      bool has_constructor) {
  this->write_byte(Opcode::PutClass);
  this->write_long(symbol);
  this->write_int(propertycount);
  this->write_int(staticpropertycount);
  this->write_int(methodcount);
  this->write_int(staticmethodcount);
  this->write_int(parentclasscount);
  this->write_bool(has_constructor);
}

void InstructionBlock::write_pop() {
  this->write_byte(Opcode::Pop);
}

void InstructionBlock::write_dup() {
  this->write_byte(Opcode::Dup);
}

void InstructionBlock::write_swap() {
  this->write_byte(Opcode::Swap);
}

void InstructionBlock::write_topn(uint32_t index) {
  this->write_byte(Opcode::Topn);
  this->write_int(index);
}

void InstructionBlock::write_setn(uint32_t index) {
  this->write_byte(Opcode::Setn);
  this->write_int(index);
}

void InstructionBlock::write_call(uint32_t argc) {
  this->write_byte(Opcode::Call);
  this->write_int(argc);
}

void InstructionBlock::write_callmember(uint32_t argc) {
  this->write_byte(Opcode::CallMember);
  this->write_int(argc);
}

void InstructionBlock::write_return() {
  this->write_byte(Opcode::Return);
}

void InstructionBlock::write_throw() {
  this->write_byte(Opcode::Throw);
}

void InstructionBlock::write_registercatchtable(int32_t offset) {
  this->write_byte(Opcode::RegisterCatchTable);
  this->write_int(offset);
}

void InstructionBlock::write_popcatchtable() {
  this->write_byte(Opcode::PopCatchTable);
}

void InstructionBlock::write_branch(int32_t offset) {
  this->write_byte(Opcode::Branch);
  this->write_int(offset);
}

void InstructionBlock::write_branchif(int32_t offset) {
  this->write_byte(Opcode::BranchIf);
  this->write_int(offset);
}

void InstructionBlock::write_branchunless(int32_t offset) {
  this->write_byte(Opcode::BranchUnless);
  this->write_int(offset);
}

void InstructionBlock::write_operator(Opcode opcode) {
  this->write_byte(opcode);
}

void InstructionBlock::write_halt() {
  this->write_byte(Opcode::Halt);
}

void InstructionBlock::write_gccollect() {
  this->write_byte(Opcode::GCCollect);
}

void InstructionBlock::write_typeof() {
  this->write_byte(Opcode::Typeof);
}

}  // namespace Charly
