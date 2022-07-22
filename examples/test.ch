/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

class Foo {
    func foo() {
        return "Foo::foo()"
    }

    func foo(x) {
        return "Foo::foo(x)"
    }

    func foo(x, y) {
        return "Foo::foo(x, y)"
    }
}

class Bar extends Foo {
    func foo() {
        return ("Bar::foo()", super())
    }

    func foo(x) {
        return ("Bar::foo(x)", super(x))
    }

    func foo(x, y) {
        return ("Bar::foo(x, y)", super(x, y))
    }
}

class Baz extends Bar {
    func foo() {
        return ("Baz::foo()", super())
    }

    func foo(x) {
        return ("Baz::foo(x)", super(x))
    }

    func foo(x, y) {
        return ("Baz::foo(x, y)", super(x, y))
    }
}

const f = Baz()

print(f.foo())
print(f.foo(1))
print(f.foo(1, 2))








