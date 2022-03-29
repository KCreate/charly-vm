/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2021 Leonard Schütz
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

#include "charly/core/compiler/ir/builtin.h"
#include "charly/core/compiler/parser.h"

#include "astmacros.h"

CATCH_TEST_CASE("Parser") {
  CATCH_SECTION("parses literals") {
    CHECK_AST_EXP("0", make<Int>(0))
    CHECK_AST_EXP("0x10", make<Int>(0x10))
    CHECK_AST_EXP("0xFFFF", make<Int>(0xFFFF))
    CHECK_AST_EXP("0b11111111", make<Int>(0xFF))
    CHECK_AST_EXP("0b01010101", make<Int>(0x55))
    CHECK_AST_EXP("0b00000000", make<Int>(0x00))
    CHECK_AST_EXP("0o777", make<Int>(0777))
    CHECK_AST_EXP("0o234", make<Int>(0234))
    CHECK_AST_EXP("foo", make<Id>("foo"))
    CHECK_AST_EXP("$", make<Id>("$"))
    CHECK_AST_EXP("$$foo", make<Id>("$$foo"))
    CHECK_AST_EXP("$1", make<Id>("$1"))
    CHECK_AST_EXP("__foo", make<Id>("__foo"))
    CHECK_AST_EXP("π", make<Id>("π"))
    CHECK_AST_EXP("Δ", make<Id>("Δ"))
    CHECK_AST_EXP("берегу", make<Id>("берегу"))
    CHECK_AST_EXP("@\"\"", make<Id>(""))
    CHECK_AST_EXP("@\"foobar\"", make<Id>("foobar"))
    CHECK_AST_EXP("@\"25\"", make<Id>("25"))
    CHECK_AST_EXP("@\"{}{{{}}}}}}}}{{{{\"", make<Id>("{}{{{}}}}}}}}{{{{"))
    CHECK_AST_EXP("@\"foo bar baz \\n hello world\"", make<Id>("foo bar baz \n hello world"))
    CHECK_AST_EXP("100", make<Int>(100))
    CHECK_AST_EXP("0.0", make<Float>(0.0))
    CHECK_AST_EXP("1234.12345678", make<Float>(1234.12345678))
    CHECK_AST_EXP("25.25", make<Float>(25.25))
    CHECK_AST_EXP("NaN", make<Float>(NAN))
    CHECK_AST_EXP("NAN", make<Float>(NAN))
    CHECK_AST_EXP("Infinity", make<Float>(INFINITY))
    CHECK_AST_EXP("INFINITY", make<Float>(INFINITY))
    CHECK_AST_EXP("true", make<Bool>(true))
    CHECK_AST_EXP("false", make<Bool>(false))
    CHECK_AST_EXP("null", make<Null>())
    CHECK_AST_EXP("self", make<Self>())
    CHECK_AST_EXP("'a'", make<Char>('a'))
    CHECK_AST_EXP("'π'", make<Char>(u'π'))
    CHECK_AST_EXP("'ä'", make<Char>(u'ä'))
    CHECK_AST_EXP("'ä'", make<Char>(u'ä'))
    CHECK_AST_EXP("'\n'", make<Char>('\n'))
    CHECK_AST_EXP("'\\\''", make<Char>('\''))
    CHECK_AST_EXP("' '", make<Char>(' '))
    CHECK_AST_EXP("\"\"", make<String>(""))
    CHECK_AST_EXP("\"На берегу пустынных волн\"", make<String>("На берегу пустынных волн"))
    CHECK_AST_EXP("\"hello world\"", make<String>("hello world"))

    CHECK_AST_EXP("\"\\a \\b \\n \\t \\v \\f \\\" \\{ \\\\ \"", make<String>("\a \b \n \t \v \f \" { \\ "))
    CHECK_ERROR_STMT("\"", "unexpected end of file, unclosed string")
  }

  CATCH_SECTION("incomplete number literals error") {
    CHECK_ERROR_STMT("0x", "unexpected end of file, expected a hex digit")
    CHECK_ERROR_STMT("0b", "unexpected end of file, expected either a 1 or 0")
    CHECK_ERROR_STMT("0o", "unexpected end of file, expected an octal digit")

    CHECK_ERROR_STMT("0xz", "unexpected 'z', expected a hex digit")
    CHECK_ERROR_STMT("0bz", "unexpected 'z', expected either a 1 or 0")
    CHECK_ERROR_STMT("0oz", "unexpected 'z', expected an octal digit")
  }

  CATCH_SECTION("parses tuples") {
    CHECK_ERROR_EXP("(", "unexpected end of file, expected a ')' token")
    CHECK_ERROR_EXP("(,)", "unexpected ',' token, expected an expression")
    CHECK_ERROR_EXP("(1,2,)", "unexpected ')' token, expected an expression")
    CHECK_ERROR_EXP("(1 2)", "unexpected numerical constant, expected a ')' token")

    CHECK_AST_EXP("(1,)", make<Tuple>(make<Int>(1)))
    CHECK_AST_EXP("(1, 2)", make<Tuple>(make<Int>(1), make<Int>(2)))
    CHECK_AST_EXP("(1, 2, 3)", make<Tuple>(make<Int>(1), make<Int>(2), make<Int>(3)))
    CHECK_AST_EXP("(1, 2, 3, 4)", make<Tuple>(make<Int>(1), make<Int>(2), make<Int>(3), make<Int>(4)))

    ref<Tuple> tup1 = EXP("(1, 2, 3, (1, 2, 3, 4))", Tuple);
    CATCH_CHECK(cast<Int>(tup1->elements[0])->value == 1);
    CATCH_CHECK(cast<Int>(tup1->elements[1])->value == 2);
    CATCH_CHECK(cast<Int>(tup1->elements[2])->value == 3);

    CATCH_CHECK(cast<Tuple>(tup1->elements[3])->elements.size() == 4);
    CATCH_CHECK(cast<Int>(cast<Tuple>(tup1->elements[3])->elements[0])->value == 1);
    CATCH_CHECK(cast<Int>(cast<Tuple>(tup1->elements[3])->elements[1])->value == 2);
    CATCH_CHECK(cast<Int>(cast<Tuple>(tup1->elements[3])->elements[2])->value == 3);
    CATCH_CHECK(cast<Int>(cast<Tuple>(tup1->elements[3])->elements[3])->value == 4);
  }

  CATCH_SECTION("interpolated strings") {
    CHECK_AST_EXP("\"{x}\"", make<FormatString>(make<Id>("x")))
    CHECK_AST_EXP("\"x:{x} after\"", make<FormatString>(make<String>("x:"), make<Id>("x"), make<String>(" after")))
    CHECK_AST_EXP("\"x:{x} y:{\"{y}\"}\"", make<FormatString>(make<String>("x:"), make<Id>("x"), make<String>(" y:"),
                                                              make<FormatString>(make<Id>("y"))))
    CHECK_AST_EXP("\"{\"{x}\"}\"", make<FormatString>(make<FormatString>(make<Id>("x"))))
    CHECK_AST_EXP("\"x:{(foo, bar)}\"",
                  make<FormatString>(make<String>("x:"), make<Tuple>(make<Id>("foo"), make<Id>("bar"))))

    CHECK_ERROR_EXP("\"{", "unexpected end of file, unclosed string interpolation")
  }

  CATCH_SECTION("mismatched brackets") {
    CHECK_ERROR_EXP("(", "unexpected end of file, expected a ')' token")
    CHECK_ERROR_STMT("{", "unexpected end of file, expected a '}' token")

    CHECK_ERROR_EXP("(}", "unexpected '}', expected a ')' token")
    CHECK_ERROR_STMT("{)", "unexpected ')', expected a '}' token")

    CHECK_ERROR_EXP("(]", "unexpected ']', expected a ')' token")
    CHECK_ERROR_STMT("{]", "unexpected ']', expected a '}' token")
  }

  CATCH_SECTION("unclosed multiline comments") {
    CHECK_ERROR_EXP("/*", "unexpected end of file, unclosed comment")
  }

  CATCH_SECTION("assignments") {
    CHECK_EXP("x = 1")
    CHECK_EXP("x = 1 + 2")
    CHECK_EXP("(x) = 1")
    CHECK_EXP("foo.bar = 1")
    CHECK_EXP("foo[0] = 1")
    CHECK_EXP("(a, b) = 1")
    CHECK_EXP("(...b,) = 1")
    CHECK_EXP("(a, ...b, c) = 1")
    CHECK_EXP("{a, b} = 1")
    CHECK_EXP("{a, ...b, c} = 1")
    CHECK_EXP("x += 1")
    CHECK_EXP("foo.bar += 1")
    CHECK_EXP("foo[0] += 1")
  }

  CATCH_SECTION("ternary if") {
    CHECK_AST_EXP("true ? 1 : 0", make<Ternary>(make<Bool>(true), make<Int>(1), make<Int>(0)))
    CHECK_AST_EXP(
      "true ? foo ? bar : baz : 0",
      make<Ternary>(make<Bool>(true), make<Ternary>(make<Id>("foo"), make<Id>("bar"), make<Id>("baz")), make<Int>(0)))
    CHECK_AST_EXP("(foo ? bar : baz) ? foo ? bar : baz : foo ? bar : baz",
                  make<Ternary>(make<Ternary>(make<Id>("foo"), make<Id>("bar"), make<Id>("baz")),
                                make<Ternary>(make<Id>("foo"), make<Id>("bar"), make<Id>("baz")),
                                make<Ternary>(make<Id>("foo"), make<Id>("bar"), make<Id>("baz"))))
  }

  CATCH_SECTION("binary operators") {
    CHECK_AST_EXP("1 + 1", make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 - 1", make<BinaryOp>(TokenType::Minus, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 * 1", make<BinaryOp>(TokenType::Mul, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 / 1", make<BinaryOp>(TokenType::Div, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 % 1", make<BinaryOp>(TokenType::Mod, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 ** 1", make<BinaryOp>(TokenType::Pow, make<Int>(1), make<Int>(1)))

    CHECK_AST_EXP("1 == 1", make<BinaryOp>(TokenType::Equal, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 != 1", make<BinaryOp>(TokenType::NotEqual, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 < 1", make<BinaryOp>(TokenType::LessThan, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 > 1", make<BinaryOp>(TokenType::GreaterThan, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 <= 1", make<BinaryOp>(TokenType::LessEqual, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 >= 1", make<BinaryOp>(TokenType::GreaterEqual, make<Int>(1), make<Int>(1)))

    CHECK_AST_EXP("1 || 1", make<BinaryOp>(TokenType::Or, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 && 1", make<BinaryOp>(TokenType::And, make<Int>(1), make<Int>(1)))

    CHECK_AST_EXP("1 | 1", make<BinaryOp>(TokenType::BitOR, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 & 1", make<BinaryOp>(TokenType::BitAND, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 ^ 1", make<BinaryOp>(TokenType::BitXOR, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 << 1", make<BinaryOp>(TokenType::BitLeftShift, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 >> 1", make<BinaryOp>(TokenType::BitRightShift, make<Int>(1), make<Int>(1)))
    CHECK_AST_EXP("1 >>> 1", make<BinaryOp>(TokenType::BitUnsignedRightShift, make<Int>(1), make<Int>(1)))
  }

  CATCH_SECTION("binary operator relative precedence") {
    CHECK_AST_EXP(
      "1 + 2 + 3",
      make<BinaryOp>(TokenType::Plus, make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(2)), make<Int>(3)))

    CHECK_AST_EXP("1 + 2 * 3", make<BinaryOp>(TokenType::Plus, make<Int>(1),
                                              make<BinaryOp>(TokenType::Mul, make<Int>(2), make<Int>(3))))

    CHECK_AST_EXP(
      "1 * 2 + 3",
      make<BinaryOp>(TokenType::Plus, make<BinaryOp>(TokenType::Mul, make<Int>(1), make<Int>(2)), make<Int>(3)))

    CHECK_AST_EXP(
      "foo == 1 && 0",
      make<BinaryOp>(TokenType::And, make<BinaryOp>(TokenType::Equal, make<Id>("foo"), make<Int>(1)), make<Int>(0)))

    CHECK_AST_EXP("foo == (1 && 0)", make<BinaryOp>(TokenType::Equal, make<Id>("foo"),
                                                    make<BinaryOp>(TokenType::And, make<Int>(1), make<Int>(0))))

    CHECK_AST_EXP("1 || 2 && 3", make<BinaryOp>(TokenType::Or, make<Int>(1),
                                                make<BinaryOp>(TokenType::And, make<Int>(2), make<Int>(3))))

    CHECK_AST_EXP(
      "1 * 2 / 3",
      make<BinaryOp>(TokenType::Div, make<BinaryOp>(TokenType::Mul, make<Int>(1), make<Int>(2)), make<Int>(3)))

    CHECK_AST_EXP("1 * 2 ** 3", make<BinaryOp>(TokenType::Mul, make<Int>(1),
                                               make<BinaryOp>(TokenType::Pow, make<Int>(2), make<Int>(3))))

    CHECK_AST_EXP(
      "1 ** 2 * 3",
      make<BinaryOp>(TokenType::Mul, make<BinaryOp>(TokenType::Pow, make<Int>(1), make<Int>(2)), make<Int>(3)))

    CHECK_AST_EXP("1 ** 2 ** 3", make<BinaryOp>(TokenType::Pow, make<Int>(1),
                                                make<BinaryOp>(TokenType::Pow, make<Int>(2), make<Int>(3))))
  }

  CATCH_SECTION("unary operators") {
    CHECK_AST_EXP("-0", make<UnaryOp>(TokenType::Minus, make<Int>(0)))
    CHECK_AST_EXP("-100", make<UnaryOp>(TokenType::Minus, make<Int>(100)))
    CHECK_AST_EXP("-0x500", make<UnaryOp>(TokenType::Minus, make<Int>(0x500)))
    CHECK_AST_EXP("-0.5", make<UnaryOp>(TokenType::Minus, make<Float>(0.5)))
    CHECK_AST_EXP("-15.5", make<UnaryOp>(TokenType::Minus, make<Float>(15.5)))
    CHECK_AST_EXP("-null", make<UnaryOp>(TokenType::Minus, make<Null>()))
    CHECK_AST_EXP("-false", make<UnaryOp>(TokenType::Minus, make<Bool>(false)))
    CHECK_AST_EXP("-true", make<UnaryOp>(TokenType::Minus, make<Bool>(true)))
    CHECK_AST_EXP("+0", make<Int>(0))
    CHECK_AST_EXP("+x", make<Id>("x"))
    CHECK_AST_EXP("+(\"test\")", make<String>("test"))
    CHECK_AST_EXP("!0", make<UnaryOp>(TokenType::UnaryNot, make<Int>(0)))
    CHECK_AST_EXP("~0", make<UnaryOp>(TokenType::BitNOT, make<Int>(0)))
    CHECK_AST_EXP("-1 + -2", make<BinaryOp>(TokenType::Plus, make<UnaryOp>(TokenType::Minus, make<Int>(1)),
                                            make<UnaryOp>(TokenType::Minus, make<Int>(2))))

    CHECK_ERROR_EXP("...x", "unexpected '...' token, expected an expression")
  }

  CATCH_SECTION("parses control statements") {
    CHECK_AST_STMT("return", make<Return>())
    CHECK_AST_STMT("return 1", make<Return>(make<Int>(1)))
    CHECK_AST_STMT("return 1 + 2", make<Return>(make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(2))))
    CHECK_AST_STMT("return\n 1 + 2", make<Return>())

    CHECK_AST_STMT("loop { break }", make<Loop>(make<Block>(make<Break>())))
    CHECK_AST_STMT("loop { continue }", make<Loop>(make<Block>(make<Continue>())))

    CHECK_AST_STMT("throw null", make<Throw>(make<Null>()))
    CHECK_AST_STMT("throw 25", make<Throw>(make<Int>(25)))
    CHECK_AST_STMT("throw 1 + 2", make<Throw>(make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(2))))

    CHECK_AST_STMT("export exp", make<Export>(make<Id>("exp")))
  }

  CATCH_SECTION("import expression") {
    CHECK_AST_EXP("import foo", make<Import>(make<Name>("foo")))
    CHECK_AST_EXP("import 25", make<Import>(make<Int>(25)))
    CHECK_AST_EXP("import \"foo\"", make<Import>(make<String>("foo")))

    CHECK_AST_STMT("import foo", make<Declaration>("foo", make<Import>(make<Name>("foo")), true))
    CHECK_AST_STMT("import \"foo\"", make<Import>(make<String>("foo")))
    CHECK_AST_STMT("import \"{path}\"", make<Import>(make<FormatString>(make<Id>("path"))))

    CHECK_AST_STMT("const x = import foo", make<Declaration>("x", make<Import>(make<Name>("foo")), true))

    CHECK_ERROR_STMT("import", "unexpected end of file, expected an expression")
  }

  CATCH_SECTION("yield, await, typeof expressions") {
    CHECK_PROGRAM("func foo { yield 1 }")
    CHECK_PROGRAM("func foo { ->{ yield 1 } }")
    CHECK_PROGRAM("spawn { yield 1 }")
    CHECK_PROGRAM("spawn { ->{ yield 1 } }")
    CHECK_AST_EXP("yield 1", make<Yield>(make<Int>(1)))
    CHECK_AST_EXP("yield(1, 2, 3)", make<Yield>(make<Tuple>(make<Int>(1), make<Int>(2), make<Int>(3))))
    CHECK_AST_EXP("yield foo", make<Yield>(make<Id>("foo")))
    CHECK_AST_EXP("yield 1 + 1", make<Yield>(make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(1))))
    CHECK_AST_EXP("yield yield 1", make<Yield>(make<Yield>(make<Int>(1))))

    CHECK_AST_EXP("await 1", make<Await>(make<Int>(1)))
    CHECK_AST_EXP("await(1, 2, 3)", make<Await>(make<Tuple>(make<Int>(1), make<Int>(2), make<Int>(3))))
    CHECK_AST_EXP("await foo", make<Await>(make<Id>("foo")))
    CHECK_AST_EXP("await 1 + 1", make<BinaryOp>(TokenType::Plus, make<Await>(make<Int>(1)), make<Int>(1)))
    CHECK_AST_EXP("await await 1", make<Await>(make<Await>(make<Int>(1))))
    CHECK_AST_EXP("await x == 1", make<BinaryOp>(TokenType::Equal, make<Await>(make<Id>("x")), make<Int>(1)))

    CHECK_AST_EXP("typeof 1", make<Typeof>(make<Int>(1)))
    CHECK_AST_EXP("typeof(1, 2, 3)", make<Typeof>(make<Tuple>(make<Int>(1), make<Int>(2), make<Int>(3))))
    CHECK_AST_EXP("typeof foo", make<Typeof>(make<Id>("foo")))
    CHECK_AST_EXP("typeof 1 + 1", make<BinaryOp>(TokenType::Plus, make<Typeof>(make<Int>(1)), make<Int>(1)))
    CHECK_AST_EXP("typeof typeof 1", make<Typeof>(make<Typeof>(make<Int>(1))))
    CHECK_AST_EXP("typeof x == \"int\"",
                  make<BinaryOp>(TokenType::Equal, make<Typeof>(make<Id>("x")), make<String>("int")))
  }

  CATCH_SECTION("spawn expressions") {
    CHECK_AST_EXP("spawn foo()", make<Spawn>(make<CallOp>(make<Id>("foo"))))
    CHECK_AST_EXP("spawn foo.bar()", make<Spawn>(make<CallOp>(make<MemberOp>(make<Id>("foo"), make<Name>("bar")))))
    CHECK_AST_EXP("spawn foo.bar(1)", make<Spawn>(make<CallOp>(make<MemberOp>(make<Id>("foo"), make<Name>("bar")), make<Int>(1))))
    CHECK_AST_EXP("spawn foo()()", make<Spawn>(make<CallOp>(make<CallOp>(make<Id>("foo")))))
    CHECK_AST_EXP("spawn { yield foo }", make<Spawn>(make<Block>(make<Yield>(make<Id>("foo")))))
    CHECK_AST_EXP("spawn { return foo }", make<Spawn>(make<Block>(make<Return>(make<Id>("foo")))))

    CHECK_ERROR_STMT("loop { spawn { break } }", "break statement not allowed at this point")
    CHECK_ERROR_STMT("loop { spawn { continue } }", "continue statement not allowed at this point")
  }

  CATCH_SECTION("call expressions") {
    CHECK_AST_EXP("foo()", make<CallOp>(make<Id>("foo")))
    CHECK_AST_EXP("foo(1)", make<CallOp>(make<Id>("foo"), make<Int>(1)))
    CHECK_AST_EXP("foo(1) + foo(2)", make<BinaryOp>(TokenType::Plus, make<CallOp>(make<Id>("foo"), make<Int>(1)),
                                                    make<CallOp>(make<Id>("foo"), make<Int>(2))))
    CHECK_AST_EXP("foo(1, 2, 3)", make<CallOp>(make<Id>("foo"), make<Int>(1), make<Int>(2), make<Int>(3)))
    CHECK_AST_EXP("foo(bar())", make<CallOp>(make<Id>("foo"), make<CallOp>(make<Id>("bar"))))
    CHECK_AST_EXP("foo()()()", make<CallOp>(make<CallOp>(make<CallOp>(make<Id>("foo")))))
    CHECK_AST_EXP("foo(yield 1, 2)", make<CallOp>(make<Id>("foo"), make<Yield>(make<Int>(1)), make<Int>(2)))
    CHECK_AST_STMT("foo\n(0)", make<Id>("foo"))
    CHECK_AST_STMT("foo(0)\n(1)", make<CallOp>(make<Id>("foo"), make<Int>(0)))
    CHECK_AST_STMT("foo(0)(1)\n(2)", make<CallOp>(make<CallOp>(make<Id>("foo"), make<Int>(0)), make<Int>(1)))

    CHECK_AST_EXP("foo.bar(2, 3).test[1](1, 2).bar",
      make<MemberOp>(
        make<CallOp>(
          make<IndexOp>(
            make<MemberOp>(
              make<CallOp>(
                make<MemberOp>(
                  make<Id>("foo"),
                  make<Name>("bar")
                ),
                make<Int>(2),
                make<Int>(3)
              ),
              make<Name>("test")
            ),
            make<Int>(1)
          ),
          make<Int>(1),
          make<Int>(2)
        ),
        make<Name>("bar")
      )
    )

    CHECK_ERROR_EXP("foo(", "unexpected end of file, expected a ')' token")
  }

  CATCH_SECTION("member expressions") {
    CHECK_AST_EXP("foo.bar", make<MemberOp>(make<Id>("foo"), make<Name>("bar")))
    CHECK_AST_EXP("foo.bar + foo.baz",
                  make<BinaryOp>(TokenType::Plus, make<MemberOp>(make<Id>("foo"), make<Name>("bar")),
                                 make<MemberOp>(make<Id>("foo"), make<Name>("baz"))))
    CHECK_AST_EXP("foo.@\"hello world\"", make<MemberOp>(make<Id>("foo"), make<Name>("hello world")))
    CHECK_AST_EXP("1.foo", make<MemberOp>(make<Int>(1), make<Name>("foo")))
    CHECK_AST_EXP("2.2.@\"hello world\"", make<MemberOp>(make<Float>(2.2), make<Name>("hello world")))
    CHECK_AST_EXP("foo.bar.baz", make<MemberOp>(make<MemberOp>(make<Id>("foo"), make<Name>("bar")), make<Name>("baz")))
    CHECK_AST_EXP("foo.bar\n.baz",
                  make<MemberOp>(make<MemberOp>(make<Id>("foo"), make<Name>("bar")), make<Name>("baz")))
    CHECK_AST_EXP("foo\n.\nbar\n.\nbaz",
                  make<MemberOp>(make<MemberOp>(make<Id>("foo"), make<Name>("bar")), make<Name>("baz")))
    CHECK_AST_EXP("@foo", make<MemberOp>(make<Self>(), make<Name>("foo")))
  }

  CATCH_SECTION("index expressions") {
    CHECK_AST_EXP("foo[1]", make<IndexOp>(make<Id>("foo"), make<Int>(1)))
    CHECK_AST_EXP("foo[1] + foo[2]", make<BinaryOp>(TokenType::Plus, make<IndexOp>(make<Id>("foo"), make<Int>(1)),
                                                    make<IndexOp>(make<Id>("foo"), make<Int>(2))))
    CHECK_AST_EXP("foo[bar()]", make<IndexOp>(make<Id>("foo"), make<CallOp>(make<Id>("bar"))))
    CHECK_AST_EXP("foo[yield 1]", make<IndexOp>(make<Id>("foo"), make<Yield>(make<Int>(1))))
    CHECK_AST_EXP("foo[(1, 2, 3)]",
                  make<IndexOp>(make<Id>("foo"), make<Tuple>(make<Int>(1), make<Int>(2), make<Int>(3))))
    CHECK_AST_STMT("foo\n[0]", make<Id>("foo"))
    CHECK_AST_STMT("foo[0]\n[1]", make<IndexOp>(make<Id>("foo"), make<Int>(0)))
    CHECK_AST_STMT("foo[0][1]\n[2]", make<IndexOp>(make<IndexOp>(make<Id>("foo"), make<Int>(0)), make<Int>(1)))

    CHECK_ERROR_EXP("foo[]", "unexpected ']' token, expected an expression")
    CHECK_ERROR_EXP("foo[1, 2]", "unexpected ',' token, expected a ']' token")
    CHECK_ERROR_EXP("foo[", "unexpected end of file, expected a ']' token")
  }

  CATCH_SECTION("list literals") {
    CHECK_AST_EXP("[]", make<List>())
    CHECK_AST_EXP("[[]]", make<List>(make<List>()))
    CHECK_AST_EXP("[[1]]", make<List>(make<List>(make<Int>(1))))
    CHECK_AST_EXP("[1]", make<List>(make<Int>(1)))
    CHECK_AST_EXP("[1, 2]", make<List>(make<Int>(1), make<Int>(2)))
    CHECK_AST_EXP("[1, \"foo\", bar, false]",
                  make<List>(make<Int>(1), make<String>("foo"), make<Id>("bar"), make<Bool>(false)))

    CHECK_ERROR_EXP("[", "unexpected end of file, expected a ']' token")
    CHECK_ERROR_EXP("]", "unexpected ']'")
    CHECK_ERROR_EXP("[,]", "unexpected ',' token, expected an expression")
    CHECK_ERROR_EXP("[1,]", "unexpected ']' token, expected an expression")
    CHECK_ERROR_EXP("[1, 2,]", "unexpected ']' token, expected an expression")
  }

  CATCH_SECTION("dict literals") {
    CHECK_AST_EXP("{}", make<Dict>())
    CHECK_AST_EXP("{x}", make<Dict>(make<DictEntry>(make<Name>("x"))))
    CHECK_AST_EXP("{x, y}", make<Dict>(make<DictEntry>(make<Name>("x")), make<DictEntry>(make<Name>("y"))))
    CHECK_AST_EXP("{x.y}", make<Dict>(make<DictEntry>(make<MemberOp>(make<Id>("x"), make<Name>("y")))))
    CHECK_AST_EXP("{...x}", make<Dict>(make<DictEntry>(make<Spread>(make<Id>("x")))))
    CHECK_AST_EXP("{x: 1}", make<Dict>(make<DictEntry>(make<Name>("x"), make<Int>(1))))
    CHECK_AST_EXP("{x: 1, y: 2}", make<Dict>(make<DictEntry>(make<Name>("x"), make<Int>(1)),
                                             make<DictEntry>(make<Name>("y"), make<Int>(2))))
    CHECK_AST_EXP("{\"foo\": 1}", make<Dict>(make<DictEntry>(make<String>("foo"), make<Int>(1))))
    CHECK_AST_EXP("{\"foo bar\": 1}", make<Dict>(make<DictEntry>(make<String>("foo bar"), make<Int>(1))))
    CHECK_AST_EXP("{\"{name}\": 1}", make<Dict>(make<DictEntry>(make<FormatString>(make<Id>("name")), make<Int>(1))))
  }

  CATCH_SECTION("if statements") {
    CHECK_STMT("if x 1")
    CHECK_STMT("if x {}")
    CHECK_STMT("if x 1 else 2")
    CHECK_STMT("if (x) 1")
    CHECK_STMT("if (x) {}")
    CHECK_STMT("if x {} else x")
    CHECK_STMT("if x x else {}")
    CHECK_STMT("if x {} else {}")
    CHECK_STMT("if x {} else if y {}")
    CHECK_STMT("if x {} else if y {} else {}")

    CHECK_ERROR_STMT("if", "unexpected end of file, expected an expression")
    CHECK_ERROR_STMT("if x", "unexpected end of file, expected an expression")
    CHECK_ERROR_STMT("if x 1 else", "unexpected end of file, expected an expression")
    CHECK_ERROR_STMT("if else x", "unexpected 'else' token, expected an expression")
  }

  CATCH_SECTION("while statements") {
    CHECK_STMT("while x 1")
    CHECK_STMT("while (x) {}")
    CHECK_STMT("while (x) foo()")

    CHECK_ERROR_STMT("while", "unexpected end of file, expected an expression")
    CHECK_ERROR_STMT("while x", "unexpected end of file, expected an expression")
  }

  CATCH_SECTION("loop statements") {
    CHECK_AST_STMT("loop 1", make<Loop>(make<Block>(make<Int>(1))))
    CHECK_AST_STMT("loop {}", make<Loop>(make<Block>()))

    CHECK_ERROR_STMT("loop", "unexpected end of file, expected an expression")
  }

  CATCH_SECTION("declarations") {
    CHECK_STMT("let a")
    CHECK_STMT("let a = 1")
    CHECK_STMT("let a = 1 + 2")
    CHECK_STMT("const a = 1")
    CHECK_STMT("const a = 1 + 2")

    CHECK_STMT("let (a) = x")
    CHECK_STMT("let (a, b) = x")
    CHECK_STMT("let (a, ...b) = x")
    CHECK_STMT("let (a, ...b, c) = x")

    CHECK_STMT("const (a) = x")
    CHECK_STMT("const (a, b) = x")
    CHECK_STMT("const (a, ...b) = x")
    CHECK_STMT("const (a, ...b, c) = x")

    CHECK_STMT("let {a} = x")
    CHECK_STMT("let {a, b} = x")
    CHECK_STMT("let {a, ...b} = x")
    CHECK_STMT("let {a, ...b, c} = x")

    CHECK_STMT("const {a} = x")
    CHECK_STMT("const {a, b} = x")
    CHECK_STMT("const {a, ...b} = x")
    CHECK_STMT("const {a, ...b, c} = x")

    CHECK_ERROR_STMT("let (a)", "unexpected end of file, expected a '=' token")
    CHECK_ERROR_STMT("let {a}", "unexpected end of file, expected a '=' token")
    CHECK_ERROR_STMT("const a", "unexpected end of file, expected a '=' token")
    CHECK_ERROR_STMT("const (a)", "unexpected end of file, expected a '=' token")
    CHECK_ERROR_STMT("const {a}", "unexpected end of file, expected a '=' token")
  }

  CATCH_SECTION("functions") {
    CHECK_EXP("func foo = null")
    CHECK_EXP("func foo = 2 + 2")
    CHECK_EXP("func foo {}")
    CHECK_EXP("func foo { x }")
    CHECK_EXP("func foo(a) {}")
    CHECK_EXP("func foo(a, b) {}")
    CHECK_EXP("func foo(a, ...b) {}")
    CHECK_EXP("func foo(...b) {}")
    CHECK_EXP("func foo(a = 1) {}")
    CHECK_EXP("func foo(a = 1, b = 2) {}")
    CHECK_AST_EXP("func foo(x, a = 1, b = 2, ...c) {}",
                  make<Function>(false, make<Name>("foo"), make<Block>(), make<FunctionArgument>(make<Name>("x")),
                                 make<FunctionArgument>(make<Name>("a"), make<Int>(1)),
                                 make<FunctionArgument>(make<Name>("b"), make<Int>(2)),
                                 make<FunctionArgument>(false, true, make<Name>("c"))))

    CHECK_EXP("->null")
    CHECK_EXP("->{}")
    CHECK_EXP("->{ x }")
    CHECK_EXP("->(a) {}")
    CHECK_EXP("->(a, b) {}")
    CHECK_EXP("->(a, ...b) {}")
    CHECK_EXP("->(...b) {}")
    CHECK_EXP("->(a = 1) {}")
    CHECK_EXP("->(a = 1, b = 2) {}")
    CHECK_EXP("->(a = 1, b = 2, ...c) {}")

    CHECK_EXP("func foo = import 25")
    CHECK_EXP("func foo = throw 1")
    CHECK_EXP("->import \"test\"")
    CHECK_EXP("->import \"test\"")
    CHECK_EXP("->yield 1")
    CHECK_EXP("->return")
    CHECK_EXP("->return 1")
    CHECK_EXP("->throw 1")

    CHECK_ERROR_EXP("func", "unexpected end of file, expected a 'identifier' token")
    CHECK_ERROR_EXP("func foo", "unexpected end of file, expected a '{' token")
    CHECK_ERROR_EXP("func foo =", "unexpected end of file, expected an expression")
    CHECK_ERROR_EXP("func foo(1) {}", "unexpected numerical constant, expected a 'identifier' token")
    CHECK_ERROR_EXP("func foo(a.b) {}", "unexpected '.' token, expected a ')' token")
    CHECK_ERROR_EXP("func foo(\"test\") {}", "unexpected string literal, expected a 'identifier' token")
    CHECK_ERROR_EXP("func foo(...a.b) {}", "unexpected '.' token, expected a ')' token")
    CHECK_ERROR_EXP("func foo(...1) {}", "unexpected numerical constant, expected a 'identifier' token")
    CHECK_ERROR_EXP("func foo(...1 = 25) {}", "unexpected numerical constant, expected a 'identifier' token")
    CHECK_ERROR_EXP("->", "unexpected end of file, expected an expression")
    CHECK_ERROR_EXP("-> =", "unexpected '=' token, expected an expression")
    CHECK_ERROR_EXP("->(1) {}", "unexpected numerical constant, expected a 'identifier' token")
    CHECK_ERROR_EXP("->(a.b) {}", "unexpected '.' token, expected a ')' token")
    CHECK_ERROR_EXP("->(\"test\") {}", "unexpected string literal, expected a 'identifier' token")
    CHECK_ERROR_EXP("->(...a.b) {}", "unexpected '.' token, expected a ')' token")
    CHECK_ERROR_EXP("->(...1) {}", "unexpected numerical constant, expected a 'identifier' token")
    CHECK_ERROR_EXP("->(...1 = 25) {}", "unexpected numerical constant, expected a 'identifier' token")

    CHECK_ERROR_EXP("->break", "break statement not allowed at this point")
    CHECK_ERROR_EXP("->continue", "continue statement not allowed at this point")
    CHECK_ERROR_EXP("->if true x", "unexpected 'if' token, expected an expression")
  }

  CATCH_SECTION("catches illegal control statements") {
    CHECK_PROGRAM("return 1")
    CHECK_PROGRAM("defer { ->{ return 1 } }")
    CHECK_PROGRAM("func foo { return 42 }")
    CHECK_PROGRAM("loop { break }")
    CHECK_PROGRAM("loop { continue }")
    CHECK_PROGRAM("loop { if 1 { break continue } }")
    CHECK_PROGRAM("import foo")
    CHECK_PROGRAM("export foo")
    CHECK_PROGRAM("spawn { return x }")
    CHECK_PROGRAM("spawn { yield x }")
    CHECK_ERROR_PROGRAM("defer { return 1 }", "return statement not allowed at this point")
    CHECK_ERROR_PROGRAM("break", "break statement not allowed at this point")
    CHECK_ERROR_PROGRAM("if true { break }", "break statement not allowed at this point")
    CHECK_ERROR_PROGRAM("continue", "continue statement not allowed at this point")
    CHECK_ERROR_PROGRAM("if true { continue }", "continue statement not allowed at this point")
    CHECK_ERROR_PROGRAM("loop { ->{ continue } }", "continue statement not allowed at this point")
    CHECK_ERROR_PROGRAM("{ export foo }", "export statement not allowed at this point")
  }

  CATCH_SECTION("spread operator") {
    CHECK_EXP("(...x)")
    CHECK_EXP("(a, ...b, c)")
    CHECK_EXP("(...b, ...c)")

    CHECK_EXP("[...x]")
    CHECK_EXP("[a, ...b, c]")
    CHECK_EXP("[...b, ...c]")

    CHECK_EXP("{...x}")
    CHECK_EXP("{a, ...b, c}")
    CHECK_EXP("{...b, ...c}")

    CHECK_EXP("a(...b)")
    CHECK_EXP("a(...b, ...c)")

    CHECK_EXP("->(...x) {}")
    CHECK_EXP("->(a, ...x) {}")

    CHECK_STMT("let (...copy) = original")

    CHECK_STMT("let (a, ...copy, b) = original")

    CHECK_STMT("let {...copy} = original")

    CHECK_STMT("let {a, ...copy, b} = original")
  }

  CATCH_SECTION("class literals") {
    CHECK_STMT("class A { func foo() }")
    CHECK_STMT("class A extends B { func foo() }")
    CHECK_STMT(("class Foo extends Bar {\n"
                "  func constructor(a) {}\n"
                "  property foo = 100\n"
                "  static property foo = 200\n"
                "  func foo(a) {}\n"
                "  func bar(a) {}\n"
                "  static func foo(a) {}\n"
                "  static func bar(a) {}\n"
                "}"))
    CHECK_STMT("class A { property a property b func foo(@a, @b) }")
    CHECK_STMT("class A { property a property b func foo(@a, @b, ...@rest) }")
    CHECK_STMT("final class A { }")
    CHECK_STMT("final class A extends B { }")

    CHECK_ERROR_STMT("class A { func constructor { super } }", "super must be used as part of a call operation")
    CHECK_ERROR_STMT("class A { func constructor { super = 1 } }", "super must be used as part of a call operation")
    CHECK_ERROR_STMT("class A { func constructor { super.foo } }", "super must be used as part of a call operation")
    CHECK_ERROR_STMT("class A { func constructor { super[1] = 1 } }", "super must be used as part of a call operation")
    CHECK_ERROR_STMT("class A { func constructor { super.foo = 25 } }", "super must be used as part of a call operation")
    CHECK_ERROR_STMT("class A { func constructor { super + 25 } }", "super must be used as part of a call operation")

    CHECK_ERROR_STMT("class A { private func constructor { } }", "class constructors cannot be private")
  }

  CATCH_SECTION("super expressions") {
    CHECK_ERROR_PROGRAM("->super", "super is not allowed at this point")
    CHECK_ERROR_PROGRAM("->super.foo()", "super is not allowed at this point")
    CHECK_ERROR_PROGRAM("class A { static func foo { super() } }", "super is not allowed at this point")
  }

  CATCH_SECTION("try statements") {
    CHECK_STMT("try foo catch bar")
    CHECK_STMT("try foo catch(err) bar")
    CHECK_STMT("try foo catch(err) bar finally baz")
    CHECK_STMT("try foo finally baz")
    CHECK_STMT("loop { try { break continue } catch { break continue } }")

    CHECK_ERROR_STMT("try {}", "unexpected end of file, expected a 'catch' token")
    CHECK_ERROR_STMT("loop { try {} catch {} finally { break } }", "break statement not allowed at this point")
    CHECK_ERROR_STMT("loop { try {} catch {} finally { return } }", "return statement not allowed at this point")
  }

  CATCH_SECTION("switch statements") {
    CHECK_PROGRAM("switch x {}")
    CHECK_PROGRAM("switch (x) {}")
    CHECK_PROGRAM("switch (x) { case 1 foo }")
    CHECK_PROGRAM("switch (x) { case 1 foo case 2 bar }")
    CHECK_PROGRAM("switch (x) { case 1 foo default bar }")
    CHECK_PROGRAM("switch (x) { case 1 {} default {} }")
    CHECK_PROGRAM("switch x { case 1 { break } }")
    CHECK_PROGRAM("switch x { default { break } }")

    CHECK_ERROR_PROGRAM("switch x { case 1 { continue } }", "continue statement not allowed at this point")
  }

  CATCH_SECTION("for statements") {
    CHECK_STMT("for foo in bar baz")
    CHECK_STMT("for foo in bar {}")
    CHECK_STMT("for let foo in bar {}")
    CHECK_STMT("for const foo in bar {}")
    CHECK_STMT("for (foo) in bar {}")
    CHECK_STMT("for (foo, bar) in bar {}")
    CHECK_STMT("for {foo} in bar {}")
    CHECK_STMT("for {foo, bar} in bar {}")
    CHECK_STMT("for let (foo) in bar {}")
    CHECK_STMT("for let (foo, bar) in bar {}")
    CHECK_STMT("for let {foo} in bar {}")
    CHECK_STMT("for let {foo, bar} in bar {}")
    CHECK_STMT("for const (foo) in bar {}")
    CHECK_STMT("for const (foo, bar) in bar {}")
    CHECK_STMT("for const { foo } in bar {}")
    CHECK_STMT("for const { foo, bar } in bar {}")
  }

  CATCH_SECTION("wraps functions and classes into declarations") {
    CHECK_AST_STMT("func foo {}",
                   make<Declaration>("foo", make<Function>(false, make<Name>("foo"), make<Block>()), true))
    CHECK_AST_STMT("class foo {}", make<Declaration>("foo", make<Class>("foo", nullptr), true))
    CHECK_AST_STMT("->{}", make<Function>(true, make<Name>(""), make<Block>()))
  }

  CATCH_SECTION("__builtin expressions") {
    CHECK_STMT("__builtin(\"caststring\", x)")
    CHECK_STMT("__builtin(\"castsymbol\", x)")
    CHECK_STMT("__builtin(\"makefiber\", x, y, z)")
    CHECK_STMT("__builtin(\"fiberjoin\", x)")

    CHECK_ERROR_STMT("__builtin", "unexpected end of file, expected a '(' token")
    CHECK_ERROR_STMT("__builtin(", "unexpected end of file, expected a ')' token")
    CHECK_ERROR_STMT("__builtin()", "unexpected ')' token, expected a 'string' token")
    CHECK_ERROR_STMT("__builtin(x)", "unexpected 'identifier' token, expected a 'string' token")
    CHECK_ERROR_STMT("__builtin(25)", "unexpected numerical constant, expected a 'string' token")
    CHECK_ERROR_STMT("__builtin(\"caststring\")", "incorrect amount of arguments. expected 1, got 0")
  }
}
