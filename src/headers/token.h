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
  BooleanFalse,
  BooleanTrue,
  Null,
  Nan,

  // Keywords
  Keyword,
  Break,
  Case,
  Catch,
  Class,
  Const,
  Continue,
  Default,
  Else,
  Extends,
  Func,
  Guard,
  If,
  Let,
  Loop,
  Primitive,
  Property,
  Return,
  Static,
  Switch,
  Throw,
  Try,
  Typeof,
  Unless,
  Until,
  While,

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

// String representation of token types
// clang-format off
static const std::string kTokenTypeStrings[] = {
  "Integer",
  "Float",
  "Identifier",
  "String",
  "Boolean",
  "Null",
  "Nan",
  "Keyword",
  "Break",
  "Case",
  "Catch",
  "Class",
  "Const",
  "Continue",
  "Default",
  "Else",
  "Extends",
  "Func",
  "Guard",
  "If",
  "Let",
  "Loop",
  "Primitive",
  "Property",
  "Return",
  "Static",
  "Switch",
  "Throw",
  "Try",
  "Typeof",
  "Unless",
  "Until",
  "While",
  "Plus",
  "Minus",
  "Mul",
  "Div",
  "Mod",
  "Pow",
  "Assignment",
  "BitOR",
  "BitXOR",
  "BitNOT",
  "BitAND",
  "LeftShift",
  "RightShift",
  "PlusAssignment",
  "MinusAssignment",
  "MulAssignment",
  "DivAssignment",
  "ModAssignment",
  "PowAssignment",
  "Equal",
  "Not",
  "Less",
  "Greater",
  "LessEqual",
  "GreaterEqual",
  "AND",
  "OR",
  "LeftParen",
  "RightParen",
  "LeftCurly",
  "RightCurly",
  "LeftBracket",
  "RightBracket",
  "Semicolon",
  "Comma",
  "Point",
  "Comment",
  "AtSign",
  "RightArrow",
  "LeftArrow",
  "QuestionMark",
  "Colon",
  "Whitespace",
  "Newline",
  "Eof",
  "Unknown"
};
// clang-format on

struct Token {
  TokenType type;
  std::string value;
  Location location;

  Token() {
  }
  Token(TokenType t) : type(t) {
  }
  Token(TokenType t, const std::string& v) : type(t), value(v) {
  }
  Token(TokenType t, const std::string& v, const Location& l) : type(t), value(v), location(l) {
  }

  inline bool is_and_operator() {
    return (this->type == TokenType::PlusAssignment || this->type == TokenType::MinusAssignment ||
            this->type == TokenType::MulAssignment || this->type == TokenType::DivAssignment ||
            this->type == TokenType::ModAssignment || this->type == TokenType::PowAssignment);
  }

  inline bool is_keyword() {
    return (Break == this->type || Case == this->type || Catch == this->type || Class == this->type ||
            Const == this->type || Continue == this->type || Default == this->type || Else == this->type ||
            Extends == this->type || Func == this->type || Guard == this->type || If == this->type ||
            Let == this->type || Loop == this->type || Primitive == this->type || Property == this->type ||
            Return == this->type || Static == this->type || Switch == this->type || Throw == this->type ||
            Try == this->type || Typeof == this->type || Unless == this->type || Until == this->type ||
            While == this->type);
  }

  template <class T>
  inline void write_to_stream(T&& stream) const {
    stream << kTokenTypeStrings[this->type] << " : " << this->value << " ";
    this->location.write_to_stream(stream);
  }
};
}  // namespace Charly::Compiler
