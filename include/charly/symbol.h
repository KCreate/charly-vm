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

#include <cstdint>
#include <cstring>
#include <string>

#pragma once

namespace charly {

// compile-time crc32 hash
// source: https://stackoverflow.com/questions/28675727/using-crc32-algorithm-to-hash-string-at-compile-time
namespace crc32 {
  namespace internal {
    template <unsigned c, int k = 8>
    struct f : f<((c & 1) ? 0xedb88320 : 0) ^ (c >> 1), k - 1> {};
    template <unsigned c>
    struct f<c, 0> {
      enum {
        value = c
      };
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
#undef A
#undef B
#undef C
#undef D
#undef E
#undef F
#undef G
#undef H
#undef I

    namespace constexpr_impl {
      constexpr uint32_t crc32_impl(const char* p, size_t len, uint32_t crc) {
        return len ? crc32_impl(p + 1, len - 1, (crc >> 8) ^ crc_table[(crc & 0xFF) ^ *p]) : crc;
      }

      constexpr uint32_t crc32(const char* data, size_t length) {
        return ~crc32_impl(data, length, ~0);
      }

      constexpr size_t strlen_c(const char* str) {
        return *str ? 1 + strlen_c(str + 1) : 0;
      }
    }  // namespace constexpr_impl

    inline constexpr uint32_t hash_constexpr(const char* str) {
      return constexpr_impl::crc32(str, constexpr_impl::strlen_c(str));
    }

    inline uint32_t hash_block(const char* data, size_t size) {
      uint32_t c = 0xFFFFFFFF;
      auto u = reinterpret_cast<const uint8_t*>(data);
      for (size_t i = 0; i < size; i++) {
        c = crc_table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
      }
      return c ^ 0xFFFFFFFF;
    }
  }  // namespace internal

  inline uint32_t hash_block(const char* data, size_t size) {
    return internal::hash_block(data, size);
  }

  inline uint32_t hash_string(const std::string& string) {
    return internal::hash_block(string.data(), string.size());
  }

  inline uint32_t hash_block(const std::string_view& view) {
    return internal::hash_block(view.data(), view.size());
  }
}  // namespace crc32

using SYMBOL = uint32_t;

inline constexpr SYMBOL SYM(const char* str) {
  return crc32::internal::hash_constexpr(str);
}

}  // namespace charly
