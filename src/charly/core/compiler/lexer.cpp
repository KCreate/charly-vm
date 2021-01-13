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

#include <cstdlib>
#include <limits>
#include <memory>

#include "charly/utils/cast.h"

#include "charly/core/compiler/lexer.h"

namespace charly::core::compiler {

Token Lexer::read_token_all() {
  Token token;

  size_t offset_start = m_source.readoffset();

  token.location.filename = m_filename;
  token.location.offset = offset_start;
  token.location.length = 1;
  token.location.row = m_row;
  token.location.column = m_column;

  if (m_mode == Mode::String) {
    consume_string(token);
  } else
    switch (peek_char()) {
      // null byte means end of file
      case '\0': {
        read_char();
        token.type = TokenType::Eof;

        if (m_interpolation_bracket_stack.size())
          throw CompilerError("unfinished string interpolation", token.location);

        if (m_bracket_stack.size())
          throw CompilerError("unclosed bracket", token.location);

        break;
      }

      // binary, hex, octal literals
      case '0': {
        bool parse_decimal = false;

        switch (peek_char(1)) {
          case 'b': {
            read_char();
            read_char();
            m_source.reset_window();
            consume_binary(token);
            break;
          }
          case 'x': {
            read_char();
            read_char();
            m_source.reset_window();
            consume_hex(token);
            break;
          }
          case 'o': {
            read_char();
            read_char();
            m_source.reset_window();
            consume_octal(token);
            break;
          }
          default: {
            parse_decimal = true;
          }
        }

        if (!parse_decimal)
          break;
      }
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        consume_decimal(token);
        break;
      }
      case '\"': {
        read_char();
        consume_string(token);
        break;
      }
      case '\t':
      case '\r':
      case ' ': {
        consume_whitespace(token);
        break;
      }
      case '\n': {
        read_char();
        token.type = TokenType::Newline;
        increment_row();
        break;
      }
      case '+': {
        read_char();
        token.type = TokenType::Plus;
        break;
      }
      case '-': {
        read_char();

        if (peek_char() == '>') {
          read_char();
          token.type = TokenType::RightArrow;
          break;
        }

        token.type = TokenType::Minus;
        break;
      }
      case '*': {
        read_char();

        if (peek_char() == '*') {
          read_char();
          token.type = TokenType::Pow;
          break;
        }

        token.type = TokenType::Mul;
        break;
      }
      case '/': {
        read_char();

        switch (peek_char()) {
          case '/': {
            read_char();
            consume_comment(token);
            break;
          }
          case '*': {
            read_char();
            consume_multiline_comment(token);
            break;
          }
          default: {
            token.type = TokenType::Div;
            break;
          }
        }

        break;
      }
      case '%': {
        read_char();
        token.type = TokenType::Mod;
        break;
      }
      case '=': {
        read_char();

        switch (peek_char()) {
          case '=': {
            read_char();
            token.type = TokenType::Equal;
            break;
          }
          case '>': {
            read_char();
            token.type = TokenType::RightThickArrow;
            break;
          }
          default: {
            token.type = TokenType::Assignment;
            token.assignment_operator = TokenType::Assignment;
            break;
          }
        }

        break;
      }
      case '!': {
        read_char();

        if (peek_char() == '=') {
          read_char();
          token.type = TokenType::NotEqual;
          break;
        }

        token.type = TokenType::UnaryNot;
        break;
      }
      case '<': {
        read_char();

        switch (peek_char()) {
          case '=': {
            read_char();
            token.type = TokenType::LessEqual;
            break;
          }
          case '<': {
            read_char();
            token.type = TokenType::BitLeftShift;
            break;
          }
          case '-': {
            read_char();
            token.type = TokenType::LeftArrow;
            break;
          }
          default: {
            token.type = TokenType::LessThan;
            break;
          }
        }

        break;
      }
      case '>': {
        read_char();

        switch (peek_char()) {
          case '=': {
            read_char();
            token.type = TokenType::GreaterEqual;
            break;
          }
          case '>': {
            read_char();

            if (peek_char() == '>') {
              read_char();
              token.type = TokenType::BitUnsignedRightShift;
              break;
            }

            token.type = TokenType::BitRightShift;
            break;
          }
          default: {
            token.type = TokenType::GreaterThan;
            break;
          }
        }

        break;
      }
      case '&': {
        read_char();

        if (peek_char() == '&') {
          read_char();
          token.type = TokenType::And;
          break;
        }

        token.type = TokenType::BitAND;
        break;
      }
      case '|': {
        read_char();

        if (peek_char() == '|') {
          read_char();
          token.type = TokenType::Or;
          break;
        }

        token.type = TokenType::BitOR;
        break;
      }
      case '^': {
        read_char();
        token.type = TokenType::BitXOR;
        break;
      }
      case '~': {
        read_char();
        token.type = TokenType::BitNOT;
        break;
      }
      case '(': {
        read_char();
        token.type = TokenType::LeftParen;
        m_bracket_stack.push_back(TokenType::RightParen);
        break;
      }
      case ')': {
        read_char();
        token.type = TokenType::RightParen;

        // check matching brace
        if (m_bracket_stack.size() == 0 || m_bracket_stack.back() != token.type) {
          throw CompilerError("unexpected )", token.location);
        }

        m_bracket_stack.pop_back();

        break;
      }
      case '{': {
        read_char();
        token.type = TokenType::LeftCurly;
        m_bracket_stack.push_back(TokenType::RightCurly);
        break;
      }
      case '}': {
        read_char();
        token.type = TokenType::RightCurly;

        if (m_bracket_stack.size() == 0 || m_bracket_stack.back() != token.type) {
          throw CompilerError("unexpected }", token.location);
        }

        // switch lexer mode
        if (m_interpolation_bracket_stack.size() && m_interpolation_bracket_stack.back() == m_bracket_stack.size()) {
          m_interpolation_bracket_stack.pop_back();
          m_mode = Mode::String;
        }

        m_bracket_stack.pop_back();

        break;
      }
      case '[': {
        read_char();
        token.type = TokenType::LeftBracket;
        m_bracket_stack.push_back(TokenType::RightBracket);
        break;
      }
      case ']': {
        read_char();
        token.type = TokenType::RightBracket;

        if (m_bracket_stack.size() == 0 || m_bracket_stack.back() != token.type) {
          throw CompilerError("unexpected ]", token.location);
        }

        m_bracket_stack.pop_back();

        break;
      }
      case '.': {
        read_char();
        token.type = TokenType::Point;
        break;
      }
      case ':': {
        read_char();
        token.type = TokenType::Colon;
        break;
      }
      case ',': {
        read_char();
        token.type = TokenType::Comma;
        break;
      }
      case ';': {
        read_char();
        token.type = TokenType::Semicolon;
        break;
      }
      case '@': {
        read_char();
        token.type = TokenType::AtSign;
        break;
      }
      case '?': {
        read_char();
        token.type = TokenType::QuestionMark;
        break;
      }
      default: {
        // parse identifiers
        if (is_id_begin(peek_char())) {
          consume_identifier(token);
          break;
        }

        throw CompilerError("unexpected character", token.location);
      }
    }

