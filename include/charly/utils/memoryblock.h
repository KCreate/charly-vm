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

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#pragma once

namespace charly::utils {

class MemoryBlock {
public:
  MemoryBlock(size_t initial_capacity = kInitialCapacity) {
    this->reserve_space(initial_capacity);
  }

  MemoryBlock(const std::string& str) : MemoryBlock(str.size()) {
    this->write_string(str);
  }

  // copy constructor
  MemoryBlock(const MemoryBlock& other) {
    this->reserve_space(other.m_capacity);
    this->write_to(0, other.m_data, other.m_size);
  }

  // move constructor
  MemoryBlock(MemoryBlock&& other) :
    m_data(other.m_data), m_capacity(other.m_capacity), m_size(other.m_size), m_cursor(other.m_cursor) {
    other.m_data = nullptr;
    other.m_capacity = 0;
    other.m_size = 0;
    other.m_cursor = 0;
  }

  ~MemoryBlock() {
    if (m_data)
      std::free(m_data);

    m_data = nullptr;
    m_capacity = 0;
    m_size = 0;
    m_cursor = 0;
  }

  // move the write offset to some new offset
  void seek(size_t offset);

  // write value into buffer
  // clang-format off
  template <typename T>
  void write(T&& value) {
    write_block(&value, sizeof(T));
  }
  template <typename T>
  T* write_struct() {
    size_t size = sizeof(T);
    char* start = data();
    write_zeroes(size);
    return (T*)start;
  }
  void write_u8(uint8_t value) { write(value); }
  void write_u16(uint16_t value) { write(value); }
  void write_u32(uint32_t value) { write(value); }
  void write_u64(uint64_t value) { write(value); }
  void write_i8(int8_t value) { write(value); }
  void write_i16(int16_t value) { write(value); }
  void write_i32(int32_t value) { write(value); }
  void write_i64(int64_t value) { write(value); }
  void write_ptr(uintptr_t value) { write(value); }
  void write_ptr(void* value) { write(value); }
  // clang-format on

  // write size zeroes into the buffer
  void write_zeroes(size_t size);

  // write data into the buffer
  void write_block(const void* data, size_t length);

  // write other buffer into the buffer
  void write_buffer(const MemoryBlock& other);

  // write string into the buffer
  void write_string(const std::string& str);

  // write null terminated string into the buffer
  void write_string(const char* str);

  // create a string copy of the current buffer
  std::string buffer_string() const;

  // data pointer
  char* data() const {
    return m_data;
  }

  // current backing capacity
  size_t capacity() const {
    return m_capacity;
  }

  // total amount of bytes written into buffer
  size_t size() const {
    return m_size;
  }

  // current cursor position
  size_t cursor() const {
    return m_cursor;
  }

  // dump a hexdump of the buffer
  void dump(std::ostream& out) const;

protected:
  static const size_t kInitialCapacity = 64;
  static const size_t kMaximumSize = 0xFFFFFFFF;  // ca. 4.2Gb

  // reserve space for at least size bytes
  void reserve_space(size_t size);

  // copy data into buffer at offset
  void write_to(size_t offset, const void* data, size_t length);

  char*  m_data     = nullptr; // raw buffer
  size_t m_capacity = 0;       // total backing buffer capacity
  size_t m_size     = 0;       // amount of data written
  size_t m_cursor   = 0;       // current cursor position
};

}  // namespace charly::utils
