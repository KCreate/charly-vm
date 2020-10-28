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

void Container::init(ValueType type, uint32_t initial_capacity) {
  Header::init(type);

  assert(this->container == nullptr);
  this->container = new Container::ContainerType();
  this->container->reserve(initial_capacity);
}

void Container::init(Container* source) {
  Header::init(source->type);
  assert(this->container == nullptr);
  this->container = new Container::ContainerType(*(source->container));
}

void Container::clean() {
  Header::clean();
  assert(this->container);
  delete this->container;
  this->container = nullptr;
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

void Container::access_container(std::function<void(Container::ContainerType*)> cb) {
  assert(this->container);
  cb(this->container);
}

void Container::access_container_shared(std::function<void(Container::ContainerType*)> cb) {
  assert(this->container);
  cb(this->container);
}

}  // namespace Charly
