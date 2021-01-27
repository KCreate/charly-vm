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
#include "charly/core/compiler/passes/dump.h"

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
  CHECK_AST_EXP("super", make<Super>());
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

  CHECK(EXP("(1,)", Tuple)->elements.size() == 1);
  CHECK(EXP("(1, 2)", Tuple)->elements.size() == 2);
  CHECK(EXP("(1, 2, 3)", Tuple)->elements.size() == 3);
  CHECK(EXP("(1, 2, 3, 4)", Tuple)->elements.size() == 4);

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
  CHECK_AST_EXP("(a, b) = 1", make<Assignment>(make<Tuple>(make<Id>("a"), make<Id>("b")), make<Int>(1)));
  CHECK_AST_EXP("(...b,) = 1",
                make<Assignment>(make<Tuple>(make<UnaryOp>(TokenType::TriplePoint, make<Id>("b"))), make<Int>(1)));
  CHECK_AST_EXP(
    "(a, ...b, c) = 1",
    make<Assignment>(make<Tuple>(make<Id>("a"), make<UnaryOp>(TokenType::TriplePoint, make<Id>("b")), make<Id>("c")),
                     make<Int>(1)));
  CHECK_AST_EXP(
    "{a, b} = 1",
    make<Assignment>(make<Dict>(make<DictEntry>(make<Id>("a")), make<DictEntry>(make<Id>("b"))), make<Int>(1)));
  CHECK_AST_EXP("{a, ...b, c} = 1",
                make<Assignment>(make<Dict>(make<DictEntry>(make<Id>("a")),
                                            make<DictEntry>(make<UnaryOp>(TokenType::TriplePoint, make<Id>("b"))),
                                            make<DictEntry>(make<Id>("c"))),
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
  CHECK_AST_EXP("...foo", make<UnaryOp>(TokenType::TriplePoint, make<Id>("foo")));
  CHECK_AST_EXP("-1 + -2", make<BinaryOp>(TokenType::Plus, make<UnaryOp>(TokenType::Minus, make<Int>(1)),
                                          make<UnaryOp>(TokenType::Minus, make<Int>(2))));
  CHECK_AST_EXP("-1..-5", make<BinaryOp>(TokenType::DoublePoint, make<UnaryOp>(TokenType::Minus, make<Int>(1)),
                                         make<UnaryOp>(TokenType::Minus, make<Int>(5))));
}

TEST_CASE("parses control statements") {
  CHECK_AST_STMT("return", make<Return>(make<Null>()));
  CHECK_AST_STMT("return 1", make<Return>(make<Int>(1)));
  CHECK_AST_STMT("return 1 + 2", make<Return>(make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(2))));

  CHECK_AST_STMT("break", make<Break>());
  CHECK_AST_STMT("continue", make<Continue>());

  CHECK_ERROR_STMT("defer foo", "expected a call expression");
  CHECK_AST_STMT("defer foo()", make<Defer>(make<CallOp>(make<Id>("foo"))));

  CHECK_AST_STMT("throw null", make<Throw>(make<Null>()));
  CHECK_AST_STMT("throw 25", make<Throw>(make<Int>(25)));
  CHECK_AST_STMT("throw 1 + 2", make<Throw>(make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(2))));

  CHECK_AST_STMT("export exp", make<Export>(make<Id>("exp")));
}

TEST_CASE("import statement") {
  CHECK_AST_STMT("import foo", make<Import>(make<Id>("foo")));
  CHECK_AST_STMT("import foo as bar", make<Import>(make<As>(make<Id>("foo"), "bar")));
  CHECK_AST_STMT("import \"foo\" as bar", make<Import>(make<As>(make<String>("foo"), "bar")));
  CHECK_AST_STMT("import \"{path}\" as bar", make<Import>(make<As>(make<FormatString>(make<Id>("path")), "bar")));

  CHECK_ERROR_STMT("import", "unexpected end of file, expected an expression");
  CHECK_ERROR_STMT("import 25", "expected an identifier");
  CHECK_ERROR_STMT("import 25 as bar", "expected an identifier or a string literal");
}

