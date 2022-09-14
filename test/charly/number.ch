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

export class NumberTest {
    static func test_int_addition {
        assert 100 + 100 == 200
        assert 100 + 1000 == 1100
        assert 100 + (-100) == 0
        assert 0 + 0 == 0
        assert 1 + 0 == 1
        assert 0 + 1 == 1
    }

    static func test_int_subtraction {
        assert 100 - 100 == 0
        assert 100 - 1000 == -900
        assert 100 - (-100) == 200
        assert 0 - 0 == 0
        assert 1 - 0 == 1
        assert 0 - 1 == -1
    }

    static func test_int_multiplication {
        assert 100 * 100 == 10000
        assert 100 * 1000 == 100000
        assert 100 * (-100) == -10000
        assert 0 * 0 == 0
        assert 1 * 0 == 0
        assert 0 * 1 == 0
        assert 3 * 4 == 12
    }

    static func test_int_division {
        assert 10 / 10 == 1
        assert 100 / 10 == 10
        assert 90 / 9 == 10
        assert 5 / 2 == 2.5
        assert 1 / 2 == 0.5
        assert 0 / 0 == NaN
        assert 1 / 0 == Infinity
        assert -1 / 0 == -Infinity
        assert 100 / 0 == Infinity
        assert -100 / 0 == -Infinity
    }

//    static func test_int_modulus {
//        assert 100 % 150 == 100
//        assert 9 % 8 == 1
//        assert 10 % 10 == 0
//        assert 16 % 8 == 0
//        assert 0 % 8 == 0
//        assert 12 % 8 == 4
//    }

//    static func test_int_exponent {
//        assert 2 ** -3 == 0.125
//        assert 2 ** -2 == 0.25
//        assert 2 ** -1 == 0.5
//        assert 2 ** 0 == 1
//        assert 2 ** 1 == 2
//        assert 2 ** 2 == 4
//        assert 2 ** 3 == 8
//        assert 2 ** 4 == 16
//        assert 2 ** 5 == 32
//        assert 2 ** 6 == 64
//        assert 2 ** 7 == 128
//        assert 2 ** 8 == 256
//        assert 2 ** 32 == 4294967296
//
//        assert 1 ** -3 == 1
//        assert 1 ** -2 == 1
//        assert 1 ** -1 == 1
//        assert 1 ** 0 == 1
//        assert 1 ** 1 == 1
//        assert 1 ** 2 == 1
//        assert 1 ** 3 == 1
//        assert 1 ** 4 == 1
//
//        assert 0 ** -3 == Infinity
//        assert 0 ** -2 == Infinity
//        assert 0 ** -1 == Infinity
//        assert 0 ** 0 == 1
//        assert 0 ** 1 == 0
//        assert 0 ** 2 == 0
//        assert 0 ** 3 == 0
//
//        assert 10 ** -4 == 0.0001
//        assert 10 ** -3 == 0.001
//        assert 10 ** -2 == 0.01
//        assert 10 ** -1 == 0.1
//        assert 10 ** 0 == 1
//        assert 10 ** 1 == 10
//        assert 10 ** 2 == 100
//        assert 10 ** 3 == 1000
//        assert 10 ** 4 == 10000
//        assert 10 ** 5 == 100000
//    }

    static func test_float_addition {
        assert 25.25 + 75.25 == 100.5
        assert 0.1234 + 0.1234 == 0.2468
        assert 0.0 + 0.0001 == 0.0001
        assert -0.25 + 0.5 == 0.25
        assert 0.0 + 0.0 == 0
        assert 0.1 + 0.01 + 0.001 == 0.111
        assert 1.0 + NaN == NaN
        assert 1.0 + Infinity == Infinity
        assert 1.0 + (-Infinity) == -Infinity
    }

