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

#include <cassert>

#include "value.h"
#include "opcode.h"

namespace Charly {

void Array::init(uint32_t initial_capacity) {
  Header::init(kTypeArray);
  this->data = new Array::VectorType();
  this->data->reserve(initial_capacity);
}

void Array::init(Array* source) {
  Header::init(kTypeArray);
  this->data = new Array::VectorType(*(source->data));
}

void Array::clean() {
  Header::clean();
  assert(this->data);
  delete this->data;
  this->data = nullptr;
}

uint32_t Array::size() {
  assert(this->data);
  return this->data->size();
}

VALUE Array::read(int64_t index) {
  assert(this->data);

  // wrap around negative indices
  if (index < 0) {
    index += this->data->size();
  }

  // bounds check
  if (index < 0 || index >= static_cast<int64_t>(this->data->size())) {
    return kNull;
  }

  return (* this->data)[index];
}

VALUE Array::write(int64_t index, VALUE value) {
  assert(this->data);

  // check if appending to array
  if (index == static_cast<int64_t>(this->data->size())) {
    this->data->push_back(value);
    return value;
  }

  // wrap around negative indices
  if (index < 0) {
    index += this->data->size();
  }

  // bounds check
  if (index < 0 || index > static_cast<int64_t>(this->data->size())) {
    return kNull;
  }

  (* this->data)[index] = value;
  return value;
}

void Array::insert(int64_t index, VALUE value) {
  assert(this->data);

  // check if appending to array
  if (index == static_cast<int64_t>(this->data->size())) {
    this->data->push_back(value);
    return;
  }

  // wrap around negative indices
  if (index < 0) {
    index += this->data->size();
  }

  // bounds check
  if (index < 0 || index > static_cast<int64_t>(this->data->size())) {
    return;
  }

  auto it = this->data->begin() + index;
  this->data->insert(it, value);
}

void Array::push(VALUE value) {
  assert(this->data);
  this->data->push_back(value);
}

void Array::remove(int64_t index) {
  assert(this->data);

  // wrap around negative indices
  if (index < 0) {
    index += this->data->size();
  }

  // bounds check
  if (index < 0 || index >= static_cast<int64_t>(this->data->size())) {
    return;
  }

  auto it = this->data->begin() + index;
  this->data->erase(it);
}

void Array::clear() {
  assert(this->data);
  this->data->clear();
}

void Array::fill(VALUE value, uint32_t count) {
  assert(this->data);
  this->data->assign(count, value);
}

void Array::access_vector(std::function<void(Array::VectorType*)> cb) {
  assert(this->data);
  cb(this->data);
}

void Array::access_vector_shared(std::function<void(Array::VectorType*)> cb) {
  assert(this->data);
  cb(this->data);
}

}  // namespace Charly
