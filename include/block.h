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

#include <cstring>
#include <vector>

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

  char* data;
  uint32_t data_size;
  uint32_t writeoffset;

  // Keeps track of static data
  char* staticdata;
  uint32_t staticdata_size;
  uint32_t staticdata_writeoffset;

  InstructionBlock() {
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
    this->data_size = other.data_size;
    this->writeoffset = other.writeoffset;
    this->staticdata_size = other.staticdata_size;
    this->staticdata_writeoffset = other.staticdata_writeoffset;
  }

  virtual ~InstructionBlock() {
    this->clean();
  }

  inline virtual void reset() {
    std::memset(this->data, 0, this->data_size);
    this->writeoffset = 0;
    std::memset(this->staticdata, 0, this->staticdata_size);
    this->staticdata_writeoffset = 0;
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

  // Interface to code and staticdata buffers
  inline bool& bool_at(uint32_t offset) {
    return *reinterpret_cast<bool*>(this->data + offset);
  }
  inline double& double_at(uint32_t offset) {
    return *reinterpret_cast<double*>(this->data + offset);
  }
  inline uint8_t& uint8_at(uint32_t offset) {
    return *reinterpret_cast<uint8_t*>(this->data + offset);
  }
  inline uint16_t& uint16_at(uint32_t offset) {
    return *reinterpret_cast<uint16_t*>(this->data + offset);
  }
  inline uint32_t& uint32_at(uint32_t offset) {
    return *reinterpret_cast<uint32_t*>(this->data + offset);
  }
  inline uint64_t& uint64_at(uint32_t offset) {
    return *reinterpret_cast<uint64_t*>(this->data + offset);
  }
  inline int8_t& int8_at(uint32_t offset) {
    return *reinterpret_cast<int8_t*>(this->data + offset);
  }
  inline int16_t& int16_at(uint32_t offset) {
    return *reinterpret_cast<int16_t*>(this->data + offset);
  }
  inline int32_t& int32_at(uint32_t offset) {
    return *reinterpret_cast<int32_t*>(this->data + offset);
  }
  inline int64_t& int64_at(uint32_t offset) {
    return *reinterpret_cast<int64_t*>(this->data + offset);
  }
  inline void*& voidptr_at(uint32_t offset) {
    return *reinterpret_cast<void**>(this->data + offset);
  }
  inline VALUE& value_at(uint32_t offset) {
    return *reinterpret_cast<VALUE*>(this->data + offset);
  }

  // Wrappers around the write functions which can encode instructions
  void write_readlocal(uint32_t index, uint32_t level);
  void write_readmembersymbol(VALUE symbol);
  void write_readmembervalue();
  void write_readarrayindex(uint32_t index);
  void write_setlocal(uint32_t index, uint32_t level);
  void write_setmembersymbol(VALUE symbol);
  void write_setmembervalue();
  void write_setarrayindex(uint32_t index);
  void write_putself();
  void write_putvalue(VALUE value);
  void write_putfloat(double value);
  void write_putstring(const std::string& data);
  void write_putstring(uint32_t offset, uint32_t length);
  void write_putfunction(VALUE symbol, int32_t body_offset, bool anonymous, uint32_t argc, uint32_t lvarcount);
  void write_putcfunction(VALUE symbol, FPOINTER funcptr, uint32_t argc);
  void write_putarray(uint32_t count);
  void write_puthash(uint32_t count);
  void write_putclass(VALUE symbol,
                      uint32_t propertycount,
                      uint32_t staticpropertycount,
                      uint32_t methodcount,
                      uint32_t staticmethodcount,
                      uint32_t parentclasscount,
                      bool has_constructor);
  void write_pop();
  void write_dup();
  void write_swap();
  void write_topn(uint32_t index);
  void write_setn(uint32_t index);
  void write_call(uint32_t argc);
  void write_callmember(uint32_t argc);
  void write_return();
  void write_throw();
  void write_registercatchtable(int32_t offset);
  void write_popcatchtable();
  void write_branch(int32_t offset);
  void write_branchif(int32_t offset);
  void write_branchunless(int32_t offset);
  void write_operator(Opcode opcode);
  void write_halt();
  void write_gccollect();
  void write_typeof();
};
}  // namespace Charly
