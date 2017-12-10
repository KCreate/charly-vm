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

#include <sstream>

#include "lexer.h"

namespace Charly::Compiler {
void Lexer::tokenize() {
  while (this->token.type != TokenType::Eof) {
    this->read_token();
  }
}

void Lexer::reset_token() {
  this->token.type = TokenType::Unknown;
  this->token.value = "";
  this->token.location = Location(this->source.pos - 1, this->source.row, this->source.column, 0, this->source.filename);
}

void Lexer::read_token() {
  this->reset_token();

  switch (this->source.current_char) {
    case L'\0': {
      this->token.type = TokenType::Eof;
      break;
    }
    case L' ':
    case L'\t': {
      this->consume_whitespace();
      break;
    }
    case L'\r':
    case L'\n': {
      this->consume_newline();
      break;
    }
    case L';': {
      this->token.type = TokenType::Semicolon;
      this->source.read_char();
      break;
    }
    case L',': {
      this->token.type = TokenType::Comma;
      this->source.read_char();
      break;
    }
    case L'.': {
      this->token.type = TokenType::Point;
      this->source.read_char();
      break;
    }
    case L'+': {
      this->consume_operator_or_assignment(TokenType::Plus);
      break;
    }
    case L'-': {
      switch (this->source.peek_char()) {
        case L'>': {
          this->source.read_char();
          this->source.read_char();
          this->token.type = TokenType::RightArrow;
          break;
        }
        default: {
          this->consume_operator_or_assignment(TokenType::Minus);
          break;
        }
      }
      break;
    }
    default: {
      if (Lexer::is_ident_start(this->source.current_char)) {
        this->consume_ident();
      } else {
        this->unexpected_char();
      }
      break;
    }
  }

  this->token.location.length = this->source.frame.size() - 1;
  this->source.reset_frame();
  Buffer::write_cp_to_string(this->source.current_char, this->source.frame);
  this->tokens.push_back(this->token);
}

void Lexer::consume_operator_or_assignment(TokenType type) {
  if (this->source.read_char() == L'=') {
    switch (type) {
      case TokenType::Plus: {
        this->token.type = TokenType::PlusAssignment;
        this->source.read_char();
        break;
      }
      case TokenType::Minus: {
        this->token.type = TokenType::MinusAssignment;
        this->source.read_char();
        break;
      }
      case TokenType::Mul: {
        this->token.type = TokenType::MulAssignment;
        this->source.read_char();
        break;
      }
      case TokenType::Div: {
        this->token.type = TokenType::DivAssignment;
        this->source.read_char();
        break;
      }
      case TokenType::Mod: {
        this->token.type = TokenType::ModAssignment;
        this->source.read_char();
        break;
      }
      case TokenType::Pow: {
        this->token.type = TokenType::PowAssignment;
        this->source.read_char();
        break;
      }
      default: { break; }
    }
  } else {
    this->token.type = type;
  }
}

void Lexer::consume_whitespace() {
  this->token.type = TokenType::Whitespace;

  while (true) {
    uint32_t cp = this->source.read_char();

    if (cp == L' ' || cp == L'\t') {
      // Nothing to do
    } else {
      break;
    }
  }
}

void Lexer::consume_newline() {
  this->token.type = TokenType::Newline;

  while (true) {
    uint32_t cp = this->source.current_char;

    if (cp == L'\n') {
      this->source.read_char();
      break;
    } else if (cp == L'\r') {
      cp = this->source.read_char();

      if (cp == L'\n') {
        this->source.read_char();
        break;
      } else {
        this->unexpected_char();
      }
    } else {
      break;
    }
  }
}

void Lexer::consume_ident() {
  while (Lexer::is_ident_part(this->source.current_char)) {
    this->source.read_char();
  }

  this->token.type = TokenType::Identifier;
  this->token.value = this->source.get_current_frame();
}

bool Lexer::is_ident_start(uint32_t cp) {
  return (cp >= 0x41 && cp <= 0x5a) || (cp >= 0x61 && cp <= 0x7a) || cp == L'_' || cp == L'$';
}

bool Lexer::is_ident_part(uint32_t cp) {
  return Lexer::is_ident_start(cp) || (cp >= 0x30 && cp <= 0x39);
}

void Lexer::consume_ident_or_keyword(TokenType type) {
  uint32_t cp = this->source.read_char();

  if (Lexer::is_ident_part(cp)) {
    this->consume_ident();
  } else {
    this->token.type = type;
    this->token.value = this->source.get_current_frame();
  }
}

void Lexer::unexpected_char() {
  Location loc(this->source.pos - 1, this->source.row, this->source.column, 1, this->source.filename);
  throw UnexpectedCharError(loc, this->source.current_char);
}

void Lexer::throw_error(Location loc, const std::string& message) {
  throw SyntaxError(loc, message);
}
}  // namespace Charly::Compiler