  // and assignment operators
  if (token.is_operator() && peek_char() == '=') {
    read_char();
    token.assignment_operator = token.type;
    token.type = TokenType::Assignment;
  }

  if (!(token.type == TokenType::String || token.type == TokenType::FormatString))
    token.source = m_source.window_string();

  token.location.length = m_source.window_size();
  m_source.reset_window();

  if (token.type != TokenType::Newline) {
    increment_column(m_source.readoffset() - offset_start);
  }

  if (kKeywordsAndLiterals.count(token.source)) {
    token.type = kKeywordsAndLiterals.at(token.source);

    if (token.type == TokenType::Float) {
      token.floatval = std::numeric_limits<double>::quiet_NaN();
    }
  }

  m_tokens.push_back(token);

  return token;
}

Token Lexer::read_token() {
  Token token;

  for (;;) {
    token = read_token_all();

    if (token.type == TokenType::Whitespace || token.type == TokenType::Newline || token.type == TokenType::Comment) {
      continue;
    }

    break;
  }

  return token;
}

Token Lexer::last_token() {
  if (m_tokens.size()) {
    return m_tokens.back();
  }

  return Token();
}

void Lexer::increment_row() {
  m_row++;
  m_column = 1;
}

void Lexer::increment_column(size_t delta) {
  m_column += delta;
}

