/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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

#include <functional>

#include "ast.h"
#include "lexer.h"
#include "location.h"
#include "token.h"

#pragma once

namespace Charly::Compilation {

typedef std::function<void()> ParseFunc;

// Holds a syntax error
struct SyntaxError {
  Location location;
  std::string message;
};

// Holds the result of the parsing step
struct ParserResult {
  std::optional<AST::AbstractNode*> abstract_syntax_tree;
  std::optional<std::vector<Token>> tokens;
  std::optional<SyntaxError> syntax_error;
};

// Context data for where keywords are allowed to appear
struct KeywordContext {
  bool break_allowed = false;
  bool continue_allowed = false;
  bool return_allowed = false;
  bool yield_allowed = false;
};

class Parser : public Lexer {
public:
  KeywordContext keyword_context;
  ParserResult parse();

  Parser(SourceFile& file) : Lexer(file) {
  }

public:
  // Utility methods
  Token& advance();
  void unexpected_token();
  void unexpected_token(TokenType expected);
  void unexpected_token(const std::string& expected_value);
  void illegal_token();
  void illegal_token(const std::string& message);
  void illegal_node(AST::AbstractNode* node, const std::string& message);
  void assert_token(TokenType type);
  void expect_token(TokenType type);
  void expect_token(TokenType type, ParseFunc func);
  void skip_token(TokenType type);
  void if_token(TokenType type, ParseFunc func);
  void interpret_keyword_as_identifier();

  // Parse methods
  AST::AbstractNode* parse_program();
  AST::AbstractNode* parse_block();
  AST::AbstractNode* parse_ignore_const();
  AST::AbstractNode* parse_import();
  AST::AbstractNode* parse_statement();
  AST::AbstractNode* parse_control_statement();
  AST::AbstractNode* parse_class_statement();
  AST::AbstractNode* parse_if_statement();
  AST::AbstractNode* parse_unless_statement();
  AST::AbstractNode* parse_guard_statement();
  AST::AbstractNode* parse_switch_statement();
  AST::AbstractNode* parse_switch_node();
  AST::AbstractNode* parse_match_statement();
  AST::AbstractNode* parse_match_arm();
  AST::AbstractNode* parse_do_statement();
  AST::AbstractNode* parse_while_statement();
  AST::AbstractNode* parse_until_statement();
  AST::AbstractNode* parse_loop_statement();
  AST::AbstractNode* parse_try_statement();
  AST::AbstractNode* parse_expression();
  AST::AbstractNode* parse_yield();
  AST::AbstractNode* parse_assignment();
  AST::AbstractNode* parse_ternary_if();
  AST::AbstractNode* parse_or();
  AST::AbstractNode* parse_and();
  AST::AbstractNode* parse_bitwise_or();
  AST::AbstractNode* parse_bitwise_xor();
  AST::AbstractNode* parse_bitwise_and();
  AST::AbstractNode* parse_equal_not();
  AST::AbstractNode* parse_less_greater();
  AST::AbstractNode* parse_bitwise_shift();
  AST::AbstractNode* parse_add_sub();
  AST::AbstractNode* parse_mul_div();
  AST::AbstractNode* parse_mod();
  AST::AbstractNode* parse_unary();
  AST::AbstractNode* parse_pow();
  AST::AbstractNode* parse_typeof();
  AST::AbstractNode* parse_member_call();
  AST::AbstractNode* parse_literal();
  AST::AbstractNode* parse_array();
  AST::AbstractNode* parse_hash();
  std::pair<std::string, AST::AbstractNode*> parse_hash_entry();
  AST::AbstractNode* parse_func();
  AST::AbstractNode* parse_arrowfunc();
  AST::AbstractNode* parse_class();

  // Helper methods
  void assign_default_name(AST::AbstractNode* node, const std::string& name);
};
};  // namespace Charly::Compilation
