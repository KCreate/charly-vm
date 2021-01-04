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

#include "charly/utils/map.h"
#include "charly/utils/string.h"
#include "charly/core/compiler/location.h"

#pragma once

namespace charly::core::compiler {

enum TokenType : uint8_t {
  Eof,

  // literals
  Int,
  Float,
  NaN,
  True,
  False,
  Identifier,
  String,
  StringPart,
  Null,
  Self,
  Super,

  // keywords
  LiteralAnd,
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
  Func,
  Guard,
  If,
  IgnoreConst,
  Import,
  In,
  Let,
  Loop,
  Match,
  Module,
  New,
  Operator,
  LiteralOr,
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
  "Eof",

  "Int",
  "Float",
  "NaN",
  "True",
  "False",
  "Identifier",
  "String",
  "StringPart",
  "Null",
  "Self",
  "Super",

  "LiteralAnd",
  "As",
  "Await",
  "Break",
  "Case",
  "Catch",
  "Class",
  "Const",
  "Continue",
  "Default",
  "Defer",
  "Do",
  "Else",
  "Export",
  "Extends",
  "Finally",
  "For",
  "Func",
  "Guard",
  "If",
  "IgnoreConst",
  "Import",
  "In",
  "Let",
  "Loop",
  "Match",
  "Module",
  "New",
  "Operator",
  "LiteralOr",
  "Property",
  "Return",
  "Spawn",
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
  "PlusAssignment",
  "MinusAssignment",
  "MulAssignment",
  "DivAssignment",
  "ModAssignment",
  "PowAssignment",
  "BitANDAssignment",
  "BitORAssignment",
  "BitXORAssignment",
  "BitLeftShiftAssignment",
  "BitRightShiftAssignment",
  "BitUnsignedRightShiftAssignment",

  "Equal",
  "NotEqual",
  "LessThan",
  "GreaterThan",
  "LessEqual",
  "GreaterEqual",
  "And",
  "Or",
  "UnaryNot",

  "BitOR",
  "BitXOR",
  "BitNOT",
  "BitAND",
  "BitLeftShift",
  "BitRightShift",
  "BitUnsignedRightShift",

  "LeftParen",
  "RightParen",
  "LeftCurly",
  "RightCurly",
  "LeftBracket",
  "RightBracket",
  "Point",
  "Colon",
  "Comma",
  "Semicolon",
  "AtSign",
  "LeftArrow",
  "RightArrow",
  "RightThickArrow",
  "QuestionMark",

  "Comment",
  "Newline",
  "Whitespace"
};

// identifiers with these names get remapped to keyword tokens
static const utils::unordered_map<utils::string, TokenType> kKeywordsAndLiterals = {
  {"false",       TokenType::False},
  {"NaN",         TokenType::NaN},
  {"null",        TokenType::Null},
  {"self",        TokenType::Self},
  {"super",       TokenType::Super},
  {"true",        TokenType::True},
  {"and",         TokenType::LiteralAnd},
  {"as",          TokenType::As},
  {"await",       TokenType::Await},
  {"break",       TokenType::Break},
  {"case",        TokenType::Case},
  {"catch",       TokenType::Catch},
  {"class",       TokenType::Class},
  {"const",       TokenType::Const},
  {"continue",    TokenType::Continue},
  {"default",     TokenType::Default},
  {"defer",       TokenType::Defer},
  {"do",          TokenType::Do},
  {"else",        TokenType::Else},
  {"export",      TokenType::Export},
  {"extends",     TokenType::Extends},
  {"finally",     TokenType::Finally},
  {"for",         TokenType::For},
  {"func",        TokenType::Func},
  {"guard",       TokenType::Guard},
  {"if",          TokenType::If},
  {"import",      TokenType::Import},
  {"in",          TokenType::In},
  {"let",         TokenType::Let},
  {"loop",        TokenType::Loop},
  {"match",       TokenType::Match},
  {"module",      TokenType::Module},
  {"new",         TokenType::New},
  {"operator",    TokenType::Operator},
  {"or",          TokenType::LiteralOr},
  {"property",    TokenType::Property},
  {"return",      TokenType::Return},
  {"spawn",       TokenType::Spawn},
  {"static",      TokenType::Static},
  {"switch",      TokenType::Switch},
  {"throw",       TokenType::Throw},
  {"try",         TokenType::Try},
  {"typeof",      TokenType::Typeof},
  {"unless",      TokenType::Unless},
  {"until",       TokenType::Until},
  {"while",       TokenType::While},
  {"yield",       TokenType::Yield}
};

// mapping from original binary operator to AND assignment token
static const utils::unordered_map<TokenType, TokenType> kANDAssignmentOperators = {
  {TokenType::Plus,                  TokenType::PlusAssignment},
  {TokenType::Minus,                 TokenType::MinusAssignment},
  {TokenType::Mul,                   TokenType::MulAssignment},
  {TokenType::Div,                   TokenType::DivAssignment},
  {TokenType::Mod,                   TokenType::ModAssignment},
  {TokenType::Pow,                   TokenType::PowAssignment},
  {TokenType::BitAND,                TokenType::BitANDAssignment},
  {TokenType::BitOR,                 TokenType::BitORAssignment},
  {TokenType::BitXOR,                TokenType::BitXORAssignment},
  {TokenType::BitLeftShift,          TokenType::BitLeftShiftAssignment},
  {TokenType::BitRightShift,         TokenType::BitRightShiftAssignment},
  {TokenType::BitUnsignedRightShift, TokenType::BitUnsignedRightShiftAssignment}
};

struct Token {
  TokenType     type = TokenType::Eof;
  Location      location;
  utils::string source;
  int64_t       intval;
  double        floatval;

  // checks wether this token is an operator that could be used in an AND
  // assignment
  //
  // foo += 25
  bool is_and_assignment_operator() {
    return kANDAssignmentOperators.count(type);
  }

  // write a formatted version of the token to the stream
  void dump(std::ostream& io) {
    io << '(';
    io << kTokenTypeStrings[this->type];

    if (this->type == TokenType::Int ||
        this->type == TokenType::Float ||
        this->type == TokenType::Comment ||
        this->type == TokenType::String ||
        this->type == TokenType::StringPart ||
        this->type == TokenType::Identifier) {
      io << ',';
      io << ' ';

      switch (this->type) {
        case TokenType::Int:          io << this->intval;   break;
        case TokenType::Float:        io << this->floatval; break;
        case TokenType::Comment:
        case TokenType::Identifier:   io << this->source; break;
        case TokenType::String:
        case TokenType::StringPart:   io << '"' << this->source << '"'; break;
      }
    }

    io << ')';
    io << ' ';
    io << *location.filename;
    io << ':';
    io << location.row;
    io << ':';
    io << location.column;
  }
};

}
