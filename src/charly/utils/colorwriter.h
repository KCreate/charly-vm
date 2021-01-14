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

#include <iostream>

#include <termcolor/termcolor.h>

#pragma once

namespace charly::utils {

enum class Color : uint8_t { Grey, Red, Green, Yellow, Blue, Magenta, Cyan, White };

class ColorWriter {
public:
  ColorWriter(std::ostream& stream = std::cout) : m_stream(stream) {}

  template <typename T>
  void write(const T& v) {
    m_stream << v;
  }

  template <typename T>
  void grey(const T& v) {
    m_stream << termcolor::grey;
    m_stream << v;
    m_stream << termcolor::reset;
  }

  template <typename T>
  void red(const T& v) {
    m_stream << termcolor::red;
    m_stream << v;
    m_stream << termcolor::reset;
  }

  template <typename T>
  void green(const T& v) {
    m_stream << termcolor::green;
    m_stream << v;
    m_stream << termcolor::reset;
  }

  template <typename T>
  void yellow(const T& v) {
    m_stream << termcolor::yellow;
    m_stream << v;
    m_stream << termcolor::reset;
  }

  template <typename T>
  void blue(const T& v) {
    m_stream << termcolor::blue;
    m_stream << v;
    m_stream << termcolor::reset;
  }

  template <typename T>
  void magenta(const T& v) {
    m_stream << termcolor::magenta;
    m_stream << v;
    m_stream << termcolor::reset;
  }

  template <typename T>
  void cyan(const T& v) {
    m_stream << termcolor::cyan;
    m_stream << v;
    m_stream << termcolor::reset;
  }

  template <typename T>
  void white(const T& v) {
    m_stream << termcolor::white;
    m_stream << v;
    m_stream << termcolor::reset;
  }

private:
  std::ostream& m_stream;
};

}  // namespace charly::utils
