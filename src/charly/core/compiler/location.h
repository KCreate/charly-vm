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
#include <iostream>
#include <memory>
#include <string>

#pragma once

namespace charly::core::compiler {

struct Location {
  bool valid = false;     // wether this location contains actual data
  bool compound = false;  // wether this location is a mixup of multiple other locations

  // offset in source bytestream
  size_t offset = 0;
  size_t end_offset = 0;

  // file coordinates
  uint32_t row = 0;
  uint32_t column = 0;
  uint32_t end_row = 0;
  uint32_t end_column = 0;

  void set_begin(const Location& other) {
    this->compound = true;
    this->offset = other.offset;
    this->row = other.row;
    this->column = other.column;
  }

  void set_end(const Location& other) {
    this->compound = true;
    this->end_offset = other.end_offset;
    this->end_row = other.end_row;
    this->end_column = other.end_column;
  }

  friend std::ostream& operator<<(std::ostream& out, const Location& loc) {
    return out << loc.row + 1 << ":" << loc.column + 1;
  }
};

}  // namespace charly::core::compiler
