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
  explicit ColorWriter(std::ostream& stream = std::cout) : m_stream(stream) {}

  template <typename... Args>
  void write(Args&&... params) {
    (m_stream << ... << params);
  }

  template <typename T>
  ColorWriter& operator<<(T&& v) {
    m_stream << v;
    return *this;
  }

  template <typename... Args>
  void fg(Color color, Args&&... params) {
    switch (color) {
      case Color::Grey: write(termcolor::dark, std::forward<Args>(params)..., termcolor::reset); break;
      case Color::Red: write(termcolor::red, std::forward<Args>(params)..., termcolor::reset); break;
      case Color::Green: write(termcolor::green, std::forward<Args>(params)..., termcolor::reset); break;
      case Color::Yellow: write(termcolor::yellow, std::forward<Args>(params)..., termcolor::reset); break;
      case Color::Blue: write(termcolor::blue, std::forward<Args>(params)..., termcolor::reset); break;
      case Color::Magenta: write(termcolor::magenta, std::forward<Args>(params)..., termcolor::reset); break;
      case Color::Cyan: write(termcolor::cyan, std::forward<Args>(params)..., termcolor::reset); break;
      case Color::White: write(termcolor::white, std::forward<Args>(params)..., termcolor::reset); break;
    }
  }

  template <typename... Args>
  void bg(Color color, Args&&... params) {
    switch (color) {
      case Color::Grey: write(termcolor::on_grey, std::forward<Args>(params)..., termcolor::reset); break;
      case Color::Red: write(termcolor::on_red, std::forward<Args>(params)..., termcolor::reset); break;
      case Color::Green: write(termcolor::on_green, std::forward<Args>(params)..., termcolor::reset); break;
      case Color::Yellow:
        write(termcolor::on_yellow, termcolor::grey, std::forward<Args>(params)..., termcolor::reset);
        break;
      case Color::Blue: write(termcolor::on_blue, std::forward<Args>(params)..., termcolor::reset); break;
      case Color::Magenta: write(termcolor::on_magenta, std::forward<Args>(params)..., termcolor::reset); break;
      case Color::Cyan: write(termcolor::on_cyan, std::forward<Args>(params)..., termcolor::reset); break;
      case Color::White:
        write(termcolor::on_white, termcolor::grey, std::forward<Args>(params)..., termcolor::reset);
        break;
    }
  }

private:
  std::ostream& m_stream;
};

}  // namespace charly::utils
