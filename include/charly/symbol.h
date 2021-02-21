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
#include <cstring>

#pragma once

namespace charly {

// compile-time crc32 hash
// source: https://stackoverflow.com/questions/28675727/using-crc32-algorithm-to-hash-string-at-compile-time
namespace crc32 {
  template <unsigned c, int k = 8>
  struct f : f<((c & 1) ? 0xedb88320 : 0) ^ (c >> 1), k - 1> {};
  template <unsigned c>
  struct f<c, 0> {
    enum { value = c };
  };

#define A(x) B(x) B(x + 128)
#define B(x) C(x) C(x + 64)
#define C(x) D(x) D(x + 32)
#define D(x) E(x) E(x + 16)
#define E(x) F(x) F(x + 8)
#define F(x) G(x) G(x + 4)
#define G(x) H(x) H(x + 2)
#define H(x) I(x) I(x + 1)
#define I(x) f<x>::value,

  constexpr unsigned crc_table[] = { A(0) };

  // Constexpr implementation and helpers
  constexpr uint32_t crc32_impl(const uint8_t* p, size_t len, uint32_t crc) {
    return len ? crc32_impl(p + 1, len - 1, (crc >> 8) ^ crc_table[(crc & 0xFF) ^ *p]) : crc;
  }

  constexpr uint32_t crc32(const uint8_t* data, size_t length) {
    return ~crc32_impl(data, length, ~0);
  }

  constexpr size_t strlen_c(const char* str) {
    return *str ? 1 + strlen_c(str + 1) : 0;
  }
};

using SYMBOL = uint32_t;

// constexpr 32-bit symbol
inline constexpr SYMBOL SYM(const char* str) {
  return crc32::crc32((uint8_t*)str, crc32::strlen_c(str));
}

inline SYMBOL SYM(const std::string& str) {
  return SYM(str.c_str());
}

}
