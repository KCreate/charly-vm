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

#include <sstream>

#include <catch2/catch_all.hpp>

#include "charly/core/compiler/parser.h"

#include "astmacros.h"

TEST_CASE("parses literals") {
  CHECK_AST_EXP("0", make<Int>(0));
  CHECK_AST_EXP("0x10", make<Int>(0x10));
  CHECK_AST_EXP("0xFFFF", make<Int>(0xFFFF));
  CHECK_AST_EXP("0b11111111", make<Int>(0xFF));
  CHECK_AST_EXP("0b01010101", make<Int>(0x55));
  CHECK_AST_EXP("0b00000000", make<Int>(0x00));
  CHECK_AST_EXP("0o777", make<Int>(0777));
  CHECK_AST_EXP("0o234", make<Int>(0234));
  CHECK_AST_EXP("foo", make<Id>("foo"));
  CHECK_AST_EXP("$", make<Id>("$"));
  CHECK_AST_EXP("$$foo", make<Id>("$$foo"));
  CHECK_AST_EXP("$1", make<Id>("$1"));
  CHECK_AST_EXP("__foo", make<Id>("__foo"));
  CHECK_AST_EXP("π", make<Id>("π"));
  CHECK_AST_EXP("Δ", make<Id>("Δ"));
  CHECK_AST_EXP("берегу", make<Id>("берегу"));
  CHECK_AST_EXP("@\"\"", make<Id>(""));
  CHECK_AST_EXP("@\"foobar\"", make<Id>("foobar"));
  CHECK_AST_EXP("@\"25\"", make<Id>("25"));
  CHECK_AST_EXP("@\"{}{{{}}}}}}}}{{{{\"", make<Id>("{}{{{}}}}}}}}{{{{"));
  CHECK_AST_EXP("@\"foo bar baz \\n hello world\"", make<Id>("foo bar baz \n hello world"));
  CHECK_AST_EXP("100", make<Int>(100));
  CHECK_AST_EXP("0.0", make<Float>(0.0));
  CHECK_AST_EXP("1234.12345678", make<Float>(1234.12345678));
  CHECK_AST_EXP("25.25", make<Float>(25.25));
  CHECK_AST_EXP("NaN", make<Float>(NAN));
  CHECK_AST_EXP("true", make<Bool>(true));
  CHECK_AST_EXP("false", make<Bool>(false));
  CHECK_AST_EXP("null", make<Null>());
  CHECK_AST_EXP("self", make<Self>());
  CHECK_AST_EXP("'a'", make<Char>('a'));
  CHECK_AST_EXP("'π'", make<Char>(u'π'));
  CHECK_AST_EXP("'ä'", make<Char>(u'ä'));
  CHECK_AST_EXP("'ä'", make<Char>(u'ä'));
  CHECK_AST_EXP("'\n'", make<Char>('\n'));
  CHECK_AST_EXP("'\\\''", make<Char>('\''));
  CHECK_AST_EXP("' '", make<Char>(' '));
  CHECK_AST_EXP("\"\"", make<String>(""));
  CHECK_AST_EXP("\"На берегу пустынных волн\"", make<String>("На берегу пустынных волн"));
  CHECK_AST_EXP("\"hello world\"", make<String>("hello world"));

  CHECK_AST_EXP("\"\\a \\b \\n \\t \\v \\f \\\" \\{ \\\\ \"", make<String>("\a \b \n \t \v \f \" { \\ "));
  CHECK_ERROR_STMT("\"", "unexpected end of file, unclosed string");
}

TEST_CASE("incomplete number literals error") {
  CHECK_ERROR_STMT("0x", "unexpected end of file, expected a hex digit");
  CHECK_ERROR_STMT("0b", "unexpected end of file, expected either a 1 or 0");
  CHECK_ERROR_STMT("0o", "unexpected end of file, expected an octal digit");

  CHECK_ERROR_STMT("0xz", "unexpected 'z', expected a hex digit");
  CHECK_ERROR_STMT("0bz", "unexpected 'z', expected either a 1 or 0");
  CHECK_ERROR_STMT("0oz", "unexpected 'z', expected an octal digit");
}

TEST_CASE("parses tuples") {
  CHECK_ERROR_EXP("(", "unexpected end of file, expected a ')' token");
  CHECK_ERROR_EXP("(,)", "unexpected ',' token, expected an expression");
  CHECK_ERROR_EXP("(1,2,)", "unexpected ')' token, expected an expression");
  CHECK_ERROR_EXP("(1 2)", "unexpected numerical constant, expected a ')' token");

  CHECK_AST_EXP("(1,)", make<Tuple>(make<Int>(1)));
  CHECK_AST_EXP("(1, 2)", make<Tuple>(make<Int>(1), make<Int>(2)));
  CHECK_AST_EXP("(1, 2, 3)", make<Tuple>(make<Int>(1), make<Int>(2), make<Int>(3)));
  CHECK_AST_EXP("(1, 2, 3, 4)", make<Tuple>(make<Int>(1), make<Int>(2), make<Int>(3), make<Int>(4)));

  ref<Tuple> tup1 = EXP("(1, 2, 3, (1, 2, 3, 4))", Tuple);
  CHECK(cast<Int>(tup1->elements[0])->value == 1);
  CHECK(cast<Int>(tup1->elements[1])->value == 2);
  CHECK(cast<Int>(tup1->elements[2])->value == 3);

  CHECK(cast<Tuple>(tup1->elements[3])->elements.size() == 4);
  CHECK(cast<Int>(cast<Tuple>(tup1->elements[3])->elements[0])->value == 1);
  CHECK(cast<Int>(cast<Tuple>(tup1->elements[3])->elements[1])->value == 2);
  CHECK(cast<Int>(cast<Tuple>(tup1->elements[3])->elements[2])->value == 3);
  CHECK(cast<Int>(cast<Tuple>(tup1->elements[3])->elements[3])->value == 4);
}

TEST_CASE("interpolated strings") {
  CHECK_AST_EXP("\"{x}\"", make<FormatString>(make<Id>("x")));
  CHECK_AST_EXP("\"x:{x} after\"", make<FormatString>(make<String>("x:"), make<Id>("x"), make<String>(" after")));
  CHECK_AST_EXP("\"x:{x} y:{\"{y}\"}\"", make<FormatString>(make<String>("x:"), make<Id>("x"), make<String>(" y:"),
                                                            make<FormatString>(make<Id>("y"))));
  CHECK_AST_EXP("\"{\"{x}\"}\"", make<FormatString>(make<FormatString>(make<Id>("x"))));
  CHECK_AST_EXP("\"x:{(foo, bar)}\"",
                make<FormatString>(make<String>("x:"), make<Tuple>(make<Id>("foo"), make<Id>("bar"))));

  CHECK_ERROR_EXP("\"{", "unexpected end of file, unclosed string interpolation");
}

TEST_CASE("mismatched brackets") {
  CHECK_ERROR_EXP("(", "unexpected end of file, expected a ')' token");
  CHECK_ERROR_STMT("{", "unexpected end of file, expected a '}' token");

  CHECK_ERROR_EXP("(}", "unexpected '}', expected a ')' token");
  CHECK_ERROR_STMT("{)", "unexpected ')', expected a '}' token");

  CHECK_ERROR_EXP("(]", "unexpected ']', expected a ')' token");
  CHECK_ERROR_STMT("{]", "unexpected ']', expected a '}' token");
}

TEST_CASE("unclosed multiline comments") {
  CHECK_ERROR_EXP("/*", "unexpected end of file, unclosed comment");
}

