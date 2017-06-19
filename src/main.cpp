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

#include <iostream>

#include "charly.h"

using namespace std;
using namespace Charly;
using namespace Charly::Machine;
using namespace Charly::Primitive;
using namespace Charly::Scope;

int main() {

  VM* vm = new VM();

  vm->push_stack(100);
  vm->push_stack(200);
  vm->push_stack(300);
  vm->push_stack(400);

  VALUE res1 = 0x00;
  VALUE res2 = 0x00;
  VALUE res3 = 0x00;
  VALUE res4 = 0x00;
  VALUE res5 = 0x00;
  VALUE res6 = 0x00;

  printf("Status: 0x%016lx, Value: 0x%016lx\n", vm->pop_stack(&res1), res1 );
  printf("Status: 0x%016lx, Value: 0x%016lx\n", vm->pop_stack(&res2), res2 );
  printf("Status: 0x%016lx, Value: 0x%016lx\n", vm->pop_stack(&res3), res3 );
  printf("Status: 0x%016lx, Value: 0x%016lx\n", vm->pop_stack(&res4), res4 );
  printf("Status: 0x%016lx, Value: 0x%016lx\n", vm->pop_stack(&res5), res5 );
  printf("Status: 0x%016lx, Value: 0x%016lx\n", vm->pop_stack(&res6), res6 );

  delete vm;
  return 0;
}
