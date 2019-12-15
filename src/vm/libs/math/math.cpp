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

#include <iostream>

#include <cmath>
#include <complex>
#include <random>
#include "math.h"
#include "vm.h"

using namespace std;

namespace Charly {
namespace Internals {
namespace Math {

std::random_device rand_engine;

VALUE cos(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::cos(charly_number_to_double(n)));
}

VALUE cosh(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::cosh(charly_number_to_double(n)));
}

VALUE acos(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::acos(charly_number_to_double(n)));
}

VALUE acosh(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::acosh(charly_number_to_double(n)));
}

VALUE sin(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::sin(charly_number_to_double(n)));
}

VALUE sinh(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::sinh(charly_number_to_double(n)));
}

VALUE asin(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::asin(charly_number_to_double(n)));
}

VALUE asinh(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::asinh(charly_number_to_double(n)));
}

VALUE tan(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::tan(charly_number_to_double(n)));
}

VALUE tanh(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::tanh(charly_number_to_double(n)));
}

VALUE atan(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::atan(charly_number_to_double(n)));
}

VALUE atanh(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::atanh(charly_number_to_double(n)));
}

VALUE cbrt(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::cbrt(charly_number_to_double(n)));
}

VALUE sqrt(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::sqrt(charly_number_to_double(n)));
}

VALUE ceil(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::ceil(charly_number_to_double(n)));
}

VALUE floor(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::floor(charly_number_to_double(n)));
}

VALUE log(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::log(charly_number_to_double(n)));
}

VALUE log2(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::log2(charly_number_to_double(n)));
}

VALUE log10(VM& vm, VALUE n) {
  (void)vm;
  CHECK(number, n);
  return charly_create_number(std::log10(charly_number_to_double(n)));
}

VALUE rand(VM& vm, VALUE min, VALUE max) {
  (void)vm;
  CHECK(number, min);
  CHECK(number, max);
  return charly_create_double(std::uniform_real_distribution<double>(
    charly_number_to_double(min),
    charly_number_to_double(max)
  )(rand_engine));
}

}  // namespace Math
}  // namespace Internals
}  // namespace Charly
