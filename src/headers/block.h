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

#include <vector>

#include "defines.h"
#include "opcode.h"

#pragma once

namespace Charly {
class InstructionBlock {
public:
  VALUE flags;  // Needed by GC

  static const uint32_t kBlockInitialWriteOffset = 0;
  static const uint32_t kBlockInitialBlockSize = 256;
  static const uint32_t kBlockSizeGrowthFactor = 2;
  static const uint32_t kBlockInitialTextSize = 32;
  static const uint32_t kBlockTextDataGrowthFactor = 2;

  uint32_t lvarcount;
  uint8_t* data;
  uint32_t data_size;
  uint32_t writeoffset;
  std::vector<InstructionBlock*> child_blocks;

  // Keeps track of TEXT data
  char* textdata;
  uint32_t textdata_size;
  uint32_t textdata_writeoffset;

  InstructionBlock(uint32_t lvarcount) : lvarcount(lvarcount) {
    this->data = (uint8_t*)calloc(kBlockInitialBlockSize, sizeof(uint8_t));
    this->data_size = kBlockInitialBlockSize * sizeof(uint8_t);
    this->writeoffset = kBlockInitialWriteOffset;

    this->textdata = new char[kBlockInitialTextSize];
    this->textdata_size = kBlockInitialTextSize;
    this->textdata_writeoffset = 0;
  }

  InstructionBlock(const InstructionBlock& other) {
    // Copy the data and text buffers of the other block
    uint8_t* data_copy = (uint8_t*)malloc(other.data_size);
    char* textdata_copy = (char*)malloc(other.textdata_size);
    memcpy((char*)data_copy, (char*)other.data, other.data_size);
    memcpy(textdata_copy, other.textdata, other.textdata_size);
    this->data = data_copy;
    this->textdata = textdata_copy;

    // Copy trivial variables
    this->lvarcount = other.lvarcount;
    this->data_size = other.data_size;
    this->writeoffset = other.writeoffset;
    this->textdata_size = other.textdata_size;
    this->textdata_writeoffset = other.textdata_writeoffset;

    // Recursively copy all child blocks
    for (const auto& block : other.child_blocks) {
      InstructionBlock* block_copy = new InstructionBlock(*block);
      this->child_blocks.push_back(block_copy);
    }
  }

  ~InstructionBlock() {
    this->clean();
  }

  void inline clean() {
    free(this->data);
    free(this->textdata);
  }

  void inline check_needs_resize() {
    if (this->writeoffset >= this->data_size - sizeof(uint64_t)) {
      this->grow();
    }
  }

  void inline check_text_needs_resize(size_t size) {
    if (this->textdata_writeoffset >= this->textdata_size - size) {
      size_t newsize = this->textdata_size * kBlockTextDataGrowthFactor;
      while (newsize < this->textdata_size + size)
        newsize *= kBlockTextDataGrowthFactor;

      this->textdata = (char*)realloc(this->textdata, newsize);
      this->textdata_size = newsize;
    }
  }

  void inline grow() {
    this->data = (uint8_t*)realloc(this->data, this->data_size * kBlockSizeGrowthFactor);
    this->data_size *= kBlockSizeGrowthFactor;
  }

  void inline write_byte(uint8_t val) {
    this->check_needs_resize();
    *(uint8_t*)(this->data + this->writeoffset) = val;
    this->writeoffset += sizeof(uint8_t);
  }

  void inline write_bool(bool val) {
    this->check_needs_resize();
    *(bool*)(this->data + this->writeoffset) = val;
    this->writeoffset += sizeof(bool);
  }

  void inline write_short(uint16_t val) {
    this->check_needs_resize();
    *(uint16_t*)(this->data + this->writeoffset) = val;
    this->writeoffset += sizeof(uint16_t);
  }

  void inline write_int(uint32_t val) {
    this->check_needs_resize();
    *(uint32_t*)(this->data + this->writeoffset) = val;
    this->writeoffset += sizeof(uint32_t);
  }

  void inline write_long(uint64_t val) {
    this->check_needs_resize();
    *(uint64_t*)(this->data + this->writeoffset) = val;
    this->writeoffset += sizeof(uint64_t);
  }

  void inline write_pointer(void* val) {
    this->check_needs_resize();
    *(uint8_t**)(this->data + this->writeoffset) = (uint8_t*)val;
    this->writeoffset += sizeof(void*);
  }

  void inline write_double(double val) {
    this->check_needs_resize();
    *(double*)(this->data + this->writeoffset) = val;
    this->writeoffset += sizeof(double);
  }