TEST_CASE("assignments") {
  CHECK_AST_EXP("x = 1", make<Assignment>(make<Id>("x"), make<Int>(1)));
  CHECK_AST_EXP("x = 1 + 2",
                make<Assignment>(make<Id>("x"), make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(2))));
  CHECK_AST_EXP("(x) = 1", make<Assignment>(make<Id>("x"), make<Int>(1)));

  CHECK_AST_EXP("foo.bar = 1", make<Assignment>(make<MemberOp>(make<Id>("foo"), "bar"), make<Int>(1)));
  CHECK_AST_EXP("foo[0] = 1", make<Assignment>(make<IndexOp>(make<Id>("foo"), make<Int>(0)), make<Int>(1)));
  CHECK_AST_EXP("(a, b) = 1", make<Assignment>(make<Tuple>(make<Name>("a"), make<Name>("b")), make<Int>(1)));
  CHECK_AST_EXP("(...b,) = 1", make<Assignment>(make<Tuple>(make<Spread>(make<Name>("b"))), make<Int>(1)));
  CHECK_AST_EXP(
    "(a, ...b, c) = 1",
    make<Assignment>(make<Tuple>(make<Name>("a"), make<Spread>(make<Name>("b")), make<Name>("c")), make<Int>(1)));
  CHECK_AST_EXP(
    "{a, b} = 1",
    make<Assignment>(make<Dict>(make<DictEntry>(make<Name>("a")), make<DictEntry>(make<Name>("b"))), make<Int>(1)));
  CHECK_AST_EXP("{a, ...b, c} = 1", make<Assignment>(make<Dict>(make<DictEntry>(make<Name>("a")),
                                                                make<DictEntry>(make<Spread>(make<Name>("b"))),
                                                                make<DictEntry>(make<Name>("c"))),
                                                     make<Int>(1)));
  CHECK_AST_EXP("x += 1", make<Assignment>(TokenType::Plus, make<Id>("x"), make<Int>(1)));
  CHECK_AST_EXP("foo.bar += 1",
                make<Assignment>(TokenType::Plus, make<MemberOp>(make<Id>("foo"), "bar"), make<Int>(1)));
  CHECK_AST_EXP("foo[0] += 1",
                make<Assignment>(TokenType::Plus, make<IndexOp>(make<Id>("foo"), make<Int>(0)), make<Int>(1)));

  CHECK_ERROR_EXP("2 = 25", "left-hand side of assignment is not assignable");
  CHECK_ERROR_EXP("false = 25", "left-hand side of assignment is not assignable");
  CHECK_ERROR_EXP("self = 25", "left-hand side of assignment is not assignable");
  CHECK_ERROR_EXP("(a, b) += 25",
                  "this type of expression cannot be used as the left-hand side of an operator assignment");
  CHECK_ERROR_EXP("{a, b} += 25",
                  "this type of expression cannot be used as the left-hand side of an operator assignment");
  CHECK_ERROR_EXP("() = 25", "left-hand side of assignment is not assignable");
  CHECK_ERROR_EXP("(1) = 25", "left-hand side of assignment is not assignable");
  CHECK_ERROR_EXP("(...a, ...b) = 25", "left-hand side of assignment is not assignable");
  CHECK_ERROR_EXP("{} = 25", "left-hand side of assignment is not assignable");
  CHECK_ERROR_EXP("{a: 1} = 25", "left-hand side of assignment is not assignable");
  CHECK_ERROR_EXP("{...a, ...b} = 25", "left-hand side of assignment is not assignable");
}

TEST_CASE("ternary if") {
  CHECK_AST_EXP("true ? 1 : 0", make<Ternary>(make<Bool>(true), make<Int>(1), make<Int>(0)));
  CHECK_AST_EXP(
    "true ? foo ? bar : baz : 0",
    make<Ternary>(make<Bool>(true), make<Ternary>(make<Id>("foo"), make<Id>("bar"), make<Id>("baz")), make<Int>(0)));
  CHECK_AST_EXP("(foo ? bar : baz) ? foo ? bar : baz : foo ? bar : baz",
                make<Ternary>(make<Ternary>(make<Id>("foo"), make<Id>("bar"), make<Id>("baz")),
                              make<Ternary>(make<Id>("foo"), make<Id>("bar"), make<Id>("baz")),
                              make<Ternary>(make<Id>("foo"), make<Id>("bar"), make<Id>("baz"))));
}

