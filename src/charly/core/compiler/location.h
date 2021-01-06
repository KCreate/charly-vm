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

#include "charly/utils/string.h"

#pragma once

namespace charly::core::compiler {

struct Location {
  std::shared_ptr<utils::string> filename;

  // offset in source bytestream
  size_t offset = 0;
  size_t length = 0;

  // file coordinates
  uint32_t row = 0;
  uint32_t column = 0;

  void dump(std::ostream& io) {
    io << "<";

    if (row && column) {
      io << "line " << row << ":" << column;
    } else {
      io << "-:-";
    }

    if (row)
      io << row;
    else
      io << "-";

    io << ":";

    if (column)
      io << column;
    else
      io << ">";
  }
};

struct LocationRange {
  Location begin;
  Location end;

  void dump(std::ostream& io, bool with_filename = false) {
    io << "<";

    if (with_filename) {
      io << (begin.filename ? *begin.filename : "???");
      io << " ";
    }

    // B.row = E.row          <B.filename:B.row:B.col : E.col>
    // B.row ! E.row          <B.filename:B.row:B.col : E.row:E.col>
    if (begin.row == end.row) {
      if (begin.row == end.row && begin.row == 0) {
        io << "-:-";
      } else {
        if (begin.column == end.column) {
          io << "line " << begin.row << ":" << begin.column;
        } else {
          io << "line " << begin.row << ":" << begin.column << " col:" << end.column + end.length - 1;
        }
      }
    } else {
      io << begin.row << ":" << begin.column;
      io << " : ";
      io << end.row << ":" << end.column;
    }

    io << ">";
  }
};

}  // namespace charly::core::compiler
