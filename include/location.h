/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2018 Leonard Schütz
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

#include <iostream>
#include <string>

#pragma once

namespace Charly::Compilation {
struct Location {
  uint32_t pos = 0;
  uint32_t row = 0;
  uint32_t column = 0;
  uint32_t length = 0;
  std::string filename;

  Location() {
  }
  Location(uint32_t p, uint32_t r, uint32_t c, uint32_t l, const std::string& s)
      : pos(p), row(r), column(c), length(l), filename(s) {
  }
  Location(const Location& o) : pos(o.pos), row(o.row), column(o.column), length(o.length), filename(o.filename) {
  }
  Location(Location&& o) : pos(o.pos), row(o.row), column(o.column), length(o.length), filename(std::move(o.filename)) {
  }

  Location& operator=(const Location& o) {
    this->pos = o.pos;
    this->row = o.row;
    this->column = o.column;
    this->length = o.length;
    this->filename = o.filename;

    return *this;
  }

  Location& operator=(Location&& o) {
    this->pos = o.pos;
    this->row = o.row;
    this->column = o.column;
    this->length = o.length;
    std::swap(this->filename, o.filename);

    return *this;
  }

  inline void write_to_stream(std::ostream& stream) const {
    stream << this->filename << ":" << this->row << ":" << this->column;
  }
};
}  // namespace Charly::Compilation
