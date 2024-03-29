/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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
#include <unordered_set>

#include "charly/core/compiler/location.h"

#pragma once

namespace charly::core::compiler {

enum class TokenType {
  Eof,

  // literals
  Int,
  Float,
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
  Assert,
  Await,
  Break,
  Builtin,
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
  Final,
  Finally,
  For,
  From,
  Func,
  Guard,
  If,
  Import,
  In,
  InstanceOf,
  Let,
  Loop,
  Match,
  Private,
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
  Spaceship,
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
inline static const std::string kTokenTypeStrings[] = {
  "EOF",       "integer",  "float",   "true",  "false",      "identifier", "string",  "formatstring",
  "null",      "self",     "super",   "as",    "assert",     "await",      "break",   "__builtin",
  "case",      "catch",    "class",   "const", "continue",   "default",    "defer",   "do",
  "else",      "export",   "extends", "final", "finally",    "for",        "from",    "func",
  "guard",     "if",       "import",  "in",    "instanceof", "let",        "loop",    "match",
  "private",   "property", "return",  "spawn", "static",     "switch",     "throw",   "try",
  "typeof",    "unless",   "until",   "while", "yield",      "=",          "+",       "-",
  "*",         "/",        "%",       "**",    "==",         "!=",         "<",       ">",
  "<=",        ">=",       "<=>",     "&&",    "||",         "|",          "^",       "&",
  "<<",        ">>",       ">>>",     "!",     "~",          "(",          ")",       "{",
  "}",         "[",        "]",       ".",     "..",         "...",        ":",       ",",
  ";",         "@",        "<-",      "->",    "=>",         "?",          "comment", "newline",
  "whitespace"
};

// identifiers with these names get remapped to keyword tokens
inline static const std::unordered_map<std::string, TokenType> kKeywordsAndLiterals = { { "NaN", TokenType::Float },
                                                                                 { "NAN", TokenType::Float },
                                                                                 { "Infinity", TokenType::Float },
                                                                                 { "INFINITY", TokenType::Float },
                                                                                 { "false", TokenType::False },
                                                                                 { "null", TokenType::Null },
                                                                                 { "self", TokenType::Self },
                                                                                 { "super", TokenType::Super },
                                                                                 { "true", TokenType::True },
                                                                                 { "as", TokenType::As },
                                                                                 { "assert", TokenType::Assert },
                                                                                 { "await", TokenType::Await },
                                                                                 { "break", TokenType::Break },
                                                                                 { "__builtin", TokenType::Builtin },
                                                                                 { "case", TokenType::Case },
                                                                                 { "catch", TokenType::Catch },
                                                                                 { "class", TokenType::Class },
                                                                                 { "const", TokenType::Const },
                                                                                 { "continue", TokenType::Continue },
                                                                                 { "default", TokenType::Default },
                                                                                 { "defer", TokenType::Defer },
                                                                                 { "do", TokenType::Do },
                                                                                 { "else", TokenType::Else },
                                                                                 { "export", TokenType::Export },
                                                                                 { "extends", TokenType::Extends },
                                                                                 { "final", TokenType::Final },
                                                                                 { "finally", TokenType::Finally },
                                                                                 { "for", TokenType::For },
                                                                                 { "from", TokenType::From },
                                                                                 { "func", TokenType::Func },
                                                                                 { "guard", TokenType::Guard },
                                                                                 { "if", TokenType::If },
                                                                                 { "import", TokenType::Import },
                                                                                 { "in", TokenType::In },
                                                                                 { "instanceof",
                                                                                   TokenType::InstanceOf },
                                                                                 { "let", TokenType::Let },
                                                                                 { "loop", TokenType::Loop },
                                                                                 { "match", TokenType::Match },
                                                                                 { "private", TokenType::Private },
                                                                                 { "property", TokenType::Property },
                                                                                 { "return", TokenType::Return },
                                                                                 { "spawn", TokenType::Spawn },
                                                                                 { "static", TokenType::Static },
                                                                                 { "switch", TokenType::Switch },
                                                                                 { "throw", TokenType::Throw },
                                                                                 { "try", TokenType::Try },
                                                                                 { "typeof", TokenType::Typeof },
                                                                                 { "unless", TokenType::Unless },
                                                                                 { "until", TokenType::Until },
                                                                                 { "while", TokenType::While },
                                                                                 { "yield", TokenType::Yield } };

inline static const std::unordered_set<TokenType> kExpressionValidInitialTokens = {
  TokenType::Int,         TokenType::Float,        TokenType::True,      TokenType::False,     TokenType::Identifier,
  TokenType::String,      TokenType::FormatString, TokenType::Null,      TokenType::Self,      TokenType::Super,
  TokenType::Await,       TokenType::Class,        TokenType::Func,      TokenType::Import,    TokenType::Match,
  TokenType::Spawn,       TokenType::Typeof,       TokenType::Yield,     TokenType::Plus,      TokenType::Minus,
  TokenType::UnaryNot,    TokenType::BitNOT,       TokenType::LeftParen, TokenType::LeftCurly, TokenType::LeftBracket,
  TokenType::TriplePoint, TokenType::AtSign,       TokenType::RightArrow
};

inline static const std::unordered_set<TokenType> kBinaryOperatorTokens = {
  TokenType::Plus,      TokenType::Minus,        TokenType::Mul,           TokenType::Div,
  TokenType::Mod,       TokenType::Pow,          TokenType::BitAND,        TokenType::BitOR,
  TokenType::BitXOR,    TokenType::BitLeftShift, TokenType::BitRightShift, TokenType::BitUnsignedRightShift,
  TokenType::Or,        TokenType::And,          TokenType::Equal,         TokenType::NotEqual,
  TokenType::LessThan,  TokenType::GreaterThan,  TokenType::LessEqual,     TokenType::GreaterEqual,
  TokenType::Spaceship, TokenType::InstanceOf
};

inline static const std::unordered_set<TokenType> kBinaryComparisonTokens = { TokenType::Equal,     TokenType::NotEqual,
                                                                       TokenType::LessThan,  TokenType::GreaterThan,
                                                                       TokenType::LessEqual, TokenType::GreaterEqual,
                                                                       TokenType::Spaceship, TokenType::InstanceOf };

// Note: The TriplePoint token or spread operator (...) is not included in this list is it is
// being parsed in a special manner by the parser.
inline static const std::unordered_set<TokenType> kUnaryOperatorTokens = { TokenType::Plus, TokenType::Minus,
                                                                    TokenType::UnaryNot, TokenType::BitNOT };

inline static const std::unordered_set<TokenType> kAssignmentOperators = {
  TokenType::Plus,   TokenType::Minus,        TokenType::Mul,           TokenType::Div,
  TokenType::Mod,    TokenType::Pow,          TokenType::BitAND,        TokenType::BitOR,
  TokenType::BitXOR, TokenType::BitLeftShift, TokenType::BitRightShift, TokenType::BitUnsignedRightShift,
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
    return kBinaryOperatorTokens.count(type);
  }

  bool is_unary_operator() const {
    return kUnaryOperatorTokens.count(type);
  }

  bool legal_assignment_operator() const {
    return kAssignmentOperators.count(type);
  }

  bool could_start_expression() const {
    return kExpressionValidInitialTokens.count(type);
  }
};

}  // namespace charly::core::compiler