TEST_CASE("import expression") {
  CHECK_AST_EXP("import foo", make<ImportExpression>(make<Id>("foo")));
  CHECK_AST_EXP("import 25", make<ImportExpression>(make<Int>(25)));
  CHECK_AST_EXP("import \"foo\"", make<ImportExpression>(make<String>("foo")));

  CHECK_ERROR_EXP("(import foo as bar)", "unexpected 'as' token, expected a ')' token");
}

TEST_CASE("yield, await, typeof expressions") {
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
  CHECK_ERROR_EXP("spawn foo", "expected a call expression");
  CHECK_AST_EXP("spawn foo()", make<Spawn>(make<CallOp>(make<Id>("foo"))));
  CHECK_AST_EXP("spawn foo.bar()", make<Spawn>(make<CallOp>(make<MemberOp>(make<Id>("foo"), make<Id>("bar")))));
  CHECK_AST_EXP("spawn foo()()", make<Spawn>(make<CallOp>(make<CallOp>(make<Id>("foo")))));
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
      make<CallOp>(make<IndexOp>(make<MemberOp>(make<CallOp>(make<MemberOp>(make<Id>("foo"), make<Id>("bar")),
                                                             make<Int>(2), make<Int>(3)),
                                                make<Id>("test")),
                                 make<Int>(1)),
                   make<Int>(1), make<Int>(2)),
      make<Id>("bar")));

  CHECK_ERROR_EXP("foo(", "unexpected end of file, expected a ')' token");
}

TEST_CASE("member expressions") {
  CHECK_AST_EXP("foo.bar", make<MemberOp>(make<Id>("foo"), make<Id>("bar")));
  CHECK_AST_EXP("foo.bar + foo.baz", make<BinaryOp>(TokenType::Plus, make<MemberOp>(make<Id>("foo"), make<Id>("bar")),
                                                    make<MemberOp>(make<Id>("foo"), make<Id>("baz"))));
  CHECK_AST_EXP("foo.@\"hello world\"", make<MemberOp>(make<Id>("foo"), make<Id>("hello world")));
  CHECK_AST_EXP("1.foo", make<MemberOp>(make<Int>(1), make<Id>("foo")));
  CHECK_AST_EXP("2.2.@\"hello world\"", make<MemberOp>(make<Float>(2.2), make<Id>("hello world")));
  CHECK_AST_EXP("foo.bar.baz", make<MemberOp>(make<MemberOp>(make<Id>("foo"), make<Id>("bar")), make<Id>("baz")));
  CHECK_AST_EXP("foo.bar\n.baz", make<MemberOp>(make<MemberOp>(make<Id>("foo"), make<Id>("bar")), make<Id>("baz")));
  CHECK_AST_EXP("foo\n.\nbar\n.\nbaz",
                make<MemberOp>(make<MemberOp>(make<Id>("foo"), make<Id>("bar")), make<Id>("baz")));
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
  CHECK_AST_EXP("{x}", make<Dict>(make<DictEntry>(make<Id>("x"), nullptr)));
  CHECK_AST_EXP("{x, y}", make<Dict>(make<DictEntry>(make<Id>("x"), nullptr), make<DictEntry>(make<Id>("y"), nullptr)));
  CHECK_AST_EXP("{x.y}", make<Dict>(make<DictEntry>(make<MemberOp>(make<Id>("x"), make<Id>("y")), nullptr)));
  CHECK_AST_EXP("{...x}", make<Dict>(make<DictEntry>(make<UnaryOp>(TokenType::TriplePoint, make<Id>("x")), nullptr)));
  CHECK_AST_EXP("{x: 1}", make<Dict>(make<DictEntry>(make<Id>("x"), make<Int>(1))));
  CHECK_AST_EXP("{x: 1, y: 2}",
                make<Dict>(make<DictEntry>(make<Id>("x"), make<Int>(1)), make<DictEntry>(make<Id>("y"), make<Int>(2))));
  CHECK_AST_EXP("{\"foo\": 1}", make<Dict>(make<DictEntry>(make<Id>("foo"), make<Int>(1))));
  CHECK_AST_EXP("{\"foo bar\": 1}", make<Dict>(make<DictEntry>(make<Id>("foo bar"), make<Int>(1))));
  CHECK_AST_EXP("{\"{name}\": 1}", make<Dict>(make<DictEntry>(make<FormatString>(make<Id>("name")), make<Int>(1))));
  CHECK_AST_EXP("{[name]: 1}", make<Dict>(make<DictEntry>(make<FormatString>(make<Id>("name")), make<Int>(1))));

  CHECK_ERROR_EXP("{25}", "expected identifier, member access or spread expression");
  CHECK_ERROR_EXP("{false}", "expected identifier, member access or spread expression");
  CHECK_ERROR_EXP("{,}", "unexpected ',' token, expected an expression");
  CHECK_ERROR_EXP("{:}", "unexpected ':' token, expected an expression");
  CHECK_ERROR_EXP("{\"foo\"}", "expected identifier, member access or spread expression");
  CHECK_ERROR_EXP("{[x]}", "expected identifier, member access or spread expression");
  CHECK_ERROR_EXP("{-5}", "unexpected operation");
  CHECK_ERROR_EXP("{[1, 2]: 1}", "list can only contain a single element");
  CHECK_ERROR_EXP("{25: 1}", "expected identifier, string literal, formatstring or '[x]: y' expression");
  CHECK_ERROR_EXP("{true: 1}", "expected identifier, string literal, formatstring or '[x]: y' expression");
}

