/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include <string>

#include "location.h"

#pragma once

namespace Charly::Compiler {
enum TokenType : uint8_t {

  // Literals
  Integer,
  Float,
  Identifier,
  String,
  Boolean,
  Null,
  NAN,
  Keyword,

  // Operators
  Plus,
  Minus,
  Mul,
  Div,
  Mod,
  Pow,
  Assignment,

  // Bitwise operators
  BitOR,
  BitXOR,
  BitNOT,
  BitAND,
  LeftShift,
  RightShift,

  // AND assignments
  PlusAssignment,
  MinusAssignment,
  MulAssignment,
  DivAssignment,
  ModAssignment,
  PowAssignment,

  // Comparison
  Equal,
  Not,
  Less,
  Greater,
  LessEqual,
  GreaterEqual,
  AND,
  OR,

  // Structure
  LeftParen,
  RightParen,
  LeftCurly,
  RightCurly,
  LeftBracket,
  RightBracket,
  Semicolon,
  Comma,
  Point,
  Comment,
  AtSign,
  RightArrow,
  LeftArrow,
  QuestionMark,
  Colon,

  // Whitespace
  Whitespace,
  Newline,

  // Misc
  Eof,
  Unknown
};

struct Token {
  TokenType type;
  std::string value;
  Location location;

  inline bool is_and_operator() {
    return (this->type == TokenType::PlusAssignment || this->type == TokenType::MinusAssignment ||
            this->type == TokenType::MulAssignment || this->type == TokenType::DivAssignment ||
            this->type == TokenType::ModAssignment || this->type == TokenType::PowAssignment);
  }

  // TODO: Add location information
};
}  // namespace Charly::Compiler
