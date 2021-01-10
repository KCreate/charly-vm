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

#include <stdexcept>

#include "charly/core/compiler/location.h"

#pragma once

namespace charly::core::compiler {

// base class of all compiler errors
struct CompilerError : public std::runtime_error {
  Location location;

  CompilerError(const utils::string& message, const Location& location) :
    std::runtime_error(message), location(location) {}

  // write a formatted version of this error to the stream:
  //
  // <filename>:<row>:<col>: <message>
  friend std::ostream& operator<<(std::ostream& out, const CompilerError& error) {
    out << *error.location.filename;
    out << ':';
    out << error.location.row;
    out << ':';
    out << error.location.column;
    out << ':';
    out << ' ';
    out << error.what();

    return out;
  }
};

}  // namespace charly::core::compiler
