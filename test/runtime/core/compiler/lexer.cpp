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

#include "charly/core/compiler/lexer.h"

using Catch::Matchers::Contains;

using namespace charly::core::compiler;

TEST_CASE("Lexer") {

  SECTION("tokenizes integers") {
    Lexer lexer("test", "0 1 25 0b1111 0o777 0777 0xffff 0xFF 0");

    CHECK(lexer.read_token_skip_whitespace().intval == 0);
    CHECK(lexer.read_token_skip_whitespace().intval == 1);
    CHECK(lexer.read_token_skip_whitespace().intval == 25);
    CHECK(lexer.read_token_skip_whitespace().intval == 15);
    CHECK(lexer.read_token_skip_whitespace().intval == 511);
    CHECK(lexer.read_token_skip_whitespace().intval == 511);
    CHECK(lexer.read_token_skip_whitespace().intval == 65535);
    CHECK(lexer.read_token_skip_whitespace().intval == 255);
    CHECK(lexer.read_token_skip_whitespace().intval == 0);
  }

  SECTION("throws an error on incomplete number literals") {
    {
      Lexer lexer("test", "0x");
      CHECK_THROWS_WITH(lexer.read_token_skip_whitespace(), "Hex number literal expected at least one digit");
    }
    {
      Lexer lexer("test", "0b");
      CHECK_THROWS_WITH(lexer.read_token_skip_whitespace(), "Binary number literal expected at least one digit");
    }
    {
      Lexer lexer("test", "0o");
      CHECK_THROWS_WITH(lexer.read_token_skip_whitespace(), "Octal number literal expected at least one digit");
    }
  }

  SECTION("tokenizes floats") {
    Lexer lexer("test", "1.0 2.0 0.0 0.1 0.5 2.5 25.25 1234.12345678");

    CHECK(lexer.read_token_skip_whitespace().floatval == 1.0);
    CHECK(lexer.read_token_skip_whitespace().floatval == 2.0);
    CHECK(lexer.read_token_skip_whitespace().floatval == 0.0);
    CHECK(lexer.read_token_skip_whitespace().floatval == 0.1);
    CHECK(lexer.read_token_skip_whitespace().floatval == 0.5);
    CHECK(lexer.read_token_skip_whitespace().floatval == 2.5);
    CHECK(lexer.read_token_skip_whitespace().floatval == 25.25);
    CHECK(lexer.read_token_skip_whitespace().floatval == 1234.12345678);
  }

  SECTION("tokenizes identifiers") {
    Lexer lexer("test", "foo foo25 $foo $_foobar foo$bar");

    CHECK(lexer.read_token_skip_whitespace().source.compare("foo") == 0);
    CHECK(lexer.read_token_skip_whitespace().source.compare("foo25") == 0);
    CHECK(lexer.read_token_skip_whitespace().source.compare("$foo") == 0);
    CHECK(lexer.read_token_skip_whitespace().source.compare("$_foobar") == 0);
    CHECK(lexer.read_token_skip_whitespace().source.compare("foo$bar") == 0);
  }

  SECTION("tokenizes whitespace and newlines") {
    Lexer lexer("test", "  \n\r\n\t\n");

    CHECK(lexer.read_token().type == TokenType::Whitespace);
    CHECK(lexer.read_token().type == TokenType::Newline);
    CHECK(lexer.read_token().type == TokenType::Whitespace);
    CHECK(lexer.read_token().type == TokenType::Newline);
    CHECK(lexer.read_token().type == TokenType::Whitespace);
    CHECK(lexer.read_token().type == TokenType::Newline);
  }

  SECTION("returns eof token after last token parsed") {
    Lexer lexer("test", "25");

    CHECK(lexer.read_token().type == TokenType::Int);
    CHECK(lexer.read_token().type == TokenType::Eof);
    CHECK(lexer.read_token().type == TokenType::Eof);
    CHECK(lexer.read_token().type == TokenType::Eof);
    CHECK(lexer.read_token().type == TokenType::Eof);
  }

  SECTION("writes location information to tokens") {
    Lexer lexer("test", "\n\n\n   hello_world");
    lexer.read_token_skip_whitespace();

    CHECK(lexer.last_token().type == TokenType::Identifier);
    CHECK(lexer.last_token().location.offset == 6);
    CHECK(lexer.last_token().location.length == 11);
    CHECK(lexer.last_token().location.row == 4);
    CHECK(lexer.last_token().location.column == 4);
    CHECK(lexer.last_token().source.compare("hello_world") == 0);
  }

  SECTION("throws on unexpected characters") {
    Lexer lexer("test", "π");
    CHECK_THROWS_WITH(lexer.read_token_skip_whitespace(), "Unexpected character");
  }

  SECTION("formats a token") {
    Lexer lexer("test", "foobarbaz\n  25\n     25.25");

    {
      std::stringstream stream;
      lexer.read_token_skip_whitespace().dump(stream);
      CHECK(stream.str().compare("(Identifier, foobarbaz) test:1:1") == 0);
    }

    {
      std::stringstream stream;
      lexer.read_token_skip_whitespace().dump(stream);
      CHECK(stream.str().compare("(Int, 25) test:2:3") == 0);
    }

    {
      std::stringstream stream;
      lexer.read_token_skip_whitespace().dump(stream);
      CHECK(stream.str().compare("(Float, 25.25) test:3:6") == 0);
    }
  }

  SECTION("formats a LexerException") {
    Lexer lexer("test", "0x");

    try {
      lexer.read_token();
    } catch(LexerException& exc) {
      std::stringstream stream;
      exc.dump(stream);

      CHECK(stream.str().compare("test:1:1: Hex number literal expected at least one digit") == 0);
    }
  }
}
