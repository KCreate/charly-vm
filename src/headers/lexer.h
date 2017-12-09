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
#include <vector>

#include "location.h"
#include "sourcefile.h"
#include "token.h"

#pragma once

namespace Charly::Compiler {
class Lexer {
public:
  SourceFile& source;
  Token token;
  std::vector<Token> tokens;

public:
  Lexer(SourceFile& s) : source(s), token(TokenType::Unknown) {
  }

  void tokenize();
  void reset_token();
  void read_token();
  void consume_operator_or_assignment(TokenType type);
  void consume_whitespace();
  void consume_newline();
  void consume_numeric();
  void consume_string();
  void consume_comment();
  void consume_multiline_comment();
  void consume_ident();
  static bool is_ident_start(uint32_t cp);
  static bool is_ident_part(uint32_t cp);
  void consume_ident_or_keyword(TokenType type);
  void unexpected_char();
  void throw_error(Location loc, const std::string& message);
};

// An unexpected char was found during lexing of the source code
struct UnexpectedCharError {
  Location location;
  uint32_t cp;

  UnexpectedCharError(Location l, uint32_t cp) : location(l), cp(cp) {
  }
};

// A more general exception for any syntax error which might come up
struct SyntaxError {
  Location location;
  std::string message;

  SyntaxError(Location l, const std::string& m) : location(l), message(m) {
  }
  SyntaxError(Location l, std::string&& m) : location(l), message(std::move(m)) {
  }
};
};  // namespace Charly::Compiler
