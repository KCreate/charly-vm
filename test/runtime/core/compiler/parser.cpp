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

using Catch::Matchers::Contains;

using namespace charly::core::compiler;

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

TEST_CASE("parses literals") {
  ASSERT_AST_VALUE("100",   Int,   100);
  ASSERT_AST_VALUE("0",     Int,   0);
  ASSERT_AST_VALUE("25.25", Float, 25.25);
  ASSERT_AST_TYPE("NaN",    Float);
  ASSERT_AST_VALUE("true",  Bool,  true);
  ASSERT_AST_VALUE("false", Bool,  false);
  ASSERT_AST_TYPE("null",   Null);
  ASSERT_AST_TYPE("self",   Self);
  ASSERT_AST_TYPE("super",  Super);

  ASSERT_AST_STRING("\"\"",                                       String, "");
  ASSERT_AST_STRING("\"hello world\"",                            String, "hello world");
  ASSERT_AST_STRING("\"\\a \\b \\n \\t \\v \\f \\\" \\{ \\\\ \"", String, "\a \b \n \t \v \f \" { \\ ");
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
  ref<FormatString> f1 = EXP("\"x:{x}\"", FormatString);

  CHECK(f1->elements.size() == 3);
  REQUIRE(isa<String>(f1->elements[0]));
  CHECK(cast<String>(f1->elements[0])->value.compare("x:") == 0);

  REQUIRE(isa<Id>(f1->elements[1]));
  CHECK(cast<Id>(f1->elements[1])->value.compare("x") == 0);

  REQUIRE(isa<String>(f1->elements[2]));
  CHECK(cast<String>(f1->elements[2])->value.compare("") == 0);



  ref<FormatString> f2 = EXP("\"x:{x} y:{\"{y}\"}\"", FormatString);

  CHECK(f2->elements.size() == 5);
  REQUIRE(isa<String>(f2->elements[0]));
  CHECK(cast<String>(f2->elements[0])->value.compare("x:") == 0);

  REQUIRE(isa<Id>(f2->elements[1]));
  CHECK(cast<Id>(f2->elements[1])->value.compare("x") == 0);

  REQUIRE(isa<String>(f2->elements[2]));
  CHECK(cast<String>(f2->elements[2])->value.compare(" y:") == 0);

  REQUIRE(isa<FormatString>(f2->elements[3]));
  CHECK(cast<FormatString>(f2->elements[3])->elements.size() == 3);

  ref<FormatString> f2y = cast<FormatString>(f2->elements[3]);

  REQUIRE(isa<String>(f2y->elements[0]));
  CHECK(cast<String>(f2y->elements[0])->value.compare("") == 0);

  REQUIRE(isa<Id>(f2y->elements[1]));
  CHECK(cast<Id>(f2y->elements[1])->value.compare("y") == 0);

  REQUIRE(isa<String>(f2y->elements[2]));
  CHECK(cast<String>(f2y->elements[2])->value.compare("") == 0);
}
