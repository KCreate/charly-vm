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

#include "charly/utils/memoryblock.h"

#pragma once

namespace charly::utils {

// utf8 access to MemoryBlock
class Buffer : public MemoryBlock {
public:
  Buffer(size_t initial_capacity = kInitialCapacity) : MemoryBlock(initial_capacity) {}
  Buffer(const std::string& str) : MemoryBlock(str) {}

  // copy constructor
  Buffer(const Buffer& other) : MemoryBlock(other) {
    m_readoffset = other.m_readoffset;
  }

  // move constructor
  Buffer(Buffer&& other) : MemoryBlock(other), m_readoffset(other.m_readoffset), m_window(other.m_window) {
    other.m_readoffset = 0;
    other.m_window = 0;
  }

  ~Buffer() {
    m_readoffset = 0;
    m_window = 0;
  }

  // returns the amount of bytes neede to encode this
  // utf8 character into a character stream
  static size_t utf8_encoded_length(uint32_t character) {
    if (character < 0x80)
      return 1;
    if (character < 0x800)
      return 2;
    if (character < 0x10000)
      return 3;
    return 4;
  }

  // encode the utf8 character into a string
  static std::string utf8_encode_character(uint32_t character) {
    Buffer buf;
    buf.write_utf8(character);
    return buf.buffer_string();
  }
  static constexpr auto u8 = utf8_encode_character;

  // write null terminated string into the buffer
  void write_utf8(uint32_t cp);

  // peek the next utf8 character
  uint32_t peek_utf8(uint32_t nth = 0) const;

  // read a utf8 character
  uint32_t read_utf8();

  // reset the window
  void reset_window();

  // create a string copy of the current window
  std::string window_string() const;

  // move the write offset to some new offset
  void seek(size_t offset) = delete;

  const char* window() const {
    return m_data + m_window;
  }
  size_t window_size() const {
    return m_readoffset - m_window;
  }
  size_t readoffset() const {
    return m_readoffset;
  }

protected:
  size_t m_readoffset = 0; // offset of the first unread character
  size_t m_window     = 0; // begin offset of current window
};

}  // namespace charly::utils
