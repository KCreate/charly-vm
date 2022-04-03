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

#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

#pragma once

namespace charly::utils {

inline int64_t charptr_to_int(const char* data, size_t length, int base = 10) {
  char buffer[length + 1];
  std::memset(buffer, 0, length + 1);
  std::memcpy(buffer, data, length);

  char* end_ptr;
  int64_t result = std::strtol(buffer, &end_ptr, base);

  if (errno == ERANGE) {
    errno = 0;
    return 0;
  }

  if (end_ptr == buffer) {
    return 0;
  }

  return result;
}

inline int64_t string_to_int(const std::string& str, int base = 10) {
  return charptr_to_int(str.data(), str.size(), base);
}

inline int64_t string_view_to_int(const std::string_view& view, int base = 10) {
  return charptr_to_int(view.data(), view.size(), base);
}

inline double charptr_to_double(const char* data, size_t length) {
  char buffer[length + 1];
  std::memset(buffer, 0, length + 1);
  std::memcpy(buffer, data, length);

  char* end_ptr;
  double result = std::strtod(buffer, &end_ptr);

  if (result == HUGE_VAL || end_ptr == buffer)
    return NAN;

  return result;
}

inline double string_to_double(const std::string& str) {
  return charptr_to_double(str.data(), str.size());
}

inline double string_view_to_double(const std::string_view& view) {
  return charptr_to_double(view.data(), view.size());
}

}  // namespace charly::utils
