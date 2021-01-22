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

  template <typename... Args>
  void write(Args&&... params) {
    (m_stream << ... << params);
  }

  template <typename... Args>
  void grey(Args&&... params) {
    write(termcolor::dark, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void grey_bg(Args&&... params) {
    write(termcolor::on_grey, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void red(Args&&... params) {
    write(termcolor::red, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void red_bg(Args&&... params) {
    write(termcolor::on_red, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void green(Args&&... params) {
    write(termcolor::green, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void green_bg(Args&&... params) {
    write(termcolor::on_green, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void yellow(Args&&... params) {
    write(termcolor::yellow, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void yellow_bg(Args&&... params) {
    write(termcolor::on_yellow, termcolor::grey, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void blue(Args&&... params) {
    write(termcolor::blue, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void blue_bg(Args&&... params) {
    write(termcolor::on_blue, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void magenta(Args&&... params) {
    write(termcolor::magenta, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void magenta_bg(Args&&... params) {
    write(termcolor::on_magenta, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void cyan(Args&&... params) {
    write(termcolor::cyan, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void cyan_bg(Args&&... params) {
    write(termcolor::on_cyan, std::forward<Args>(params)..., termcolor::reset);
  }

  template <typename... Args>
  void white(Args&&... params) {
    write(termcolor::white, params..., termcolor::reset);
  }

  template <typename... Args>
  void white_bg(Args&&... params) {
    write(termcolor::on_white, std::forward<Args>(params)..., termcolor::reset);
  }

private:
  std::ostream& m_stream;
};

}  // namespace charly::utils
