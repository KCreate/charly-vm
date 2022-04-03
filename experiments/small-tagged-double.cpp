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
#include <cstdlib>
#include <cassert>
#include <iostream>

#include "math.h"

static const uintptr_t kTagFloat = 0b00011111;

static const uintptr_t kMaskSign          = (uintptr_t{0b10000000} << 56);
static const uintptr_t kMaskExponent      = (uintptr_t{0b0111111111110000} << 48);
static const uintptr_t kMaskSmallExponent = (uintptr_t{0b0000111111110000} << 48);
static const uintptr_t kMaskMantissa      = 0x000fffffffffffff;
static const uintptr_t kMaskSmallMantissa = 0x000fffffffffffe0;
static const uintptr_t kMaskTag           = 0b00011111;

// uintptr_t encode(double value) {
//   uintptr_t raw = *reinterpret_cast<uintptr_t*>(&value);
//   uintptr_t sign = (raw & kMaskSign);
//   uintptr_t exponent = (raw & kMaskSmallExponent);
//   uintptr_t mantissa = (raw & kMaskMantissa);
//   return (exponent << 4) + (mantissa << 4) + (sign >> 60) + kTagFloat;
// }

// double decode(uintptr_t value) {
//   value >>= 3;
//   uintptr_t sign_bit = value & 1;
//   value >>= 1;
//   value |= (sign_bit << 63);
//   return *reinterpret_cast<double*>(&value);
// }

uintptr_t encode(double value) {
  uintptr_t raw = *reinterpret_cast<uintptr_t*>(&value);
  return (raw & ~kMaskTag) + kTagFloat;
}

double decode(uintptr_t value) {
  value -= kTagFloat;
  return *reinterpret_cast<double*>(&value);
}

int main() {
  double base = 8388608.0;
  double step = 1.0;
  int n = 100;

  std::cout.precision(30);

  // test precision without boxing
  {
    double sum = base;

    for (int i = 0; i < n; i++) {
      sum += step;
    }

    std::cout << "unboxed sum: " << sum << std::endl;
  }

  // test precision with boxing
  {
    double sum = base;
    uintptr_t encoded_step = encode(step);
    uintptr_t encoded_sum = encode(sum);

    for (int i = 0; i < n; i++) {
      double tmp = decode(encoded_sum) + decode(encoded_step);
      encoded_sum = encode(tmp);
    }

    sum = decode(encoded_sum);

    std::cout << "boxed sum:   " << sum << std::endl;
  }
}
