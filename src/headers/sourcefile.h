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
#include <fstream>
#include <sstream>
#include <string>
#include "buffer.h"

#pragma once

namespace Charly {

static const uint32_t kEOFChar = EOF;
class SourceFile {
public:
  SourceFile(std::string& filename, std::string& source) {
    this->filename = filename;
    this->buffer.read(source);
    this->read_char();
  }

  SourceFile(std::string& source) {
    this->filename = filename;
    this->buffer.read(source);
    this->read_char();
  }

  // Sets the filename of this sourcefile
  inline SourceFile& set_filename(std::string& newname) {
    this->filename = newname;
    return *this;
  }

  // Returns the filename of this sourcefile
  inline std::string get_filename() {
    return this->filename;
  }

  // Returns the contents of the current frame as a string
  inline std::string get_current_frame() {
    return this->frame.str();
  }

  // Reset the current frame
  inline SourceFile& reset_frame() {
    this->frame.str("");  // Reset the buffer
    this->frame.clear();  // Clear any error flags as we don't care about them
    return *this;
  }

  // Returns the current position of the reader in the source
  inline size_t get_pos() {
    return this->pos;
  }

  // Returns the position the frame begins at
  inline size_t get_frame_pos() {
    return this->frame_pos;
  }

  // Returns the last read char
  inline uint32_t get_current_char() {
    return this->current_char;
  }

  // Read char, append it to the current frame and advance one position
  uint32_t read_char() {
    uint32_t cp = this->buffer.next_utf8();

    if (cp == static_cast<uint32_t>(EOF)) {
      return cp;
    }

    std::string utf8bytes;
    utf8::append(cp, std::back_inserter(utf8bytes));
    this->frame << utf8bytes;

    this->pos += 1;
    this->current_char = cp;

    return cp;
  }

  // Read a char without appendin to the frame or advancing the position
  uint32_t peek_char() {
    return this->buffer.peek_next_utf8();
  }

  // Close the source
  // Note: This is only useful if the source is a file or an IO of some sorts
  SourceFile& close() {
    return *this;
  }

private:
  std::string filename = "unnamed-source";
  size_t frame_pos = 0;
  size_t pos = 0;
  std::ostringstream frame;
  Buffer buffer;
  uint32_t current_char = kEOFChar;
};
}  // namespace Charly
