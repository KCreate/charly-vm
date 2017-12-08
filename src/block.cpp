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

#include "block.h"
#include "label.h"

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
  if (this->staticdata_writeoffset >= this->staticdata_size - size) {
    size_t newsize = this->staticdata_size * kBlockTextDataGrowthFactor;
    while (newsize < this->staticdata_size + size)
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
  this->check_text_needs_resize(data.size());
  memcpy(this->staticdata + this->staticdata_writeoffset, data.c_str(), data.size());

  uint32_t old_offset = this->staticdata_writeoffset;
  this->staticdata_writeoffset += data.size();
  return old_offset;
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

void InstructionBlock::write_putself() {
  this->write_byte(Opcode::PutSelf);
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

OffsetLabel InstructionBlock::write_putfunction(VALUE symbol, int32_t body_offset, bool anonymous, uint32_t argc) {
  OffsetLabel label(*this);
  label.set_instruction_base();

  this->write_byte(Opcode::PutFunction);
  this->write_long(symbol);

  label.set_target_offset();
  this->write_int(body_offset);

  this->write_byte(anonymous);
  this->write_int(argc);

  return label;
}

PointerLabel InstructionBlock::write_putcfunction(VALUE symbol, FPOINTER funcptr, uint32_t argc) {
  PointerLabel label(*this);
  label.set_instruction_base();

  this->write_byte(Opcode::PutCFunction);
  this->write_long(symbol);

  label.set_target_offset();

  this->write_pointer(funcptr);
  this->write_int(argc);

  return label;
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
                                      uint32_t parentclasscount) {
  this->write_byte(Opcode::PutClass);
  this->write_long(symbol);
  this->write_int(propertycount);
  this->write_int(staticpropertycount);
  this->write_int(methodcount);
  this->write_int(staticmethodcount);
  this->write_int(parentclasscount);
}

void InstructionBlock::write_makeconstant(uint32_t index) {
  this->write_byte(Opcode::MakeConstant);
  this->write_int(index);
}

void InstructionBlock::write_pop(uint32_t count) {
  this->write_byte(Opcode::Pop);
  this->write_int(count);
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

void InstructionBlock::write_throw(ThrowType type) {
  this->write_byte(Opcode::Throw);
  this->write_byte(type);
}

OffsetLabel InstructionBlock::write_registercatchtable(ThrowType type, int32_t offset) {
  OffsetLabel label(*this);
  label.set_instruction_base();

  this->write_byte(Opcode::RegisterCatchTable);
  this->write_byte(type);

  label.set_target_offset();

  this->write_int(offset);

  return label;
}

void InstructionBlock::write_popcatchtable() {
  this->write_byte(Opcode::PopCatchTable);
}

OffsetLabel InstructionBlock::write_branch(int32_t offset) {
  OffsetLabel label(*this);
  label.set_instruction_base();
  this->write_byte(Opcode::Branch);
  label.set_target_offset();
  this->write_int(offset);
  return label;
}

OffsetLabel InstructionBlock::write_branchif(int32_t offset) {
  OffsetLabel label(*this);
  label.set_instruction_base();
  this->write_byte(Opcode::BranchIf);
  label.set_target_offset();
  this->write_int(offset);
  return label;
}

OffsetLabel InstructionBlock::write_branchunless(int32_t offset) {
  OffsetLabel label(*this);
  label.set_instruction_base();
  this->write_byte(Opcode::BranchUnless);
  label.set_target_offset();
  this->write_int(offset);
  return label;
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

void InstructionBlock::write_if_statement(IBlockGenFunc condition, IBlockGenFunc then_block) {
  condition(*this);
  OffsetLabel skiplabel = this->write_branchunless(0);
  then_block(*this);
  skiplabel.write_current_block_offset();
}

void InstructionBlock::write_ifelse_statement(IBlockGenFunc condition,
                                              IBlockGenFunc then_block,
                                              IBlockGenFunc else_block) {
  condition(*this);
  OffsetLabel skiplabel = this->write_branchunless(0);
  then_block(*this);
  OffsetLabel endlabel = this->write_branch(0);
  skiplabel.write_current_block_offset();
  else_block(*this);
  endlabel.write_current_block_offset();
}

void InstructionBlock::write_unless_statement(IBlockGenFunc condition, IBlockGenFunc then_block) {
  condition(*this);
  OffsetLabel skiplabel = this->write_branchif(0);
  then_block(*this);
  skiplabel.write_current_block_offset();
}

void InstructionBlock::write_unlesselse_statement(IBlockGenFunc condition,
                                                  IBlockGenFunc then_block,
                                                  IBlockGenFunc else_block) {
  condition(*this);
  OffsetLabel skiplabel = this->write_branchif(0);
  then_block(*this);
  OffsetLabel endlabel = this->write_branch(0);
  skiplabel.write_current_block_offset();
  else_block(*this);
  endlabel.write_current_block_offset();
}

void InstructionBlock::write_try_statement(IBlockGenFunc block, IBlockGenFunc handler) {
  OffsetLabel handlerlabel = this->write_registercatchtable(ThrowType::Exception, 0);
  block(*this);
  this->write_popcatchtable();
  OffsetLabel skiphandlerlabel = this->write_branch(0);
  handlerlabel.write_current_block_offset();
  this->write_popcatchtable();
  handler(*this);
  skiphandlerlabel.write_current_block_offset();
}

void InstructionBlock::write_while_statement(IBlockGenFunc condition, IBlockGenFunc then_block) {
  OffsetLabel end_target_1 = this->write_registercatchtable(ThrowType::Break, 0);
  this->write_registercatchtable(ThrowType::Continue, kInstructionLengths[Opcode::RegisterCatchTable]);

  OffsetLabel lcondition(*this);
  condition(*this);
  OffsetLabel end_target_2 = this->write_branchunless(0);
  then_block(*this);
  this->write_branch(lcondition.relative_offset());

  end_target_1.write_current_block_offset();
  end_target_2.write_current_block_offset();
  this->write_popcatchtable();
}

void InstructionBlock::write_until_statement(IBlockGenFunc condition, IBlockGenFunc then_block) {
  OffsetLabel end_target_1 = this->write_registercatchtable(ThrowType::Break, 0);
  this->write_registercatchtable(ThrowType::Continue, kInstructionLengths[Opcode::RegisterCatchTable]);

  OffsetLabel lcondition(*this);
  condition(*this);
  OffsetLabel end_target_2 = this->write_branchif(0);
  then_block(*this);
  this->write_branch(lcondition.relative_offset());

  end_target_1.write_current_block_offset();
  end_target_2.write_current_block_offset();
  this->write_popcatchtable();
}

void InstructionBlock::write_loop_statement(IBlockGenFunc then_block) {
  OffsetLabel end_target_1 = this->write_registercatchtable(ThrowType::Break, 0);
  this->write_registercatchtable(ThrowType::Continue, kInstructionLengths[Opcode::RegisterCatchTable]);

  OffsetLabel loopstart(*this);
  then_block(*this);
  this->write_branch(loopstart.relative_offset());

  end_target_1.write_current_block_offset();
  this->write_popcatchtable();
}

void InstructionBlock::write_function_literal(VALUE symbol, bool anonymous, uint32_t argc, IBlockGenFunc body) {
  int32_t jump_length = kInstructionLengths[Opcode::PutFunction] + kInstructionLengths[Opcode::Branch];
  this->write_putfunction(symbol, jump_length, anonymous, argc);
  OffsetLabel end_body_target = this->write_branch(0);
  body(*this);
  end_body_target.write_current_block_offset();
}

}  // namespace Charly
