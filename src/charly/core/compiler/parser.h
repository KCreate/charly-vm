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
#include "charly/core/compiler/diagnostic.h"
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
  static ref<Program> parse_program(const std::string& source) {
    utils::Buffer buffer(source);
    DiagnosticConsole console("unknown", buffer);
    return parse_program(buffer, console);
  }

  static ref<Statement> parse_statement(const std::string& source) {
    utils::Buffer buffer(source);
    DiagnosticConsole console("unknown", buffer);
    return parse_statement(buffer, console);
  }

  static ref<Expression> parse_expression(const std::string& source) {
    utils::Buffer buffer(source);
    DiagnosticConsole console("unknown", buffer);
    return parse_expression(buffer, console);
  }

  static ref<Program> parse_program(utils::Buffer& source, DiagnosticConsole& console);
  static ref<Statement> parse_statement(utils::Buffer& source, DiagnosticConsole& console);
  static ref<Expression> parse_expression(utils::Buffer& source, DiagnosticConsole& console);

private:
  Parser(utils::Buffer& source, DiagnosticConsole& console) : Lexer(source, console) {
    advance();
  }

  // structural and statements
  ref<Program> parse_program();
  ref<Block> parse_block();
  void parse_block_body(const ref<Block>& block);
  ref<Statement> parse_statement();
  ref<Statement> parse_return();
  ref<Statement> parse_break();
  ref<Statement> parse_continue();
  ref<Statement> parse_defer();
  ref<Statement> parse_throw();
  ref<Statement> parse_export();

  // expressions
  void parse_comma_expression(std::vector<ref<Expression>>& result);
  void parse_comma_as_expression(std::vector<ref<Expression>>& result);
  ref<Expression> parse_expression();
  ref<Expression> parse_as_expression();
  ref<Expression> parse_yield();
  ref<Import> parse_import();
  ref<Import> parse_from();
  ref<Expression> parse_assignment();
  ref<Expression> parse_ternary();
  ref<Expression> parse_binaryop();
  ref<Expression> parse_binaryop_1(ref<Expression> lhs, uint32_t min_precedence);
  ref<Expression> parse_unaryop();

  // control expressions
  ref<Expression> parse_control_expression();
  ref<Expression> parse_spawn();
  ref<Expression> parse_await();
  ref<Expression> parse_typeof();

  // expressions
  ref<Expression> parse_call_member_index();
  ref<CallOp> parse_call(ref<Expression> target);
  ref<MemberOp> parse_member(ref<Expression> target);
  ref<IndexOp> parse_index(ref<Expression> target);
  ref<Expression> parse_literal();

  // compound literals
  ref<FormatString> parse_format_string();
  ref<Expression> parse_tuple();

  // literals
  ref<Int> parse_int_token();
  ref<Float> parse_float_token();
  ref<Bool> parse_bool_token();
  ref<Id> parse_identifier_token();
  ref<Char> parse_char_token();
  ref<String> parse_string_token();
  ref<Null> parse_null_token();
  ref<Self> parse_self_token();
  ref<Super> parse_super_token();

  void validate_defer(const ref<Defer>& node);
  void validate_import(const ref<Import>& node);
  void validate_assignment(const ref<Assignment>& node);
  void validate_andassignment(const ref<ANDAssignment>& node);
  void validate_spawn(const ref<Spawn>& node);

  [[noreturn]] void unexpected_token();
  [[noreturn]] void unexpected_token(const std::string& message);
  [[noreturn]] void unexpected_token(TokenType expected);

  // advance to the next token
  void advance() {
    read_token();
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
    node->set_location(m_token.location);
  }
};

}  // namespace charly::core::compiler
