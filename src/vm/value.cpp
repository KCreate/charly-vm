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

namespace Charly {

void Header::init(ValueType type) {
  this->type = type;
  this->mark = false;
}

ValueType Header::get_type() {
  return this->type;
}

bool Header::get_gc_mark() {
  return this->mark;
}

void Header::set_gc_mark() {
  this->mark = true;
}

void Header::clear_gc_mark() {
  this->mark = false;
}

VALUE Header::as_value() {
  return reinterpret_cast<VALUE>(this);
}

void Header::clean() {
  // do nothing, this method is kept for possible future expansion
}

void Container::init(ValueType type, uint32_t initial_capacity) {
  Header::init(type);

  assert(this->container == nullptr);
  this->container = new ContainerType();
  this->container->reserve(initial_capacity);
}

void Container::copy_container_from(const Container* other) {
  assert(this->container);
  this->container->insert(other->container->begin(), other->container->end());
}

bool Container::read(VALUE key, VALUE* result) {
  assert(this->container);
  if (this->container->count(key) == 0) return false;
  *result = (* this->container)[key];
  return true;
}

VALUE Container::read_or(VALUE key, VALUE fallback) {
  assert(this->container);
  if (this->container->count(key) == 1) {
    return (* this->container)[key];
  }

  return fallback;
}

bool Container::contains(VALUE key) {
  assert(this->container);
  return this->container->count(key);
}

uint32_t Container::keycount() {
  assert(this->container);
  return this->container->size();
}

bool Container::erase(VALUE key) {
  assert(this->container);
  return this->container->erase(key) > 0;
}

void Container::write(VALUE key, VALUE value) {
  assert(this->container);
  this->container->insert_or_assign(key, value);
}

bool Container::assign(VALUE key, VALUE value) {
  assert(this->container);
  if (this->container->count(key) == 0) return false;
  (* this->container)[key] = value;
  return true;
}

void Container::access_container(std::function<void(ContainerType*)> cb) {
  assert(this->container);
  cb(this->container);
}

void Container::access_container_shared(std::function<void(ContainerType*)> cb) {
  assert(this->container);
  cb(this->container);
}

void Container::clean() {
  Header::clean();
  assert(this->container);
  delete this->container;
  this->container = nullptr;
}

void Object::init(VALUE klass, uint32_t initial_capacity) {
  Container::init(kTypeObject, initial_capacity);
  this->klass = klass;
}

VALUE Object::get_klass() {
  return this->klass;
}

void Object::set_klass(VALUE klass) {
  this->klass = klass;
}

void Array::init(uint32_t initial_capacity) {
  Header::init(kTypeArray);
  this->data = new VectorType();
  this->data->reserve(initial_capacity);
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

void Array::access_vector(std::function<void(VectorType*)> cb) {
  assert(this->data);
  cb(this->data);
}

void Array::access_vector_shared(std::function<void(VectorType*)> cb) {
  assert(this->data);
  cb(this->data);
}

void String::init(char* data, uint32_t length) {
  Header::init(kTypeString);
  this->data = data;
  this->length = length;
}

void String::init_copy(const char* data, uint32_t length) {
  Header::init(kTypeString);

  // allocate new buffer and copy the data
  char* buffer = (char*)malloc(length);
  std::memcpy(buffer, data, length);
  this->data = buffer;
  this->length = length;
}

void String::init_copy(const std::string& source) {
  this->init_copy(source.c_str(), source.size());
}

void String::clean() {
  Header::clean();
  std::free(data);
}

char* String::get_data() {
  return this->data;
}

uint32_t String::get_length() {
  return this->length;
}

}  // namespace Charly
