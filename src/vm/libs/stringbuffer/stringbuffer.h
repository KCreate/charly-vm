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

#include "defines.h"
#include "internals.h"
#include "utf8buffer.h"

#pragma once

namespace Charly {
namespace Internals {
namespace StringBuffer {

VALUE create(VM& vm, VALUE size);
VALUE reserve(VM& vm, VALUE buf, VALUE size);
VALUE get_size(VM& vm, VALUE buf);
VALUE get_offset(VM& vm, VALUE buf);
VALUE write(VM& vm, VALUE buf, VALUE src);
VALUE write_partial(VM& vm, VALUE buf, VALUE src, VALUE off, VALUE cnt);
VALUE write_bytes(VM& vm, VALUE buf, VALUE bytes);
VALUE to_s(VM& vm, VALUE buf);
VALUE bytes(VM& vm, VALUE buf);
VALUE clear(VM& vm, VALUE buf);

}  // namespace StringBuffer
}  // namespace Internals
}  // namespace Charly
