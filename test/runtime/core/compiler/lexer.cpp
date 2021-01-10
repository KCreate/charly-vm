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

TEST_CASE("tokenizes integers") {
  Lexer lexer("test", "0 1 25 0b1111 0o777 0777 0xffff 0xFF 0");

  CHECK(lexer.read_token().intval == 0);
  CHECK(lexer.read_token().intval == 1);
  CHECK(lexer.read_token().intval == 25);
  CHECK(lexer.read_token().intval == 15);
  CHECK(lexer.read_token().intval == 511);
  CHECK(lexer.read_token().intval == 777);
  CHECK(lexer.read_token().intval == 65535);
  CHECK(lexer.read_token().intval == 255);
  CHECK(lexer.read_token().intval == 0);
}

TEST_CASE("throws an error on incomplete number literals") {
  {
    Lexer lexer("test", "0x");
    CHECK_THROWS_WITH(lexer.read_token(), "hex number literal expected at least one digit");
  }
  {
    Lexer lexer("test", "0b");
    CHECK_THROWS_WITH(lexer.read_token(), "binary number literal expected at least one digit");
  }
  {
    Lexer lexer("test", "0o");
    CHECK_THROWS_WITH(lexer.read_token(), "octal number literal expected at least one digit");
  }
}

TEST_CASE("tokenizes floats") {
  Lexer lexer("test", "1.0 2.0 0.0 0.1 0.5 2.5 25.25 1234.12345678");

  CHECK(lexer.read_token().floatval == 1.0);
  CHECK(lexer.read_token().floatval == 2.0);
  CHECK(lexer.read_token().floatval == 0.0);
  CHECK(lexer.read_token().floatval == 0.1);
  CHECK(lexer.read_token().floatval == 0.5);
  CHECK(lexer.read_token().floatval == 2.5);
  CHECK(lexer.read_token().floatval == 25.25);
  CHECK(lexer.read_token().floatval == 1234.12345678);
}

TEST_CASE("tokenizes identifiers") {
  Lexer lexer("test", "foo foo25 $foo $_foobar foo$bar");

  CHECK(lexer.read_token().source.compare("foo") == 0);
  CHECK(lexer.read_token().source.compare("foo25") == 0);
  CHECK(lexer.read_token().source.compare("$foo") == 0);
  CHECK(lexer.read_token().source.compare("$_foobar") == 0);
  CHECK(lexer.read_token().source.compare("foo$bar") == 0);
}

TEST_CASE("tokenizes whitespace and newlines") {
  Lexer lexer("test", "  \n\r\n\t\n");

  CHECK(lexer.read_token_all().type == TokenType::Whitespace);
  CHECK(lexer.read_token_all().type == TokenType::Newline);
  CHECK(lexer.read_token_all().type == TokenType::Whitespace);
  CHECK(lexer.read_token_all().type == TokenType::Newline);
  CHECK(lexer.read_token_all().type == TokenType::Whitespace);
  CHECK(lexer.read_token_all().type == TokenType::Newline);
}

TEST_CASE("returns eof token after last token parsed") {
  Lexer lexer("test", "25");

  CHECK(lexer.read_token().type == TokenType::Int);
  CHECK(lexer.read_token().type == TokenType::Eof);
  CHECK(lexer.read_token().type == TokenType::Eof);
  CHECK(lexer.read_token().type == TokenType::Eof);
  CHECK(lexer.read_token().type == TokenType::Eof);
}

TEST_CASE("writes location information to tokens") {
  Lexer lexer("test", "\n\n\n   hello_world");
  lexer.read_token();

  CHECK(lexer.last_token().type == TokenType::Identifier);
  CHECK(lexer.last_token().location.offset == 6);
  CHECK(lexer.last_token().location.length == 11);
  CHECK(lexer.last_token().location.row == 4);
  CHECK(lexer.last_token().location.column == 4);
  CHECK(lexer.last_token().source.compare("hello_world") == 0);
}

TEST_CASE("throws on unexpected characters") {
  Lexer lexer("test", "π");
  CHECK_THROWS_WITH(lexer.read_token(), "unexpected character");
}