TEST_CASE("if statements") {
  CHECK_AST_STMT("if x 1", make<If>(make<Id>("x"), make<Int>(1), make<Nop>()));
  CHECK_AST_STMT("if x {}", make<If>(make<Id>("x"), make<Block>(), make<Nop>()));
  CHECK_AST_STMT("if x 1 else 2", make<If>(make<Id>("x"), make<Int>(1), make<Int>(2)));
  CHECK_AST_STMT("if (x) 1", make<If>(make<Id>("x"), make<Int>(1), make<Nop>()));
  CHECK_AST_STMT("if (x) {}", make<If>(make<Id>("x"), make<Block>(), make<Nop>()));
  CHECK_AST_STMT("if x {} else x", make<If>(make<Id>("x"), make<Block>(), make<Id>("x")));
  CHECK_AST_STMT("if x x else {}", make<If>(make<Id>("x"), make<Id>("x"), make<Block>()));
  CHECK_AST_STMT("if x {} else {}", make<If>(make<Id>("x"), make<Block>(), make<Block>()));

  CHECK_ERROR_STMT("if", "unexpected end of file, expected an expression");
  CHECK_ERROR_STMT("if x", "unexpected end of file, expected a statement");
  CHECK_ERROR_STMT("if x 1 else", "unexpected end of file, expected a statement");
  CHECK_ERROR_STMT("if else x", "unexpected 'else' token, expected an expression");
}

TEST_CASE("while statements") {
  CHECK_AST_STMT("while x 1", make<While>(make<Id>("x"), make<Int>(1)));
  CHECK_AST_STMT("while (x) {}", make<While>(make<Id>("x"), make<Block>()));
  CHECK_AST_STMT("while (x) foo()", make<While>(make<Id>("x"), make<CallOp>(make<Id>("foo"))));

  CHECK_ERROR_STMT("while", "unexpected end of file, expected an expression");
  CHECK_ERROR_STMT("while x", "unexpected end of file, expected a statement");
}

TEST_CASE("loop statements") {
  CHECK_AST_STMT("loop 1", make<While>(make<Bool>(true), make<Int>(1)));
  CHECK_AST_STMT("loop {}", make<While>(make<Bool>(true), make<Block>()));

  CHECK_ERROR_STMT("loop", "unexpected end of file, expected a statement");
}

