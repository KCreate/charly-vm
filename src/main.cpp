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
  Container map;

  map.insert(100);
  map.insert(200);
  map.insert(300);

  map.register_offset("foo", 0);
  map.register_offset("bar", 1);
  map.register_offset("baz", 2);

  VALUE res1 = 0x00;
  VALUE res2 = 0x00;
  VALUE res3 = 0x00;

  STATUS stat1 = map.read("foo", &res1);
  STATUS stat2 = map.read("bar", &res2);
  STATUS stat3 = map.read("baz", &res3);

  cout << "stat1: " << stat1 << ", value: " << res1 << endl;
  cout << "stat2: " << stat2 << ", value: " << res2 << endl;
  cout << "stat3: " << stat3 << ", value: " << res3 << endl;

  return 0;
}
