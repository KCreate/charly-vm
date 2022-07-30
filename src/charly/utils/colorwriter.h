/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

enum class Color : uint8_t
{
  Grey,
  Red,
  Green,
  Yellow,
  Blue,
  Magenta,
  Cyan,
  White
};

class ColorWriter {
public:
  explicit ColorWriter(std::ostream& stream = std::cout) : m_stream(stream) {}

  void set_fg_color(Color color) {
    switch (color) {
      case Color::Grey: m_stream << termcolor::dark; break;
      case Color::Red: m_stream << termcolor::red; break;
      case Color::Green: m_stream << termcolor::green; break;
      case Color::Yellow: m_stream << termcolor::yellow; break;
      case Color::Blue: m_stream << termcolor::blue; break;
      case Color::Magenta: m_stream << termcolor::magenta; break;
      case Color::Cyan: m_stream << termcolor::cyan; break;
      case Color::White: m_stream << termcolor::white; break;
    }
  }

  void set_bg_color(Color color) {
    switch (color) {
      case Color::Grey: m_stream << termcolor::on_grey; break;
      case Color::Red: m_stream << termcolor::on_red; break;
      case Color::Green: m_stream << termcolor::on_green; break;
      case Color::Yellow: m_stream << termcolor::on_yellow << termcolor::grey; break;
      case Color::Blue: m_stream << termcolor::on_blue; break;
      case Color::Magenta: m_stream << termcolor::on_magenta; break;
      case Color::Cyan: m_stream << termcolor::on_cyan; break;
      case Color::White: m_stream << termcolor::on_white << termcolor::grey; break;
    }
  }

  void reset_color() {
    m_stream << termcolor::reset;
  }

  template <typename T>
  ColorWriter& operator<<(T&& v) {
    m_stream << v;
    return *this;
  }

  template <typename... Args>
  void fg(Color color, Args&&... params) {
    set_fg_color(color);
    ((m_stream << std::forward<Args>(params)), ...);
    reset_color();
  }

  template <typename... Args>
  void bg(Color color, Args&&... params) {
    set_bg_color(color);
    ((m_stream << std::forward<Args>(params)), ...);
    reset_color();
  }

private:
  std::ostream& m_stream;
};

}  // namespace charly::utils