TEST_CASE("declarations") {
  CHECK_AST_STMT("let a", make<Declaration>(make<Id>("a"), make<Null>()));
  CHECK_AST_STMT("let a = 1", make<Declaration>(make<Id>("a"), make<Int>(1)));
  CHECK_AST_STMT("let a = 1 + 2",
                 make<Declaration>(make<Id>("a"), make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(2))));
  CHECK_AST_STMT("const a = 1", make<Declaration>(make<Id>("a"), make<Int>(1), true));
  CHECK_AST_STMT("const a = 1 + 2",
                 make<Declaration>(make<Id>("a"), make<BinaryOp>(TokenType::Plus, make<Int>(1), make<Int>(2)), true));

  CHECK_AST_STMT("let (a) = x", make<Declaration>(make<Tuple>(make<Id>("a")), make<Id>("x")));
  CHECK_AST_STMT("let (a, b) = x", make<Declaration>(make<Tuple>(make<Id>("a"), make<Id>("b")), make<Id>("x")));
  CHECK_AST_STMT(
    "let (a, ...b) = x",
    make<Declaration>(make<Tuple>(make<Id>("a"), make<UnaryOp>(TokenType::TriplePoint, make<Id>("b"))), make<Id>("x")));
  CHECK_AST_STMT(
    "let (a, ...b, c) = x",
    make<Declaration>(make<Tuple>(make<Id>("a"), make<UnaryOp>(TokenType::TriplePoint, make<Id>("b")), make<Id>("c")),
                      make<Id>("x")));

  CHECK_AST_STMT("const (a) = x", make<Declaration>(make<Tuple>(make<Id>("a")), make<Id>("x"), true));
  CHECK_AST_STMT("const (a, b) = x", make<Declaration>(make<Tuple>(make<Id>("a"), make<Id>("b")), make<Id>("x"), true));
  CHECK_AST_STMT("const (a, ...b) = x",
                 make<Declaration>(make<Tuple>(make<Id>("a"), make<UnaryOp>(TokenType::TriplePoint, make<Id>("b"))),
                                   make<Id>("x"), true));
  CHECK_AST_STMT(
    "const (a, ...b, c) = x",
    make<Declaration>(make<Tuple>(make<Id>("a"), make<UnaryOp>(TokenType::TriplePoint, make<Id>("b")), make<Id>("c")),
                      make<Id>("x"), true));

  CHECK_AST_STMT("let {a} = x", make<Declaration>(make<Dict>(make<DictEntry>(make<Id>("a"))), make<Id>("x")));
  CHECK_AST_STMT(
    "let {a, b} = x",
    make<Declaration>(make<Dict>(make<DictEntry>(make<Id>("a")), make<DictEntry>(make<Id>("b"))), make<Id>("x")));
  CHECK_AST_STMT("let {a, ...b} = x",
                 make<Declaration>(make<Dict>(make<DictEntry>(make<Id>("a")),
                                              make<DictEntry>(make<UnaryOp>(TokenType::TriplePoint, make<Id>("b")))),
                                   make<Id>("x")));
  CHECK_AST_STMT("let {a, ...b, c} = x",
                 make<Declaration>(make<Dict>(make<DictEntry>(make<Id>("a")),
                                              make<DictEntry>(make<UnaryOp>(TokenType::TriplePoint, make<Id>("b"))),
                                              make<DictEntry>(make<Id>("c"))),
                                   make<Id>("x")));

  CHECK_AST_STMT("const {a} = x", make<Declaration>(make<Dict>(make<DictEntry>(make<Id>("a"))), make<Id>("x"), true));
  CHECK_AST_STMT(
    "const {a, b} = x",
    make<Declaration>(make<Dict>(make<DictEntry>(make<Id>("a")), make<DictEntry>(make<Id>("b"))), make<Id>("x"), true));
  CHECK_AST_STMT("const {a, ...b} = x",
                 make<Declaration>(make<Dict>(make<DictEntry>(make<Id>("a")),
                                              make<DictEntry>(make<UnaryOp>(TokenType::TriplePoint, make<Id>("b")))),
                                   make<Id>("x"), true));
  CHECK_AST_STMT("const {a, ...b, c} = x",
                 make<Declaration>(make<Dict>(make<DictEntry>(make<Id>("a")),
                                              make<DictEntry>(make<UnaryOp>(TokenType::TriplePoint, make<Id>("b"))),
                                              make<DictEntry>(make<Id>("c"))),
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
