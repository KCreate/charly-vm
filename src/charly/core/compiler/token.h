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
  False,
  Float,
  Identifier,
  Int,
  NaN,
  Null,
  Self,
  Super,
  True,

  // keywords
  And,
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
  Or,
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

  // misc
  Newline,
  Whitespace
};

static utils::string kTokenTypeStrings[] = {
  "Eof",

  "False",
  "Float",
  "Identifier",
  "Int",
  "NaN",
  "Null",
  "Self",
  "Super",
  "True",

  "And",
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
  "Or",
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

  "Newline",
  "Whitespace"
};

static const utils::unordered_map<utils::string, TokenType> kKeywordsAndLiterals = {
  {"false",       TokenType::False},
  {"NaN",         TokenType::NaN},
  {"null",        TokenType::Null},
  {"self",        TokenType::Self},
  {"super",       TokenType::Super},
  {"true",        TokenType::True},
  {"and",         TokenType::And},
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
  {"or",          TokenType::Or},
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

struct Token {
  TokenType     type = TokenType::Eof;
  Location      location;
  utils::string source;
  int64_t       intval;
  double        floatval;

  // write a formatted version of the token to the stream
  void dump(std::ostream& io) {
    io << '(';
    io << kTokenTypeStrings[this->type];

    if (this->type == TokenType::Int ||
        this->type == TokenType::Float ||
        this->type == TokenType::Identifier) {
      io << ',';
      io << ' ';

      switch (this->type) {
        case TokenType::Int:          io << this->intval;   break;
        case TokenType::Float:        io << this->floatval; break;
        case TokenType::Identifier:   io << this->source; break;
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
