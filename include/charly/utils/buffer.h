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
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <iostream>

#include "charly/symbol.h"

#pragma once

namespace charly::utils {

// base allocation unuaware buffer type
// subclasses add the purpose-specific backing storage allocators
// and protection mechanisms
class BufferBase : private std::streambuf, public std::ostream, public std::istream {
public:
  BufferBase() :
    std::ostream(this),
    std::istream(this),
    m_buffer(nullptr),
    m_capacity(0),
    m_size(0),
    m_writeoffset(0),
    m_readoffset(0),
    m_windowoffset(0) {}

  BufferBase(BufferBase&& other) noexcept :
    std::ostream(this),
    std::istream(this),
    m_buffer(other.m_buffer),
    m_capacity(other.m_capacity),
    m_size(other.m_size),
    m_writeoffset(other.m_writeoffset),
    m_readoffset(other.m_readoffset),
    m_windowoffset(other.m_windowoffset) {
    other.m_buffer = nullptr;
    other.m_capacity = 0;
    other.m_size = 0;
    other.m_writeoffset = 0;
    other.m_readoffset = 0;
    other.m_windowoffset = 0;
  }

  ~BufferBase() override {
    m_buffer = nullptr;
    m_capacity = 0;
    m_size = 0;
    m_writeoffset = 0;
    m_readoffset = 0;
    m_windowoffset = 0;
  }

  // move write head to offset
  void seek(size_t offset);

  // reset the current read window
  void reset_window();

  // emit a block into the buffer
  void emit_block(const void* data, size_t size);

  // emit size zeroes into the buffer
  void emit_zeroes(size_t size);

  // emit the contents of another buffer
  void emit_buffer(const BufferBase& other);

  // emit stdlib string into buffer
  void emit_string(const std::string& string);

  // emit null-terminated string into buffer
  void emit_string(const char* string);

  // emit string_view contents into buffer
  void emit_string_view(const std::string_view& view);

  // return a copy of the entire buffer
  std::string buffer_string() const;
  std::string str() const { return buffer_string(); }

  // return a copy of the current read window
  std::string window_string() const;

  // return a view of the entire buffer
  std::string_view buffer_view() const;
  std::string_view view() const { return buffer_view(); }

  // return a view of the current read window
  std::string_view window_view() const;

  // returns a hash value of the buffer or window
  SYMBOL buffer_hash() const;
  SYMBOL hash() const { return buffer_hash(); }
  SYMBOL window_hash() const;

  // emit primitive types
  // clang-format off
  template <typename T>
  void emit(T&& value) {
    emit_block(&value, sizeof(T));
  }
  void emit_u8(uint8_t value) { emit(value); }
  void emit_u16(uint16_t value) { emit(value); }
  void emit_u32(uint32_t value) { emit(value); }
  void emit_u64(uint64_t value) { emit(value); }
  void emit_i8(int8_t value) { emit(value); }
  void emit_i16(int16_t value) { emit(value); }
  void emit_i32(int32_t value) { emit(value); }
  void emit_i64(int64_t value) { emit(value); }
  void emit_ptr(uintptr_t value) { emit(value); }
  void emit_ptr(void* value) { emit(value); }
  // clang-format on

  // emit an utf8 encoded codepoint into the buffer
  void emit_utf8_cp(uint32_t cp);

  // peek the next utf8 encoded codepoint from the readoffset of the buffer
  uint32_t peek_utf8_cp(uint32_t nth = 0) const;

  // read the next utf8 encoded codepoint from the readoffset of the buffer
  uint32_t read_utf8_cp();

  // returns the amount of bytes neede to encode this
  // utf8 codepoint into a character stream
  static size_t utf8_cp_length(uint32_t cp) {
    if (cp < 0x80)
      return 1;
    if (cp < 0x800)
      return 2;
    if (cp < 0x10000)
      return 3;
    return 4;
  }

  // encode some utf8 character into a string
  static std::string utf8_encode_cp(uint32_t cp);
  static constexpr auto u8 = utf8_encode_cp;

  // format buffer as hexdump into out stream
  void dump(std::ostream& out, bool absolute = false) const;

