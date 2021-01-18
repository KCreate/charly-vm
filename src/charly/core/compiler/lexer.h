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

#include <memory>
#include <unordered_map>
#include <vector>

#include "charly/core/compiler/error.h"
#include "charly/core/compiler/token.h"
#include "charly/utils/buffer.h"

#pragma once

namespace charly::core::compiler {

// splits source input into individual tokens for parsing
class Lexer {
public:
  Lexer(const std::string& filename, const std::string& source, uint32_t row = 1, uint32_t column = 1) :
    m_filename(std::make_shared<std::string>(filename)), m_source(source), m_row(row), m_column(column) {
    m_last_character = '\0';
    m_mode = Mode::TopLevel;
  }

  // reads the next token
  Token read_token_all();

  // read the next token
  // skips over whitespace, newlines and comments
  Token read_token();

  // returns the last read token
  Token last_token();

protected:
  // full path of the source file (or empty buffer)
  std::shared_ptr<std::string> m_filename;

  // source code
  utils::Buffer m_source;

private:
  Lexer(Lexer&) = delete;
  Lexer(Lexer&&) = delete;

  void increment_row();
  void increment_column(size_t delta);

  uint32_t peek_char(uint32_t nth = 0) const;  // peek the next char
  uint32_t read_char();                        // read the next char
  uint32_t last_char() const;                  // return the last read char or \0

  // character identification
  bool is_whitespace(uint32_t cp) const;    // \r \t ' '
  bool is_decimal(uint32_t cp) const;       // 0-9
  bool is_hex(uint32_t cp) const;           // 0-9a-fA-F
  bool is_binary(uint32_t cp) const;        // 0-1
  bool is_octal(uint32_t cp) const;         // 0-7
  bool is_alpha_lower(uint32_t cp) const;   // a-z
  bool is_alpha_upper(uint32_t cp) const;   // A-Z
  bool is_alpha(uint32_t cp) const;         // a-zA-Z
  bool is_alphanumeric(uint32_t cp) const;  // a-zA-Z0-9
  bool is_id_begin(uint32_t cp) const;      // alpha $ _
  bool is_id_part(uint32_t cp) const;       // alpha $ _ numeric

  // consume some token type
  void consume_whitespace(Token& token);
  void consume_decimal(Token& token);
  void consume_hex(Token& token);
  void consume_octal(Token& token);
  void consume_binary(Token& token);
  void consume_identifier(Token& token);
  void consume_comment(Token& token);
  void consume_multiline_comment(Token& token);
  void consume_char(Token& token);
  void consume_string(Token& token, bool allow_format = true);

  // current source row / column
  uint32_t m_row;
  uint32_t m_column;

  // the last character that was read from the source
  uint32_t m_last_character;

  // current parsing mode
  enum class Mode { TopLevel, String, InterpolatedExpression };
  Mode m_mode;

  // keep track of interpolation brackets
  std::vector<size_t> m_interpolation_bracket_stack;

  // Opening brackets get pushed onto the stack, closing brackets pop
  // their corresponding opening bracket from the stack
  // throws an error on invalid bracket arrangement
  std::vector<TokenType> m_bracket_stack;

  // list of parsed tokens
  std::vector<Token> m_tokens;
};

}  // namespace charly::core::compiler
