/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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

#include "buffer.h"
#include "vm.h"

using namespace std;

namespace Charly {
namespace Internals {
namespace Buffer {

VALUE create(VM& vm, VALUE size) {
  CHECK(number, size);
  (void)vm;
  return kNull;
}

VALUE reserve(VM& vm, VALUE buf, VALUE size) {
  CHECK(number, buf);
  CHECK(number, size);
  (void)vm;
  return kNull;
}

VALUE get_size(VM& vm, VALUE buf) {
  CHECK(number, buf);
  (void)vm;
  return kNull;
}

VALUE get_offset(VM& vm, VALUE buf) {
  CHECK(number, buf);
  (void)vm;
  return kNull;
}

VALUE seek(VM& vm, VALUE buf, VALUE off) {
  CHECK(number, buf);
  CHECK(number, off);
  (void)vm;
  return kNull;
}

VALUE write(VM& vm, VALUE buf, VALUE src, VALUE off, VALUE cnt) {
  CHECK(number, buf);
  CHECK(string, src);
  CHECK(number, off);
  CHECK(number, cnt);
  (void)vm;
  return kNull;
}

VALUE set(VM& vm, VALUE buf, VALUE off, VALUE cnt, VALUE v) {
  CHECK(number, buf);
  CHECK(number, off);
  CHECK(number, cnt);
  CHECK(string, v);
  (void)vm;
  return kNull;
}

VALUE create_string(VM& vm, VALUE buf) {
  CHECK(number, buf);
  (void)vm;
  return kNull;
}

VALUE destroy(VM& vm, VALUE buf) {
  CHECK(number, buf);
  (void)vm;
  return kNull;
}


}  // namespace Buffer
}  // namespace Internals
}  // namespace Charly
