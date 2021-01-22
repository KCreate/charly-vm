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

#include <utf8/utf8.h>

#pragma once

namespace charly::utils {

// provides utf8 access to raw byte buffers
class Buffer {
public:
  Buffer(size_t initial_capacity = kInitialCapacity) {
    this->reserve_space(initial_capacity);
  }

  Buffer(const std::string& str) : Buffer(str.size()) {
    this->append_string(str);
  }

  // copy constructor
  Buffer(const Buffer& other) {
    this->reserve_space(other.m_capacity);
    this->write_to(other.m_data, other.m_writeoffset, 0);
    m_readoffset = other.m_readoffset;
  }

  // move constructor
  Buffer(Buffer&& other) :
    m_data(other.m_data),
    m_capacity(other.m_capacity),
    m_writeoffset(other.m_writeoffset),
    m_readoffset(other.m_readoffset) {
    other.m_data = nullptr;
    other.m_capacity = 0;
    other.m_writeoffset = 0;
    other.m_readoffset = 0;
  }

  ~Buffer() {
    if (m_data)
      std::free(m_data);

    m_data = nullptr;
    m_capacity = 0;
    m_writeoffset = 0;
    m_readoffset = 0;
  }

  Buffer& operator=(Buffer& other) = delete;
  Buffer& operator=(Buffer&& other) = delete;

  // returns the amount of bytes neede to encode this
  // utf8 character into a character stream
  static size_t encoded_length_utf8(uint32_t character) {
    if (character < 0x80)
      return 1;
    if (character < 0x800)
      return 2;
    if (character < 0x10000)
      return 3;
    return 4;
  }

  // write data into the buffer
  void append_block(const void* data, size_t length);

  // append other buffer to this buffer
  void append_buffer(const Buffer& other);

  // write string into the buffer
  void append_string(const std::string& str);

  // write null terminated string into the buffer
  void append_string(const char* str);

  // write null terminated string into the buffer
  void append_utf8(uint32_t cp);

  // peek the next utf8 character
  uint32_t peek_utf8(uint32_t nth = 0) const;

  // read a utf8 character
  uint32_t read_utf8();

  // reset the window
  void reset_window();

  // create a string copy of the current window
  std::string window_string() const;

  // create a string copy of the current buffer
  std::string buffer_string() const;

  const char* data() const {
    return m_data;
  }
  const char* window() const {
    return m_data + m_window;
  }
  size_t capacity() const {
    return m_capacity;
  }
  size_t writeoffset() const {
    return m_writeoffset;
  }
  size_t size() const {
    return m_writeoffset;
  }
  size_t readoffset() const {
    return m_readoffset;
  }
  size_t window_size() const {
    return m_readoffset - m_window;
  }

protected:
  static const size_t kInitialCapacity = 64;
  static const size_t kMaximumSize = 0xFFFFFFFF;  // ca. 4.2Gb

  // reserve space for at least size bytes
  void reserve_space(size_t size);

  // copy data into buffer at offset
  void write_to(const void* data, size_t length, size_t offset);

  char* m_data = 0;          // raw buffer
  size_t m_capacity = 0;     // total backing buffer capacity
  size_t m_writeoffset = 0;  // offset of the first unwritten byte
  size_t m_readoffset = 0;   // offset of the first unread character
  size_t m_window = 0;       // begin offset of current window
};

inline void Buffer::append_block(const void* data, size_t length) {
  this->write_to(data, length, m_writeoffset);
  m_writeoffset += length;
}

inline void Buffer::append_buffer(const Buffer& other) {
  this->write_to(other.data(), other.size(), m_writeoffset);
  m_writeoffset += other.size();
}

inline void Buffer::append_string(const std::string& str) {
  this->write_to(str.c_str(), str.size(), m_writeoffset);
  m_writeoffset += str.size();
}

inline void Buffer::append_string(const char* str) {
  size_t length = std::strlen(str);
  this->write_to(str, length, m_writeoffset);
  m_writeoffset += length;
}

inline void Buffer::append_utf8(uint32_t cp) {
  this->reserve_space(m_writeoffset + 4);

  char* buffer_initial = m_data + m_writeoffset;
  char* buffer_ptr = buffer_initial;
  buffer_ptr = utf8::append(cp, buffer_ptr);
  m_writeoffset += buffer_ptr - buffer_initial;
}

inline uint32_t Buffer::peek_utf8(uint32_t nth) const {
  char* buffer_initial = m_data + m_readoffset;
  char* buffer_ptr = buffer_initial;

  if (m_readoffset >= m_writeoffset)
    return '\0';

  // walk forward to requested character
  while (nth--) {
    utf8::next(buffer_ptr, m_data + m_writeoffset);

    // overflow check
    if (buffer_ptr >= m_data + m_writeoffset) {
      return '\0';
    }
  }

  return utf8::peek_next(buffer_ptr, m_data + m_writeoffset);
}

inline uint32_t Buffer::read_utf8() {
  if (m_readoffset >= m_writeoffset)
    return L'\0';

  // advance by one utf8 codepoint
  char* buffer_initial = m_data + m_readoffset;
  char* buffer_ptr = buffer_initial;
  uint32_t codepoint = utf8::next(buffer_ptr, m_data + m_writeoffset);
  m_readoffset += buffer_ptr - buffer_initial;
  return codepoint;
}

inline void Buffer::reset_window() {
  m_window = m_readoffset;
}

inline std::string Buffer::window_string() const {
  size_t window_size = m_readoffset - m_window;

  if (window_size == 0)
    return std::string("");

  return std::string(m_data + m_window, window_size);
}

inline std::string Buffer::buffer_string() const {
  if (m_writeoffset == 0)
    return std::string("");

  return std::string(m_data, m_writeoffset);
}

inline void Buffer::reserve_space(size_t size) {
  if (m_capacity >= size)
    return;

  size_t new_capacity = m_capacity ? m_capacity * 2 : kInitialCapacity;
  while (new_capacity < size) {
    new_capacity *= 2;

    if (new_capacity >= kMaximumSize)
      throw new std::runtime_error("Reached maximum buffer size");
  }

  char* new_buffer = static_cast<char*>(std::realloc(m_data, new_capacity));
  if (new_buffer == nullptr)
    throw new std::runtime_error("Could not realloc buffer");

  m_data = new_buffer;
  m_capacity = new_capacity;
}

inline void Buffer::write_to(const void* data, size_t length, size_t offset) {
  this->reserve_space(offset + length);
  std::memmove(m_data + offset, data, length);
}

}  // namespace charly::utils
