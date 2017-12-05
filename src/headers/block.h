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
  char* data;
  uint32_t data_size;
  uint32_t writeoffset;
  std::vector<InstructionBlock*> child_blocks;

  // Keeps track of static data
  char* staticdata;
  uint32_t staticdata_size;
  uint32_t staticdata_writeoffset;

  InstructionBlock(uint32_t lvarcount) : lvarcount(lvarcount) {
    this->data = static_cast<char*>(calloc(kBlockInitialBlockSize, sizeof(uint8_t)));
    this->data_size = kBlockInitialBlockSize * sizeof(uint8_t);
    this->writeoffset = kBlockInitialWriteOffset;
    this->staticdata = static_cast<char*>(malloc(sizeof(char) * kBlockInitialTextSize));
    this->staticdata_size = kBlockInitialTextSize;
    this->staticdata_writeoffset = 0;
  }

  InstructionBlock(const InstructionBlock& other) {
    // Copy the data and text buffers of the other block
    char* data_copy = static_cast<char*>(malloc(other.data_size));
    char* staticdata_copy = static_cast<char*>(malloc(other.staticdata_size));
    memcpy(data_copy, other.data, other.data_size);
    memcpy(staticdata_copy, other.staticdata, other.staticdata_size);
    this->data = data_copy;
    this->staticdata = staticdata_copy;

    // Copy trivial variables
    this->lvarcount = other.lvarcount;
    this->data_size = other.data_size;
    this->writeoffset = other.writeoffset;
    this->staticdata_size = other.staticdata_size;
    this->staticdata_writeoffset = other.staticdata_writeoffset;

    // Recursively copy all child blocks
    for (const auto& block : other.child_blocks) {
      InstructionBlock* block_copy = new InstructionBlock(*block);
      this->child_blocks.push_back(block_copy);
    }
  }

  ~InstructionBlock() {
    this->clean();
  }

  void clean();
  void check_needs_resize();
  void check_text_needs_resize(size_t size);
  void grow();

  // Methods which write directly into the instruction or staticdata buffer
  void write_byte(char val);
  void write_bool(bool val);
  void write_short(uint16_t val);
  void write_int(uint32_t val);
  void write_long(uint64_t val);
  void write_pointer(void* val);
  void write_double(double val);
  uint32_t write_string(const std::string& data);

  // Wrappers around the write functions which can encode instructions
  void write_readlocal(uint32_t index, uint32_t level);
  void write_readmembersymbol(VALUE symbol);
  void write_readmembervalue();
  void write_setlocal(uint32_t index, uint32_t level);
  void write_setmembersymbol(VALUE symbol);
  void write_setmembervalue();
  void write_putself();
  void write_putvalue(VALUE value);
  void write_putfloat(double value);
  void write_putstring(const std::string& data);
  IBlockPointerLabel write_putfunction(VALUE symbol, InstructionBlock* block, bool anonymous, uint32_t argc);
  PointerLabel write_putcfunction(VALUE symbol, FPOINTER funcptr, uint32_t argc);
  void write_putarray(uint32_t count);
  void write_puthash(uint32_t count);
  void write_putclass(VALUE symbol,
                      uint32_t propertycount,
                      uint32_t staticpropertycount,
                      uint32_t methodcount,
                      uint32_t staticmethodcount,
                      uint32_t parentclasscount);
  void write_makeconstant(uint32_t index);
  void write_pop(uint32_t count);
  void write_dup();
  void write_swap();
  void write_topn(uint32_t index);
  void write_setn(uint32_t index);
  void write_call(uint32_t argc);
  void write_callmember(uint32_t argc);
  void write_return();
  void write_throw(ThrowType type);
  Int32Label write_registercatchtable(ThrowType type, int32_t offset);
  void write_popcatchtable();
  Int32Label write_branch(int32_t offset);
  Int32Label write_branchif(int32_t offset);
  Int32Label write_branchunless(int32_t offset);
  void write_operator(Opcode opcode);

  // Wrappers for more high-level language structures
  void write_if_statement(IBlockGenFunc condition, IBlockGenFunc then_block);
  void write_ifelse_statement(IBlockGenFunc condition, IBlockGenFunc then_block, IBlockGenFunc else_block);
  void write_unless_statement(IBlockGenFunc condition, IBlockGenFunc then_block);
  void write_unlesselse_statement(IBlockGenFunc condition, IBlockGenFunc then_block, IBlockGenFunc else_block);
  void write_try_statement(IBlockGenFunc block, IBlockGenFunc handler);
  void write_while_statement(IBlockGenFunc condition, IBlockGenFunc then_block);
  void write_until_statement(IBlockGenFunc condition, IBlockGenFunc then_block);
};
}  // namespace Charly