uint32_t Lexer::peek_char(uint32_t nth) const {
  return m_source.peek_utf8(nth);
}

uint32_t Lexer::read_char() {
  uint32_t cp = m_source.read_utf8();
  m_last_character = cp;
  return cp;
}

uint32_t Lexer::last_char() const {
  return m_last_character;
}

bool Lexer::is_whitespace(uint32_t cp) const {
  return (cp == ' ' || cp == '\r' || cp == '\t');
}

bool Lexer::is_decimal(uint32_t cp) const {
  return (cp >= '0' && cp <= '9');
}

bool Lexer::is_hex(uint32_t cp) const {
  return is_decimal(cp) || (cp >= 'a' && cp <= 'f') || (cp >= 'A' && cp <= 'F');
}

bool Lexer::is_binary(uint32_t cp) const {
  return (cp == '0' || cp == '1');
}

bool Lexer::is_octal(uint32_t cp) const {
  return (cp >= '0' && cp <= '7');
}

bool Lexer::is_alpha_lower(uint32_t cp) const {
  return (cp >= 'a' && cp <= 'z');
}

bool Lexer::is_alpha_upper(uint32_t cp) const {
  return (cp >= 'A' && cp <= 'Z');
}

bool Lexer::is_alpha(uint32_t cp) const {
  return is_alpha_lower(cp) || is_alpha_upper(cp);
}

bool Lexer::is_alphanumeric(uint32_t cp) const {
  return is_alpha(cp) || is_decimal(cp);
}

bool Lexer::is_id_begin(uint32_t cp) const {
  return is_alpha(cp) || cp == '$' || cp == '_';
}

bool Lexer::is_id_part(uint32_t cp) const {
  return is_id_begin(cp) || is_decimal(cp);
}

void Lexer::consume_whitespace(Token& token) {
  token.type = TokenType::Whitespace;

  while (is_whitespace(peek_char())) {
    read_char();
  }
}

void Lexer::consume_decimal(Token& token) {
  token.type = TokenType::Int;

  bool point_passed = false;

  for (;;) {
    uint32_t cp = peek_char();

    // floating point dot
    if (cp == '.') {
      if (point_passed)
        break;

      // only parse the dot as a number if it
      // is followed by more decimal digits
      if (is_decimal(peek_char(1))) {
        point_passed = true;
        read_char();
        continue;
      }

      break;
    }

    if (is_decimal(cp)) {
      read_char();
      continue;
    }

    break;
  }

  if (point_passed) {
    token.type = TokenType::Float;
    token.floatval = utils::charptr_to_double(m_source.window(), m_source.window_size());
  } else {
    token.type = TokenType::Int;
    token.intval = utils::charptr_to_int(m_source.window(), m_source.window_size(), 10);
  }
}

void Lexer::consume_hex(Token& token) {
  token.type = TokenType::Int;

  // there has to be at least one hex character
  if (!is_hex(peek_char())) {
    throw CompilerError("hex number literal expected at least one digit", token.location);
  }

  while (is_hex(peek_char())) {
    read_char();
  }

  token.intval = utils::charptr_to_int(m_source.window(), m_source.window_size(), 16);
}

void Lexer::consume_octal(Token& token) {
  token.type = TokenType::Int;

  // there has to be at least one hex character
  if (!is_octal(peek_char())) {
    throw CompilerError("octal number literal expected at least one digit", token.location);
  }

  while (is_octal(peek_char())) {
    read_char();
  }

  token.intval = utils::charptr_to_int(m_source.window(), m_source.window_size(), 8);
}