  uint32_t inline write_string(const std::string& data) {
    this->check_text_needs_resize(data.size());
    memcpy((char*)(this->textdata + this->textdata_writeoffset), data.c_str(), data.size());

    uint32_t old_offset = this->textdata_writeoffset;
    this->textdata_writeoffset += data.size();
    return old_offset;
  }

  void inline write_readlocal(uint32_t index, uint32_t level) {
    this->write_byte(Opcode::ReadLocal);
    this->write_int(index);
    this->write_int(level);
  }

  void inline write_readmembersymbol(VALUE symbol) {
    this->write_byte(Opcode::ReadMemberSymbol);
    this->write_long(symbol);
  }

  void inline write_readmembervalue() {
    this->write_byte(Opcode::ReadMemberValue);
  }

  void inline write_setlocal(uint32_t index, uint32_t level) {
    this->write_byte(Opcode::SetLocal);
    this->write_int(index);
    this->write_int(level);
  }

  void inline write_setmembersymbol(VALUE symbol) {
    this->write_byte(Opcode::SetMemberSymbol);
    this->write_long(symbol);
  }

  void inline write_setmembervalue() {
    this->write_byte(Opcode::SetMemberValue);
  }

  void inline write_putself() {
    this->write_byte(Opcode::PutSelf);
  }

  void inline write_putvalue(VALUE value) {
    this->write_byte(Opcode::PutValue);
    this->write_long(value);
  }

  void inline write_putfloat(double value) {
    this->write_byte(Opcode::PutFloat);
    this->write_double(value);
  }

  void inline write_putstring(const std::string& data) {
    this->write_byte(Opcode::PutString);
    uint32_t offset = this->write_string(data);
    this->write_int(offset);
    this->write_int(data.size());
  }

  void inline write_putfunction(VALUE symbol, InstructionBlock* block, bool anonymous, uint32_t argc) {
    this->write_byte(Opcode::PutFunction);
    this->write_long(symbol);
    this->write_pointer(block);
    this->child_blocks.push_back(block);
    this->write_byte(anonymous);
    this->write_int(argc);
  }

  void inline write_putcfunction(VALUE symbol, void* funcptr, uint32_t argc) {
    this->write_byte(Opcode::PutCFunction);
    this->write_long(symbol);
    this->write_pointer(funcptr);
    this->write_int(argc);
  }

  void inline write_putarray(uint32_t count) {
    this->write_byte(Opcode::PutArray);
    this->write_int(count);
  }

  void inline write_puthash(uint32_t count) {
    this->write_byte(Opcode::PutHash);
    this->write_int(count);
  }

  void inline write_putclass(VALUE symbol,
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

  void inline write_makeconstant(uint32_t index) {
    this->write_byte(Opcode::MakeConstant);
    this->write_int(index);
  }

  void inline write_pop(uint32_t count) {
    this->write_byte(Opcode::Pop);
    this->write_int(count);
  }

  void inline write_dup() {
    this->write_byte(Opcode::Dup);
  }

  void inline write_swap() {
    this->write_byte(Opcode::Swap);
  }

  void inline write_topn(uint32_t index) {
    this->write_byte(Opcode::Topn);
    this->write_int(index);
  }

  void inline write_setn(uint32_t index) {
    this->write_byte(Opcode::Setn);
    this->write_int(index);
  }

  void inline write_call(uint32_t argc) {
    this->write_byte(Opcode::Call);
    this->write_int(argc);
  }

  void inline write_callmember(uint32_t argc) {
    this->write_byte(Opcode::CallMember);
    this->write_int(argc);
  }

  void inline write_return() {
    this->write_byte(Opcode::Return);
  }

  void inline write_throw(ThrowType type) {
    this->write_byte(Opcode::Throw);
    this->write_byte(type);
  }

  void inline write_registercatchtable(ThrowType type, int32_t offset) {
    this->write_byte(Opcode::RegisterCatchTable);
    this->write_byte(type);
    this->write_int(offset);
  }

  void inline write_popcatchtable() {
    this->write_byte(Opcode::PopCatchTable);
  }

  void inline write_branch(int32_t offset) {
    this->write_byte(Opcode::Branch);
    this->write_int(offset);
  }

  void inline write_branchif(int32_t offset) {
    this->write_byte(Opcode::BranchIf);
    this->write_int(offset);
  }

  void inline write_branchunless(int32_t offset) {
    this->write_byte(Opcode::BranchUnless);
    this->write_int(offset);
  }

  void inline write_operator(Opcode opcode) {
    this->write_byte(opcode);
  }
};
};  // namespace Charly
