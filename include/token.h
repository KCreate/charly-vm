/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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
  Number,
  Identifier,
  String,
  BooleanFalse,
  BooleanTrue,
  Null,
  Nan,
  Self,

  // Keywords
  Break,
  Case,
  Catch,
  Class,
  Const,
  Continue,
  Default,
  Do,
  Else,
  Extends,
  Finally,
  Func,
  Guard,
  If,
  IgnoreConst,
  Let,
  Loop,
  Match,
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
  Yield,

  // Operators
  Plus,
  Minus,
  Mul,
  Div,
  Mod,
  Pow,
  Assignment,

  // Special unary operator tokens
  UPlus,
  UMinus,
  UNot,

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
  BitANDAssignment,
  BitORAssignment,
  BitXORAssignment,
  LeftShiftAssignment,
  RightShiftAssignment,

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
  RightThickArrow,
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
  "Number",
  "Identifier",
  "String",
  "BooleanFalse",
  "BooleanTrue",
  "Null",
  "Nan",
  "Self",
  "Break",
  "Case",
  "Catch",
  "Class",
  "Const",
  "Continue",
  "Default",
  "Do",
  "Else",
  "Extends",
  "Finally",
  "Func",
  "Guard",
  "If",
  "IgnoreConst",
  "Let",
  "Loop",
  "Match",
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
  "Yield",
  "Plus",
  "Minus",
  "Mul",
  "Div",
  "Mod",
  "Pow",
  "Assignment",
  "UPlus",
  "UMinus",
  "UNot",
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
  "BitANDAssignment",
  "BitORAssignment",
  "BitXORAssignment",
  "LeftShiftAssignment",
  "RightShiftAssignment",
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
  "RightThickArrow",
  "QuestionMark",
  "Colon",
  "Whitespace",
  "Newline",
  "Eof",
  "Unknown"
};
static const std::unordered_map<std::string, TokenType> kTokenKeywordsAndLiterals = {
  {"NaN", TokenType::Nan},
  {"break", TokenType::Break},
  {"case", TokenType::Case},
  {"catch", TokenType::Catch},
  {"class", TokenType::Class},
  {"const", TokenType::Const},
  {"continue", TokenType::Continue},
  {"default", TokenType::Default},
  {"do", TokenType::Do},
  {"else", TokenType::Else},
  {"extends", TokenType::Extends},
  {"false", TokenType::BooleanFalse},
  {"finally", TokenType::Finally},
  {"func", TokenType::Func},
  {"guard", TokenType::Guard},
  {"if", TokenType::If},
  {"ignoreconst", TokenType::IgnoreConst},
  {"let", TokenType::Let},
  {"loop", TokenType::Loop},
  {"match", TokenType::Match},
  {"null", TokenType::Null},
  {"primitive", TokenType::Primitive},
  {"property", TokenType::Property},
  {"return", TokenType::Return},
  {"self", TokenType::Self},
  {"static", TokenType::Static},
  {"switch", TokenType::Switch},
  {"throw", TokenType::Throw},
  {"true", TokenType::BooleanTrue},
  {"try", TokenType::Try},
  {"typeof", TokenType::Typeof},
  {"unless", TokenType::Unless},
  {"until", TokenType::Until},
  {"while", TokenType::While},
  {"yield", TokenType::Yield}
};
static std::unordered_map<TokenType, TokenType> kTokenAndAssignmentOperators = {

  // Regular arithmetic operators
  {TokenType::PlusAssignment, TokenType::Plus},
  {TokenType::MinusAssignment, TokenType::Minus},
  {TokenType::MulAssignment, TokenType::Mul},
  {TokenType::DivAssignment, TokenType::Div},
  {TokenType::ModAssignment, TokenType::Mod},
  {TokenType::PowAssignment, TokenType::Pow},

  // Binary operators
  {TokenType::BitANDAssignment, TokenType::BitAND},
  {TokenType::BitORAssignment, TokenType::BitOR},
  {TokenType::BitXORAssignment, TokenType::BitXOR},
  {TokenType::LeftShiftAssignment, TokenType::LeftShift},
  {TokenType::RightShiftAssignment, TokenType::RightShift}
};
// clang-format on

struct Token {
  TokenType type;
  std::string value;
  double numeric_value;

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
            this->type == TokenType::ModAssignment || this->type == TokenType::PowAssignment ||
            this->type == TokenType::BitANDAssignment || this->type == TokenType::BitORAssignment ||
            this->type == TokenType::BitXORAssignment || this->type == TokenType::LeftShiftAssignment ||
            this->type == TokenType::RightShiftAssignment);
  }

  inline bool could_start_expression() {
    return (this->type == TokenType::Number || this->type == TokenType::Identifier || this->type == TokenType::String ||
            this->type == TokenType::BooleanFalse || this->type == TokenType::BooleanTrue ||
            this->type == TokenType::Null || this->type == TokenType::Nan || this->type == TokenType::Self ||
            this->type == TokenType::Func || this->type == TokenType::Typeof || this->type == TokenType::Yield ||
            this->type == TokenType::Plus || this->type == TokenType::Minus || this->type == TokenType::BitNOT ||
            this->type == TokenType::Not || this->type == TokenType::LeftParen || this->type == TokenType::LeftCurly ||
            this->type == TokenType::LeftBracket || this->type == TokenType::AtSign ||
            this->type == TokenType::RightArrow || this->type == TokenType::Match);
  }

  template <class T>
  inline void write_to_stream(T&& stream) const {
    stream << kTokenTypeStrings[this->type] << " : ";

    if (this->type == TokenType::Number) {
      stream << this->numeric_value;
    } else {
      stream << this->value;
    }

    stream << ' ';
    this->location.write_to_stream(stream);
  }
};
}  // namespace Charly::Compilation
