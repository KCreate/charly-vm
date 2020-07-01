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

class TypeError extends Error {
  constructor(expected_type, name, value) {
    const actual_type = typeof value
    super("Expected argument '" + name + "' to be of type " + expected_type + ", not " + actual_type)
  }
}

class A {
  property name
  property age

  constructor(@name, @age) {
    if typeof name ! "string" throw new TypeError("string", "name", name)
  }

  foo {
    print("inside A foo")
  }

  bar {
    print("inside A bar")
  }
}

class B extends A {
  property height

  constructor(name, age, @height) {
    super(name, age)
  }

  foo {
    print("inside B foo")
    @bar()
  }

  bar {
    print("inside B bar")
    super.bar()
  }
}

const o = new B("leonard", 20, 186)
print(o)
o.foo()

try {
 new B(null, null, null)
} catch(e) {
  print("caught exception:")
  print(e)
}

print("Success!")
