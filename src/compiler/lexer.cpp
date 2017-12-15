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

#include <algorithm>
#include <iterator>
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
  this->token.location =
      Location(this->source.pos - 1, this->source.row, this->source.column, 0, this->source.filename);
}

void Lexer::read_token() {
  this->reset_token();

  switch (this->source.current_char) {
    case L'\0': {
      this->token.type = TokenType::Eof;
      break;
    }
    case L'0':
    case L'1':
    case L'2':
    case L'3':
    case L'4':
    case L'5':
    case L'6':
    case L'7':
    case L'8':
    case L'9': {
      this->consume_numeric();
      break;
    }
    case L'"': {
      this->consume_string();
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
    case L'*': {
      switch (this->source.peek_char()) {
        case L'*': {
          this->source.read_char();
          this->consume_operator_or_assignment(TokenType::Pow);
          break;
        }
        default: {
          this->consume_operator_or_assignment(TokenType::Mul);
          break;
        }
      }

      break;
    }
    case L'/': {
      switch (this->source.peek_char()) {
        case L'/': {
          this->source.read_char();
          this->consume_comment();
          break;
        }
        case L'*': {
          this->source.read_char();
          this->consume_multiline_comment();
          break;
        }
        default: {
          this->consume_operator_or_assignment(TokenType::Div);
          break;
        }
      }

      break;
    }
    case L'%': {
      this->consume_operator_or_assignment(TokenType::Mod);
      break;
    }
    case L'=': {
      switch (this->source.read_char()) {
        case L'=': {
          this->source.read_char();
          this->token.type = TokenType::Equal;
          break;
        }
        default: {
          this->token.type = TokenType::Assignment;
          break;
        }
      }
      break;
    }
    case L'&': {
      switch (this->source.read_char()) {
        case L'&': {
          this->source.read_char();
          this->token.type = TokenType::AND;
          break;
        }
        default: {
          this->token.type = TokenType::BitAND;
          break;
        }
      }
      break;
    }
    case L'|': {
      switch (this->source.read_char()) {
        case L'|': {
          this->source.read_char();
          this->token.type = TokenType::OR;
          break;
        }
        default: {
          this->token.type = TokenType::BitOR;
          break;
        }
      }
      break;
    }
    case L'^': {
      this->token.type = TokenType::BitXOR;
      this->source.read_char();
      break;
    }
    case L'~': {
      this->token.type = TokenType::BitNOT;
      this->source.read_char();
      break;
    }
    case L'!': {
      this->token.type = TokenType::Not;
      this->source.read_char();
      break;
    }
    case L'<': {
      switch (this->source.read_char()) {
        case L'=': {
          this->source.read_char();
          this->token.type = TokenType::LessEqual;
          break;
        }
        case L'-': {
          this->source.read_char();
          this->token.type = TokenType::LeftArrow;
          break;
        }
        case L'<': {
          this->source.read_char();
          this->token.type = TokenType::LeftShift;
          break;
        }
        default: {
          this->token.type = TokenType::Less;
          break;
        }
      }
      break;
    }
    case L'>': {
      switch (this->source.read_char()) {
        case L'=': {
          this->source.read_char();
          this->token.type = TokenType::GreaterEqual;
          break;
        }
        case L'>': {
          this->source.read_char();
          this->token.type = TokenType::RightShift;
          break;
        }
        default: {
          this->token.type = TokenType::Greater;
          break;
        }
      }
      break;
    }
    case L'(': {
      this->source.read_char();
      this->token.type = TokenType::LeftParen;
      break;
    }
    case L')': {
      this->source.read_char();
      this->token.type = TokenType::RightParen;
      break;
    }
    case L'{': {
      this->source.read_char();
      this->token.type = TokenType::LeftCurly;
      break;
    }
    case L'}': {
      this->source.read_char();
      this->token.type = TokenType::RightCurly;
      break;
    }
    case L'[': {
      this->source.read_char();
      this->token.type = TokenType::LeftBracket;
      break;
    }
    case L']': {
      this->source.read_char();
      this->token.type = TokenType::RightBracket;
      break;
    }
    case L'@': {
      if (this->source.peek_char() == L'"') {
        this->source.read_char();
        this->consume_string();
        this->token.type = TokenType::Identifier;
      } else {
        this->source.read_char();
        this->token.type = TokenType::AtSign;
      }

      break;
    }
    case L'?': {
      this->source.read_char();
      this->token.type = TokenType::QuestionMark;
      break;
    }
    case L':': {
      this->source.read_char();
      this->token.type = TokenType::Colon;
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

  // Change the type of keyword tokens
  if (this->token.type == TokenType::Identifier) {
    auto search = kTokenKeywordsAndLiterals.find(this->token.value);

    if (search != kTokenKeywordsAndLiterals.end()) {
      this->token.type = search->second;
      this->token.value = "";
    }
  }

  // Ignore tokens which are not relevant for parsing
  if (this->token.type != TokenType::Comment && this->token.type != TokenType::Newline &&
      this->token.type != TokenType::Whitespace) {
    this->tokens.push_back(this->token);
  } else {
    this->read_token();
  }
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

void Lexer::consume_numeric() {
  // Check the number prefix
  if (this->source.current_char == L'0') {
    uint32_t cp = this->source.peek_char();

    if (cp == L'x') {
      this->source.read_char();
      this->source.read_char();
      this->consume_hex();
    } else if (Lexer::is_numeric(cp)) {
      this->consume_octal();
    } else {
      this->consume_decimal();
    }

    return;
  }

  this->consume_decimal();
}

void Lexer::consume_decimal() {
  bool point_passed = false;

  std::stringstream decoder;
  decoder << std::dec;
  decoder << static_cast<char>(this->source.current_char);

  while (true) {
    uint32_t cp = this->source.read_char();

    // Skip underscores
    if (cp == L'_')
      continue;

    // Parse float point
    if (cp == L'.') {
      if (point_passed) {
        break;
      }

      if (Lexer::is_numeric(this->source.peek_char())) {
        point_passed = true;
        decoder << '.';
      } else {
        break;
      }

      continue;
    }

    if (Lexer::is_numeric(cp)) {
      char numchar = static_cast<char>(cp);
      decoder << numchar;
    } else {
      break;
    }
  }

  if (point_passed) {
    double num = 0;
    decoder >> num;

    this->token.type = TokenType::Float;
    this->token.numeric_value.dbl_value = num;
  } else {
    int64_t num = 0;
    decoder >> num;

    this->token.type = TokenType::Integer;
    this->token.numeric_value.i64_value = num;
  }
}

void Lexer::consume_hex() {
  this->token.type = TokenType::Integer;

  std::stringstream decoder;
  decoder << std::hex;

  // There has to be at least one hex character
  if (!Lexer::is_hex(this->source.current_char)) {
    this->unexpected_char();
  }

  while (true) {
    uint32_t cp = this->source.current_char;

    if (Lexer::is_hex(cp)) {
      char numchar = static_cast<char>(cp);
      decoder << numchar;
      this->source.read_char();
    } else {
      break;
    }
  }

  int64_t num = 0;
  decoder >> num;

  this->token.numeric_value.i64_value = num;
}

void Lexer::consume_octal() {
  this->token.type = TokenType::Integer;

  std::stringstream decoder;
  decoder << std::oct;

  // There has to be at least one hex character
  if (!Lexer::is_octal(this->source.current_char)) {
    this->unexpected_char();
  }

  while (true) {
    uint32_t cp = this->source.current_char;

    if (Lexer::is_octal(cp)) {
      char numchar = static_cast<char>(cp);
      decoder << numchar;
      this->source.read_char();
    } else {
      break;
    }
  }

  int64_t num = 0;
  decoder >> num;

  this->token.numeric_value.i64_value = num;
}

void Lexer::consume_string() {
  this->token.type = TokenType::String;
  std::stringstream strbuff;

  bool keep_parsing = true;
  while (keep_parsing) {
    switch (this->source.read_char()) {
      case L'\\': {
        switch (this->source.read_char()) {
          case L'a': {
            strbuff << '\a';
            break;
          }
          case L'b': {
            strbuff << '\b';
            break;
          }
          case L'n': {
            strbuff << '\n';
            break;
          }
          case L'r': {
            strbuff << '\r';
            break;
          }
          case L't': {
            strbuff << '\t';
            break;
          }
          case L'v': {
            strbuff << '\v';
            break;
          }
          case L'f': {
            strbuff << '\f';
            break;
          }
          case L'e': {
            strbuff << '\e';
            break;
          }
          case L'"': {
            strbuff << '"';
            break;
          }
          case L'\\': {
            strbuff << '\\';
            break;
          }
          case L'\0': {
            this->unexpected_char();
            break;
          }
        }
        break;
      }
      case L'"': {
        keep_parsing = false;
        break;
      }
      case L'\0': {
        this->unexpected_char();
      }
      default: {
        Buffer::write_cp_to_stream(this->source.current_char, strbuff);
        break;
      }
    }
  }

  this->token.value = strbuff.str();
  this->source.read_char();
}

void Lexer::consume_comment() {
  this->token.type = TokenType::Comment;

  while (true) {
    uint32_t cp = this->source.current_char;

    if (cp == L'\n' || cp == L'\r') {
      break;
    } else {
      this->source.read_char();
    }
  }

  this->token.value = this->source.get_current_frame();
}

void Lexer::consume_multiline_comment() {
  this->token.type = TokenType::Comment;

  while (true) {
    uint32_t cp = this->source.current_char;

    if (cp == L'*') {
      cp = this->source.read_char();

      if (cp == L'/') {
        this->source.read_char();
        break;
      } else {
        // Do nothing and keep consuming comment
      }
    } else {
      this->source.read_char();
    }
  }

  this->token.value = this->source.get_current_frame();
}

void Lexer::consume_ident() {
  while (Lexer::is_ident_part(this->source.current_char)) {
    this->source.read_char();
  }

  this->token.type = TokenType::Identifier;
  this->token.value = this->source.get_current_frame();
}

bool Lexer::is_ident_start(uint32_t cp) {
  return Lexer::is_alpha(cp) || cp == L'_' || cp == L'$';
}

bool Lexer::is_ident_part(uint32_t cp) {
  return Lexer::is_ident_start(cp) || Lexer::is_numeric(cp);
}

bool Lexer::is_alpha(uint32_t cp) {
  return Lexer::is_alpha_lowercase(cp) || Lexer::is_alpha_uppercase(cp);
}

bool Lexer::is_alpha_lowercase(uint32_t cp) {
  return (cp >= 0x41 && cp <= 0x5a);
}

bool Lexer::is_alpha_uppercase(uint32_t cp) {
  return (cp >= 0x61 && cp <= 0x7a);
}

bool Lexer::is_numeric(uint32_t cp) {
  return (cp >= 0x30 && cp <= 0x39);
}

bool Lexer::is_hex(uint32_t cp) {
  return Lexer::is_numeric(cp) || (cp >= 0x41 && cp <= 0x46) || (cp >= 0x61 && cp <= 0x66);
}

bool Lexer::is_octal(uint32_t cp) {
  return (cp >= 0x30 && cp <= 0x37);
}

void Lexer::unexpected_char() {
  Location loc(this->source.pos - 1, this->source.row, this->source.column, 1, this->source.filename);
  throw UnexpectedCharError(loc, this->source.current_char);
}
}  // namespace Charly::Compiler
