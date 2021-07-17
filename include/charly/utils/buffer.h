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
#include <stdexcept>
#include <string>
#include <string_view>

#pragma once

namespace charly::utils {

// abstract interface each backing buffer has to conform to to be used by the
// various different buffer adapters
class BufferBase {
public:
  BufferBase() : m_size(0), m_writeoffset(0), m_readoffset(0), m_windowoffset(0) {}
  BufferBase(BufferBase&& other) = delete;
  BufferBase(BufferBase& other) = delete;
  virtual ~BufferBase() {
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

  // return a copy of the current read window
  std::string window_string() const;

  // return a view of the entire buffer
  std::string_view buffer_view() const;

  // return a view of the current read window
  std::string_view window_view() const;

  // returns a hash value of the buffer or window
  uint32_t buffer_hash() const;
  uint32_t window_hash() const;

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

  virtual char* data() const = 0;
  virtual size_t capacity() const = 0;

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

private:

  // copy size bytes from the source address to some target offset in the backing buffer
  // grows the backing buffer to fit the write
  void write_to_offset(size_t target_offset, const void* source, size_t size);

  // advances the size offset if the previous write exceeded it
  void update_size();

  // reserve enough space in the backing buffer for size bytes
  virtual void reserve_space(size_t size) = 0;

private:
  size_t m_size;          // total written bytes
  size_t m_writeoffset;   // offset of the write head
  size_t m_readoffset;    // offset of the read head
  size_t m_windowoffset;  // offset of the window head
};

// buffer backed by memory obtained via malloc
class Buffer : public BufferBase {
public:
  static const size_t kInitialCapacity = 32;
  static const size_t kMaximumSize = 0x00000000FFFFFFFF; // ~ 4.2 GB

  Buffer(size_t initial_capacity = kInitialCapacity) : BufferBase(), m_buffer(nullptr), m_capacity(0) {
    reserve_space(initial_capacity);
  }

  Buffer(const std::string& string) : Buffer(string.size()) {
    emit_string(string);
  }

  Buffer(const Buffer& other) : Buffer(other.size()) {
    emit_buffer(other);
  }

  virtual ~Buffer() {
    if (m_buffer) {
      std::free(m_buffer);
      m_buffer = nullptr;
    }
    m_capacity = 0;
  }

  virtual char* data() const override;
  virtual size_t capacity() const override;

private:
  virtual void reserve_space(size_t size) override;

  void* m_buffer;
  size_t m_capacity;
};

// buffer that can be protected
// internal buffer will always have a minimum size of the system page size
// and will only grow in multiples of that size
//
// class ProtectedBuffer: public Buffer {
// public:
//   using Buffer::Buffer;
//   bool is_readonly() const;
//   void set_readonly(bool option);
//
// private:
//   virtual void reserve_space(size_t size) override;
//
// private:
//   static const size_t kPageSize = 4096;
//
//   bool m_readonly;
// };

}  // namespace charly::utils
