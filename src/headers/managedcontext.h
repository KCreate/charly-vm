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

#include "defines.h"
#include "gc.h"
#include "value.h"
#include "vm.h"

#pragma once

namespace Charly {
class ManagedContext {
private:
  VM& vm;
  std::vector<VALUE> temporaries;

public:
  ManagedContext(VM& t_vm) : vm(t_vm) {
  }
  ~ManagedContext() {
    for (auto& temp : this->temporaries) {
      this->vm.gc.unregister_temporary(temp);
    }
  }

  // Misc. VM data structures
  Frame* create_frame(VALUE self, Function* calling_function, uint8_t* return_address);
  InstructionBlock* create_instructionblock(uint32_t lvarcount);
  CatchTable* create_catchtable(ThrowType type, uint8_t* address);

  // VALUE types
  VALUE create_object(uint32_t initial_capacity);
  VALUE create_array(uint32_t initial_capacity);
  VALUE create_integer(int64_t value);
  VALUE create_float(double value);
  VALUE create_string(char* data, uint32_t length);
  VALUE create_function(VALUE name, uint32_t argc, bool anonymous, InstructionBlock* block);
  VALUE create_cfunction(VALUE name, uint32_t argc, FPOINTER pointer);
};
}  // namespace Charly
