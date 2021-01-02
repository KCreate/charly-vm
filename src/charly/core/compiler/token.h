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
  Int,
  Float,
  True,
  False,
  Identifier,
  Whitespace,
  Newline,
  Unknown,
  Eof
};

static utils::string kTokenTypeStrings[] = {
  "Int",
  "Float",
  "True",
  "False",
  "Identifier",
  "Whitespace",
  "Newline",
  "Unknown",
  "Eof"
};

static const utils::unordered_map<utils::string, TokenType> kKeywordsAndLiterals = {
  {"true",  TokenType::True},
  {"false", TokenType::False}
};

struct Token {
  TokenType     type = TokenType::Unknown;
  Location      location;
  utils::string source;
  int64_t       intval;
  double        floatval;

  // check wether this token is a whitespace / newline token
  bool is_whitespace() {
    return type == TokenType::Whitespace || type == TokenType::Newline;
  }

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