    static func test_float_subtraction {
        assert 25.25 - 75.25 == -50
        assert 0.1234 - 0.1234 == 0
        assert 0.0 - 0.0001 == -0.0001
        assert -0.25 - 0.5 == -0.75
        assert 0.0 - 0.0 == 0
        assert 0.1 - 0.01 - 0.001 == 0.089
        assert 1.0 - NaN == NaN
        assert 1.0 - Infinity == -Infinity
        assert 1.0 - (-Infinity) == Infinity
    }

    static func test_float_multiplication {
        assert 25.25 * 75.25 == 1900.0625
        assert 0.1234 * 0.1234 == 0.01522756
        assert 0.0 * 0.0001 == 0
        assert -0.25 * 0.5 == -0.125
        assert 0.0 * 0.0 == 0
        assert 0.1 * 0.01 * 0.001 == 0.000001
        assert 1.0 * NaN == NaN
        assert 1.0 * Infinity == Infinity
        assert 1.0 * (-Infinity) == -Infinity
        assert -1.0 * NaN == NaN
        assert -1.0 * Infinity == -Infinity
        assert -1.0 * (-Infinity) == Infinity
    }

    static func test_float_division {
        assert 10.0 / 4.0 == 2.5
        assert 5.0 / 2.0 == 2.5
        assert 1.0 / 2.0 == 0.5
        assert 90.0 / 100.0 == 0.9
        assert 25.25 / 75.25 == 0.33554817
        assert 0.1234 / 0.1234 == 1
        assert 0.0 / 0.0001 == 0
        assert -0.25 / 0.5 == -0.5
        assert 0.0 / 0.0 == NaN
        assert 1.0 / 0.0 == Infinity
        assert -1.0 / 0.0 == -Infinity
        assert 0.1 / 0.01 / 0.001 == 10000
        assert 1.0 / NaN == NaN
        assert 1.0 / Infinity == 0
        assert 1.0 / (-Infinity) == 0
        assert -1.0 / NaN == NaN
        assert -1.0 / Infinity == 0
        assert -1.0 / (-Infinity) == 0
    }

//    static func test_float_modulus {
//        assert 5.5 % 4.0 == 1.5
//        assert 10 % 2.4 == 0.4
//        assert 16 % 10 == 6
//        assert 0.0 % 1.0 == 0
//        assert -5.5 % 4.0 == -1.5
//        assert -10 % 2.4 == -0.4
//        assert -16 % 10 == -6
//        assert -0.0 % 1.0 == 0
//        assert 1.0 % 0.0 == NaN
//        assert NaN % 10 == NaN
//        assert Infinity % 10 == NaN
//        assert -Infinity % 10 == NaN
//        assert 10 % NaN == NaN
//        assert 10 % Infinity == 10
//        assert 10 % -Infinity == 10
//    }

//    static func test_float_exponent {
//        assert 2.5 ** -3 == 0.064
//        assert 2.5 ** -2 == 0.16
//        assert 2.5 ** -1 == 0.4
//        assert 2.5 ** 0 == 1
//        assert 2.5 ** 1 == 2.5
//        assert 2.5 ** 2 == 6.25
//        assert 2.5 ** 3 == 15.625
//        assert 2.5 ** 4 == 39.0625
//
//        assert 2 ** 2.5 == 5.65685425
//        assert 2 ** -2.5 == 0.1767767
//
//        assert 0 ** -3.5 == Infinity
//        assert 0 ** -2.5 == Infinity
//        assert 0 ** -1.5 == Infinity
//        assert 0 ** 0 == 1
//        assert 0 ** 1.5 == 0
//        assert 0 ** 2.5 == 0
//        assert 0 ** 3.5 == 0
//    }

