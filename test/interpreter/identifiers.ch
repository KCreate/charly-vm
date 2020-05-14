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

import "math"

export = ->(describe, it, assert) {

  it("parses different identifier styles", ->{

    // is_ident_start = is_alpha(c) || c == _ || c == $
    // is_ident_part  = is_ident_start(c) || is_numeric(c)
    let foo = 1
    let _foo = 1
    let $foo = 1
    let f123 = 1
    let _123 = 1

    assert(foo, 1)
    assert(_foo, 1)
    assert($foo, 1)
    assert(f123, 1)
    assert(_123, 1)
  })

  it("parses the @ syntax", ->{
    let @"hello world" = 1
    let @"           " = 1
    let @"return" = 1
    let @"class" = 1
    let @"let" = 1

    assert(@"hello world", 1)
    assert(@"           ", 1)
    assert(@"return", 1)
    assert(@"class", 1)
    assert(@"let", 1)
  })

}
