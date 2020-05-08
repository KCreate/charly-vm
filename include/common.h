/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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
  return (*(uint16_t*)"\0\xff" < 0x100);
}

template <typename T>
bool FP_ARE_EQUAL(T f1, T f2) {
  if (std::isnan(f1) && std::isnan(f2)) return true;
  return (std::fabs(f1 - f2) <= std::numeric_limits<T>::epsilon() * std::fmax(fabs(f1), fabs(f2)));
}

// Branchlessly replaces NAN with 0.0
inline double FP_STRIP_NAN(double value) {
  return std::isnan(value) ? 0.0 : value;
}

// Branchlessly replaces NAN with 0.0
inline double FP_STRIP_INF(double value) {
  return std::isinf(value) ? 0.0 : value;
}

inline double BITCAST_DOUBLE(int64_t value) {
  return *reinterpret_cast<double*>(&value);
}
}  // namespace Charly
