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

class TestSuite {
    property name
    property tests
    property failed_tests = []
}

export class UnitTest {
    static func run(...test_classes) {
        const suites = test_classes.map(->(test_class) {
            assert test_class instanceof Class : "Expected test suite to be a class instance"

            const name = test_class.klass.name
            const tests = test_class.klass.function_table.filter(->(method) {
                "{method.name}".begins_with("test_")
            })

            return TestSuite(name, tests)
        })

        let some_tests_failed = false

        suites.each(->(suite) {
            suite.tests.each(->(test) {
                try test() catch(e) {
                    suite.failed_tests.push((test, e))
                    some_tests_failed = true
                }
            })
        })

        // print passed tests
        suites.each(->(suite) {
            if suite.failed_tests.empty() {
                print("{suite.name}: All {suite.tests.length} tests passed!")
            }
        })

        // print failed tests
        suites.each(->(suite) {
            if !suite.failed_tests.empty() {
                print("{suite.name}: {suite.failed_tests.length} tests failed!")

                suite.failed_tests.each(->(failed_test) {
                    const (test, error) = failed_test
                    const {message} = error
                    print("\t{test.name}:", message)
                })

                print()
            }
        })

        if some_tests_failed {
            exit(1)
        } else {
            exit(0)
        }
    }
}
