/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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

#include <cmath>

#pragma once

namespace Charly {

// Returns true if the target is big endian
inline bool IS_BIG_ENDIAN() {
#ifdef __BIG_ENDIAN__
  return true;
#else
  return false;
#endif
}

inline bool IS_NAN(double f) {
  uint64_t c = *reinterpret_cast<uint64_t*>(&f);
  return c == 0x7ff8000000000000; // NaN
}

template <typename T>
inline bool FP_ARE_EQUAL(T f1, T f2) {
  if (IS_NAN(f1) && IS_NAN(f2)) return true;
  if (IS_NAN(f1) || IS_NAN(f2)) return false;
  return f1 == f2;
}

inline double FP_STRIP_NAN(double value) {
  return IS_NAN(value) ? 0.0 : value;
}

inline double FP_STRIP_INF(double value) {
  return std::isinf(value) ? 0.0 : value;
}

inline double BITCAST_DOUBLE(int64_t value) {
  return *reinterpret_cast<double*>(&value);
}
}  // namespace Charly
