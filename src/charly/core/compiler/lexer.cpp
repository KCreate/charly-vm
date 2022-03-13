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

#include "charly/utf8.h"

#include "charly/utils/cast.h"

#include "charly/core/compiler/lexer.h"

namespace charly::core::compiler {

Token Lexer::read_token_all() {
  Token& token = m_token;

  reset_token();

  if (m_mode == Mode::String) {
    consume_string(token);
  } else
    switch (peek_char()) {
      case -1: {
        read_char();
        token.type = TokenType::Eof;

        if (!m_interpolation_bracket_stack.empty())
          unexpected_character("unclosed string interpolation");

        if (!m_bracket_stack.empty())
          unexpected_character(m_bracket_stack.back());

        break;
      }

      // binary, hex, octal literals
      case '0': {
        bool parse_decimal = false;

        switch (peek_char(1)) {
          case 'b': {
            read_char();
            read_char();
            consume_binary(token);
            break;
          }
          case 'x': {
            read_char();
            read_char();
            consume_hex(token);
            break;
          }
          case 'o': {
            read_char();
            read_char();
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
      case '\'': {
        read_char();
        consume_char(token);
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

        if (m_bracket_stack.empty()) {
          unexpected_character();
        }

        if (m_bracket_stack.back() != token.type) {
          unexpected_character(m_bracket_stack.back());
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

        if (m_bracket_stack.empty()) {
          unexpected_character();
        }

        if (m_bracket_stack.back() != token.type) {
          unexpected_character(m_bracket_stack.back());
        }

        // switch lexer mode
        if (!m_interpolation_bracket_stack.empty() && m_interpolation_bracket_stack.back() == m_bracket_stack.size()) {
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

        if (m_bracket_stack.empty()) {
          unexpected_character();
        }

        if (m_bracket_stack.back() != token.type) {
          unexpected_character(m_bracket_stack.back());
        }

        m_bracket_stack.pop_back();

        break;
      }
      case '.': {
        read_char();

        if (peek_char() == '.') {
          read_char();

          if (peek_char() == '.') {
            read_char();
            token.type = TokenType::TriplePoint;
            break;
          }

          token.type = TokenType::DoublePoint;
          break;
        }

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

        if (peek_char() == '\"') {
          read_char();
          consume_string(token, false);
          token.type = TokenType::Identifier;
          break;
        }
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

        read_char();
        unexpected_character();
      }
    }

  // operator assignment
  if (token.legal_assignment_operator() && peek_char() == '=') {
    read_char();
    token.assignment_operator = token.type;
    token.type = TokenType::Assignment;
  }

  // the token.source value is set by the consume_* methods
  // of the above tokens, so we do nothing here
  switch (token.type) {
    case TokenType::String:
    case TokenType::FormatString:
    case TokenType::Identifier: {
      break;
    }
    default: {
      token.source = m_source.window_str();
      break;
    }
  }

  token.location.end_column = m_column;

  // remove the '{' from the stored length
  if (token.type == TokenType::FormatString) {
    token.location.end_column--;
  }

  if (token.type == TokenType::Identifier && kKeywordsAndLiterals.count(token.source)) {
    token.type = kKeywordsAndLiterals.at(token.source);

    if (token.type == TokenType::Float) {
      if (token.source == "INFINITY" || token.source == "Infinity") {
        token.floatval = INFINITY;
      } else if (token.source == "NAN" || token.source == "NaN") {
        token.floatval = NAN;
      } else {
        FAIL("unexpected value");
      }
    }
  }

  m_tokens.push_back(token);

  return token;
}

Token Lexer::read_token() {
  Token token;

  for (;;) {
    token = read_token_all();

    if (token.type != TokenType::Whitespace && token.type != TokenType::Newline && token.type != TokenType::Comment) {
      break;
    }
  }

  return token;
}

void Lexer::unexpected_character() {
  utils::Buffer formatbuf;

  switch (m_last_character) {
    case -1: {
      m_console.fatal(m_token.location, "unexpected end of file");
    }
    default: {
      m_token.location.offset = m_source.read_offset() - utf8::sequence_length(m_last_character);
      m_token.location.end_offset = m_source.read_offset();
      m_token.location.end_row = m_row;
      m_token.location.end_column = m_column;

      m_console.fatal(m_token.location, "unexpected '", utf8::codepoint_to_string(m_last_character), "'");
    }
  }
}

void Lexer::unexpected_character(uint32_t expected) {
  utils::Buffer formatbuf;

  switch (m_last_character) {
    case -1: {
      m_console.fatal(m_token.location, "unexpected end of file, expected the character '", expected, "'");
    }
    default: {
      m_token.location.offset = m_source.read_offset() - utf8::sequence_length(m_last_character);
      m_token.location.end_offset = m_source.read_offset();
      m_token.location.end_row = m_row;
      m_token.location.end_column = m_column;

      m_console.fatal(m_token.location, "unexpected, '", utf8::codepoint_to_string(m_last_character),
                      "' expected the character '", utf8::codepoint_to_string(expected), "'");
    }
  }
}

void Lexer::unexpected_character(TokenType expected) {
  utils::Buffer formatbuf;

  switch (m_last_character) {
    case -1: {
      m_console.fatal(m_token.location, "unexpected end of file, expected a '",
                      kTokenTypeStrings[static_cast<int>(expected)], "' token");
    }
    default: {
      m_token.location.offset = m_source.read_offset() - utf8::sequence_length(m_last_character);
      m_token.location.end_offset = m_source.read_offset();
      m_token.location.end_row = m_row;
      m_token.location.end_column = m_column;

      m_console.fatal(m_token.location, "unexpected '", utf8::codepoint_to_string(m_last_character), "', expected a '",
                      kTokenTypeStrings[static_cast<int>(expected)], "' token");
    }
  }
}

void Lexer::unexpected_character(const std::string& message) {
  utils::Buffer formatbuf;

  switch (m_last_character) {
    case -1: {
      m_console.fatal(m_token.location, "unexpected end of file, ", message);
    }
    default: {
      m_token.location.offset = m_source.read_offset() - utf8::sequence_length(m_last_character);
      m_token.location.end_offset = m_source.read_offset();
      m_token.location.end_row = m_row;
      m_token.location.end_column = m_column;

      m_console.fatal(m_token.location, "unexpected '", utf8::codepoint_to_string(m_last_character), "', ", message);
    }
  }
}

void Lexer::increment_row() {
  m_row++;
  m_column = 0;
  m_token.location.end_row++;
  m_token.location.end_column = 0;
}

void Lexer::increment_column(size_t delta) {
  m_column += delta;
  m_token.location.end_column++;
}

int64_t Lexer::peek_char(uint32_t nth) {
  int64_t ch = m_source.peek_utf8_cp(nth);
  if (ch == 0) {
    unexpected_character("unexpected null-byte in source file");
  }
  return ch;
}

int64_t Lexer::read_char() {
  int64_t cp = m_source.read_utf8_cp();

  if (cp == 0) {
    unexpected_character("unexpected null-byte in source file");
  }

  m_last_character = cp;
  m_token.location.end_offset = m_source.read_offset();

  if (cp == '\n') {
    increment_row();
  } else {
    increment_column(utf8::sequence_length(cp));
  }

  return cp;
}

bool Lexer::is_whitespace(int64_t cp) {
  return (cp == ' ' || cp == '\r' || cp == '\t');
}

bool Lexer::is_decimal(int64_t cp) {
  return (cp >= '0' && cp <= '9');
}

bool Lexer::is_hex(int64_t cp) {
  return is_decimal(cp) || (cp >= 'a' && cp <= 'f') || (cp >= 'A' && cp <= 'F');
}

bool Lexer::is_binary(int64_t cp) {
  return (cp == '0' || cp == '1');
}

bool Lexer::is_octal(int64_t cp) {
  return (cp >= '0' && cp <= '7');
}

bool Lexer::is_alpha_lower(int64_t cp) {
  return (cp >= 'a' && cp <= 'z');
}

bool Lexer::is_alpha_upper(int64_t cp) {
  return (cp >= 'A' && cp <= 'Z');
}

bool Lexer::is_alpha(int64_t cp) {
  return is_alpha_lower(cp) || is_alpha_upper(cp);
}

bool Lexer::is_id_begin(int64_t cp) {
  return is_alpha(cp) || cp == '$' || cp == '_' || cp > 0x80;
}

bool Lexer::is_id_part(int64_t cp) {
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
    int64_t cp = peek_char();

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
    token.floatval = utils::string_view_to_double(m_source.window_view());
  } else {
    token.type = TokenType::Int;
    token.intval = utils::string_view_to_int(m_source.window_view(), 10);
  }
}

void Lexer::consume_hex(Token& token) {
  token.type = TokenType::Int;

  // there has to be at least one hex character
  if (!is_hex(read_char())) {
    unexpected_character("expected a hex digit");
  }

  while (is_hex(peek_char())) {
    read_char();
  }

  std::string_view view = m_source.window_view();
  view.remove_prefix(2);
  token.intval = utils::string_view_to_int(view, 16);
}

void Lexer::consume_octal(Token& token) {
  token.type = TokenType::Int;

  // there has to be at least one hex character
  if (!is_octal(read_char())) {
    unexpected_character("expected an octal digit");
  }

  while (is_octal(peek_char())) {
    read_char();
  }

  std::string_view view = m_source.window_view();
  view.remove_prefix(2);
  token.intval = utils::string_view_to_int(view, 8);
}

void Lexer::consume_binary(Token& token) {
  token.type = TokenType::Int;

  // there has to be at least one hex character
  if (!is_binary(read_char())) {
    unexpected_character("expected either a 1 or 0");
  }

  while (is_binary(peek_char())) {
    read_char();
  }

  std::string_view view = m_source.window_view();
  view.remove_prefix(2);
  token.intval = utils::string_view_to_int(view, 2);
}

void Lexer::consume_identifier(Token& token) {
  token.type = TokenType::Identifier;

  while (is_id_part(peek_char())) {
    read_char();
  }
  token.source = m_source.window_str();
}

void Lexer::consume_comment(Token& token) {
  token.type = TokenType::Comment;

  for (;;) {
    int64_t cp = peek_char();

    if (cp == '\n' || cp == -1) {
      break;
    }

    read_char();
  }
}

void Lexer::consume_multiline_comment(Token& token) {
  token.type = TokenType::Comment;

  uint32_t comment_depth = 1;
  while (comment_depth > 0) {
    int64_t cp = peek_char();

    switch (cp) {
      case -1: {
        read_char();
        unexpected_character("unclosed comment");
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
      case '\n': {
        read_char();
        break;
      }
      default: {
        read_char();
      }
    }
  }
}

void Lexer::consume_char(Token& token) {
  token.type = TokenType::Character;

  int64_t cp = read_char();

  // escape sequences
  if (cp == '\\') {
    switch (peek_char()) {
      case 'a': {
        read_char();
        cp = '\a';
        break;
      }
      case 'b': {
        read_char();
        cp = '\b';
        break;
      }
      case 'n': {
        read_char();
        cp = '\n';
        break;
      }
      case 'r': {
        read_char();
        cp = '\r';
        break;
      }
      case 't': {
        read_char();
        cp = '\t';
        break;
      }
      case 'v': {
        read_char();
        cp = '\v';
        break;
      }
      case 'f': {
        read_char();
        cp = '\f';
        break;
      }
      case '\'': {
        read_char();
        cp = '\'';
        break;
      }
      case '\\': {
        read_char();
        cp = '\\';
        break;
      }
      default: {
        read_char();
        unexpected_character("expected a valid escape sequence");
      }
    }
  }

  if (read_char() != '\'') {
    unexpected_character('\'');
  }

  token.charval = cp;
}

void Lexer::consume_string(Token& token, bool allow_format) {
  token.type = TokenType::String;
  m_mode = Mode::String;

  // string is built inside this buffer
  utils::Buffer string_buf;

  // consume string component
  for (;;) {
    int64_t cp = peek_char();

    // end of string
    if (cp == '"') {
      read_char();

      if (!m_interpolation_bracket_stack.empty()) {
        m_mode = Mode::InterpolatedExpression;
      } else {
        m_mode = Mode::TopLevel;
      }

      break;
    }

    // end of file reached, unclosed string detected
    if (cp == -1) {
      read_char();
      unexpected_character("unclosed string");
    }

    // string interpolation
    if (allow_format && cp == '{') {
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
          string_buf.write_utf8_cp('\a');
          break;
        }
        case 'b': {
          read_char();
          string_buf.write_utf8_cp('\b');
          break;
        }
        case 'n': {
          read_char();
          string_buf.write_utf8_cp('\n');
          break;
        }
        case 'r': {
          read_char();
          string_buf.write_utf8_cp('\r');
          break;
        }
        case 't': {
          read_char();
          string_buf.write_utf8_cp('\t');
          break;
        }
        case 'v': {
          read_char();
          string_buf.write_utf8_cp('\v');
          break;
        }
        case 'f': {
          read_char();
          string_buf.write_utf8_cp('\f');
          break;
        }
        case '"': {
          read_char();
          string_buf.write_utf8_cp('\"');
          break;
        }
        case '{': {
          read_char();
          string_buf.write_utf8_cp('{');
          break;
        }
        case '\\': {
          read_char();
          string_buf.write_utf8_cp('\\');
          break;
        }
        default: {
          read_char();
          unexpected_character("expected a valid escape sequence");
        }
      }
      continue;
    }

    string_buf.write_utf8_cp(read_char());
  }

  token.source = string_buf.str();
}

void Lexer::reset_token() {
  size_t offset_start = m_source.read_offset();
  m_token.location.valid = true;
  m_token.location.offset = offset_start;
  m_token.location.end_offset = offset_start;
  m_token.location.row = m_row;
  m_token.location.column = m_column;
  m_token.location.end_row = m_row;
  m_token.location.end_column = m_column;
  m_source.reset_window();
}

}  // namespace charly::core::compiler
