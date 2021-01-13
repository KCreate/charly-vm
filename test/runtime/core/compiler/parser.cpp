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

using Catch::Matchers::Contains;

using namespace charly::core::compiler;
using namespace charly::core::compiler::ast;

#define ASSERT_AST_TYPE(S, T)                          \
  {                                                    \
    ref<Expression> exp = Parser::parse_expression(S); \
    CHECK(isa<T>(exp));                                \
  }

#define ASSERT_AST_VALUE(S, T, V)                      \
  {                                                    \
    ref<Expression> exp = Parser::parse_expression(S); \
    CHECK(isa<T>(exp));                                \
    if (isa<T>(exp)) {                                 \
      CHECK(cast<T>(exp)->value == V);                 \
    }                                                  \
  }

#define ASSERT_AST_STRING(S, T, V)                     \
  {                                                    \
    ref<Expression> exp = Parser::parse_expression(S); \
    CHECK(isa<T>(exp));                                \
    if (isa<T>(exp)) {                                 \
      CHECK(cast<T>(exp)->value.compare(V) == 0);      \
    }                                                  \
  }

#define EXP(S, T) cast<T>(Parser::parse_expression(S))

#define CHECK_AST_EXP(S, N)                                           \
  {                                                               \
    std::stringstream exp_dump;                                   \
    std::stringstream ref_dump;                                   \
    DumpPass(exp_dump, false).visit(Parser::parse_expression(S)); \
    DumpPass(ref_dump, false).visit(N);                           \
    CHECK(exp_dump.str().compare(ref_dump.str()) == 0);           \
  }

TEST_CASE("parses literals") {
  CHECK_AST_EXP("0",               make<Int>(0));
  CHECK_AST_EXP("0x10",            make<Int>(0x10));
  CHECK_AST_EXP("0xFFFF",          make<Int>(0xFFFF));
  CHECK_AST_EXP("0b11111111",      make<Int>(0xFF));
  CHECK_AST_EXP("0b01010101",      make<Int>(0x55));
  CHECK_AST_EXP("0b00000000",      make<Int>(0x00));
  CHECK_AST_EXP("0o777",           make<Int>(0777));
  CHECK_AST_EXP("0o234",           make<Int>(0234));
  CHECK_AST_EXP("foo",             make<Id>("foo"));
  CHECK_AST_EXP("$",               make<Id>("$"));
  CHECK_AST_EXP("$$foo",           make<Id>("$$foo"));
  CHECK_AST_EXP("$1",              make<Id>("$1"));
  CHECK_AST_EXP("__foo",           make<Id>("__foo"));
  CHECK_AST_EXP("100",             make<Int>(100));
  CHECK_AST_EXP("25.25",           make<Float>(25.25));
  CHECK_AST_EXP("NaN",             make<Float>(NAN));
  CHECK_AST_EXP("true",            make<Bool>(true));
  CHECK_AST_EXP("false",           make<Bool>(false));
  CHECK_AST_EXP("null",            make<Null>());
  CHECK_AST_EXP("self",            make<Self>());
  CHECK_AST_EXP("super",           make<Super>());
  CHECK_AST_EXP("\"\"",            make<String>(""));
  CHECK_AST_EXP("\"hello world\"", make<String>("hello world"));

  CHECK_AST_EXP("\"\\a \\b \\n \\t \\v \\f \\\" \\{ \\\\ \"", make<String>("\a \b \n \t \v \f \" { \\ "));
}

TEST_CASE("parses tuples") {
  ASSERT_AST_TYPE("(1)",   Int);
  ASSERT_AST_TYPE("(1,)",  Tuple);
  ASSERT_AST_TYPE("(1,2)", Tuple);

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
  CHECK_AST_EXP("\"x:{x} y:{\"{y}\"}\"", make<FormatString>(
    make<String>("x:"),
    make<Id>("x"),
    make<String>(" y:"),
    make<FormatString>(make<Id>("y"))
  ));
  CHECK_AST_EXP("\"{\"{x}\"}\"", make<FormatString>(
    make<FormatString>(make<Id>("x"))
  ));
  CHECK_AST_EXP("\"x:{(foo, bar)}\"", make<FormatString>(
    make<String>("x:"), make<Tuple>(make<Id>("foo"), make <Id>("bar")
  )));
}

TEST_CASE("assignments") {
  CHECK_AST_EXP("x = 0", make<Assignment>(make<Id>("x"), make<Int>(0)));
  CHECK_AST_EXP("x = 1", make<Assignment>(make<Id>("x"), make<Int>(1)));
  CHECK_AST_EXP("x = true", make<Assignment>(make<Id>("x"), make<Bool>(true)));
  CHECK_AST_EXP("x = (1, 2)", make<Assignment>(make<Id>("x"), make<Tuple>(make<Int>(1), make<Int>(2))));
  CHECK_AST_EXP("(a, b, c) = 25", make<Assignment>(
    make<Tuple>(make<Id>("a"), make<Id>("b"), make<Id>("c")),
    make<Int>(25)
  ));
}
