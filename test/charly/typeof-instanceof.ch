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

import { assert_throws } from unittest

export class TypeofInstanceofTest {
    static func test_type_builtin_types {
        assert typeof 25 == Int
        assert typeof 25.25 == Float
        assert typeof NaN == Float
        assert typeof Infinity == Float
        assert typeof true == Bool
        assert typeof false == Bool
        assert typeof null == Null
        assert typeof "" == String
        assert typeof "test" == String
        assert typeof "hello world" == String
        assert typeof print.name == Symbol
        assert typeof () == Tuple
        assert typeof (1,) == Tuple
        assert typeof (1, 2, 3) == Tuple
        assert typeof [] == List
        assert typeof [1] == List
        assert typeof [1, 2, 3] == List
        assert typeof Future.create() == Future
        assert typeof spawn {} == Fiber
        assert typeof ->{} == Function
        assert typeof builtin_exit == BuiltinFunction
    }

    static func test_type_user_types {
        assert typeof Exception("error") == Exception
        assert typeof assert_throws(->assert false : "test") == AssertionException

        class A {}
        class B extends A {}
        class C extends B {}

        const a = A()
        const b = B()
        const c = C()

        assert typeof a == A
        assert typeof b == B
        assert typeof c == C

        assert typeof a != B
        assert typeof a != C
        assert typeof b != A
        assert typeof b != C
        assert typeof c != A
        assert typeof c != B
    }

    static func test_type_instanceof {
        class A {}
        class B extends A {}
        class C extends B {}

        const a = A()
        const b = B()
        const c = C()

        assert a instanceof A
        assert b instanceof A
        assert c instanceof A

        assert a instanceof B == false
        assert b instanceof B
        assert c instanceof B

        assert a instanceof C == false
        assert b instanceof C == false
        assert c instanceof C

        assert 25 instanceof Number
        assert 25.25 instanceof Number
        assert "test" instanceof String
        assert null instanceof Null
        assert Exception instanceof Value
    }
}