TEST_CASE("formats a token") {
  Lexer lexer("test", "foobarbaz\n  25\n     25.25");

  {
    std::stringstream stream;
    stream << lexer.read_token();
    CHECK(stream.str().compare("(Identifier, foobarbaz) test:1:1") == 0);
  }

  {
    std::stringstream stream;
    stream << lexer.read_token();
    CHECK(stream.str().compare("(Int, 25) test:2:3") == 0);
  }

  {
    std::stringstream stream;
    stream << lexer.read_token();
    CHECK(stream.str().compare("(Float, 25.25) test:3:6") == 0);
  }
}

TEST_CASE("recognizes keywords") {
  Lexer lexer("test",
              ("false\n"
               "NaN\n"
               "null\n"
               "self\n"
               "super\n"
               "true\n"
               "and\n"
               "as\n"
               "await\n"
               "break\n"
               "case\n"
               "catch\n"
               "class\n"
               "const\n"
               "continue\n"
               "default\n"
               "defer\n"
               "do\n"
               "else\n"
               "export\n"
               "extends\n"
               "finally\n"
               "for\n"
               "from\n"
               "func\n"
               "guard\n"
               "if\n"
               "import\n"
               "in\n"
               "let\n"
               "loop\n"
               "match\n"
               "new\n"
               "operator\n"
               "or\n"
               "property\n"
               "return\n"
               "spawn\n"
               "static\n"
               "switch\n"
               "throw\n"
               "try\n"
               "typeof\n"
               "unless\n"
               "until\n"
               "while\n"
               "yield\n"));

  CHECK(lexer.read_token().type == TokenType::False);
  CHECK(lexer.read_token().type == TokenType::Float);
  CHECK(lexer.read_token().type == TokenType::Null);
  CHECK(lexer.read_token().type == TokenType::Self);
  CHECK(lexer.read_token().type == TokenType::Super);
  CHECK(lexer.read_token().type == TokenType::True);
  CHECK(lexer.read_token().type == TokenType::AndLiteral);
  CHECK(lexer.read_token().type == TokenType::As);
  CHECK(lexer.read_token().type == TokenType::Await);
  CHECK(lexer.read_token().type == TokenType::Break);
  CHECK(lexer.read_token().type == TokenType::Case);
  CHECK(lexer.read_token().type == TokenType::Catch);
  CHECK(lexer.read_token().type == TokenType::Class);
  CHECK(lexer.read_token().type == TokenType::Const);
  CHECK(lexer.read_token().type == TokenType::Continue);
  CHECK(lexer.read_token().type == TokenType::Default);
  CHECK(lexer.read_token().type == TokenType::Defer);
  CHECK(lexer.read_token().type == TokenType::Do);
  CHECK(lexer.read_token().type == TokenType::Else);
  CHECK(lexer.read_token().type == TokenType::Export);
  CHECK(lexer.read_token().type == TokenType::Extends);
  CHECK(lexer.read_token().type == TokenType::Finally);
  CHECK(lexer.read_token().type == TokenType::For);
  CHECK(lexer.read_token().type == TokenType::From);
  CHECK(lexer.read_token().type == TokenType::Func);
  CHECK(lexer.read_token().type == TokenType::Guard);
  CHECK(lexer.read_token().type == TokenType::If);
  CHECK(lexer.read_token().type == TokenType::Import);
  CHECK(lexer.read_token().type == TokenType::In);
  CHECK(lexer.read_token().type == TokenType::Let);
  CHECK(lexer.read_token().type == TokenType::Loop);
  CHECK(lexer.read_token().type == TokenType::Match);
  CHECK(lexer.read_token().type == TokenType::New);
  CHECK(lexer.read_token().type == TokenType::Operator);
  CHECK(lexer.read_token().type == TokenType::OrLiteral);
  CHECK(lexer.read_token().type == TokenType::Property);
  CHECK(lexer.read_token().type == TokenType::Return);
  CHECK(lexer.read_token().type == TokenType::Spawn);
  CHECK(lexer.read_token().type == TokenType::Static);
  CHECK(lexer.read_token().type == TokenType::Switch);
  CHECK(lexer.read_token().type == TokenType::Throw);
  CHECK(lexer.read_token().type == TokenType::Try);
  CHECK(lexer.read_token().type == TokenType::Typeof);
  CHECK(lexer.read_token().type == TokenType::Unless);
  CHECK(lexer.read_token().type == TokenType::Until);
  CHECK(lexer.read_token().type == TokenType::While);
  CHECK(lexer.read_token().type == TokenType::Yield);
  CHECK(lexer.read_token().type == TokenType::Eof);
}

