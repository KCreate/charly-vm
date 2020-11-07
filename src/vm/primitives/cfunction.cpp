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

void CFunction::init(VALUE name, void* pointer, uint32_t argc) {
  assert(charly_is_symbol(name));

  Container::init(kTypeCFunction, 2);
  this->name              = name;
  this->pointer           = pointer;
  this->argc              = argc;
  this->push_return_value = true;
  this->halt_after_return = false;
}

void CFunction::init(CFunction* source) {
  Container::init(source);
  this->name              = source->name;
  this->pointer           = source->pointer;
  this->argc              = source->argc;
  this->push_return_value = source->push_return_value;
  this->halt_after_return = source->halt_after_return;
}

void CFunction::set_push_return_value(bool value) {
  this->push_return_value = value;
}

void CFunction::set_halt_after_return(bool value) {
  this->halt_after_return = value;
}

VALUE CFunction::get_name() {
  return this->name;
}

void* CFunction::get_pointer() {
  return this->pointer;
}

uint32_t CFunction::get_argc() {
  return this->argc;
}

bool CFunction::get_push_return_value() {
  return this->push_return_value;
}

bool CFunction::get_halt_after_return() {
  return this->halt_after_return;
}

}  // namespace Charly