TEST_CASE("binary operators") {
  CHECK_AST_EXP("1 + 1", make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 - 1", make<BinaryOp>(TokenType::Minus, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 * 1", make<BinaryOp>(TokenType::Mul, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 / 1", make<BinaryOp>(TokenType::Div, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 % 1", make<BinaryOp>(TokenType::Mod, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 ** 1", make<BinaryOp>(TokenType::Pow, make<Int>(1), make<Int>(1)));

  CHECK_AST_EXP("1 == 1", make<BinaryOp>(TokenType::Equal, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 != 1", make<BinaryOp>(TokenType::NotEqual, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 < 1", make<BinaryOp>(TokenType::LessThan, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 > 1", make<BinaryOp>(TokenType::GreaterThan, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 <= 1", make<BinaryOp>(TokenType::LessEqual, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 >= 1", make<BinaryOp>(TokenType::GreaterEqual, make<Int>(1), make<Int>(1)));

  CHECK_AST_EXP("1 and 1", make<BinaryOp>(TokenType::And, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 or 1", make<BinaryOp>(TokenType::Or, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 && 1", make<BinaryOp>(TokenType::And, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 or 1", make<BinaryOp>(TokenType::Or, make<Int>(1), make<Int>(1)));

  CHECK_AST_EXP("1 | 1", make<BinaryOp>(TokenType::BitOR, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 & 1", make<BinaryOp>(TokenType::BitAND, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 ^ 1", make<BinaryOp>(TokenType::BitXOR, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 << 1", make<BinaryOp>(TokenType::BitLeftShift, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 >> 1", make<BinaryOp>(TokenType::BitRightShift, make<Int>(1), make<Int>(1)));
  CHECK_AST_EXP("1 >>> 1", make<BinaryOp>(TokenType::BitUnsignedRightShift, make<Int>(1), make<Int>(1)));

  CHECK_AST_EXP("1..2", make<BinaryOp>(TokenType::DoublePoint, make<Int>(1), make<Int>(2)));
  CHECK_AST_EXP("1...2", make<BinaryOp>(TokenType::TriplePoint, make<Int>(1), make<Int>(2)));
}

TEST_CASE("binary operator relative precedence") {
  CHECK_AST_EXP("1 + 2 + 3", make<BinaryOp>(TokenType::Plus,
                                            make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(2)), make<Int>(3)));

  CHECK_AST_EXP("1 + 2 * 3", make<BinaryOp>(TokenType::Plus, make<Int>(1),
                                            make<BinaryOp>(TokenType::Mul, make<Int>(2), make<Int>(3))));

  CHECK_AST_EXP("1 * 2 + 3", make<BinaryOp>(TokenType::Plus, make<BinaryOp>(TokenType::Mul, make<Int>(1), make<Int>(2)),
                                            make<Int>(3)));

  CHECK_AST_EXP(
    "foo == 1 && 0",
    make<BinaryOp>(TokenType::And, make<BinaryOp>(TokenType::Equal, make<Id>("foo"), make<Int>(1)), make<Int>(0)));

  CHECK_AST_EXP("foo == (1 && 0)", make<BinaryOp>(TokenType::Equal, make<Id>("foo"),
                                                  make<BinaryOp>(TokenType::And, make<Int>(1), make<Int>(0))));

  CHECK_AST_EXP("1 || 2 && 3", make<BinaryOp>(TokenType::Or, make<Int>(1),
                                              make<BinaryOp>(TokenType::And, make<Int>(2), make<Int>(3))));

  CHECK_AST_EXP("1 * 2 / 3", make<BinaryOp>(TokenType::Div, make<BinaryOp>(TokenType::Mul, make<Int>(1), make<Int>(2)),
                                            make<Int>(3)));

  CHECK_AST_EXP("1 * 2 ** 3", make<BinaryOp>(TokenType::Mul, make<Int>(1),
                                             make<BinaryOp>(TokenType::Pow, make<Int>(2), make<Int>(3))));

  CHECK_AST_EXP("1 ** 2 * 3", make<BinaryOp>(TokenType::Mul, make<BinaryOp>(TokenType::Pow, make<Int>(1), make<Int>(2)),
                                             make<Int>(3)));

  CHECK_AST_EXP("1 ** 2 ** 3", make<BinaryOp>(TokenType::Pow, make<Int>(1),
                                              make<BinaryOp>(TokenType::Pow, make<Int>(2), make<Int>(3))));

  CHECK_AST_EXP(
    "1 + 1 .. 2 && 3",
    make<BinaryOp>(
      TokenType::And,
      make<BinaryOp>(TokenType::DoublePoint, make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(1)), make<Int>(2)),
      make<Int>(3)));

  CHECK_AST_EXP("1 .. 1 ... 2 ** 3", make<BinaryOp>(TokenType::TriplePoint,
                                                    make<BinaryOp>(TokenType::DoublePoint, make<Int>(1), make<Int>(1)),
                                                    make<BinaryOp>(TokenType::Pow, make<Int>(2), make<Int>(3))));

  CHECK_AST_EXP("1..1 && 2..2",
                make<BinaryOp>(TokenType::And, make<BinaryOp>(TokenType::DoublePoint, make<Int>(1), make<Int>(1)),
                               make<BinaryOp>(TokenType::DoublePoint, make<Int>(2), make<Int>(2))));

  CHECK_AST_EXP("1..1 == 2..2",
                make<BinaryOp>(TokenType::Equal, make<BinaryOp>(TokenType::DoublePoint, make<Int>(1), make<Int>(1)),
                               make<BinaryOp>(TokenType::DoublePoint, make<Int>(2), make<Int>(2))));

  CHECK_AST_EXP("1..1 < 2..2",
                make<BinaryOp>(TokenType::LessThan, make<BinaryOp>(TokenType::DoublePoint, make<Int>(1), make<Int>(1)),
                               make<BinaryOp>(TokenType::DoublePoint, make<Int>(2), make<Int>(2))));
}

TEST_CASE("unary operators") {
  CHECK_AST_EXP("-0", make<UnaryOp>(TokenType::Minus, make<Int>(0)));
  CHECK_AST_EXP("-100", make<UnaryOp>(TokenType::Minus, make<Int>(100)));
  CHECK_AST_EXP("-0x500", make<UnaryOp>(TokenType::Minus, make<Int>(0x500)));
  CHECK_AST_EXP("-0.5", make<UnaryOp>(TokenType::Minus, make<Float>(0.5)));
  CHECK_AST_EXP("-15.5", make<UnaryOp>(TokenType::Minus, make<Float>(15.5)));
  CHECK_AST_EXP("-null", make<UnaryOp>(TokenType::Minus, make<Null>()));
  CHECK_AST_EXP("-false", make<UnaryOp>(TokenType::Minus, make<Bool>(false)));
  CHECK_AST_EXP("-true", make<UnaryOp>(TokenType::Minus, make<Bool>(true)));
  CHECK_AST_EXP("+0", make<UnaryOp>(TokenType::Plus, make<Int>(0)));
  CHECK_AST_EXP("!0", make<UnaryOp>(TokenType::UnaryNot, make<Int>(0)));
  CHECK_AST_EXP("~0", make<UnaryOp>(TokenType::BitNOT, make<Int>(0)));
  CHECK_AST_EXP("-1 + -2", make<BinaryOp>(TokenType::Plus, make<UnaryOp>(TokenType::Minus, make<Int>(1)),
                                          make<UnaryOp>(TokenType::Minus, make<Int>(2))));
  CHECK_AST_EXP("-1..-5", make<BinaryOp>(TokenType::DoublePoint, make<UnaryOp>(TokenType::Minus, make<Int>(1)),
                                         make<UnaryOp>(TokenType::Minus, make<Int>(5))));

  CHECK_ERROR_EXP("...x", "unexpected '...' token, expected an expression");
}

TEST_CASE("parses control statements") {
  CHECK_AST_STMT("return", make<Return>(make<Null>()));
  CHECK_AST_STMT("return 1", make<Return>(make<Int>(1)));
  CHECK_AST_STMT("return 1 + 2", make<Return>(make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(2))));

  CHECK_AST_STMT("loop { break }", make<While>(make<Bool>(1), make<Block>(make<Break>())));
  CHECK_AST_STMT("loop { continue }", make<While>(make<Bool>(1), make<Block>(make<Continue>())));

  CHECK_ERROR_STMT("defer foo", "expected a call expression");
  CHECK_AST_STMT("defer foo()", make<Defer>(make<CallOp>(make<Id>("foo"))));

  CHECK_AST_STMT("throw null", make<Throw>(make<Null>()));
  CHECK_AST_STMT("throw 25", make<Throw>(make<Int>(25)));
  CHECK_AST_STMT("throw 1 + 2", make<Throw>(make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(2))));

  CHECK_AST_STMT("export exp", make<Export>(make<Id>("exp")));
}

TEST_CASE("import expression") {
  CHECK_AST_EXP("import foo", make<Import>(make<Name>("foo")));
  CHECK_AST_EXP("import 25", make<Import>(make<Int>(25)));
  CHECK_AST_EXP("import \"foo\"", make<Import>(make<String>("foo")));

  CHECK_AST_STMT("import foo", make<Declaration>("foo", make<Import>(make<Name>("foo")), true));
  CHECK_AST_STMT("import \"foo\"", make<Import>(make<String>("foo")));
  CHECK_AST_STMT("import \"{path}\"", make<Import>(make<FormatString>(make<Id>("path"))));

  CHECK_AST_STMT("const x = import foo", make<Declaration>("x", make<Import>(make<Name>("foo")), true));

  CHECK_ERROR_STMT("import", "unexpected end of file, expected an expression");
}

TEST_CASE("yield, await, typeof expressions") {
  CHECK_PROGRAM("func foo { yield 1 }");
  CHECK_PROGRAM("->{ yield 1 }");
  CHECK_PROGRAM("spawn { yield 2 }");
  CHECK_PROGRAM("spawn { defer { yield 2 } }");
  CHECK_AST_EXP("yield 1", make<Yield>(make<Int>(1)));
  CHECK_AST_EXP("yield(1, 2, 3)", make<Yield>(make<Tuple>(make<Int>(1), make<Int>(2), make<Int>(3))));
  CHECK_AST_EXP("yield foo", make<Yield>(make<Id>("foo")));
  CHECK_AST_EXP("yield 1 + 1", make<Yield>(make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(1))));
  CHECK_AST_EXP("yield yield 1", make<Yield>(make<Yield>(make<Int>(1))));

  CHECK_AST_EXP("await 1", make<Await>(make<Int>(1)));
  CHECK_AST_EXP("await(1, 2, 3)", make<Await>(make<Tuple>(make<Int>(1), make<Int>(2), make<Int>(3))));
  CHECK_AST_EXP("await foo", make<Await>(make<Id>("foo")));
  CHECK_AST_EXP("await 1 + 1", make<BinaryOp>(TokenType::Plus, make<Await>(make<Int>(1)), make<Int>(1)));
  CHECK_AST_EXP("await await 1", make<Await>(make<Await>(make<Int>(1))));
  CHECK_AST_EXP("await x == 1", make<BinaryOp>(TokenType::Equal, make<Await>(make<Id>("x")), make<Int>(1)));

  CHECK_AST_EXP("typeof 1", make<Typeof>(make<Int>(1)));
  CHECK_AST_EXP("typeof(1, 2, 3)", make<Typeof>(make<Tuple>(make<Int>(1), make<Int>(2), make<Int>(3))));
  CHECK_AST_EXP("typeof foo", make<Typeof>(make<Id>("foo")));
  CHECK_AST_EXP("typeof 1 + 1", make<BinaryOp>(TokenType::Plus, make<Typeof>(make<Int>(1)), make<Int>(1)));
  CHECK_AST_EXP("typeof typeof 1", make<Typeof>(make<Typeof>(make<Int>(1))));
  CHECK_AST_EXP("typeof x == \"int\"",
                make<BinaryOp>(TokenType::Equal, make<Typeof>(make<Id>("x")), make<String>("int")));
}

TEST_CASE("spawn expressions") {
  CHECK_AST_EXP("spawn foo()", make<Spawn>(make<CallOp>(make<Id>("foo"))));
  CHECK_AST_EXP("spawn foo.bar()", make<Spawn>(make<CallOp>(make<MemberOp>(make<Id>("foo"), make<Name>("bar")))));
  CHECK_AST_EXP("spawn foo()()", make<Spawn>(make<CallOp>(make<CallOp>(make<Id>("foo")))));
  CHECK_AST_EXP("spawn { yield foo }", make<Spawn>(make<Block>(make<Yield>(make<Id>("foo")))));
  CHECK_AST_EXP("spawn { return foo }", make<Spawn>(make<Block>(make<Return>(make<Id>("foo")))));

  CHECK_ERROR_EXP("spawn foo", "expected a call expression");
  CHECK_ERROR_STMT("loop { spawn { break } }", "break statement not allowed at this point");
  CHECK_ERROR_STMT("loop { spawn { continue } }", "continue statement not allowed at this point");
}

TEST_CASE("call expressions") {
  CHECK_AST_EXP("foo()", make<CallOp>(make<Id>("foo")));
  CHECK_AST_EXP("foo(1)", make<CallOp>(make<Id>("foo"), make<Int>(1)));
  CHECK_AST_EXP("foo(1) + foo(2)", make<BinaryOp>(TokenType::Plus, make<CallOp>(make<Id>("foo"), make<Int>(1)),
                                                  make<CallOp>(make<Id>("foo"), make<Int>(2))));
  CHECK_AST_EXP("foo(1, 2, 3)", make<CallOp>(make<Id>("foo"), make<Int>(1), make<Int>(2), make<Int>(3)));
  CHECK_AST_EXP("foo(bar())", make<CallOp>(make<Id>("foo"), make<CallOp>(make<Id>("bar"))));
  CHECK_AST_EXP("foo()()()", make<CallOp>(make<CallOp>(make<CallOp>(make<Id>("foo")))));
  CHECK_AST_EXP("foo(yield 1, 2)", make<CallOp>(make<Id>("foo"), make<Yield>(make<Int>(1)), make<Int>(2)));
  CHECK_AST_STMT("foo\n(0)", make<Id>("foo"));
  CHECK_AST_STMT("foo(0)\n(1)", make<CallOp>(make<Id>("foo"), make<Int>(0)));
  CHECK_AST_STMT("foo(0)(1)\n(2)", make<CallOp>(make<CallOp>(make<Id>("foo"), make<Int>(0)), make<Int>(1)));

  CHECK_AST_EXP(
    "foo.bar(2, 3).test[1](1, 2).bar",
    make<MemberOp>(
      make<CallOp>(make<IndexOp>(make<MemberOp>(make<CallOp>(make<MemberOp>(make<Id>("foo"), make<Name>("bar")),
                                                             make<Int>(2), make<Int>(3)),
                                                make<Name>("test")),
                                 make<Int>(1)),
                   make<Int>(1), make<Int>(2)),
      make<Name>("bar")));

  CHECK_ERROR_EXP("foo(", "unexpected end of file, expected a ')' token");
}

TEST_CASE("member expressions") {
  CHECK_AST_EXP("foo.bar", make<MemberOp>(make<Id>("foo"), make<Name>("bar")));
  CHECK_AST_EXP("foo.bar + foo.baz", make<BinaryOp>(TokenType::Plus, make<MemberOp>(make<Id>("foo"), make<Name>("bar")),
                                                    make<MemberOp>(make<Id>("foo"), make<Name>("baz"))));
  CHECK_AST_EXP("foo.@\"hello world\"", make<MemberOp>(make<Id>("foo"), make<Name>("hello world")));
  CHECK_AST_EXP("1.foo", make<MemberOp>(make<Int>(1), make<Name>("foo")));
  CHECK_AST_EXP("2.2.@\"hello world\"", make<MemberOp>(make<Float>(2.2), make<Name>("hello world")));
  CHECK_AST_EXP("foo.bar.baz", make<MemberOp>(make<MemberOp>(make<Id>("foo"), make<Name>("bar")), make<Name>("baz")));
  CHECK_AST_EXP("foo.bar\n.baz", make<MemberOp>(make<MemberOp>(make<Id>("foo"), make<Name>("bar")), make<Name>("baz")));
  CHECK_AST_EXP("foo\n.\nbar\n.\nbaz",
                make<MemberOp>(make<MemberOp>(make<Id>("foo"), make<Name>("bar")), make<Name>("baz")));
  CHECK_AST_EXP("@foo", make<MemberOp>(make<Self>(), make<Name>("foo")));
}

TEST_CASE("index expressions") {
  CHECK_AST_EXP("foo[1]", make<IndexOp>(make<Id>("foo"), make<Int>(1)));
  CHECK_AST_EXP("foo[1] + foo[2]", make<BinaryOp>(TokenType::Plus, make<IndexOp>(make<Id>("foo"), make<Int>(1)),
                                                  make<IndexOp>(make<Id>("foo"), make<Int>(2))));
  CHECK_AST_EXP("foo[bar()]", make<IndexOp>(make<Id>("foo"), make<CallOp>(make<Id>("bar"))));
  CHECK_AST_EXP("foo[yield 1]", make<IndexOp>(make<Id>("foo"), make<Yield>(make<Int>(1))));
  CHECK_AST_EXP("foo[(1, 2, 3)]",
                make<IndexOp>(make<Id>("foo"), make<Tuple>(make<Int>(1), make<Int>(2), make<Int>(3))));
  CHECK_AST_STMT("foo\n[0]", make<Id>("foo"));
  CHECK_AST_STMT("foo[0]\n[1]", make<IndexOp>(make<Id>("foo"), make<Int>(0)));
  CHECK_AST_STMT("foo[0][1]\n[2]", make<IndexOp>(make<IndexOp>(make<Id>("foo"), make<Int>(0)), make<Int>(1)));

  CHECK_ERROR_EXP("foo[]", "unexpected ']' token, expected an expression");
  CHECK_ERROR_EXP("foo[1, 2]", "unexpected ',' token, expected a ']' token");
  CHECK_ERROR_EXP("foo[", "unexpected end of file, expected a ']' token");
}

TEST_CASE("list literals") {
  CHECK_AST_EXP("[]", make<List>());
  CHECK_AST_EXP("[[]]", make<List>(make<List>()));
  CHECK_AST_EXP("[[1]]", make<List>(make<List>(make<Int>(1))));
  CHECK_AST_EXP("[1]", make<List>(make<Int>(1)));
  CHECK_AST_EXP("[1, 2]", make<List>(make<Int>(1), make<Int>(2)));
  CHECK_AST_EXP("[1, \"foo\", bar, false]",
                make<List>(make<Int>(1), make<String>("foo"), make<Id>("bar"), make<Bool>(false)));

  CHECK_ERROR_EXP("[", "unexpected end of file, expected a ']' token");
  CHECK_ERROR_EXP("]", "unexpected ']'");
  CHECK_ERROR_EXP("[,]", "unexpected ',' token, expected an expression");
  CHECK_ERROR_EXP("[1,]", "unexpected ']' token, expected an expression");
  CHECK_ERROR_EXP("[1, 2,]", "unexpected ']' token, expected an expression");
}

TEST_CASE("dict literals") {
  CHECK_AST_EXP("{}", make<Dict>());
  CHECK_AST_EXP("{x}", make<Dict>(make<DictEntry>(make<Name>("x"))));
  CHECK_AST_EXP("{x, y}", make<Dict>(make<DictEntry>(make<Name>("x")), make<DictEntry>(make<Name>("y"))));
  CHECK_AST_EXP("{x.y}", make<Dict>(make<DictEntry>(make<Name>("y"), make<MemberOp>(make<Id>("x"), make<Name>("y")))));
  CHECK_AST_EXP("{...x}", make<Dict>(make<DictEntry>(make<Spread>(make<Id>("x")))));
  CHECK_AST_EXP("{x: 1}", make<Dict>(make<DictEntry>(make<Name>("x"), make<Int>(1))));
  CHECK_AST_EXP("{x: 1, y: 2}", make<Dict>(make<DictEntry>(make<Name>("x"), make<Int>(1)),
                                           make<DictEntry>(make<Name>("y"), make<Int>(2))));
  CHECK_AST_EXP("{\"foo\": 1}", make<Dict>(make<DictEntry>(make<Name>("foo"), make<Int>(1))));
  CHECK_AST_EXP("{\"foo bar\": 1}", make<Dict>(make<DictEntry>(make<Name>("foo bar"), make<Int>(1))));
  CHECK_AST_EXP("{\"{name}\": 1}", make<Dict>(make<DictEntry>(make<FormatString>(make<Id>("name")), make<Int>(1))));

  CHECK_ERROR_EXP("{25}", "expected identifier, member access or spread expression");
  CHECK_ERROR_EXP("{false}", "expected identifier, member access or spread expression");
  CHECK_ERROR_EXP("{,}", "unexpected ',' token, expected an expression");
  CHECK_ERROR_EXP("{:}", "unexpected ':' token, expected an expression");
  CHECK_ERROR_EXP("{\"foo\"}", "expected identifier, member access or spread expression");
  CHECK_ERROR_EXP("{[x]}", "expected identifier, member access or spread expression");
  CHECK_ERROR_EXP("{-5}", "expected identifier, member access or spread expression");
  CHECK_ERROR_EXP("{[1, 2]: 1}", "expected identifier or string literal");
  CHECK_ERROR_EXP("{25: 1}", "expected identifier or string literal");
  CHECK_ERROR_EXP("{true: 1}", "expected identifier or string literal");
  CHECK_ERROR_EXP("{...x: 1}", "expected identifier or string literal");
}

TEST_CASE("if statements") {
  CHECK_AST_STMT("if x 1", make<If>(make<Id>("x"), make<Int>(1)));
  CHECK_AST_STMT("if x {}", make<If>(make<Id>("x"), make<Block>()));
  CHECK_AST_STMT("if x 1 else 2", make<If>(make<Id>("x"), make<Int>(1), make<Int>(2)));
  CHECK_AST_STMT("if (x) 1", make<If>(make<Id>("x"), make<Int>(1)));
  CHECK_AST_STMT("if (x) {}", make<If>(make<Id>("x"), make<Block>()));
  CHECK_AST_STMT("if x {} else x", make<If>(make<Id>("x"), make<Block>(), make<Id>("x")));
  CHECK_AST_STMT("if x x else {}", make<If>(make<Id>("x"), make<Id>("x"), make<Block>()));
  CHECK_AST_STMT("if x {} else {}", make<If>(make<Id>("x"), make<Block>(), make<Block>()));
  CHECK_AST_STMT("if x {} else if y {}",
                 make<If>(make<Id>("x"), make<Block>(), make<If>(make<Id>("y"), make<Block>())));
  CHECK_AST_STMT("if x {} else if y {} else {}",
                 make<If>(make<Id>("x"), make<Block>(), make<If>(make<Id>("y"), make<Block>(), make<Block>())));

  CHECK_ERROR_STMT("if", "unexpected end of file, expected an expression");
  CHECK_ERROR_STMT("if x", "unexpected end of file, expected an expression");
  CHECK_ERROR_STMT("if x 1 else", "unexpected end of file, expected an expression");
  CHECK_ERROR_STMT("if else x", "unexpected 'else' token, expected an expression");
}

TEST_CASE("while statements") {
  CHECK_AST_STMT("while x 1", make<While>(make<Id>("x"), make<Int>(1)));
  CHECK_AST_STMT("while (x) {}", make<While>(make<Id>("x"), make<Block>()));
  CHECK_AST_STMT("while (x) foo()", make<While>(make<Id>("x"), make<CallOp>(make<Id>("foo"))));

  CHECK_ERROR_STMT("while", "unexpected end of file, expected an expression");
  CHECK_ERROR_STMT("while x", "unexpected end of file, expected an expression");
}

TEST_CASE("loop statements") {
  CHECK_AST_STMT("loop 1", make<While>(make<Bool>(true), make<Int>(1)));
  CHECK_AST_STMT("loop {}", make<While>(make<Bool>(true), make<Block>()));

  CHECK_ERROR_STMT("loop", "unexpected end of file, expected an expression");
}

TEST_CASE("declarations") {
  CHECK_AST_STMT("let a", make<Declaration>("a", make<Null>()));
  CHECK_AST_STMT("let a = 1", make<Declaration>("a", make<Int>(1)));
  CHECK_AST_STMT("let a = 1 + 2", make<Declaration>("a", make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(2))));
  CHECK_AST_STMT("const a = 1", make<Declaration>("a", make<Int>(1), true));
  CHECK_AST_STMT("const a = 1 + 2",
                 make<Declaration>("a", make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(2)), true));

  CHECK_AST_STMT("let (a) = x", make<UnpackDeclaration>(make<Tuple>(make<Name>("a")), make<Id>("x")));
  CHECK_AST_STMT("let (a, b) = x",
                 make<UnpackDeclaration>(make<Tuple>(make<Name>("a"), make<Name>("b")), make<Id>("x")));
  CHECK_AST_STMT("let (a, ...b) = x",
                 make<UnpackDeclaration>(make<Tuple>(make<Name>("a"), make<Spread>(make<Name>("b"))), make<Id>("x")));
  CHECK_AST_STMT("let (a, ...b, c) = x",
                 make<UnpackDeclaration>(make<Tuple>(make<Name>("a"), make<Spread>(make<Name>("b")), make<Name>("c")),
                                         make<Id>("x")));

  CHECK_AST_STMT("const (a) = x", make<UnpackDeclaration>(make<Tuple>(make<Name>("a")), make<Id>("x"), true));
  CHECK_AST_STMT("const (a, b) = x",
                 make<UnpackDeclaration>(make<Tuple>(make<Name>("a"), make<Name>("b")), make<Id>("x"), true));
  CHECK_AST_STMT(
    "const (a, ...b) = x",
    make<UnpackDeclaration>(make<Tuple>(make<Name>("a"), make<Spread>(make<Name>("b"))), make<Id>("x"), true));
  CHECK_AST_STMT("const (a, ...b, c) = x",
                 make<UnpackDeclaration>(make<Tuple>(make<Name>("a"), make<Spread>(make<Name>("b")), make<Name>("c")),
                                         make<Id>("x"), true));

  CHECK_AST_STMT("let {a} = x", make<UnpackDeclaration>(make<Dict>(make<DictEntry>(make<Name>("a"))), make<Id>("x")));
  CHECK_AST_STMT("let {a, b} = x",
                 make<UnpackDeclaration>(make<Dict>(make<DictEntry>(make<Name>("a")), make<DictEntry>(make<Name>("b"))),
                                         make<Id>("x")));
  CHECK_AST_STMT(
    "let {a, ...b} = x",
    make<UnpackDeclaration>(
      make<Dict>(make<DictEntry>(make<Name>("a")), make<DictEntry>(make<Spread>(make<Name>("b")))), make<Id>("x")));
  CHECK_AST_STMT(
    "let {a, ...b, c} = x",
    make<UnpackDeclaration>(make<Dict>(make<DictEntry>(make<Name>("a")), make<DictEntry>(make<Spread>(make<Name>("b"))),
                                       make<DictEntry>(make<Name>("c"))),
                            make<Id>("x")));

  CHECK_AST_STMT("const {a} = x",
                 make<UnpackDeclaration>(make<Dict>(make<DictEntry>(make<Name>("a"))), make<Id>("x"), true));
  CHECK_AST_STMT("const {a, b} = x",
                 make<UnpackDeclaration>(make<Dict>(make<DictEntry>(make<Name>("a")), make<DictEntry>(make<Name>("b"))),
                                         make<Id>("x"), true));
  CHECK_AST_STMT("const {a, ...b} = x",
                 make<UnpackDeclaration>(
                   make<Dict>(make<DictEntry>(make<Name>("a")), make<DictEntry>(make<Spread>(make<Name>("b")))),
                   make<Id>("x"), true));
  CHECK_AST_STMT(
    "const {a, ...b, c} = x",
    make<UnpackDeclaration>(make<Dict>(make<DictEntry>(make<Name>("a")), make<DictEntry>(make<Spread>(make<Name>("b"))),
                                       make<DictEntry>(make<Name>("c"))),
                            make<Id>("x"), true));

  CHECK_ERROR_STMT("let () = 1", "left-hand side of declaration is not assignable");
  CHECK_ERROR_STMT("let (1) = 1", "left-hand side of declaration is not assignable");
  CHECK_ERROR_STMT("let (a.a) = 1", "left-hand side of declaration is not assignable");
  CHECK_ERROR_STMT("let (2 + 2) = 1", "left-hand side of declaration is not assignable");
  CHECK_ERROR_STMT("let (...a, ...d) = 1", "left-hand side of declaration is not assignable");
  CHECK_ERROR_STMT("let ([1, 2]) = 1", "left-hand side of declaration is not assignable");
  CHECK_ERROR_STMT("let (\"a\") = 1", "left-hand side of declaration is not assignable");

  CHECK_ERROR_STMT("let {} = 1", "left-hand side of declaration is not assignable");
  CHECK_ERROR_STMT("let {1} = 1", "expected identifier, member access or spread expression");
  CHECK_ERROR_STMT("let {a.a} = 1", "left-hand side of declaration is not assignable");
  CHECK_ERROR_STMT("let {2 + 2} = 1", "expected identifier, member access or spread expression");
  CHECK_ERROR_STMT("let {...a, ...d} = 1", "left-hand side of declaration is not assignable");
  CHECK_ERROR_STMT("let {[1, 2]} = 1", "expected identifier, member access or spread expression");
  CHECK_ERROR_STMT("let {\"a\"} = 1", "expected identifier, member access or spread expression");

  CHECK_ERROR_STMT("let (a)", "unexpected end of file, expected a '=' token");
  CHECK_ERROR_STMT("let {a}", "unexpected end of file, expected a '=' token");
  CHECK_ERROR_STMT("const a", "unexpected end of file, expected a '=' token");
  CHECK_ERROR_STMT("const (a)", "unexpected end of file, expected a '=' token");
  CHECK_ERROR_STMT("const {a}", "unexpected end of file, expected a '=' token");
}

TEST_CASE("functions") {
  CHECK_EXP("func foo = null");
  CHECK_EXP("func foo = 2 + 2");
  CHECK_EXP("func foo {}");
  CHECK_EXP("func foo { x }");
  CHECK_EXP("func foo(a) {}");
  CHECK_EXP("func foo(a, b) {}");
  CHECK_EXP("func foo(a, ...b) {}");
  CHECK_EXP("func foo(...b) {}");
  CHECK_EXP("func foo(a = 1) {}");
  CHECK_EXP("func foo(a = 1, b = 2) {}");
  CHECK_AST_EXP("func foo(x, @a = 1, b = 2, ...c) {}",
                make<Function>("foo", make<Block>(), make<FunctionArgument>(make<Name>("x")),
                               make<FunctionArgument>(true, false, make<Name>("a"), make<Int>(1)),
                               make<FunctionArgument>(make<Name>("b"), make<Int>(2)),
                               make<FunctionArgument>(false, true, make<Name>("c"))));

  CHECK_EXP("->null");
  CHECK_EXP("->{}");
  CHECK_EXP("->{ x }");
  CHECK_EXP("->(a) {}");
  CHECK_EXP("->(a, b) {}");
  CHECK_EXP("->(a, ...b) {}");
  CHECK_EXP("->(...b) {}");
  CHECK_EXP("->(a = 1) {}");
  CHECK_EXP("->(a = 1, b = 2) {}");
  CHECK_EXP("->(a = 1, b = 2, ...c) {}");

  CHECK_EXP("func foo = import 25");
  CHECK_EXP("func foo = throw 1");
  CHECK_EXP("->import \"test\"");
  CHECK_EXP("->import \"test\"");
  CHECK_EXP("->yield 1");
  CHECK_EXP("->return");
  CHECK_EXP("->return 1");
  CHECK_EXP("->throw 1");

  CHECK_ERROR_EXP("func", "unexpected end of file, expected a 'identifier' token");
  CHECK_ERROR_EXP("func foo", "unexpected end of file, expected a '{' token");
  CHECK_ERROR_EXP("func foo =", "unexpected end of file, expected an expression");
  CHECK_ERROR_EXP("func foo(1) {}", "unexpected numerical constant, expected a 'identifier' token");
  CHECK_ERROR_EXP("func foo(a.b) {}", "unexpected '.' token, expected a ')' token");
  CHECK_ERROR_EXP("func foo(\"test\") {}", "unexpected string literal, expected a 'identifier' token");
  CHECK_ERROR_EXP("func foo(a = 1, b) {}", "argument 'b' is missing a default value");
  CHECK_ERROR_EXP("func foo(...foo, ...rest) {}", "excess parameter(s)");
  CHECK_ERROR_EXP("func foo(...foo, a, b, c) {}", "excess parameter(s)");
  CHECK_ERROR_EXP("func foo(...a.b) {}", "unexpected '.' token, expected a ')' token");
  CHECK_ERROR_EXP("func foo(...1) {}", "unexpected numerical constant, expected a 'identifier' token");
  CHECK_ERROR_EXP("func foo(...1 = 25) {}", "unexpected numerical constant, expected a 'identifier' token");
  CHECK_ERROR_EXP("->", "unexpected end of file, expected an expression");
  CHECK_ERROR_EXP("-> =", "unexpected '=' token, expected an expression");
  CHECK_ERROR_EXP("->(1) {}", "unexpected numerical constant, expected a 'identifier' token");
  CHECK_ERROR_EXP("->(a.b) {}", "unexpected '.' token, expected a ')' token");
  CHECK_ERROR_EXP("->(\"test\") {}", "unexpected string literal, expected a 'identifier' token");
  CHECK_ERROR_EXP("->(a = 1, b) {}", "argument 'b' is missing a default value");
  CHECK_ERROR_EXP("->(...foo, ...rest) {}", "excess parameter(s)");
  CHECK_ERROR_EXP("->(...foo, a, b, c) {}", "excess parameter(s)");
  CHECK_ERROR_EXP("->(...a.b) {}", "unexpected '.' token, expected a ')' token");
  CHECK_ERROR_EXP("->(...a = 2) {}", "spread argument cannot have a default value");
  CHECK_ERROR_EXP("->(...1) {}", "unexpected numerical constant, expected a 'identifier' token");
  CHECK_ERROR_EXP("->(...1 = 25) {}", "unexpected numerical constant, expected a 'identifier' token");

  CHECK_ERROR_EXP("->break", "break statement not allowed at this point");
  CHECK_ERROR_EXP("->continue", "continue statement not allowed at this point");
  CHECK_ERROR_EXP("->if true x", "unexpected 'if' token, expected an expression");
}

TEST_CASE("catches illegal control statements") {
  CHECK_PROGRAM("return 1");
  CHECK_PROGRAM("defer { ->{ return 1 } }");
  CHECK_PROGRAM("func foo { return 42 }");
  CHECK_PROGRAM("loop { break }");
  CHECK_PROGRAM("loop { continue }");
  CHECK_PROGRAM("loop { if 1 { break continue } }");
  CHECK_PROGRAM("import foo");
  CHECK_PROGRAM("export foo");
  CHECK_PROGRAM("spawn { return x }");
  CHECK_PROGRAM("spawn { yield x }");
  CHECK_ERROR_PROGRAM("defer { return 1 }", "return statement not allowed at this point");
  CHECK_ERROR_PROGRAM("break", "break statement not allowed at this point");
  CHECK_ERROR_PROGRAM("if true { break }", "break statement not allowed at this point");
  CHECK_ERROR_PROGRAM("continue", "continue statement not allowed at this point");
  CHECK_ERROR_PROGRAM("if true { continue }", "continue statement not allowed at this point");
  CHECK_ERROR_PROGRAM("loop { ->{ continue } }", "continue statement not allowed at this point");
  CHECK_ERROR_PROGRAM("{ export foo }", "export statement not allowed at this point");
}

TEST_CASE("spread operator") {
  CHECK_AST_EXP("(...x)", make<Tuple>(make<Spread>(make<Id>("x"))));
  CHECK_AST_EXP("(a, ...b, c)", make<Tuple>(make<Id>("a"), make<Spread>(make<Id>("b")), make<Id>("c")));
  CHECK_AST_EXP("(...b, ...c)", make<Tuple>(make<Spread>(make<Id>("b")), make<Spread>(make<Id>("c"))));

  CHECK_AST_EXP("[...x]", make<List>(make<Spread>(make<Id>("x"))));
  CHECK_AST_EXP("[a, ...b, c]", make<List>(make<Id>("a"), make<Spread>(make<Id>("b")), make<Id>("c")));
  CHECK_AST_EXP("[...b, ...c]", make<List>(make<Spread>(make<Id>("b")), make<Spread>(make<Id>("c"))));

  CHECK_AST_EXP("{...x}", make<Dict>(make<DictEntry>(make<Spread>(make<Id>("x")))));
  CHECK_AST_EXP("{a, ...b, c}",
                make<Dict>(make<DictEntry>(make<Name>("a")), make<DictEntry>(make<Spread>(make<Id>("b"))),
                           make<DictEntry>(make<Name>("c"))));
  CHECK_AST_EXP("{...b, ...c}",
                make<Dict>(make<DictEntry>(make<Spread>(make<Id>("b"))), make<DictEntry>(make<Spread>(make<Id>("c")))));

  CHECK_AST_EXP("a(...b)", make<CallOp>(make<Id>("a"), make<Spread>(make<Id>("b"))));
  CHECK_AST_EXP("a(...b, ...c)", make<CallOp>(make<Id>("a"), make<Spread>(make<Id>("b")), make<Spread>(make<Id>("c"))));

  CHECK_AST_EXP("->(...x) {}", make<Function>(make<Block>(), make<FunctionArgument>(false, true, make<Name>("x"))));
  CHECK_AST_EXP("->(a, ...x) {}", make<Function>(make<Block>(), make<FunctionArgument>(make<Name>("a")), make<FunctionArgument>(false, true, make<Name>("x"))));

  CHECK_AST_STMT("let (...copy) = original",
                 make<UnpackDeclaration>(make<Tuple>(make<Spread>(make<Name>("copy"))), make<Id>("original")));

  CHECK_AST_STMT(
    "let (a, ...copy, b) = original",
    make<UnpackDeclaration>(make<Tuple>(make<Name>("a"), make<Spread>(make<Name>("copy")), make<Name>("b")),
                            make<Id>("original")));

  CHECK_AST_STMT(
    "let {...copy} = original",
    make<UnpackDeclaration>(make<Dict>(make<DictEntry>(make<Spread>(make<Name>("copy")))), make<Id>("original")));

  CHECK_AST_STMT("let {a, ...copy, b} = original",
                 make<UnpackDeclaration>(
                   make<Dict>(make<DictEntry>(make<Name>("a")), make<DictEntry>(make<Spread>(make<Name>("copy"))),
                              make<DictEntry>(make<Name>("b"))),
                   make<Id>("original")));
}

TEST_CASE("class literals") {
  CHECK_EXP("class A { foo() }");
  CHECK_AST_EXP("class Foo extends Bar {}", make<Class>("Foo", make<Id>("Bar")));

  ref<Class> class_node = make<Class>("Foo", make<Id>("Bar"));
  class_node->constructor = make<Function>("constructor", make<Block>(), make<FunctionArgument>(make<Name>("a")));
  class_node->member_properties.push_back(make<ClassProperty>(false, "foo", make<Int>(100)));
  class_node->static_properties.push_back(make<ClassProperty>(true, "foo", make<Int>(200)));
  class_node->member_functions.push_back(make<Function>("foo", make<Block>(), make<FunctionArgument>(make<Name>("a"))));
  class_node->member_functions.push_back(make<Function>("bar", make<Block>(), make<FunctionArgument>(make<Name>("a"))));
  class_node->static_properties.push_back(
    make<ClassProperty>(true, "foo", make<Function>("foo", make<Block>(), make<FunctionArgument>(make<Name>("a")))));
  class_node->static_properties.push_back(
    make<ClassProperty>(true, "bar", make<Function>("bar", make<Block>(), make<FunctionArgument>(make<Name>("a")))));

  CHECK_AST_EXP(("class Foo extends Bar {\n"
                 "  constructor(a) {}\n"
                 "  property foo = 100\n"
                 "  static property foo = 200\n"
                 "  func foo(a) {}\n"
                 "  bar(a) {}\n"
                 "  static func foo(a) {}\n"
                 "  static bar(a) {}\n"
                 "}"),
                class_node);
}

TEST_CASE("super expressions") {
  CHECK_ERROR_EXP("->super", "super is not allowed at this point");
  CHECK_ERROR_EXP("->super.foo()", "super is not allowed at this point");
  CHECK_ERROR_EXP("class foo { foo { ->{ super } } }", "super is not allowed at this point");
  CHECK_ERROR_EXP("class foo { static foo { ->{ super } } }", "super is not allowed at this point");

  ref<Class> class_node = make<Class>("Foo", make<Id>("Bar"));
  class_node->member_functions.push_back(make<Function>("foo", make<Block>(make<Super>())));
  CHECK_AST_EXP("class Foo extends Bar { foo { super } }", class_node);

  CHECK_EXP("class Foo extends Bar { static foo { super } }");
  CHECK_EXP("class Foo extends Bar { constructor { super } }");
}

TEST_CASE("try statements") {
  CHECK_AST_STMT("try foo catch bar", make<Try>(make<Id>("foo"), "exception", make<Id>("bar")));
  CHECK_AST_STMT("try foo catch(err) bar", make<Try>(make<Id>("foo"), "err", make<Id>("bar")));
  CHECK_AST_STMT("try foo catch(err) bar finally baz",
                 make<Block>(make<Defer>(make<Id>("baz")), make<Try>(make<Id>("foo"), "err", make<Id>("bar"))));
  CHECK_AST_STMT("try {} catch(err) {} finally {}",
                 make<Block>(make<Defer>(make<Block>()), make<Try>(make<Block>(), "err", make<Block>())));

  CHECK_STMT("loop { try { break continue } catch { break continue } }");
  CHECK_ERROR_STMT("try {}", "unexpected end of file, expected a 'catch' token");
  CHECK_ERROR_STMT("try {} finally {}", "unexpected 'finally' token, expected a 'catch' token");
  CHECK_ERROR_STMT("loop { try {} catch {} finally { break } }", "break statement not allowed at this point");
  CHECK_ERROR_STMT("loop { try {} catch {} finally { return } }", "return statement not allowed at this point");
}

TEST_CASE("switch statements") {
  CHECK_AST_STMT("switch x {}", make<Switch>(make<Id>("x"), nullptr));
  CHECK_AST_STMT("switch (x) {}", make<Switch>(make<Id>("x"), nullptr));
  CHECK_AST_STMT("switch (x) { case 1 foo }",
                 make<Switch>(make<Id>("x"), nullptr, make<SwitchCase>(make<Int>(1), make<Id>("foo"))));
  CHECK_AST_STMT("switch (x) { case 1 foo case 2 bar }",
                 make<Switch>(make<Id>("x"), nullptr, make<SwitchCase>(make<Int>(1), make<Id>("foo")),
                              make<SwitchCase>(make<Int>(2), make<Id>("bar"))));
  CHECK_AST_STMT("switch (x) { case 1 foo default bar }",
                 make<Switch>(make<Id>("x"), make<Id>("bar"), make<SwitchCase>(make<Int>(1), make<Id>("foo"))));
  CHECK_AST_STMT("switch (x) { case 1 {} default {} }",
                 make<Switch>(make<Id>("x"), make<Block>(), make<SwitchCase>(make<Int>(1), make<Block>())));

  CHECK_PROGRAM("switch x { case 1 { break } }");
  CHECK_PROGRAM("switch x { default { break } }");

  CHECK_ERROR_PROGRAM("switch x { case 1 { continue } }", "continue statement not allowed at this point");
}

TEST_CASE("for statements") {
  CHECK_AST_STMT("for foo in bar baz", make<For>(make<Declaration>("foo", make<Id>("bar")), make<Id>("baz")));
  CHECK_AST_STMT("for foo in bar {}", make<For>(make<Declaration>("foo", make<Id>("bar")), make<Block>()));
  CHECK_AST_STMT("for let foo in bar {}", make<For>(make<Declaration>("foo", make<Id>("bar")), make<Block>()));
  CHECK_AST_STMT("for const foo in bar {}", make<For>(make<Declaration>("foo", make<Id>("bar"), true), make<Block>()));

  CHECK_AST_STMT("for (foo) in bar {}",
                 make<For>(make<UnpackDeclaration>(make<Tuple>(make<Name>("foo")), make<Id>("bar")), make<Block>()));
  CHECK_AST_STMT("for (foo, bar) in bar {}",
                 make<For>(make<UnpackDeclaration>(make<Tuple>(make<Name>("foo"), make<Name>("bar")), make<Id>("bar")),
                           make<Block>()));
  CHECK_AST_STMT(
    "for {foo} in bar {}",
    make<For>(make<UnpackDeclaration>(make<Dict>(make<DictEntry>(make<Name>("foo"))), make<Id>("bar")), make<Block>()));
  CHECK_AST_STMT(
    "for {foo, bar} in bar {}",
    make<For>(make<UnpackDeclaration>(
                make<Dict>(make<DictEntry>(make<Name>("foo")), make<DictEntry>(make<Name>("bar"))), make<Id>("bar")),
              make<Block>()));

  CHECK_AST_STMT("for let (foo) in bar {}",
                 make<For>(make<UnpackDeclaration>(make<Tuple>(make<Name>("foo")), make<Id>("bar")), make<Block>()));
  CHECK_AST_STMT("for let (foo, bar) in bar {}",
                 make<For>(make<UnpackDeclaration>(make<Tuple>(make<Name>("foo"), make<Name>("bar")), make<Id>("bar")),
                           make<Block>()));
  CHECK_AST_STMT(
    "for let {foo} in bar {}",
    make<For>(make<UnpackDeclaration>(make<Dict>(make<DictEntry>(make<Name>("foo"))), make<Id>("bar")), make<Block>()));
  CHECK_AST_STMT(
    "for let {foo, bar} in bar {}",
    make<For>(make<UnpackDeclaration>(
                make<Dict>(make<DictEntry>(make<Name>("foo")), make<DictEntry>(make<Name>("bar"))), make<Id>("bar")),
              make<Block>()));

  CHECK_AST_STMT(
    "for const (foo) in bar {}",
    make<For>(make<UnpackDeclaration>(make<Tuple>(make<Name>("foo")), make<Id>("bar"), true), make<Block>()));
  CHECK_AST_STMT(
    "for const (foo, bar) in bar {}",
    make<For>(make<UnpackDeclaration>(make<Tuple>(make<Name>("foo"), make<Name>("bar")), make<Id>("bar"), true),
              make<Block>()));
  CHECK_AST_STMT(
    "for const {foo} in bar {}",
    make<For>(make<UnpackDeclaration>(make<Dict>(make<DictEntry>(make<Name>("foo"))), make<Id>("bar"), true),
              make<Block>()));
  CHECK_AST_STMT("for const {foo, bar} in bar {}",
                 make<For>(make<UnpackDeclaration>(
                             make<Dict>(make<DictEntry>(make<Name>("foo")), make<DictEntry>(make<Name>("bar"))),
                             make<Id>("bar"), true),
                           make<Block>()));
}

TEST_CASE("wraps functions and classes into declarations") {
  CHECK_AST_STMT("func foo {}", make<Declaration>("foo", make<Function>("foo", make<Block>()), true));
  CHECK_AST_STMT("class foo {}", make<Declaration>("foo", make<Class>("foo", nullptr), true));
  CHECK_AST_STMT("->{}", make<Function>(make<Block>()));
}