TEST_CASE("recognizes operators") {
  Lexer lexer("test", ("+-*/%** = == != < > <= >= && || ! | ^~&<< >> >>> += -= *= /= %= **= &= |= ^= <<= >>= >>>="));

  CHECK(lexer.read_token().type == TokenType::Plus);
  CHECK(lexer.read_token().type == TokenType::Minus);
  CHECK(lexer.read_token().type == TokenType::Mul);
  CHECK(lexer.read_token().type == TokenType::Div);
  CHECK(lexer.read_token().type == TokenType::Mod);
  CHECK(lexer.read_token().type == TokenType::Pow);
  CHECK(lexer.read_token().type == TokenType::Assignment);

  CHECK(lexer.read_token().type == TokenType::Equal);
  CHECK(lexer.read_token().type == TokenType::NotEqual);
  CHECK(lexer.read_token().type == TokenType::LessThan);
  CHECK(lexer.read_token().type == TokenType::GreaterThan);
  CHECK(lexer.read_token().type == TokenType::LessEqual);
  CHECK(lexer.read_token().type == TokenType::GreaterEqual);
  CHECK(lexer.read_token().type == TokenType::And);
  CHECK(lexer.read_token().type == TokenType::Or);
  CHECK(lexer.read_token().type == TokenType::UnaryNot);

  CHECK(lexer.read_token().type == TokenType::BitOR);
  CHECK(lexer.read_token().type == TokenType::BitXOR);
  CHECK(lexer.read_token().type == TokenType::BitNOT);
  CHECK(lexer.read_token().type == TokenType::BitAND);
  CHECK(lexer.read_token().type == TokenType::BitLeftShift);
  CHECK(lexer.read_token().type == TokenType::BitRightShift);
  CHECK(lexer.read_token().type == TokenType::BitUnsignedRightShift);

  CHECK(lexer.read_token().type == TokenType::PlusAssignment);
  CHECK(lexer.read_token().type == TokenType::MinusAssignment);
  CHECK(lexer.read_token().type == TokenType::MulAssignment);
  CHECK(lexer.read_token().type == TokenType::DivAssignment);
  CHECK(lexer.read_token().type == TokenType::ModAssignment);
  CHECK(lexer.read_token().type == TokenType::PowAssignment);
  CHECK(lexer.read_token().type == TokenType::BitANDAssignment);
  CHECK(lexer.read_token().type == TokenType::BitORAssignment);
  CHECK(lexer.read_token().type == TokenType::BitXORAssignment);
  CHECK(lexer.read_token().type == TokenType::BitLeftShiftAssignment);
  CHECK(lexer.read_token().type == TokenType::BitRightShiftAssignment);
  CHECK(lexer.read_token().type == TokenType::BitUnsignedRightShiftAssignment);
}

TEST_CASE("recognizes structure tokens") {
  Lexer lexer("test", ("(){}[].:,;@<-->=>?\n"));

  CHECK(lexer.read_token().type == TokenType::LeftParen);
  CHECK(lexer.read_token().type == TokenType::RightParen);
  CHECK(lexer.read_token().type == TokenType::LeftCurly);
  CHECK(lexer.read_token().type == TokenType::RightCurly);
  CHECK(lexer.read_token().type == TokenType::LeftBracket);
  CHECK(lexer.read_token().type == TokenType::RightBracket);
  CHECK(lexer.read_token().type == TokenType::Point);
  CHECK(lexer.read_token().type == TokenType::Colon);
  CHECK(lexer.read_token().type == TokenType::Comma);
  CHECK(lexer.read_token().type == TokenType::Semicolon);
  CHECK(lexer.read_token().type == TokenType::AtSign);
  CHECK(lexer.read_token().type == TokenType::LeftArrow);
  CHECK(lexer.read_token().type == TokenType::RightArrow);
  CHECK(lexer.read_token().type == TokenType::RightThickArrow);
  CHECK(lexer.read_token().type == TokenType::QuestionMark);
}

