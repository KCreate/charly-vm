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

class Test {
    property name
    property function
    property passed = false
    property exception = null
    property duration = 0

    func constructor(@function) {
        @name = function.name
    }
}

class TestSuite {
    property name
    property tests
    property passed = false
    property duration = 0

    func constructor(@name, methods) {
        @tests = methods.map(->(m) Test(m))
    }
}

const unit_test_suspicious_execution_time = 50
const print_passed_test_duration = ARGV.contains("--print_passed_test_duration")

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

        let some_suites_failed = false
        suites.each(->(suite) {
            let some_tests_failed = false
            suite.tests.each(->(test) {
                test.duration = stopwatch(->{
                    try {
                        test.function()
                        test.passed = true
                    } catch(e) {
                        test.exception = e
                        test.passed = false
                        some_tests_failed = true
                        some_suites_failed = true
                    }
                })
            })
            suite.passed = !some_tests_failed
        })

        suites.each(->(suite) {
            const passed_tests = []
            const failed_tests = []

            suite.tests.each(->(test) {
                suite.duration += test.duration
                if test.passed {
                    passed_tests.push(test)
                } else {
                    failed_tests.push(test)
                }
            })

            if suite.passed {
                print("{suite.name}: All tests passed!", suite.duration, "ms")
            } else {
                print("{suite.name}: {failed_tests.length} tests failed!", suite.duration, "ms")
            }

            if print_passed_test_duration {
                passed_tests.sort(->(l, r) r.duration - l.duration)
                passed_tests.each(->(test) {
                    print("\t{test.name}: Passed!", test.duration, "ms")
                })
            }

            failed_tests.each(->(test) {
                print("\t{test.name}:", test.exception.message)
            })
        })

        if some_suites_failed {
            exit(1)
        } else {
            exit(0)
        }
    }

    // unit test helper methods
    static func assert_throws(callback) {
        try callback() catch(e) {
            assert e instanceof Exception : "Expected thrown value to be an exception type value"
            return e
        }

        assert false : "Expected an exception to be thrown"
    }

    static func assert_no_exception(callback) {
        try callback() catch(e) {
            assert false : "Expected no exception to be thrown"
        }
    }
}
