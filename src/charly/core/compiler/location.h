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
  std::shared_ptr<std::string> filename;

  // offset in source bytestream
  size_t offset = 0;
  size_t length = 0;

  // file coordinates
  uint32_t row = 0;
  uint32_t column = 0;

  friend std::ostream& operator<<(std::ostream& out, const Location& loc) {
    out << "<";

    if (loc.row && loc.column) {
      out << "line " << loc.row << ":" << loc.column;
    } else {
      out << "-:-";
    }

    if (loc.row)
      out << loc.row;
    else
      out << "-";

    out << ":";

    if (loc.column)
      out << loc.column;
    else
      out << "-";

    out << ">";

    return out;
  }
};

struct LocationRange {
  Location begin;
  Location end;

  friend std::ostream& operator<<(std::ostream& out, const LocationRange& range) {
    out << "<";

    // B.row = E.row          <B.filename:B.row:B.col : E.col>
    // B.row ! E.row          <B.filename:B.row:B.col : E.row:E.col>
    if (range.begin.row == range.end.row) {
      if (range.begin.row == range.end.row && range.begin.row == 0) {
        out << "-:-";
      } else {
        if (range.begin.column == range.end.column) {
          out << "line " << range.begin.row << ":" << range.begin.column;
        } else {
          out << "line " << range.begin.row << ":" << range.begin.column;
          out << " col:" << (range.end.column + range.end.length - 1);
        }
      }
    } else {
      out << range.begin.row << ":" << range.begin.column;
      out << " : ";
      out << range.end.row << ":" << range.end.column;
    }

    out << ">";

    return out;
  }
};

}  // namespace charly::core::compiler
