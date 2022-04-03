/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

#include "charly/symbol.h"
#include "charly/utf8.h"
#include "charly/utils/allocator.h"

#pragma once

namespace charly::utils {

class Buffer : public std::iostream, protected std::streambuf {
  static const size_t kDefaultCapacity = 32;

public:
  explicit Buffer(size_t initial_capacity = kDefaultCapacity) :
    std::iostream(this), m_buffer(nullptr), m_capacity(0), m_window_start(0), m_protected(false) {
    // determine initial buffer capacity
    size_t rounded_capacity = kDefaultCapacity;
    while (rounded_capacity < initial_capacity) {
      rounded_capacity *= 2;
    }

    m_buffer = (char*)Allocator::alloc(rounded_capacity, 8);
    m_capacity = rounded_capacity;

    setp(m_buffer, m_buffer + m_capacity);
    setg(m_buffer, m_buffer, pptr());
  }

  explicit Buffer(const std::string& string) : Buffer(string.size()) {
    *this << string;
  }

  Buffer(const Buffer& other) : Buffer(other.size()) {
    this->write(other.data(), std::streamsize(other.size()));
  }

  Buffer(Buffer&& other) noexcept : Buffer() {
    m_buffer = other.m_buffer;
    m_capacity = other.m_capacity;
    bool is_protected = other.m_protected;

    auto p = other.tellp();
    auto g = other.tellg();

    other.m_buffer = nullptr;
    other.m_capacity = 0;
    other.setp(nullptr, nullptr);
    other.setg(nullptr, nullptr, nullptr);
    other.clear();

    setp(m_buffer, m_buffer + m_capacity);
    seekp(p);
    setg(m_buffer, m_buffer, pptr());
    seekg(g);

    if (is_protected) {
      protect();
    }
  }

  ~Buffer() override {
    clean();
  }

  // write the contents of another buffer into this buffer
  // the get area of the other buffer isn't modified
  void write_buffer(const Buffer& other);

  // write an utf8 codepoint to the buffer
  void write_utf8_cp(uint32_t cp);

  // write primitive types
#define WRITE_PRIMITIVE(T, N)                     \
  void write_##N(const T& value) {                \
    this->write((char*)(void*)&value, sizeof(T)); \
  }
  WRITE_PRIMITIVE(uint8_t, u8)
  WRITE_PRIMITIVE(uint16_t, u16)
  WRITE_PRIMITIVE(uint32_t, u32)
  WRITE_PRIMITIVE(uint64_t, u64)
  WRITE_PRIMITIVE(int8_t, i8)
  WRITE_PRIMITIVE(int16_t, i16)
  WRITE_PRIMITIVE(int32_t, i32)
  WRITE_PRIMITIVE(int64_t, i64)
  WRITE_PRIMITIVE(uintptr_t, ptr)
  WRITE_PRIMITIVE(void*, ptr)
  WRITE_PRIMITIVE(float, float)
  WRITE_PRIMITIVE(double, double)
#undef WRITE_PRIMITIVE

  // reads a single utf8 codepoint from the buffer
  // returns -1 for EOF
  int64_t read_utf8_cp();

  // peek the next utf8 character from the buffer
  // returns -1 for EOF
  int64_t peek_utf8_cp(uint32_t nth = 0);

  // peek the next character in the buffer
  // returns -1 for EOF
  int32_t peek_char(uint32_t nth = 0);

  // protect / unprotect the backing buffer
  void protect();
  void unprotect();

  // clear buffer content
  void clear();

  // reset buffer window to current read position
  void reset_window();

  // acquire ownership of the backing buffer memory
  // buffer appears cleared afterwards
  [[nodiscard]] char* release_buffer();

  // format buffer via hexdump into out stream
  void dump(std::ostream& out, bool absolute = false) const;

  friend std::ostream& operator<<(std::ostream& out, const Buffer& buffer) {
    out.write(buffer.data(), buffer.size());
    return out;
  }

  // format some memory buffer as a hexdump into some output stream
  static void hexdump(const char* buffer, size_t size, std::ostream& out, bool absolute = false);

  char* data() const;
  size_t capacity() const;
  size_t size() const;
  size_t window_size() const;
  size_t write_offset() const;
  size_t read_offset() const;
  size_t window_offset() const;
  bool is_protected() const;
  bool is_page_aligned() const;
  SYMBOL hash() const;
  std::string str() const;
  std::string window_str() const;
  std::string_view view() const;
  std::string_view window_view() const;

  template <typename... T>
  void write_formatted(const char* template_message, const T&... args) {
    debugln_impl(*this, template_message, args...);
  }

protected:
  Buffer* setbuf(std::streambuf::traits_type::char_type* data, std::streamsize size) override;
  std::streambuf::traits_type::int_type underflow() override;
  std::streambuf::traits_type::int_type pbackfail(std::streambuf::traits_type::int_type c) override;
  std::streambuf::traits_type::int_type overflow(std::streambuf::traits_type::int_type c) override;
  std::streambuf::traits_type::pos_type seekpos(std::streambuf::traits_type::pos_type pos,
                                                std::ios_base::openmode openmode) override;
  std::streambuf::traits_type::pos_type seekoff(std::streambuf::traits_type::off_type off,
                                                std::ios_base::seekdir seekdir,
                                                std::ios_base::openmode openmode) override;

protected:
  // reserve at least *size* bytes of memory in the backing buffer
  void reserve_space(size_t size, bool page_aligned = false);

  // deallocate backing memory
  void clean();

  char* m_buffer;
  size_t m_capacity;
  size_t m_window_start;
  bool m_protected;
};

}  // namespace charly::utils
