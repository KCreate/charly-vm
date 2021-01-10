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

#include "charly/core/compiler/location.h"
#include "charly/utils/map.h"
#include "charly/utils/string.h"

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
  AndLiteral,
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
  OrLiteral,
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

  // arithmetic operators
  Plus,
  Minus,
  Mul,
  Div,
  Mod,
  Pow,

  // assignment operators
  Assignment,
  PlusAssignment,
  MinusAssignment,
  MulAssignment,
  DivAssignment,
  ModAssignment,
  PowAssignment,
  BitANDAssignment,
  BitORAssignment,
  BitXORAssignment,
  BitLeftShiftAssignment,
  BitRightShiftAssignment,
  BitUnsignedRightShiftAssignment,

  // comparison operators
  Equal,
  NotEqual,
  LessThan,
  GreaterThan,
  LessEqual,
  GreaterEqual,
  And,
  Or,
  UnaryNot,

  // bitwise operators
  BitOR,
  BitXOR,
  BitNOT,
  BitAND,
  BitLeftShift,
  BitRightShift,
  BitUnsignedRightShift,

  // structure
  LeftParen,
  RightParen,
  LeftCurly,
  RightCurly,
  LeftBracket,
  RightBracket,
  Point,
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
static utils::string kTokenTypeStrings[] = {
  "EOF",     "Int",   "Float",    "NaN",      "True",    "false",    "Identifier", "String", "FormatString",
  "null",    "self",  "super",    "and",      "as",      "await",    "break",      "case",   "catch",
  "class",   "const", "continue", "default",  "defer",   "do",       "else",       "export", "extends",
  "finally", "for",   "from",     "func",     "guard",   "if",       "import",     "in",     "let",
  "loop",    "match", "new",      "operator", "or",      "property", "return",     "spawn",  "static",
  "switch",  "throw", "try",      "typeof",   "unless",  "until",    "while",      "yield",  "+",
  "-",       "*",     "/",        "%",        "**",      "=",        "+=",         "-=",     "*=",
  "/=",      "%=",    "**=",      "&=",       "|=",      "^=",       "<<=",        ">>=",    ">>>=",
  "==",      "!=",    "<",        ">",        "<=",      ">=",       "&&",         "||",     "!",
  "|",       "^",     "~",        "&",        "<<",      ">>",       ">>>",        "(",      ")",
  "{",       "}",     "[",        "]",        ".",       ":",        ",",          ";",      "@",
  "<-",      "->",    "=>",       "?",        "Comment", "Newline",  "Whitespace"
};

// identifiers with these names get remapped to keyword tokens
static const utils::unordered_map<utils::string, TokenType> kKeywordsAndLiterals = {
  { "NaN", TokenType::Float },         { "false", TokenType::False },   { "null", TokenType::Null },
  { "self", TokenType::Self },         { "super", TokenType::Super },   { "true", TokenType::True },
  { "and", TokenType::AndLiteral },    { "as", TokenType::As },         { "await", TokenType::Await },
  { "break", TokenType::Break },       { "case", TokenType::Case },     { "catch", TokenType::Catch },
  { "class", TokenType::Class },       { "const", TokenType::Const },   { "continue", TokenType::Continue },
  { "default", TokenType::Default },   { "defer", TokenType::Defer },   { "do", TokenType::Do },
  { "else", TokenType::Else },         { "export", TokenType::Export }, { "extends", TokenType::Extends },
  { "finally", TokenType::Finally },   { "for", TokenType::For },       { "from", TokenType::From },
  { "func", TokenType::Func },         { "guard", TokenType::Guard },   { "if", TokenType::If },
  { "import", TokenType::Import },     { "in", TokenType::In },         { "let", TokenType::Let },
  { "loop", TokenType::Loop },         { "match", TokenType::Match },   { "new", TokenType::New },
  { "operator", TokenType::Operator }, { "or", TokenType::OrLiteral },  { "property", TokenType::Property },
  { "return", TokenType::Return },     { "spawn", TokenType::Spawn },   { "static", TokenType::Static },
  { "switch", TokenType::Switch },     { "throw", TokenType::Throw },   { "try", TokenType::Try },
  { "typeof", TokenType::Typeof },     { "unless", TokenType::Unless }, { "until", TokenType::Until },
  { "while", TokenType::While },       { "yield", TokenType::Yield }
};

// mapping from original binary operator to AND assignment token
static const utils::unordered_map<TokenType, TokenType> kANDAssignmentOperators = {
  { TokenType::Plus, TokenType::PlusAssignment },
  { TokenType::Minus, TokenType::MinusAssignment },
  { TokenType::Mul, TokenType::MulAssignment },
  { TokenType::Div, TokenType::DivAssignment },
  { TokenType::Mod, TokenType::ModAssignment },
  { TokenType::Pow, TokenType::PowAssignment },
  { TokenType::BitAND, TokenType::BitANDAssignment },
  { TokenType::BitOR, TokenType::BitORAssignment },
  { TokenType::BitXOR, TokenType::BitXORAssignment },
  { TokenType::BitLeftShift, TokenType::BitLeftShiftAssignment },
  { TokenType::BitRightShift, TokenType::BitRightShiftAssignment },
  { TokenType::BitUnsignedRightShift, TokenType::BitUnsignedRightShiftAssignment }
};

struct Token {
  TokenType type = TokenType::Eof;
  Location location;
  utils::string source;

  // int / float token sources
  union {
    int64_t intval;
    double floatval;
  };

  // checks wether this token is an operator that could be used in an AND
  // assignment
  //
  // foo += 25
  bool is_and_assignment_operator() const {
    return kANDAssignmentOperators.count(type);
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
