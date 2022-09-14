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

export class ImportTest {
    static func test_import {
        import testlib
        assert testlib instanceof Class
        assert testlib.name == "TestLib"
    }

    static func test_import_as {
        import testlib as alias
        assert testlib instanceof Class
        assert testlib.name == "TestLib"
        assert alias instanceof Class
        assert alias.name == "TestLib"
    }

    static func test_import_extract_fields {
        import { foo, bar, baz } from testlib
        assert foo == 100
        assert bar == 200
        assert baz == 300
        assert testlib instanceof Class
        assert testlib.name == "TestLib"
    }

    static func test_import_declare_as_name {
        import { do_foo, do_bar, do_baz } from testlib as alias
        assert testlib instanceof Class
        assert testlib.name == "TestLib"
        assert alias instanceof Class
        assert alias.name == "TestLib"
        assert do_foo() == "foo"
        assert do_bar() == "bar"
        assert do_baz() == "baz"
    }

    static func test_import_multiple_times {
        import "testlib" as tlib1
        import "testlib" as tlib2
        import testlib as tlib3

        assert testlib instanceof Class
        assert testlib.name == "TestLib"

        assert tlib1 == testlib
        assert tlib2 == testlib
        assert tlib3 == testlib
    }

    static func test_import_file_not_found {
        const exc = assert_throws(->{
            import "import-test/does-not-exist.ch"
        })
        assert exc.message == "Could not resolve 'import-test/does-not-exist.ch' to a valid path"
    }

    static func test_import_exception_in_module {
        const exc = assert_throws(->{
            import "import-test/import-test-throws.ch"
        })
        assert exc.message == "some import error"
    }

    static func test_import_compilation_error {
        const exc = assert_throws(->{
            import "import-test/compiler-error.ch"
        })
        assert exc.message.begins_with("Encountered an error while importing")
    }
}





