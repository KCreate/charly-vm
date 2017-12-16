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
#include <unordered_map>

#include "location.h"

#pragma once

namespace Charly::Compilation {

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
  Break,
  Case,
  Catch,
  Class,
  Const,
  Continue,
  Default,
  Else,
  Extends,
  Finally,
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
static std::string kTokenTypeStrings[] = {
  "Integer",
  "Float",
  "Identifier",
  "String",
  "BooleanFalse",
  "BooleanTrue",
  "Null",
  "Nan",
  "Break",
  "Case",
  "Catch",
  "Class",
  "Const",
  "Continue",
  "Default",
  "Else",
  "Extends",
  "Finally",
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
static const std::unordered_map<std::string, TokenType> kTokenKeywordsAndLiterals = {
  {"false", TokenType::BooleanFalse},
  {"true", TokenType::BooleanTrue},
  {"null", TokenType::Null},
  {"NaN", TokenType::Nan},
  {"break", TokenType::Break},
  {"case", TokenType::Case},
  {"catch", TokenType::Catch},
  {"class", TokenType::Class},
  {"const", TokenType::Const},
  {"continue", TokenType::Continue},
  {"default", TokenType::Default},
  {"else", TokenType::Else},
  {"extends", TokenType::Extends},
  {"finally", TokenType::Finally},
  {"func", TokenType::Func},
  {"guard", TokenType::Guard},
  {"if", TokenType::If},
  {"let", TokenType::Let},
  {"loop", TokenType::Loop},
  {"primitive", TokenType::Primitive},
  {"property", TokenType::Property},
  {"return", TokenType::Return},
  {"static", TokenType::Static},
  {"switch", TokenType::Switch},
  {"throw", TokenType::Throw},
  {"try", TokenType::Try},
  {"typeof", TokenType::Typeof},
  {"unless", TokenType::Unless},
  {"until", TokenType::Until},
  {"while", TokenType::While},
};
static std::unordered_map<TokenType, TokenType> kTokenAndAssignmentOperators = {
  {TokenType::PlusAssignment, TokenType::Plus},
  {TokenType::MinusAssignment, TokenType::Minus},
  {TokenType::MulAssignment, TokenType::Mul},
  {TokenType::DivAssignment, TokenType::Div},
  {TokenType::ModAssignment, TokenType::Mod},
  {TokenType::PowAssignment, TokenType::Pow}
};
// clang-format on

struct Token {
  TokenType type;
  std::string value;

  union {
    double dbl_value;
    int64_t i64_value;
  } numeric_value;

  Location location;

  Token() {
  }
  Token(TokenType t) : type(t) {
  }
  Token(TokenType t, const std::string& v) : type(t), value(v) {
  }
  Token(TokenType t, const std::string& v, const Location& l) : type(t), value(v), location(l) {
  }

  inline bool is_and_assignment() {
    return (this->type == TokenType::PlusAssignment || this->type == TokenType::MinusAssignment ||
            this->type == TokenType::MulAssignment || this->type == TokenType::DivAssignment ||
            this->type == TokenType::ModAssignment || this->type == TokenType::PowAssignment);
  }

  template <class T>
  inline void write_to_stream(T&& stream) const {
    stream << kTokenTypeStrings[this->type] << " : ";

    if (this->type == TokenType::Integer) {
      stream << this->numeric_value.i64_value;
    } else if (this->type == TokenType::Float) {
      stream << this->numeric_value.dbl_value;
    } else {
      stream << this->value;
    }

    stream << ' ';
    this->location.write_to_stream(stream);
  }
};
}  // namespace Charly::Compilation
