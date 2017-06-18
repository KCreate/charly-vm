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
using namespace Charly::Primitive;

int main() {

  VALUE fields[] = {
    Value::Object(4, 0),
    Value::Integer(25),
    Value::Float(43.778),
    Value::False,
    Value::True,
    Value::Null
  };

  for (int i = 0; i < sizeof(fields) / sizeof(VALUE); i++) {
    printf("is_special(0x%lx) = %s\n", fields[i], Value::is_special(fields[i]) ? "true" : "false");
    printf("is_integer(0x%lx) = %s\n", fields[i], Value::is_integer(fields[i]) ? "true" : "false");
    printf("is_ifloat(0x%lx) = %s\n",  fields[i], Value::is_ifloat(fields[i]) ? "true" : "false");
    printf("is_false(0x%lx) = %s\n",   fields[i], Value::is_false(fields[i]) ? "true" : "false");
    printf("is_true(0x%lx) = %s\n",    fields[i], Value::is_true(fields[i]) ? "true" : "false");
    printf("is_null(0x%lx) = %s\n",    fields[i], Value::is_null(fields[i]) ? "true" : "false");
    printf("type(0x%lx) = 0x%lx\n",    fields[i], Value::type(fields[i]));
    printf("\n");
  }

  return 0;
}
