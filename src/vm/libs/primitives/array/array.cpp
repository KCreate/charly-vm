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

#include <iostream>

#include <cmath>
#include <complex>
#include <random>
#include "math.h"
#include "vm.h"
#include "array.h"
#include "managedcontext.h"

namespace Charly {
namespace Internals {
namespace PrimitiveArray {

VALUE insert(VM& vm, VALUE a, VALUE i, VALUE v) {
  CHECK(array, a);
  CHECK(number, i);

  Array* array = charly_as_array(a);
  array->insert(charly_number_to_int64(i), v);

  return array->as_value();
}

VALUE remove(VM& vm, VALUE a, VALUE i) {
  CHECK(array, a);
  CHECK(number, i);

  Array* array = charly_as_array(a);
  array->remove(charly_number_to_int64(i));

  return array->as_value();
}

VALUE reverse(VM& vm, VALUE a) {
  CHECK(array, a);

  Array* array = charly_as_array(a);
  Charly::ManagedContext lalloc(vm);

  Array* new_array;
  array->access_vector_shared([&](Array::VectorType* vec) {
    new_array = charly_as_array(lalloc.create_array(vec->size()));

    auto it = vec->rbegin();
    while (it != vec->rend()) {
      new_array->push(*it);
      it++;
    }
  });

  return new_array->as_value();
}

VALUE index(VM& vm, VALUE a, VALUE i, VALUE o) {
  CHECK(array, a);
  CHECK(number, i);
  CHECK(number, o);

  Array* array = charly_as_array(a);
  int32_t offset = charly_number_to_int32(o);
  int32_t found_offset = -1;

  array->access_vector_shared([&](Array::VectorType* vec) {

    // wrap around negative indices
    if (offset < 0) {
      offset += vec->size();
      if (offset < 0) {
        return;
      }
    }

    // iterate through array, starting at the beginning
    while (offset < static_cast<int32_t>(vec->size())) {
      VALUE v = (* vec)[offset];

      if (vm.eq(i, v) == kTrue) {
        found_offset = offset;
        break;
      }

      offset++;
    }
  });

  return charly_create_number(found_offset);
}

VALUE rindex(VM& vm, VALUE a, VALUE i, VALUE o) {
  CHECK(array, a);
  CHECK(number, i);
  CHECK(number, o);

  Array* array = charly_as_array(a);
  int32_t offset = charly_number_to_int32(o);
  int32_t found_offset = -1;

  array->access_vector_shared([&](Array::VectorType* vec) {

    // wrap around negative indices
    if (offset < 0) {
      offset += vec->size();
      if (offset < 0) {
        return;
      }
    }

    // iterate through array, starting from the back
    while (offset >= 0) {
      if (static_cast<uint32_t>(offset) >= vec->size()) {
        offset--;
        continue;
      }

      VALUE v = (* vec)[offset];

      if (vm.eq(i, v) == kTrue) {
        found_offset = offset;
        return;
      }

      offset--;
    }
  });

  return charly_create_number(found_offset);
}

VALUE range(VM& vm, VALUE a, VALUE s, VALUE c) {
  CHECK(array, a);
  CHECK(number, s);
  CHECK(number, c);

  Array* array = charly_as_array(a);
  int32_t start = charly_number_to_uint32(s);
  uint32_t count = charly_number_to_uint32(c);

  ManagedContext lalloc(vm);
  Array* new_array = charly_as_array(lalloc.create_array(count));

  array->access_vector_shared([&](Array::VectorType* vec) {
    uint32_t offset = 0;

    while (offset < count) {
      int32_t index = start + offset;

      // wrap negative indices
      if (index < 0) {
        index += vec->size();
        if (index < 0) {
          offset++;
          continue;
        }
      }

      // break loop once we reach the end of the array
      if (index >= static_cast<int32_t>(vec->size())) {
        break;
      }

      new_array->push((* vec)[index]);
      offset++;
    }
  });

  return new_array->as_value();
}

VALUE clear(VM& vm, VALUE a) {
  CHECK(array, a);

  Array* array = charly_as_array(a);
  array->clear();

  return a;
}

}  // namespace PrimitiveArray
}  // namespace Internals
}  // namespace Charly
