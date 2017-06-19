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
  Frame* fr0 = vm->push_frame(0x00);
  Frame* fr1 = vm->push_frame(0x00);

  fr0->environment.insert(0x200);
  fr0->environment.insert(0x400);
  fr0->environment.register_offset("foo", 0);
  fr0->environment.register_offset("bar", 1);

  fr1->environment.insert(0x600);
  fr1->environment.insert(0x800);
  fr1->environment.register_offset("baz", 0);
  fr1->environment.register_offset("qux", 1);

  std::string keys[] = {"foo", "bar", "baz", "qux", "helloWorld", "boye"};
  std::vector<std::pair<uint32_t, uint32_t>> num_keys = {
    {0, 0},
    {0, 1},
    {1, 0},
    {1, 1},
    {2, 0},
    {2, 1},
    {1, 2},
    {1, 3}
  };

  for (int i = 0; i < num_keys.size(); i++) {
    VALUE result = 0x00;
    STATUS stat = vm->read(&result, num_keys[i].second, num_keys[i].first);

    printf("Status: 0x%lx\n", stat);
    printf("Value:  0x%lx\n", result);
    printf("\n");
  }

  vm->write("foo", 0xffffff);
  vm->write("bar", 0xffffff);
  vm->write("baz", 0xffffff);
  vm->write("qux", 0xffffff);

  for (int i = 0; i < num_keys.size(); i++) {
    VALUE result = 0x00;
    STATUS stat = vm->read(&result, num_keys[i].second, num_keys[i].first);

    printf("Status: 0x%lx\n", stat);
    printf("Value:  0x%lx\n", result);
    printf("\n");
  }

  delete vm;
  return 0;
}
