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

#include "defines.h"
#include "internals.h"

#pragma once

namespace Charly {
namespace Internals {
namespace Math {

VALUE cos(VM& vm, VALUE n);
VALUE cosh(VM& vm, VALUE n);
VALUE acos(VM& vm, VALUE n);
VALUE acosh(VM& vm, VALUE n);
VALUE sin(VM& vm, VALUE n);
VALUE sinh(VM& vm, VALUE n);
VALUE asin(VM& vm, VALUE n);
VALUE asinh(VM& vm, VALUE n);
VALUE tan(VM& vm, VALUE n);
VALUE tanh(VM& vm, VALUE n);
VALUE atan(VM& vm, VALUE n);
VALUE atanh(VM& vm, VALUE n);
VALUE cbrt(VM& vm, VALUE n);
VALUE sqrt(VM& vm, VALUE n);
VALUE ceil(VM& vm, VALUE n);
VALUE floor(VM& vm, VALUE n);
VALUE log(VM& vm, VALUE n);
VALUE rand(VM& vm, VALUE min, VALUE max);

}  // namespace Math
}  // namespace Internals
}  // namespace Charly
