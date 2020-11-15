/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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

#include "internals.h"

#pragma once

namespace Charly {
namespace Internals {
namespace Math {

Result cos(VM& vm, VALUE n);
Result cosh(VM& vm, VALUE n);
Result acos(VM& vm, VALUE n);
Result acosh(VM& vm, VALUE n);
Result sin(VM& vm, VALUE n);
Result sinh(VM& vm, VALUE n);
Result asin(VM& vm, VALUE n);
Result asinh(VM& vm, VALUE n);
Result tan(VM& vm, VALUE n);
Result tanh(VM& vm, VALUE n);
Result atan(VM& vm, VALUE n);
Result atanh(VM& vm, VALUE n);
Result cbrt(VM& vm, VALUE n);
Result sqrt(VM& vm, VALUE n);
Result ceil(VM& vm, VALUE n);
Result floor(VM& vm, VALUE n);
Result log(VM& vm, VALUE n);
Result log2(VM& vm, VALUE n);
Result log10(VM& vm, VALUE n);
Result rand(VM& vm, VALUE min, VALUE max);

}  // namespace Math
}  // namespace Internals
}  // namespace Charly
