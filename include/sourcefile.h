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

#include <cstdio>
#include <string>
#include "utf8buffer.h"

#pragma once

namespace Charly {
class SourceFile : public UTF8Buffer {
public:
  std::string filename = "<unknown>";
  size_t frame_pos = 0;
  size_t pos = 0;
  size_t row = 1;
  size_t column = 1;
  uint32_t current_char = L'\0';
  std::string frame;

  SourceFile(const std::string& f, const std::string& source) : UTF8Buffer(), filename(f) {
    this->write_string(source);
    this->read_char();
  }

  SourceFile(const std::string& source) : UTF8Buffer(), filename("") {
    this->write_string(source);
    this->read_char();
  }

  // Returns the contents of the current frame as a string
  inline std::string get_current_frame() {
    return std::string(this->frame, 0, this->frame.size() - 1);
  }

  // Reset the current frame
  inline SourceFile& reset_frame() {
    this->frame.clear();
    return *this;
  }

  // Read char, append it to the current frame and advance one position
  inline uint32_t read_char() {
    uint32_t cp = this->next_utf8();

    this->pos += 1;
    this->current_char = cp;

    if (cp == L'\0') {
      return cp;
    }

    utf8::append(cp, std::back_inserter(this->frame));

    if (cp == '\n') {
      this->row++;
      this->column = 0;
    } else {
      this->column++;
    }

    return cp;
  }

  // Read a char without appending to the frame or advancing the position
  inline uint32_t peek_char() {
    return this->peek_next_utf8();
  }
};
}  // namespace Charly