TEST_CASE("recognizes comments") {
  Lexer lexer("test",
              ("foo bar // some comment\n"
               "// hello\n"
               "// world\n"
               "//\n"
               "/*\n"
               "multiline comment!!\n"
               "*/\n"
               "/* hello world */ /* test */\n"
               "/* foo /* nested */ */\n"));

  CHECK(lexer.read_token().type == TokenType::Identifier);
  CHECK(lexer.read_token().type == TokenType::Identifier);
  CHECK(lexer.read_token_all().type == TokenType::Whitespace);

  CHECK(lexer.read_token_all().type == TokenType::Comment);
  CHECK(lexer.last_token().source.compare("// some comment") == 0);
  CHECK(lexer.read_token_all().type == TokenType::Newline);

  CHECK(lexer.read_token_all().type == TokenType::Comment);
  CHECK(lexer.last_token().source.compare("// hello") == 0);
  CHECK(lexer.read_token_all().type == TokenType::Newline);

  CHECK(lexer.read_token_all().type == TokenType::Comment);
  CHECK(lexer.last_token().source.compare("// world") == 0);
  CHECK(lexer.read_token_all().type == TokenType::Newline);

  CHECK(lexer.read_token_all().type == TokenType::Comment);
  CHECK(lexer.last_token().source.compare("//") == 0);
  CHECK(lexer.read_token_all().type == TokenType::Newline);

  CHECK(lexer.read_token_all().type == TokenType::Comment);
  CHECK(lexer.last_token().source.compare("/*\nmultiline comment!!\n*/") == 0);
  CHECK(lexer.read_token_all().type == TokenType::Newline);

  CHECK(lexer.read_token_all().type == TokenType::Comment);
  CHECK(lexer.last_token().source.compare("/* hello world */") == 0);
  CHECK(lexer.read_token_all().type == TokenType::Whitespace);
  CHECK(lexer.read_token_all().type == TokenType::Comment);
  CHECK(lexer.last_token().source.compare("/* test */") == 0);
  CHECK(lexer.read_token_all().type == TokenType::Newline);

  CHECK(lexer.read_token_all().type == TokenType::Comment);
  CHECK(lexer.last_token().source.compare("/* foo /* nested */ */") == 0);
  CHECK(lexer.read_token_all().type == TokenType::Newline);
}

TEST_CASE("tokenizes strings") {
  Lexer lexer("test",
              ("\"hello world\"\n"
               "\"äüöø¡œΣ€\"\n"
               "\"\"\n"));

  CHECK(lexer.read_token().type == TokenType::String);
  CHECK(lexer.last_token().source.compare("hello world") == 0);

  CHECK(lexer.read_token().type == TokenType::String);
  CHECK(lexer.last_token().source.compare("äüöø¡œΣ€") == 0);

  CHECK(lexer.read_token().type == TokenType::String);
  CHECK(lexer.last_token().source.compare("") == 0);
}

TEST_CASE("escape sequences in strings") {
  {
    Lexer lexer("test", ("\"\\a \\b \\n \\r \\t \\v \\f \\\" \\{ \\\\ \"\n"));

    CHECK(lexer.read_token().type == TokenType::String);
    CHECK(lexer.last_token().source.compare("\a \b \n \r \t \v \f \" { \\ ") == 0);
  }

  {
    Lexer lexer("test", ("\"\\k\""));

    CHECK_THROWS_WITH(lexer.read_token(), "unknown escape sequence");
  }

  {
    Lexer lexer("test", ("\"\\"));

    CHECK_THROWS_WITH(lexer.read_token(), "unfinished escape sequence");
  }
}

