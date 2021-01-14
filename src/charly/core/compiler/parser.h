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

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/error.h"
#include "charly/core/compiler/lexer.h"

#pragma once

namespace charly::core::compiler {

namespace {
  using namespace ast;
}

// clang-format off

static const std::unordered_map<TokenType, uint32_t> kBinaryOpPrecedenceLevels = {
  { TokenType::Or,                    8 },
  { TokenType::And,                   9 },

  { TokenType::Equal,                 10 },
  { TokenType::NotEqual,              10 },

  { TokenType::LessThan,              20 },
  { TokenType::GreaterThan,           20 },
  { TokenType::LessEqual,             20 },
  { TokenType::GreaterEqual,          20 },

  { TokenType::BitLeftShift,          30 },
  { TokenType::BitRightShift,         30 },
  { TokenType::BitUnsignedRightShift, 30 },

  { TokenType::DoublePoint,           40 },
  { TokenType::TriplePoint,           40 },
  { TokenType::BitOR,                 41 },
  { TokenType::BitXOR,                42 },
  { TokenType::BitAND,                43 },

  { TokenType::Plus,                  50 },
  { TokenType::Minus,                 50 },

  { TokenType::Mul,                   60 },
  { TokenType::Div,                   60 },

  { TokenType::Mod,                   60 },
  { TokenType::Pow,                   70 }
};

static const std::unordered_set<TokenType> kRightAssociativeOperators = {
  TokenType::Pow
};

// clang-format on

class Parser : public Lexer {
public:
  Parser(const std::string& filename, const std::string& source, uint32_t row = 1, uint32_t column = 1) :
    Lexer(filename, source, row, column) {
    advance();
  }

  static ref<Program> parse_program(const std::string& source);
  static ref<Statement> parse_statement(const std::string& source);
  static ref<Expression> parse_expression(const std::string& source);

  // structural and statements
  ref<Program> parse_program();
  ref<Block> parse_block();
  ref<Block> parse_block_body();
  ref<Statement> parse_statement();

  // expressions
  ref<Expression> parse_comma_expression();
  ref<Expression> parse_expression();
  ref<Expression> parse_assignment();
  ref<Expression> parse_ternary();
  ref<Expression> parse_binaryop();
  ref<Expression> parse_unaryop();

  // compound literals
  ref<Expression> parse_literal();
  ref<FormatString> parse_format_string();
  ref<Expression> parse_tuple();

  // single token ast nodes
  ref<Int> parse_int_token();
  ref<Float> parse_float_token();
  ref<Bool> parse_bool_token();
  ref<Id> parse_identifier_token();
  ref<Char> parse_char_token();
  ref<String> parse_string_token();
  ref<Null> parse_null_token();
  ref<Self> parse_self_token();
  ref<Super> parse_super_token();

private:
  ref<Expression> parse_binaryop_1(ref<Expression> lhs, uint32_t min_precedence);

  [[noreturn]] void unexpected_token() {
    std::string& real_type = kTokenTypeStrings[static_cast<uint8_t>(m_token.type)];
    throw CompilerError("Unexpected token '" + real_type + "'", m_token.location);
  }

  [[noreturn]] void unexpected_token(const std::string& expected) {
    std::string& real_type = kTokenTypeStrings[static_cast<uint8_t>(m_token.type)];
    throw CompilerError("Unexpected token '" + real_type + "' expected " + expected, m_token.location);
  }

  [[noreturn]] void unexpected_token(TokenType expected) {
    std::string& real_type = kTokenTypeStrings[static_cast<uint8_t>(m_token.type)];
    std::string& expected_type = kTokenTypeStrings[static_cast<uint8_t>(expected)];
    throw CompilerError("Unexpected token '" + real_type + "' expected '" + expected_type + "'", m_token.location);
  }

  [[noreturn]] void unexpected_node(const ref<Node> node, const std::string& message) {
    throw CompilerError(message, node->begin());
  }

  // advance to the next token
  void advance() {
    m_token = read_token();
    // std::cout << m_token << '\n';
  }

  // check the current type of the token
  bool type(TokenType t) const {
    return m_token.type == t;
  }

  // throw if token type does not match
  void match(TokenType t) {
    if (!type(t))
      unexpected_token(t);
  }

  // throw if token type does not match and advance token
  void eat(TokenType t) {
    match(t);
    advance();
  }

  // if token type matches, advance token
  // returns true if the token was advanced
  bool skip(TokenType t) {
    return type(t) ? (advance(), true) : false;
  }

  // set the begin location of a node to the current token
  void begin(const ref<Node>& node) {
    node->set_begin(m_token.location);
  }

  // set the end location of a node to the current token
  void end(const ref<Node>& node) {
    node->set_end(m_token.location);
  }

  // set the location of a node to the current token
  void at(const ref<Node>& node) {
    begin(node);
    end(node);
  }

private:
  Token m_token;
};

}  // namespace charly::core::compiler
