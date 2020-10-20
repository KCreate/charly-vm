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

void Header::init(ValueType type) {
  this->type = type;
  this->mark = false;
}

ValueType Header::get_type() {
  return this->type;
}

VALUE Header::as_value() {
  return charly_create_pointer(this);
}

void Header::clean() {
  // do nothing, this method is kept for possible future expansion
}

void Container::init(ValueType type, uint32_t initial_capacity) {
  Header::init(type);

  assert(this->container == nullptr);
  this->container = new Container::ContainerType();
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

void Container::access_container(std::function<void(Container::ContainerType*)> cb) {
  assert(this->container);
  cb(this->container);
}

void Container::access_container_shared(std::function<void(Container::ContainerType*)> cb) {
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

void Array::init(uint32_t initial_capacity) {
  Header::init(kTypeArray);
  this->data = new Array::VectorType();
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

void Array::access_vector(std::function<void(Array::VectorType*)> cb) {
  assert(this->data);
  cb(this->data);
}

void Array::access_vector_shared(std::function<void(Array::VectorType*)> cb) {
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

void Frame::init(Frame* parent, CatchTable* catchtable, Function* function, uint8_t* origin, VALUE self, bool halt) {
  Header::init(kTypeFrame);

  this->parent = parent;
  this->environment = function->get_context();
  this->catchtable = catchtable;
  this->function = function;
  this->self = self;
  this->origin_address = origin;
  this->halt_after_return = halt;

  this->locals = new std::vector<VALUE>(function->get_lvarcount(), kNull);
}

void Frame::clean() {
  Header::clean();
  if (this->locals) {
    delete this->locals;
    this->locals = nullptr;
  }
}

Frame* Frame::get_parent() {
  return this->parent;
}

Frame* Frame::get_environment() {
  return this->environment;
}

CatchTable* Frame::get_catchtable() {
  return this->catchtable;
}

Function* Frame::get_function() {
  return this->function;
}

VALUE Frame::get_self() {
  return this->self;
}

uint8_t* Frame::get_origin_address() {
  return this->origin_address;
}

uint8_t* Frame::get_return_address() {
  if (this->origin_address == nullptr) return nullptr;

  if (this->halt_after_return) {
    return this->origin_address;
  } else {
    Opcode opcode = static_cast<Opcode>(*(this->origin_address));
    return this->origin_address + kInstructionLengths[opcode];
  }
}

bool Frame::get_halt_after_return() {
  return this->halt_after_return;
}

bool Frame::read_local(uint32_t index, VALUE* result) {
  assert(this->locals);
  if (index >= this->locals->size()) return false;
  *result = (* this->locals)[index];
  return true;
}

VALUE Frame::read_local_or(uint32_t index, VALUE fallback) {
  assert(this->locals);
  if (index >= this->locals->size()) return fallback;
  return (* this->locals)[index];
}

bool Frame::write_local(uint32_t index, VALUE value) {
  assert(this->locals);
  if (index >= this->locals->size()) return false;
  (* this->locals)[index] = value;
  return true;
}

void Frame::access_locals(std::function<void(VectorType*)> cb) {
  assert(this->locals);
  cb(this->locals);
}

void Frame::access_locals_shared(std::function<void(VectorType*)> cb) {
  assert(this->locals);
  cb(this->locals);
}

void CatchTable::init(CatchTable* parent, Frame* frame, uint8_t* address, size_t stacksize) {
  Header::init(kTypeCatchTable);
  this->parent = parent;
  this->frame = frame;
  this->address = address;
  this->stacksize = stacksize;
}

CatchTable* CatchTable::get_parent() {
  return this->parent;
}

Frame* CatchTable::get_frame() {
  return this->frame;
}

uint8_t* CatchTable::get_address() {
  return this->address;
}

size_t CatchTable::get_stacksize() {
  return this->stacksize;
}

void CPointer::init(void* data, CPointer::DestructorType destructor) {
  Header::init(kTypeCPointer);
  this->data = data;
  this->destructor = destructor;
}

void CPointer::clean() {
  Header::clean();
  if (this->destructor) {
    this->destructor(this->data);
  }
}

void CPointer::set_data(void* data) {
  this->data = data;
}

void CPointer::set_destructor(CPointer::DestructorType destructor) {
  this->destructor = destructor;
}

void* CPointer::get_data() {
  return this->data;
}

CPointer::DestructorType CPointer::get_destructor() {
  return this->destructor;
}

void Function::init(VALUE name,
                    Frame* context,
                    uint8_t* body,
                    uint32_t argc,
                    uint32_t minimum_argc,
                    uint32_t lvarcount,
                    bool anonymous,
                    bool needs_arguments) {
  Container::init(kTypeFunction, 2);
  this->name = name;
  this->context = context;
  this->body_address = body;
  this->host_class = nullptr;
  this->bound_self = kNull;
  this->bound_self_set = false;
  this->argc = argc;
  this->minimum_argc = minimum_argc;
  this->lvarcount = lvarcount;
  this->anonymous = anonymous;
  this->needs_arguments = needs_arguments;
}

void Function::set_context(Frame* context) {
  this->context = context;
}

void Function::set_host_class(Class* host_class) {
  this->host_class = host_class;
}

void Function::set_bound_self(VALUE bound_self) {
  this->bound_self = bound_self;
  this->bound_self_set = true;
}

void Function::clear_bound_self() {
  this->bound_self = kNull;
  this->bound_self_set = false;
}

VALUE Function::get_name() {
  return this->name;
}

Frame* Function::get_context() {
  return this->context;
}

uint8_t* Function::get_body_address() {
  return this->body_address;
}

Class* Function::get_host_class() {
  return this->host_class;
}

bool Function::get_bound_self(VALUE* target) {
  if (this->bound_self_set) {
    *target = this->bound_self;
    return true;
  }

  return false;
}

uint32_t Function::get_argc() {
  return this->argc;
}

uint32_t Function::get_minimum_argc() {
  return this->minimum_argc;
}

uint32_t Function::get_lvarcount() {
  return this->lvarcount;
}

bool Function::get_anonymous() {
  return this->anonymous;
}

bool Function::get_needs_arguments() {
  return this->needs_arguments;
}

VALUE Function::get_self(VALUE* fallback) {
  if (this->bound_self_set) {
    return this->bound_self;
  }

  if (this->anonymous) {
    if (this->context != nullptr) {
      return this->context->get_self();
    } else {
      return kNull;
    }
  }

  if (fallback != nullptr) {
    return *fallback;
  }

  if (this->context != nullptr) {
    return this->context->get_self();
  }

  return kNull;
}

}  // namespace Charly
