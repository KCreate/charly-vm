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

  // Create frame structure
  Frame* top_frame = vm->push_frame(0x00, NULL);
  Frame* clos_frame = vm->push_frame(0x00, top_frame);

  // Insert dummy values
  clos_frame->environment->insert(0xff);
  clos_frame->environment->register_offset("foo", 0);

  vm->pop_frame();

  vm->push_frame(0x00, clos_frame);

  // Try to read these values from the function frame
  VALUE foo;
  vm->read(&foo, "foo");

  cout << foo << endl;

  vm->pop_frame();

  cout << "Correct frame restored: " << (vm->frames == top_frame ? "true" : "false") << endl;

  delete vm;
  return 0;
}
