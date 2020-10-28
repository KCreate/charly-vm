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

void Function::init(VALUE name,
                    Frame* context,
                    uint8_t* body,
                    uint32_t argc,
                    uint32_t minimum_argc,
                    uint32_t lvarcount,
                    bool anonymous,
                    bool needs_arguments) {
  assert(charly_is_symbol(name));

  Container::init(kTypeFunction, 2);
  this->name            = name;
  this->context         = context;
  this->body_address    = body;
  this->host_class      = nullptr;
  this->bound_self      = kNull;
  this->bound_self_set  = false;
  this->argc            = argc;
  this->minimum_argc    = minimum_argc;
  this->lvarcount       = lvarcount;
  this->anonymous       = anonymous;
  this->needs_arguments = needs_arguments;
}

void Function::init(Function* source) {
  Container::init(source);
  this->name            = source->name;
  this->context         = source->context;
  this->body_address    = source->body_address;
  this->host_class      = source->host_class;
  this->bound_self      = source->bound_self;
  this->bound_self_set  = source->bound_self_set;
  this->argc            = source->argc;
  this->minimum_argc    = source->minimum_argc;
  this->lvarcount       = source->lvarcount;
  this->anonymous       = source->anonymous;
  this->needs_arguments = source->needs_arguments;
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

bool Function::get_bound_self(VALUE* result) {
  if (this->bound_self_set) {
    *result = this->bound_self;
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