  // format some memory buffer as a hexdump into some output stream
  static void hexdump(const char* buffer, size_t size, std::ostream& out, bool absolute = false);

  char* data() const {
    return (char*)m_buffer;
  }

  size_t capacity() const {
    return m_capacity;
  }

  size_t size() const {
    return m_size;
  }

  size_t window_size() const {
    return m_readoffset - m_windowoffset;
  }

  size_t write_offset() const {
    return m_writeoffset;
  }

  size_t read_offset() const {
    return m_readoffset;
  }

  size_t window_offset() const {
    return m_windowoffset;
  }

  // reserve enough space in the backing buffer for size bytes
  virtual void reserve_space(size_t size) = 0;

  // clear buffer with 0
  virtual void clear();

  // std::stringbuf override
  // write a block of data into the buffer
  std::streamsize xsputn(const char* data, std::streamsize size) override;

  // std::stringbuf override
  // write a single character to the buffer
  int32_t overflow(int32_t ch) override;

protected:

  // copy size bytes from the source address to some target offset in the backing buffer
  // grows the backing buffer to fit the write
  void write_to_offset(size_t target_offset, const void* source, size_t size);

  // advances the size offset if the previous write exceeded it
  void update_size();

  void* m_buffer;         // pointer to backing storage
  size_t m_capacity;      // total capacity of the backing storage
  size_t m_size;          // total written bytes
  size_t m_writeoffset;   // offset of the write head
  size_t m_readoffset;    // offset of the read head
  size_t m_windowoffset;  // offset of the window head
};

// buffer backed by memory obtained via malloc
class Buffer : public BufferBase {
public:
  Buffer() : BufferBase() {}

  explicit Buffer(size_t initial_capacity) : Buffer() {
    reserve_space(initial_capacity);
  }

  explicit Buffer(const std::string& string) : BufferBase() {
    emit_string(string);
  }

  explicit Buffer(const BufferBase& other) : BufferBase() {
    emit_buffer(other);
  }

  Buffer(Buffer&& other) noexcept : BufferBase(std::move(other)) {}

  virtual ~Buffer() {
    if (m_buffer) {
      std::free(m_buffer);
      m_buffer = nullptr;
    }
  }

  virtual void reserve_space(size_t size) override;

  // releases the backing buffer to the caller of the function
  // buffer will be reset after this call
  char* release_buffer();
};

// buffer that can be protected
// internal buffer will always have a minimum size of the system page size
// and will only grow in multiples of that size
class ProtectedBuffer: public BufferBase {
public:
  ProtectedBuffer() : BufferBase(), m_readonly(false) {}

  ProtectedBuffer(size_t initial_capacity) : ProtectedBuffer() {
    reserve_space(initial_capacity);
  }

  ProtectedBuffer(const std::string& string) : ProtectedBuffer(string.size()) {
    emit_string(string);
  }

  ProtectedBuffer(const ProtectedBuffer& other) : ProtectedBuffer(other.size()) {
    emit_buffer(other);
  }

  virtual ~ProtectedBuffer() {
    set_readonly(false);
  }

  bool is_readonly() const;
  void set_readonly(bool option);
  virtual void reserve_space(size_t size) override;
  virtual void clear() override;

private:
  bool m_readonly;
};

class GuardedBuffer : public ProtectedBuffer {
public:
  GuardedBuffer() : ProtectedBuffer(), m_mapping_base(nullptr), m_mapping_size(0) {}

  GuardedBuffer(size_t size) : GuardedBuffer() {
    reserve_space(size);
  }

  GuardedBuffer(const std::string& string) : ProtectedBuffer(string.size()) {
    emit_string(string);
  }

  GuardedBuffer(const GuardedBuffer& other) : GuardedBuffer(other.size()) {
    emit_buffer(other);
  }

  virtual ~GuardedBuffer() {
    dealloc_mapping();
  }

  virtual void reserve_space(size_t size) override;

private:
  void dealloc_mapping();

  void* m_mapping_base;
  size_t m_mapping_size;
};

}  // namespace charly::utils
