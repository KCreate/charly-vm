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
#include "opcode.h"

namespace Charly {

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

}  // namespace Charly
