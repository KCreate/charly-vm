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

#include "value.h"

namespace Charly {

void Class::init(VALUE name, Function* constructor, Class* parent_class) {
  assert(charly_is_symbol(name));

  Container::init(kTypeClass, 2);
  this->name              = name;
  this->constructor       = constructor;
  this->parent_class      = parent_class;
  this->prototype         = nullptr;
  this->member_properties = new Class::VectorType();
}

void Class::clean() {
  assert(this->member_properties);
  delete this->member_properties;
  this->member_properties = nullptr;
}

void Class::set_prototype(Object* prototype) {
  this->prototype = prototype;
}

VALUE Class::get_name() {
  return this->name;
}

Class* Class::get_parent_class() {
  return this->parent_class;
}

Function* Class::get_constructor() {
  return this->constructor;
}

Object* Class::get_prototype() {
  return this->prototype;
}

uint32_t Class::get_member_property_count() {
  assert(this->member_properties);
  return this->member_properties->size();
}

bool Class::find_value(VALUE symbol, VALUE* result) {
  if (Object* prototype = this->prototype) {
    if (prototype->read(symbol, result)) {
      return true;
    }

    if (Class* parent = this->parent_class) {
      return parent->find_value(symbol, result);
    }
  }

  return false;
}

bool Class::find_super_value(VALUE symbol, VALUE* result) {
  if (Class* parent = this->parent_class) {
    return parent->find_value(symbol, result);
  }

  return false;
}

Function* Class::find_constructor() {
  Class* search_class = this;

  while (search_class) {
    if (search_class->constructor)
      return search_class->constructor;

    search_class = search_class->parent_class;
  }

  return nullptr;
}

Function* Class::find_super_constructor() {
  if (Class* parent = this->parent_class) {
    return parent->find_constructor();
  }

  return nullptr;
}

void Class::initialize_member_properties(Object* object) {
  if (Class* parent = this->parent_class) {
    parent->initialize_member_properties(object);
  }

  for (const auto& field : *(this->member_properties)) {
    object->write(field, kNull);
  }
}

void Class::access_member_properties(std::function<void(Class::VectorType*)> cb) {
  assert(this->member_properties);
  cb(this->member_properties);
}

void Class::access_member_properties_shared(std::function<void(Class::VectorType*)> cb) {
  assert(this->member_properties);
  cb(this->member_properties);
}

}  // namespace Charly