void Lexer::consume_binary(Token& token) {
  token.type = TokenType::Int;

  // there has to be at least one hex character
  if (!is_binary(peek_char())) {
    throw CompilerError("binary number literal expected at least one digit", token.location);
  }

  while (is_binary(peek_char())) {
    read_char();
  }

  token.intval = utils::charptr_to_int(m_source.window(), m_source.window_size(), 2);
}

void Lexer::consume_identifier(Token& token) {
  token.type = TokenType::Identifier;

  while (is_id_part(peek_char())) {
    read_char();
  }
}

void Lexer::consume_comment(Token& token) {
  token.type = TokenType::Comment;

  for (;;) {
    uint32_t cp = peek_char();

    if (cp == '\n' || cp == '\0')
      break;

    read_char();
  }
}

void Lexer::consume_multiline_comment(Token& token) {
  token.type = TokenType::Comment;

  uint32_t comment_depth = 1;

  while (comment_depth > 0) {
    uint32_t cp = peek_char();

    switch (cp) {
      case '\0': {
        throw CompilerError("unclosed comment", token.location);
      }
      case '/': {
        read_char();

        // increment comment depth if a new multiline comment is being started
        if (peek_char() == '*') {
          read_char();
          comment_depth += 1;
          continue;
        }

        break;
      }
      case '*': {
        read_char();

        // decrement comment depth if a multiline comment is being closed
        if (peek_char() == '/') {
          read_char();
          comment_depth -= 1;
          continue;
        }

        break;
      }
      default: {
        read_char();
      }
    }
  }
}

void Lexer::consume_string(Token& token) {
  token.type = TokenType::String;
  m_mode = Mode::String;

  // string is built inside this buffer
  utils::Buffer string_buf;

  // consume string component
  for (;;) {
    uint32_t cp = peek_char();

    // end of string
    if (cp == '"') {
      read_char();

      if (m_interpolation_bracket_stack.size()) {
        m_mode = Mode::InterpolatedExpression;
      } else {
        m_mode = Mode::TopLevel;
      }

      break;
    }

    // end of file reached, unclosed string detected
    if (cp == '\0') {
      throw CompilerError("unclosed string", token.location);
    }

    // string interpolation
    if (cp == '{') {
      read_char();

      // switch lexer mode
      m_mode = Mode::InterpolatedExpression;
      m_bracket_stack.push_back(TokenType::RightCurly);
      m_interpolation_bracket_stack.push_back(m_bracket_stack.size());

      token.type = TokenType::FormatString;

      break;
    }

    // escape sequences
    if (cp == '\\') {
      read_char();
      switch (peek_char()) {
        case 'a': {
          read_char();
          string_buf.append_utf8('\a');
          break;
        }
        case 'b': {
          read_char();
          string_buf.append_utf8('\b');
          break;
        }
        case 'n': {
          read_char();
          string_buf.append_utf8('\n');
          break;
        }
        case 'r': {
          read_char();
          string_buf.append_utf8('\r');
          break;
        }
        case 't': {
          read_char();
          string_buf.append_utf8('\t');
          break;
        }
        case 'v': {
          read_char();
          string_buf.append_utf8('\v');
          break;
        }
        case 'f': {
          read_char();
          string_buf.append_utf8('\f');
          break;
        }
        case '"': {
          read_char();
          string_buf.append_utf8('\"');
          break;
        }
        case '{': {
          read_char();
          string_buf.append_utf8('{');
          break;
        }
        case '\\': {
          read_char();
          string_buf.append_utf8('\\');
          break;
        }
        case '\0': throw CompilerError("unfinished escape sequence", token.location);
        default: throw CompilerError("unknown escape sequence", token.location);
      }
    }

    string_buf.append_utf8(read_char());
  }

  token.source = string_buf.buffer_string();

  return;
}

}  // namespace charly::core::compiler
