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

#include <iostream>
#include <string>
#include <unordered_map>

#include "charly/core/compiler/location.h"

#pragma once

namespace charly::core::compiler {

enum class TokenType {
  Eof,

  // literals
  Int,
  Float,
  NaN,
  True,
  False,
  Identifier,
  String,
  FormatString,
  Null,
  Self,
  Super,

  // keywords
  As,
  Await,
  Break,
  Case,
  Catch,
  Class,
  Const,
  Continue,
  Default,
  Defer,
  Do,
  Else,
  Export,
  Extends,
  Finally,
  For,
  From,
  Func,
  Guard,
  If,
  Import,
  In,
  Let,
  Loop,
  Match,
  New,
  Operator,
  Property,
  Return,
  Spawn,
  Static,
  Switch,
  Throw,
  Try,
  Typeof,
  Unless,
  Until,
  While,
  Yield,

  // assignment
  Assignment,

  // binary operations
  Plus,
  Minus,
  Mul,
  Div,
  Mod,
  Pow,
  Equal,
  NotEqual,
  LessThan,
  GreaterThan,
  LessEqual,
  GreaterEqual,
  And,
  Or,
  BitOR,
  BitXOR,
  BitAND,
  BitLeftShift,
  BitRightShift,
  BitUnsignedRightShift,

  // unary operations
  UnaryNot,
  BitNOT,

  // structure
  LeftParen,
  RightParen,
  LeftCurly,
  RightCurly,
  LeftBracket,
  RightBracket,
  Point,
  DoublePoint,
  TriplePoint,
  Colon,
  Comma,
  Semicolon,
  AtSign,
  LeftArrow,
  RightArrow,
  RightThickArrow,
  QuestionMark,

  // misc
  Comment,
  Newline,
  Whitespace
};

// string representations of token types
static std::string kTokenTypeStrings[] = {
  "EOF",   "Int",      "Float",    "NaN",       "True",   "false",  "Identifier", "String",  "FormatString",
  "null",  "self",     "super",    "as",        "await",  "break",  "case",       "catch",   "class",
  "const", "continue", "default",  "defer",     "do",     "else",   "export",     "extends", "finally",
  "for",   "from",     "func",     "guard",     "if",     "import", "in",         "let",     "loop",
  "match", "new",      "operator", "property",  "return", "spawn",  "static",     "switch",  "throw",
  "try",   "typeof",   "unless",   "until",     "while",  "yield",  "=",          "+",       "-",
  "*",     "/",        "%",        "**",        "==",     "!=",     "<",          ">",       "<=",
  ">=",    "&&",       "||",       "|",         "^",      "&",      "<<",         ">>",      ">>>",
  "!",     "~",        "(",        ")",         "{",      "}",      "[",          "]",       ".",
  "..",    "...",      ":",        ",",         ";",      "@",      "<-",         "->",      "=>",
  "?",     "Comment",  "Newline",  "Whitespace"
};

// identifiers with these names get remapped to keyword tokens
static const std::unordered_map<std::string, TokenType> kKeywordsAndLiterals = {
  { "NaN", TokenType::Float },         { "false", TokenType::False },   { "null", TokenType::Null },
  { "self", TokenType::Self },         { "super", TokenType::Super },   { "true", TokenType::True },
  { "and", TokenType::And },           { "as", TokenType::As },         { "await", TokenType::Await },
  { "break", TokenType::Break },       { "case", TokenType::Case },     { "catch", TokenType::Catch },
  { "class", TokenType::Class },       { "const", TokenType::Const },   { "continue", TokenType::Continue },
  { "default", TokenType::Default },   { "defer", TokenType::Defer },   { "do", TokenType::Do },
  { "else", TokenType::Else },         { "export", TokenType::Export }, { "extends", TokenType::Extends },
  { "finally", TokenType::Finally },   { "for", TokenType::For },       { "from", TokenType::From },
  { "func", TokenType::Func },         { "guard", TokenType::Guard },   { "if", TokenType::If },
  { "import", TokenType::Import },     { "in", TokenType::In },         { "let", TokenType::Let },
  { "loop", TokenType::Loop },         { "match", TokenType::Match },   { "new", TokenType::New },
  { "operator", TokenType::Operator }, { "or", TokenType::Or },         { "property", TokenType::Property },
  { "return", TokenType::Return },     { "spawn", TokenType::Spawn },   { "static", TokenType::Static },
  { "switch", TokenType::Switch },     { "throw", TokenType::Throw },   { "try", TokenType::Try },
  { "typeof", TokenType::Typeof },     { "unless", TokenType::Unless }, { "until", TokenType::Until },
  { "while", TokenType::While },       { "yield", TokenType::Yield }
};

struct Token {
  TokenType type = TokenType::Eof;
  Location location;
  std::string source;

  union {
    TokenType assignment_operator;
    int64_t intval;
    double floatval;
  };

  bool is_binary_operator() const {
    return type == TokenType::Plus || type == TokenType::Minus || type == TokenType::Mul || type == TokenType::Div ||
           type == TokenType::Mod || type == TokenType::Pow || type == TokenType::BitAND || type == TokenType::BitOR ||
           type == TokenType::BitXOR || type == TokenType::BitLeftShift || type == TokenType::BitRightShift ||
           type == TokenType::BitUnsignedRightShift || type == TokenType::Or || type == TokenType::And ||
           type == TokenType::Equal || type == TokenType::NotEqual || type == TokenType::LessThan ||
           type == TokenType::GreaterThan || type == TokenType::LessEqual || type == TokenType::GreaterEqual ||
           type == TokenType::DoublePoint || type == TokenType::TriplePoint;
  }

  bool legal_assignment_operator() const {
    return type == TokenType::Plus || type == TokenType::Minus || type == TokenType::Mul || type == TokenType::Div ||
           type == TokenType::Mod || type == TokenType::Pow || type == TokenType::BitAND || type == TokenType::BitOR ||
           type == TokenType::BitXOR || type == TokenType::BitLeftShift || type == TokenType::BitRightShift ||
           type == TokenType::BitUnsignedRightShift;
  }

  friend std::ostream& operator<<(std::ostream& out, const Token& token) {
    out << '(';
    out << kTokenTypeStrings[static_cast<uint8_t>(token.type)];

    if (token.type == TokenType::Int || token.type == TokenType::Float || token.type == TokenType::Comment ||
        token.type == TokenType::String || token.type == TokenType::FormatString ||
        token.type == TokenType::Identifier) {
      out << ',';
      out << ' ';

      switch (token.type) {
        case TokenType::Int: out << token.intval; break;
        case TokenType::Float: out << token.floatval; break;

        case TokenType::Comment:
        case TokenType::Identifier: out << token.source; break;

        case TokenType::String:
        case TokenType::FormatString: out << '"' << token.source << '"'; break;
        default: break;
      }
    }

    out << ')';
    out << ' ';
    out << *token.location.filename;
    out << ':';
    out << token.location.row;
    out << ':';
    out << token.location.column;

    return out;
  }
};

}  // namespace charly::core::compiler
