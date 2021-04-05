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

#include "charly/utils/buffer.h"

namespace charly::utils {

void Buffer::write_utf8(uint32_t cp) {
  this->reserve_space(m_size + 4);

  char* buffer_initial = m_data + m_size;
  char* buffer_ptr = buffer_initial;
  buffer_ptr = utf8::append(cp, buffer_ptr);
  m_size += buffer_ptr - buffer_initial;
  m_cursor = m_size;
}

uint32_t Buffer::peek_utf8(uint32_t nth) const {
  char* buffer_initial = m_data + m_readoffset;
  char* buffer_ptr = buffer_initial;

  if (m_readoffset >= m_size)
    return '\0';

  // walk forward to requested character
  while (nth--) {
    utf8::next(buffer_ptr, m_data + m_size);

    // overflow check
    if (buffer_ptr >= m_data + m_size) {
      return '\0';
    }
  }

  return utf8::peek_next(buffer_ptr, m_data + m_size);
}

uint32_t Buffer::read_utf8() {
  if (m_readoffset >= m_size)
    return L'\0';

  // advance by one utf8 codepoint
  char* buffer_initial = m_data + m_readoffset;
  char* buffer_ptr = buffer_initial;
  uint32_t codepoint = utf8::next(buffer_ptr, m_data + m_size);
  m_readoffset += buffer_ptr - buffer_initial;
  return codepoint;
}

void Buffer::reset_window() {
  m_window = m_readoffset;
}

std::string Buffer::window_string() const {
  size_t window_size = m_readoffset - m_window;

  if (window_size == 0)
    return std::string("");

  return std::string(m_data + m_window, window_size);
}

}  // namespace charly::utils
