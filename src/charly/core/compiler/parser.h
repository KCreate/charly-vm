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

struct KeywordContext {
  bool _return = false;
  bool _break = false;
  bool _continue = false;
  bool _export = false;
  bool _yield = false;
  bool _super = false;
};

class Parser : public Lexer {
public:
  static ref<Block> parse_program(const std::string& source) {
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

  static ref<Block> parse_program(utils::Buffer& source, DiagnosticConsole& console);
  static ref<Statement> parse_statement(utils::Buffer& source, DiagnosticConsole& console);
  static ref<Expression> parse_expression(utils::Buffer& source, DiagnosticConsole& console);

private:
  Parser(utils::Buffer& source, DiagnosticConsole& console) : Lexer(source, console) {
    advance();

    m_keyword_context._return = true;
    m_keyword_context._break = false;
    m_keyword_context._continue = false;
    m_keyword_context._yield = false;
    m_keyword_context._export = true;
    m_keyword_context._super = false;
  }

  // structural and statements
  ref<Block> parse_program();
  ref<Block> parse_block();
  void parse_block_body(const ref<Block>& block);
  ref<Statement> parse_block_or_statement();
  ref<Statement> parse_statement();
  ref<Statement> parse_jump_statement();
  ref<Statement> parse_throw_statement();
  ref<Return> parse_return();
  ref<Break> parse_break();
  ref<Continue> parse_continue();
  ref<TryFinally> parse_defer();
  ref<Throw> parse_throw();
  ref<Statement> parse_import();
  ref<Expression> parse_import_expression();
  ref<Export> parse_export();

  // control statements
  ref<If> parse_if();
  ref<While> parse_while();
  ref<Loop> parse_loop();
  ref<Statement> parse_try();
  ref<Switch> parse_switch();
  ref<For> parse_for();

  // declarations
  ref<Statement> parse_declaration();

  // expressions
  void parse_call_arguments(std::vector<ref<Expression>>& result);
  ref<Expression> parse_expression();
  ref<Yield> parse_yield();
  ref<Expression> parse_assignment();
  ref<Expression> parse_ternary();
  ref<Expression> parse_binaryop();
  ref<Expression> parse_binaryop_1(ref<Expression> lhs, uint32_t min_precedence);
  ref<Expression> parse_possible_spread_expression();
  ref<Expression> parse_unaryop();

  // control expressions
  ref<Expression> parse_control_expression();
  ref<Expression> parse_spawn();
  ref<Expression> parse_await();
  ref<Expression> parse_typeof();

  // expressions
  ref<Expression> parse_call_member_index();
  ref<CallOp> parse_call(const ref<Expression>& target);
  ref<MemberOp> parse_member(const ref<Expression>& target);
  ref<IndexOp> parse_index(const ref<Expression>& target);
  ref<Expression> parse_literal();
  ref<Expression> parse_builtin();

  // compound literals
  ref<FormatString> parse_format_string();
  ref<Expression> parse_tuple(bool paren_conversion = true);
  ref<List> parse_list();
  ref<Dict> parse_dict();

  struct FunctionFlags {
    bool class_function;
    bool static_function;
  };
  ref<Function> parse_function(FunctionFlags flags = FunctionFlags());
  ref<Function> parse_arrow_function();
  void parse_function_arguments(std::vector<ref<FunctionArgument>>& result, FunctionFlags flags = FunctionFlags());
  ref<Class> parse_class();

  // literals
  ref<Int> parse_int_token();
  ref<Float> parse_float_token();
  ref<Bool> parse_bool_token();
  ref<Id> parse_identifier_token();
  ref<Char> parse_char_token();
  ref<String> parse_string_token();
  ref<Null> parse_null_token();
  ref<Self> parse_self_token();

  ref<Expression> parse_super();

  ref<Statement> create_declaration(const ref<Expression>& target, const ref<Expression>& value, bool constant);

  // create an unpack target node from a source expression
  // and generate errors on invalid input
  ref<UnpackTarget> create_unpack_target(const ref<Expression>& node, bool declaration = false);

  // checks wether a given expression can be assigned to
  bool is_assignable(const ref<Expression>& expression);

  // checks wether a given expression can be part of an unpack target
  bool is_valid_unpack_target_element(const ref<Expression>& expression);

  // wrap the input statement in a block or return as-is
  // if already a block
  static ref<Block> wrap_statement_in_block(const ref<Statement>& node);

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
    if (type(t)) {
      advance();
      return true;
    }

    return false;
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

  KeywordContext m_keyword_context;
};

}  // namespace charly::core::compiler
