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
#include "value.h"
#include "vm.h"

#pragma once

namespace Charly {
  namespace Machine {
    namespace Operators {

      // Arithmetics
      VALUE add(VM* vm, VALUE left, VALUE right);
      VALUE sub(VM* vm, VALUE left, VALUE right);
      VALUE mul(VM* vm, VALUE left, VALUE right);
      VALUE div(VM* vm, VALUE left, VALUE right);
      VALUE mod(VM* vm, VALUE left, VALUE right);
      VALUE pow(VM* vm, VALUE left, VALUE right);
      VALUE uadd(VM* vm, VALUE value);
      VALUE usub(VM* vm, VALUE value);

      // Comparison operators
      VALUE eq(VM* vm, VALUE left, VALUE right);
      VALUE neq(VM* vm, VALUE left, VALUE right);
      VALUE lt(VM* vm, VALUE left, VALUE right);
      VALUE gt(VM* vm, VALUE left, VALUE right);
      VALUE le(VM* vm, VALUE left, VALUE right);
      VALUE ge(VM* vm, VALUE left, VALUE right);
      VALUE unot(VM* vm, VALUE value);

      // Bitwise operators
      VALUE shr(VM* vm, VALUE left, VALUE right);
      VALUE shr(VM* vm, VALUE left, VALUE right);
      VALUE band(VM* vm, VALUE left, VALUE right);
      VALUE bor(VM* vm, VALUE left, VALUE right);
      VALUE bxor(VM* vm, VALUE left, VALUE right);
      VALUE ubnot(VM* vm, VALUE value);
    }
  }
}









