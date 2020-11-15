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
#include <random>
#include "math.h"

namespace Charly {
namespace Internals {
namespace Math {

Result cos(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::cos(charly_number_to_double(n)));
}

Result cosh(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::cosh(charly_number_to_double(n)));
}

Result acos(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::acos(charly_number_to_double(n)));
}

Result acosh(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::acosh(charly_number_to_double(n)));
}

Result sin(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::sin(charly_number_to_double(n)));
}

Result sinh(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::sinh(charly_number_to_double(n)));
}

Result asin(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::asin(charly_number_to_double(n)));
}

Result asinh(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::asinh(charly_number_to_double(n)));
}

Result tan(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::tan(charly_number_to_double(n)));
}

Result tanh(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::tanh(charly_number_to_double(n)));
}

Result atan(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::atan(charly_number_to_double(n)));
}

Result atanh(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::atanh(charly_number_to_double(n)));
}

Result cbrt(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::cbrt(charly_number_to_double(n)));
}

Result sqrt(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::sqrt(charly_number_to_double(n)));
}

Result ceil(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::ceil(charly_number_to_double(n)));
}

Result floor(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::floor(charly_number_to_double(n)));
}

Result log(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::log(charly_number_to_double(n)));
}

Result log2(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::log2(charly_number_to_double(n)));
}

Result log10(VM&, VALUE n) {
  CHECK(number, n);
  return charly_create_number(std::log10(charly_number_to_double(n)));
}

Result rand(VM&, VALUE min, VALUE max) {
  CHECK(number, min);
  CHECK(number, max);

  static std::random_device rand_engine;
  return charly_create_double(std::uniform_real_distribution<double>(
    charly_number_to_double(min),
    charly_number_to_double(max)
  )(rand_engine));
}

}  // namespace Math
}  // namespace Internals
}  // namespace Charly