    static func test_int_comparison {
        assert 100 == 100
        assert 0 == 0
        assert -100 == -100

        assert 100 != 200
        assert 0 != 1
        assert -100 != -200

        assert 25 < 100
        assert 0 < 1
        assert -100 < 100
        assert -1000 < -999

        assert 100 > 25
        assert 1 > 0
        assert -100 > -200
        assert -1000 > -1001

        assert 100 <= 100
        assert 100 <= 150
        assert 0 <= 0
        assert 0 <= 100
        assert -100 <= -100
        assert -100 <= 100

        assert 100 >= 100
        assert 100 >= 50
        assert 0 >= 0
        assert 0 >= -50
        assert -100 >= -100
        assert -100 >= -200

        assert 0 <=> 0 == 0
        assert 1 <=> 0 == 1
        assert -1 <=> 0 == -1
    }

    static func test_float_comparison {
        assert 125.25 == 125.25
        assert 0.0 == 0.0
        assert NaN == NaN
        assert Infinity == Infinity
        assert -Infinity == -Infinity

        assert 125.25 != 125.26
        assert 0.0 != 0.1
        assert Infinity != -Infinity
        assert -Infinity != Infinity

        assert 0.0 < 1.0
        assert 25.25 < 25.26
        assert 100.999 < 101.0
        assert -25.5 < -20

        assert 1.0 > 0.0
        assert 25.25 > 25.24
        assert 101.0 > 100.999
        assert -20 > -25.5

        assert 25.25 <= 25.25
        assert 25.25 <= 26.0
        assert 0.0 <= 0.0
        assert 0.0 <= 0.1
        assert -25.25 <= -25.25
        assert -25.25 <= -25.0

        assert 25.25 >= 25.25
        assert 25.25 >= 24.0
        assert 0.0 >= 0.0
        assert 0.0 >= -0.1
        assert -25.25 >= -25.25
        assert -25.25 >= -26

        assert 0.0 <=> 0.0 == 0
        assert 0.1 <=> 0.0 == 1
        assert -0.1 <=> 0.0 == -1
        assert 25.25 <=> 25.25 == 0
        assert 25.26 <=> 25.25 == 1
        assert 25.24 <=> 25.25 == -1

        assert NaN < 0 == false
        assert NaN > 0 == false
        assert NaN <= 0 == false
        assert NaN >= 0 == false
        assert NaN == 0 == false
        assert NaN != 0
        assert NaN <=> 0 == 0
        assert NaN <=> 1 == 0
        assert NaN <=> -1 == 0
        assert 0 <=> NaN == 0
        assert 1 <=> NaN == 0
        assert -1 <=> NaN == 0

        assert Infinity > 0
        assert -Infinity < 0

        assert Infinity <=> 0 == 1
        assert -Infinity <=> 0 == -1
        assert 0 <=> Infinity == -1
        assert 0 <=> -Infinity == 1
    }

    static func test_int_usub {
        let a = 100
        let b = -100
        let c = 0

        assert -a == -100
        assert -b == 100
        assert -c == 0
    }

    static func test_float_usub {
        let a = 100.25
        let b = -100.25
        let c = 5.5 - 5.5
        let d = Infinity
        let e = -Infinity
        let f = NaN

        assert -a == -100.25
        assert -b == 100.25
        assert -c == 0
        assert -d == -Infinity
        assert -e == Infinity
        assert -f == NaN
    }

    static func test_unot {
        assert !100 == false
        assert !100.25 == false
        assert !true == false
        assert !false == true
        assert !null == true
        assert !"" == false
        assert !"hello world" == false
        assert ![] == false
        assert ![1, 2] == false
        assert !() == false
        assert !(1, 2) == false
        assert !NaN == true
        assert !0 == true
        assert !0.0 == true
    }

//    static func test_binary_ops {
//
//    }

    static func test_int_stdlib_times {
        let sum = 0
        10.times(->(i) sum += i)
        assert sum == 45

        let list = []
        10.times(->(i) list.push(i))
        assert list.length == 10
        assert list == [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

        let ran = false
        0.times(->ran = true)
        assert ran == false

        ran = false
        let c = -10
        c.times(->ran = true)
        assert ran == false
    }
}





