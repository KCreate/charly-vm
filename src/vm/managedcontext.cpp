/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include "managedcontext.h"

namespace Charly {
Frame* ManagedContext::create_frame(VALUE self, Function* calling_function, uint8_t* return_address) {
  Frame* frame = this->vm.create_frame(self, calling_function, return_address);
  this->vm.context.gc->register_temporary(reinterpret_cast<VALUE>(frame));
  this->temporaries.push_back(reinterpret_cast<VALUE>(frame));
  return frame;
}

InstructionBlock* ManagedContext::create_instructionblock() {
  InstructionBlock* block = this->vm.create_instructionblock();
  this->vm.context.gc->register_temporary(reinterpret_cast<VALUE>(block));
  this->temporaries.push_back(reinterpret_cast<VALUE>(block));
  return block;
}

CatchTable* ManagedContext::create_catchtable(uint8_t* address) {
  CatchTable* table = this->vm.create_catchtable(address);
  this->vm.context.gc->register_temporary(reinterpret_cast<VALUE>(table));
  this->temporaries.push_back(reinterpret_cast<VALUE>(table));
  return table;
}

VALUE ManagedContext::create_object(uint32_t initial_capacity) {
  VALUE object = this->vm.create_object(initial_capacity);
  this->vm.context.gc->register_temporary(object);
  this->temporaries.push_back(object);
  return object;
}

VALUE ManagedContext::create_array(uint32_t initial_capacity) {
  VALUE array = this->vm.create_array(initial_capacity);
  this->vm.context.gc->register_temporary(array);
  this->temporaries.push_back(array);
  return array;
}

VALUE ManagedContext::create_integer(int64_t value) {
  // Integers are never allocated in the GC so we just return whatever we got from the VM
  return this->vm.create_integer(value);
}

VALUE ManagedContext::create_float(double value) {
  VALUE floatval = this->vm.create_float(value);
  if (is_special(floatval)) {
    this->vm.context.gc->register_temporary(floatval);
    this->temporaries.push_back(floatval);
  }
  return floatval;
}

VALUE ManagedContext::create_string(char* data, uint32_t length) {
  VALUE string = this->vm.create_string(data, length);
  this->vm.context.gc->register_temporary(string);
  this->temporaries.push_back(string);
  return string;
}

VALUE ManagedContext::create_function(VALUE name,
                                      uint8_t* body_address,
                                      uint32_t argc,
                                      uint32_t lvarcount,
                                      bool anonymous,
                                      InstructionBlock* block) {
  VALUE func = this->vm.create_function(name, body_address, argc, lvarcount, anonymous, block);
  this->vm.context.gc->register_temporary(func);
  this->temporaries.push_back(func);
  return func;
}

VALUE ManagedContext::create_cfunction(VALUE name, uint32_t argc, FPOINTER pointer) {
  VALUE func = this->vm.create_cfunction(name, argc, pointer);
  this->vm.context.gc->register_temporary(func);
  this->temporaries.push_back(func);
  return func;
}
}  // namespace Charly
