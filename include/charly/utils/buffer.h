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

#include <cstdint>
#include <string>
#include <cstring>

#pragma once

namespace charly::utils {

class Buffer {
  static const uint32_t kInitialCapacity = 64;

public:
  inline Buffer(uint32_t initial_capacity = kInitialCapacity) :
    m_data(new char[initial_capacity]),
    m_capacity(initial_capacity),
    m_offset(0) {
  }

  inline ~Buffer() {
    if (m_data) {
      delete[] m_data;
    }
    m_data = nullptr;
    m_offset = 0;
    m_capacity = 0;
  }

  Buffer(Buffer& other) = delete;
  Buffer(Buffer&& other) = delete;

  Buffer& operator=(Buffer& other) = delete;
  Buffer& operator=(Buffer&& other) = delete;

#define WRITE(T, N)                                          \
  inline uint32_t write_##N(T value) {                       \
    return this->write(value);                               \
  }                                                          \
  inline uint32_t write_##N##_to(T value, uint32_t offset) { \
    return this->write_to(value, offset);                    \
  }
  WRITE(uint8_t,  i8);
  WRITE(uint16_t, i16);
  WRITE(uint32_t, i32);
  WRITE(uint64_t, i64);
  WRITE(float,    float);
  WRITE(double,   double);
  WRITE(void*,    ptr);
#undef WRITE

#define READ(T, N) inline T read_ ## N(uint32_t offset) { return this->read_from<T>(offset); }
  READ(uint8_t,  i8);
  READ(uint16_t, i16);
  READ(uint32_t, i32);
  READ(uint64_t, i64);
  READ(float,    float);
  READ(double,   double);
  READ(void*,    ptr);
#undef READ

  inline uint32_t write_block(const char* data, uint32_t size) {
    this->check_fit(m_offset + size);
    std::memmove(m_data + m_offset, data, size);
    m_offset += size;
    return size;
  }

  inline uint32_t write_string(const std::string& string) {
    return this->write_block(string.c_str(), string.size());
  }

  // move offset inside buffer
  inline void seek(uint32_t offset) {
    if (offset <= m_capacity) {
      m_offset = offset;
    }
  }

  // const access to member buffer
  inline const char* buf() {
    return m_data;
  }

  // amount of written bytes
  inline uint32_t size() {
    return m_offset;
  }

  // current buffer capacity
  inline uint32_t capacity() {
    return m_capacity;
  }

private:

  // checks wether *size* bytes could fit into the buffer
  // will grow the buffer if it is too small
  inline bool check_fit(uint32_t size) {

    // size does fit
    if (m_capacity >= size) {
      return true;
    }

    // increase buffer capacity until we can fit size
    uint32_t new_capacity = m_capacity;
    while (size > new_capacity) {
      if (new_capacity == 0x80000000)
        return false;
      new_capacity <<= 1;
    }

    // allocate the new buffer
    char* new_buf = new char[new_capacity];
    std::memcpy(new_buf, m_data, m_offset);
    delete[] m_data;
    m_data = new_buf;
    m_capacity = new_capacity;

    return true;
  }

  // appends the memory contents of *value* to the buffer
  // returns the amount of bytes written
  template <typename T>
  inline uint32_t write(const T& value) {
    if (this->check_fit(m_offset + sizeof(T))) {
      std::memmove(m_data + m_offset, &value, sizeof(T));
      m_offset += sizeof(T);
      return sizeof(T);
    }
    return 0;
  }

  // copies the memory contents of *value* into the buffer at *offset*
  // returns the amount of bytes written
  template <typename T>
  inline uint32_t write_to(const T& value, uint32_t offset) {
    if (this->check_fit(offset + sizeof(T))) {
      std::memmove(m_data + offset, &value, sizeof(T));
      return sizeof(T);
    }
    return 0;
  }

  // reads a certain type from the memory buffer
  template <typename T>
  inline T read_from(uint32_t offset) {
    int64_t last_byte_offset = offset + sizeof(T) - 1;
    if (last_byte_offset <= static_cast<int64_t>(m_capacity)) {
      return *reinterpret_cast<T*>(m_data + offset);
    }

    return T();
  }

  char*    m_data;
  uint32_t m_capacity;
  uint32_t m_offset;
};

}