TEST_CASE("tokenizes string interpolations") {
  Lexer lexer("test",
              ("\"before {name({{}})} {more} after\""
               "\"{\"{nested}\"}\""
               "\"{}\""
               "\"{}}\""
               "\"\\{}\""));

  CHECK(lexer.read_token().type == TokenType::FormatString);
  CHECK(lexer.last_token().source.compare("before ") == 0);
  CHECK(lexer.read_token().type == TokenType::Identifier);
  CHECK(lexer.last_token().source.compare("name") == 0);
  CHECK(lexer.read_token().type == TokenType::LeftParen);
  CHECK(lexer.read_token().type == TokenType::LeftCurly);
  CHECK(lexer.read_token().type == TokenType::LeftCurly);
  CHECK(lexer.read_token().type == TokenType::RightCurly);
  CHECK(lexer.read_token().type == TokenType::RightCurly);
  CHECK(lexer.read_token().type == TokenType::RightParen);
  CHECK(lexer.read_token().type == TokenType::RightCurly);
  CHECK(lexer.read_token().type == TokenType::FormatString);
  CHECK(lexer.last_token().source.compare(" ") == 0);
  CHECK(lexer.read_token().type == TokenType::Identifier);
  CHECK(lexer.last_token().source.compare("more") == 0);
  CHECK(lexer.read_token().type == TokenType::RightCurly);
  CHECK(lexer.read_token().type == TokenType::String);
  CHECK(lexer.last_token().source.compare(" after") == 0);

  CHECK(lexer.read_token().type == TokenType::FormatString);
  CHECK(lexer.last_token().source.compare("") == 0);
  CHECK(lexer.read_token().type == TokenType::FormatString);
  CHECK(lexer.last_token().source.compare("") == 0);
  CHECK(lexer.read_token().type == TokenType::Identifier);
  CHECK(lexer.last_token().source.compare("nested") == 0);
  CHECK(lexer.read_token().type == TokenType::RightCurly);
  CHECK(lexer.read_token().type == TokenType::String);
  CHECK(lexer.last_token().source.compare("") == 0);
  CHECK(lexer.read_token().type == TokenType::RightCurly);
  CHECK(lexer.read_token().type == TokenType::String);
  CHECK(lexer.last_token().source.compare("") == 0);

  CHECK(lexer.read_token().type == TokenType::FormatString);
  CHECK(lexer.last_token().source.compare("") == 0);
  CHECK(lexer.read_token().type == TokenType::RightCurly);
  CHECK(lexer.read_token().type == TokenType::String);
  CHECK(lexer.last_token().source.compare("") == 0);

  CHECK(lexer.read_token().type == TokenType::FormatString);
  CHECK(lexer.last_token().source.compare("") == 0);
  CHECK(lexer.read_token().type == TokenType::RightCurly);
  CHECK(lexer.read_token().type == TokenType::String);
  CHECK(lexer.last_token().source.compare("}") == 0);

  CHECK(lexer.read_token().type == TokenType::String);
  CHECK(lexer.last_token().source.compare("{}") == 0);
}

TEST_CASE("catches erronious string interpolations") {
  {
    Lexer lexer("test", ("\"{\""));

    CHECK(lexer.read_token().type == TokenType::FormatString);
    CHECK_THROWS_WITH(lexer.read_token(), "unclosed string");
  }

  {
    Lexer lexer("test", ("\"{"));

    CHECK(lexer.read_token().type == TokenType::FormatString);
    CHECK_THROWS_WITH(lexer.read_token(), "unfinished string interpolation");
  }
}

TEST_CASE("detects mismatched brackets") {
  {
    Lexer lexer("test", ("("));

    lexer.read_token();
    CHECK_THROWS_WITH(lexer.read_token(), "unclosed bracket");
  }
  {
    Lexer lexer("test", ("["));

    lexer.read_token();
    CHECK_THROWS_WITH(lexer.read_token(), "unclosed bracket");
  }
  {
    Lexer lexer("test", ("{"));

    lexer.read_token();
    CHECK_THROWS_WITH(lexer.read_token(), "unclosed bracket");
  }
  {
    Lexer lexer("test", ("(}"));

    lexer.read_token();
    CHECK_THROWS_WITH(lexer.read_token(), "unexpected }");
  }
  {
    Lexer lexer("test", ("{)"));

    lexer.read_token();
    CHECK_THROWS_WITH(lexer.read_token(), "unexpected )");
  }
  {
    Lexer lexer("test", ("(]"));

    lexer.read_token();
    CHECK_THROWS_WITH(lexer.read_token(), "unexpected ]");
  }
}

TEST_CASE("detects unclosed multiline comments") {
  Lexer lexer("test", ("/* /* */"));

  CHECK_THROWS_WITH(lexer.read_token(), "unclosed comment");
}

TEST_CASE("formats a CompilerError") {
  Lexer lexer("test", "0x");

  try {
    lexer.read_token();
  } catch (CompilerError& exc) {
    std::stringstream stream;
    stream << exc;
    CHECK(stream.str().compare("test:1:1: hex number literal expected at least one digit") == 0);
  }
}
