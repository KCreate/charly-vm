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

#include <catch2/catch_all.hpp>

#include "astmacros.h"

CATCH_TEST_CASE("Semantic") {
  CATCH_SECTION("validates assignments") {
    COMPILE_ERROR("2 = 25", "left-hand side of assignment cannot be assigned to");
    COMPILE_ERROR("false = 25", "left-hand side of assignment cannot be assigned to");
    COMPILE_ERROR("self = 25", "left-hand side of assignment cannot be assigned to");
    COMPILE_ERROR("(a, b) += 25", "cannot use operator assignment when assigning to an unpack target");
    COMPILE_ERROR("({a, b} += 25)", "cannot use operator assignment when assigning to an unpack target");
    COMPILE_ERROR("() = 25", "empty unpack target");
    COMPILE_ERROR("(1) = 25", "left-hand side of assignment cannot be assigned to");
    COMPILE_ERROR("(...a, ...b) = 25", "excess spread");
    COMPILE_ERROR("({} = 25)", "empty unpack target");
    COMPILE_ERROR("({a: 1} = 25)", "dict used as unpack target must not contain any values");
    COMPILE_ERROR("({...a, ...b} = 25)", "excess spread");

    COMPILE_ERROR("let () = 1", "empty unpack target");
    COMPILE_ERROR("let (1) = 1", "expected an identifier or spread");
    COMPILE_ERROR("let (a.a) = 1", "expected an identifier or spread");
    COMPILE_ERROR("let (2 + 2) = 1", "expected an identifier or spread");
    COMPILE_ERROR("let (...2) = 1", "expected an identifier or spread");
    COMPILE_ERROR("let (...a, ...d) = 1", "excess spread");
    COMPILE_ERROR("let ([1, 2]) = 1", "expected an identifier or spread");
    COMPILE_ERROR("let (\"a\") = 1", "expected an identifier or spread");

    COMPILE_ERROR("let {} = 1", "empty unpack target");
    COMPILE_ERROR("let {1} = 1", "expected an identifier or spread");
    COMPILE_ERROR("let {a.a} = 1", "expected an identifier or spread");
    COMPILE_ERROR("let {2 + 2} = 1", "expected an identifier or spread");
    COMPILE_ERROR("let {...2} = 1", "expected an identifier");
    COMPILE_ERROR("let {...a, ...d} = 1", "excess spread");
    COMPILE_ERROR("let {[1, 2]} = 1", "expected an identifier or spread");
    COMPILE_ERROR("let {\"a\"} = 1", "expected an identifier or spread");

    COMPILE_ERROR("for () in [] {}", "empty unpack target");
    COMPILE_ERROR("for let () in [] {}", "empty unpack target");
    COMPILE_ERROR("for const () in [] {}", "empty unpack target");

    COMPILE_ERROR("for {} in [] {}", "empty unpack target");
    COMPILE_ERROR("for let {} in [] {}", "empty unpack target");
    COMPILE_ERROR("for const {} in [] {}", "empty unpack target");
  }

  CATCH_SECTION("validates dict literals") {
    COMPILE_ERROR("({25})", "expected identifier, member access or spread expression");
    COMPILE_ERROR("({false})", "expected identifier, member access or spread expression");
    COMPILE_ERROR("({,})", "unexpected ',' token, expected an expression");
    COMPILE_ERROR("({:})", "unexpected ':' token, expected an expression");
    COMPILE_ERROR("({\"foo\"})", "expected identifier, member access or spread expression");
    COMPILE_ERROR("({[x]})", "expected identifier, member access or spread expression");
    COMPILE_ERROR("({-5})", "expected identifier, member access or spread expression");
    COMPILE_ERROR("({[1, 2]: 1})", "expected identifier or string literal");
    COMPILE_ERROR("({25: 1})", "expected identifier or string literal");
    COMPILE_ERROR("({true: 1})", "expected identifier or string literal");
    COMPILE_ERROR("({...x: 1})", "expected identifier or string literal");
  }

  CATCH_SECTION("validates spawn statements") {
    COMPILE_ERROR("spawn break", "break statement not allowed at this point");
    COMPILE_ERROR("spawn continue", "continue statement not allowed at this point");
    COMPILE_ERROR("spawn return", "expected block or expression");
    COMPILE_ERROR("spawn throw 25", "expected block or expression");
    COMPILE_OK("spawn foo");
    COMPILE_OK("spawn 1");
    COMPILE_OK("spawn foo.bar");
    COMPILE_OK("spawn await foo");
    COMPILE_OK("spawn foo()");
    COMPILE_OK("spawn foo.bar()");
    COMPILE_OK("spawn foo[x]()");
    COMPILE_OK("spawn { x() }");
  }

  CATCH_SECTION("validates super expressions") {
    COMPILE_ERROR("class A { func constructor { super() } }",
                  "call to super not allowed in constructor of non-inheriting class 'A'");
    COMPILE_OK("class A { func constructor { super.foo() } }");
    COMPILE_OK("class A extends B { func constructor { super() } }");
    COMPILE_ERROR("class A extends B { func constructor { super.foo() } }",
                  "missing super constructor call in constructor of class 'A'");
    COMPILE_OK("class A { func constructor { super.foo() } }");
    COMPILE_OK("class A { func bar { super() } }");
    COMPILE_OK("class A { func bar { super.foo() } }");
  }

  CATCH_SECTION("checks for reserved identifiers") {
    COMPILE_ERROR("const $0 = 1", "'$0' is a reserved variable name");
    COMPILE_ERROR("let $1 = 1", "'$1' is a reserved variable name");
    COMPILE_ERROR("let $5 = 1", "'$5' is a reserved variable name");
    COMPILE_ERROR("let $500 = 1", "'$500' is a reserved variable name");

    COMPILE_ERROR("func foo($10) {}", "'$10' is a reserved variable name");
    COMPILE_ERROR("func foo($10 = 1) {}", "'$10' is a reserved variable name");
    COMPILE_ERROR("func foo(...$10) {}", "'$10' is a reserved variable name");

    COMPILE_ERROR("class x { property $10 }", "'$10' cannot be the name of a property");
    COMPILE_ERROR("class x { func $10 {} }", "'$10' cannot be the name of a member function");
    COMPILE_ERROR("class x { static property $10 }", "'$10' cannot be the name of a static property");
    COMPILE_ERROR("class x { static func $10 {} }", "'$10' cannot be the name of a static function");

    COMPILE_ERROR("class x { property klass }", "'klass' cannot be the name of a property");

    COMPILE_ERROR("class x { static property klass }", "'klass' cannot be the name of a static property");
    COMPILE_ERROR("class x { static property name }", "'name' cannot be the name of a static property");
    COMPILE_ERROR("class x { static property parent }", "'parent' cannot be the name of a static property");

    COMPILE_ERROR("class x { property constructor }", "'constructor' cannot be the name of a property");
    COMPILE_ERROR("class x { static property constructor }", "'constructor' cannot be the name of a static property");
    COMPILE_ERROR("class x { static func constructor }", "'constructor' cannot be the name of a static function");
  }

  CATCH_SECTION("checks for duplicate identifiers") {
    COMPILE_ERROR("let (a, a) = x", "duplicate declaration of 'a'");
    COMPILE_ERROR("let (a, ...a) = x", "duplicate declaration of 'a'");
    COMPILE_ERROR("let {a, a} = x", "duplicate declaration of 'a'");
    COMPILE_ERROR("let {a, ...a} = x", "duplicate declaration of 'a'");

    COMPILE_ERROR("({a: 1, a: 2})", "duplicate key 'a'");

    COMPILE_ERROR("func foo(a, a) {}", "duplicate argument 'a'");
    COMPILE_ERROR("func foo(a, a = 1) {}", "duplicate argument 'a'");
    COMPILE_ERROR("func foo(a, ...a) {}", "duplicate argument 'a'");

    COMPILE_ERROR("class A { property foo property foo }", "duplicate declaration of member property 'foo'");
    COMPILE_ERROR("class A { property foo func foo {} }", "redeclaration of property 'foo' as function");
    COMPILE_ERROR("class A { func foo {} property foo }", "redeclaration of property 'foo' as function");
    COMPILE_ERROR("class A { func constructor {} func constructor {} }", "duplicate declaration of class constructor");
    COMPILE_ERROR("class A { func foo {} func foo {} }", "function overload shadows previous overload");
    COMPILE_ERROR("class A { static property foo static property foo }",
                  "duplicate declaration of static property 'foo'");
    COMPILE_ERROR("class A { static property foo static func foo {} }", "redeclaration of property 'foo' as function");
    COMPILE_ERROR("class A { static func foo {} static property foo }", "redeclaration of property 'foo' as function");

    COMPILE_OK("class A { property foo static property foo }");
    COMPILE_OK("class A { func foo {} static func foo {} }");
    COMPILE_OK("class A { func constructor {} }");
  }

  CATCH_SECTION("checks for missing function default arguments") {
    COMPILE_ERROR("func foo(a = 1, b) {}", "argument 'b' is missing a default value");
    COMPILE_ERROR("->(a = 1, b) {}", "argument 'b' is missing a default value");
  }

  CATCH_SECTION("spread arguments cannot have default arguments") {
    COMPILE_ERROR("func foo(...x = 1) {}", "spread argument cannot have a default value");
    COMPILE_ERROR("->(...x = 1) {}", "spread argument cannot have a default value");
  }

  CATCH_SECTION("checks for excess arguments in functions") {
    COMPILE_ERROR("func foo(...foo, ...rest) {}", "excess parameter(s)");
    COMPILE_ERROR("func foo(...foo, a, b, c) {}", "excess parameter(s)");
    COMPILE_ERROR("->(...foo, ...rest) {}", "excess parameter(s)");
    COMPILE_ERROR("->(...foo, a, b, c) {}", "excess parameter(s)");
  }

  CATCH_SECTION("checks for duplicate overloads in class functions") {
    COMPILE_OK("class A { func foo func foo(x) func foo(x, y) func foo(x, y, z, a = 1) }");
    COMPILE_ERROR("class A { func foo func foo }", "function overload shadows previous overload");
    COMPILE_ERROR("class A { func foo(x) func foo(x) }", "function overload shadows previous overload");
    COMPILE_ERROR("class A { func foo func foo(x = 1) }", "function overload shadows previous overload");
    COMPILE_ERROR("class A { func foo func foo(x = 1, y = 2) }", "function overload shadows previous overload");
    COMPILE_ERROR("class A { func foo(x) func foo(x, y = 2) }", "function overload shadows previous overload");
    COMPILE_ERROR("class A { func foo(x) func foo(x, ...y) }", "function overload shadows previous overload");
    COMPILE_ERROR("class A { func foo(x) func foo(x, ...y) }", "function overload shadows previous overload");
    COMPILE_ERROR("class A { func foo(...x) func foo(x) }", "function overload shadows previous overload");
    COMPILE_ERROR("class A { func foo(...x) func foo(x, y) }", "function overload shadows previous overload");
    COMPILE_ERROR("class A { func foo(x, y, z) func foo(...x) }", "function overload shadows previous overload");

    COMPILE_OK("class A { static func foo {} static func foo(x) {} static func foo(x, y) {} static func foo(x, y, z, a "
               "= 1) {} }");
    COMPILE_ERROR("class A { static func foo static func foo }", "function overload shadows previous overload");
    COMPILE_ERROR("class A { static func foo(x) static func foo(x) }", "function overload shadows previous overload");
    COMPILE_ERROR("class A { static func foo static func foo(x = 1) }", "function overload shadows previous overload");
    COMPILE_ERROR("class A { static func foo static func foo(x = 1, y = 2) }",
                  "function overload shadows previous overload");
    COMPILE_ERROR("class A { static func foo(x) static func foo(x, y = 2) }",
                  "function overload shadows previous overload");
    COMPILE_ERROR("class A { static func foo(x) static func foo(x, ...y) }",
                  "function overload shadows previous overload");
    COMPILE_ERROR("class A { static func foo(...x) static func foo(x) }",
                  "function overload shadows previous overload");
    COMPILE_ERROR("class A { static func foo(...x) static func foo(x, y) }",
                  "function overload shadows previous overload");
    COMPILE_ERROR("class A { static func foo(x, y, z) static func foo(...x) }",
                  "function overload shadows previous overload");
  }

  CATCH_SECTION("checks for missing calls to parent constructor in subclasses") {
    COMPILE_ERROR("class A extends B { func constructor {} }",
                  "missing super constructor call in constructor of class 'A'");
    COMPILE_ERROR("class A extends B { func constructor { super.foo() } }",
                  "missing super constructor call in constructor of class 'A'");
    COMPILE_OK("class A { func constructor {} }");
  }

  CATCH_SECTION("checks for illegal calls to parent constructor in non inheriting classes") {
    COMPILE_ERROR("class A { func constructor { super() } }",
                  "call to super not allowed in constructor of non-inheriting class 'A'");
  }

  CATCH_SECTION("checks for illegal return statements in constructors") {
    COMPILE_ERROR("class A { func constructor { return 25 } }", "constructors must not return a value");
  }

  CATCH_SECTION("checks for missing constructors in subclasses with properties") {
    COMPILE_ERROR("class A extends B { property x }", "class 'A' is missing a constructor");
    COMPILE_OK("let B = null class A extends B {}");
  }

  CATCH_SECTION("checks for yield statements outside regular functions") {
    // FIXME: write yield test cases when block syntax is implemented
    //    COMPILE_ERROR("yield 1", "yield expression not allowed at this point");
    //    COMPILE_ERROR("->{ yield 1 }", "yield expression not allowed at this point");
    //    COMPILE_ERROR("class A { constructor { yield 1 } }", "yield expression not allowed at this point");
    //    COMPILE_ERROR("class A { constructor { ->{ yield 1 } } }", "yield expression not allowed at this point");
    //    COMPILE_OK("class A { foo { yield 1 } }");
    //    COMPILE_OK("class A { static foo { yield 1 } }");
    //    COMPILE_OK("func foo { yield 1 }");
    //    COMPILE_OK("spawn { yield 1 }");
  }

  CATCH_SECTION("only allows @a parameter syntax inside class member functions") {
    COMPILE_ERROR("func foo(@a) {}", "unexpected '@' token, self initializer arguments are only allowed inside class "
                                     "constructors or member functions");
    COMPILE_ERROR("->(@a) {}", "unexpected '@' token, self initializer arguments are only allowed inside class "
                               "constructors or member functions");
    COMPILE_ERROR("class A { static func foo(@a) }", "unexpected '@' token, self initializer arguments are only "
                                                     "allowed inside class constructors or member functions");
    COMPILE_OK("class A { func foo(@a) }");
  }

  CATCH_SECTION("detects duplicate declarations in import statements") {
    COMPILE_ERROR("import foo as foo", "duplicate declaration of 'foo'");
    COMPILE_ERROR("import { foo } from foo ", "duplicate declaration of 'foo'");
    COMPILE_ERROR("import { foo } from bar as bar ", "duplicate declaration of 'bar'");
    COMPILE_ERROR("import { foo } from \"bar\" as foo ", "duplicate declaration of 'foo'");
    COMPILE_ERROR("import { foo as foo } from bar ", "duplicate declaration of 'foo'");
    COMPILE_ERROR("import { foo as bar } from bar ", "duplicate declaration of 'bar'");
    COMPILE_ERROR("import { foo as bar } from 25 as bar ", "duplicate declaration of 'bar'");
    COMPILE_ERROR("import { foo as bar } from 25 as foo ", "duplicate declaration of 'foo'");
  }

  CATCH_SECTION("detects duplicate declarations of the same variable") {
    COMPILE_ERROR("let a = 100 let a = 200", "duplicate declaration of 'a'");
    COMPILE_ERROR("const a = 1 const a = 2", "duplicate declaration of 'a'");
    COMPILE_ERROR("const a = 1 let a = 2", "duplicate declaration of 'a'");
    COMPILE_ERROR("let a = 1 const a = 2", "duplicate declaration of 'a'");
    COMPILE_ERROR("let a = 1 func a() {}", "duplicate declaration of 'a'");
    COMPILE_ERROR("const a = 1 class a {}", "duplicate declaration of 'a'");
    COMPILE_ERROR("class a {} class a {}", "duplicate declaration of 'a'");
  }
}
