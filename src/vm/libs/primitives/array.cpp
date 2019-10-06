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
namespace PrimitiveArray {

VALUE insert(VM& vm, VALUE a, VALUE i, VALUE v) {
  CHECK(array, a);
  CHECK(number, i);

  Array* array = charly_as_array(a);
  uint32_t index = charly_number_to_uint32(i);

  // Insert at end of array
  if (index == array->data->size()) {
    array->data->push_back(v);
    return charly_create_pointer(array);
  }

  // Wrap around negative indices
  if (index < 0) {
    index += array->data->size();
  }

  // Out-of-bounds check
  if (index >= array->data->size() || index < 0) {
    vm.throw_exception("Index out of bounds");
    return kNull;
  }

  // Insert the element into the array
  auto it = array->data->begin();
  array->data->insert(it + index, v);

  return charly_create_pointer(array);
}

VALUE remove(VM& vm, VALUE a, VALUE i) {
  CHECK(array, a);
  CHECK(number, i);
  return kNull;
}

VALUE reverse(VM& vm, VALUE a) {
  CHECK(array, a);
  return kNull;
}

VALUE flatten(VM& vm, VALUE a) {
  CHECK(array, a);
  return kNull;
}

VALUE index(VM& vm, VALUE a, VALUE i, VALUE o) {
  CHECK(array, a);
  CHECK(number, i);
  CHECK(number, o);
  return kNull;
}

VALUE rindex(VM& vm, VALUE a, VALUE i, VALUE o) {
  CHECK(array, a);
  CHECK(number, i);
  CHECK(number, o);
  return kNull;
}

VALUE range(VM& vm, VALUE a, VALUE s, VALUE c) {
  CHECK(array, a);
  CHECK(number, s);
  CHECK(number, c);
  return kNull;
}

}  // namespace Array
}  // namespace Internals
}  // namespace Charly
