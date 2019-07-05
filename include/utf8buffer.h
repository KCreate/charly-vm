/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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

#include <utf8/utf8.h>
#include <cstring>
#include <iostream>
#include <string>

#include "memoryblock.h"

#pragma once

namespace Charly {

// Handles UTF8 encoded text
class UTF8Buffer : public MemoryBlock {
  size_t readoffset = 0;

public:
  UTF8Buffer() : MemoryBlock() {
  }

  UTF8Buffer(const UTF8Buffer& other) : MemoryBlock(), readoffset(other.readoffset) {
  }

  UTF8Buffer(UTF8Buffer&& other) : MemoryBlock(), readoffset(other.readoffset) {
  }

  UTF8Buffer& operator=(const UTF8Buffer& other) {
    this->readoffset = other.readoffset;
    MemoryBlock::operator=(other);
    return *this;
  }

  UTF8Buffer& operator=(UTF8Buffer&& other) {
    this->readoffset = other.readoffset;
    MemoryBlock::operator=(std::move(other));
    return *this;
  }

  // Load raw data into the buffer
  inline void write(const UTF8Buffer& data) {
    this->grow_to_fit(this->writeoffset + data.writeoffset);
    this->write_block(data.data, data.writeoffset);
  }

  // UTF8 methods
  uint32_t append_utf8(uint32_t cp);
  uint32_t next_utf8();
  uint32_t peek_next_utf8();
  static inline bool is_valid_utf8(char* start, char* end) {
    return utf8::is_valid(start, end);
  }

  size_t codepointcount();
  size_t utf8_byteoffset(uint32_t start);

  static inline void write_cp_to_stream(uint32_t cp, std::ostream& stream) {
    char buffer[] = {0, 0, 0, 0};
    char* buffer_start = buffer;
    buffer_start = utf8::append(cp, buffer_start);
    stream.write(buffer, buffer_start - buffer);
  }

  static inline void write_cp_to_string(uint32_t cp, std::string& str) {
    char buffer[] = {0, 0, 0, 0};
    char* buffer_start = buffer;
    buffer_start = utf8::append(cp, buffer_start);
    str.append(buffer, buffer_start - buffer);
  }
};
}  // namespace Charly
