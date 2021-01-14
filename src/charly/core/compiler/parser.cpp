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

#include "charly/core/compiler/parser.h"

namespace charly::core::compiler {

namespace {
  using namespace ast;
}

ref<Program> Parser::parse_program(const std::string& source) {
  return Parser("-", source).parse_program();
}

ref<Statement> Parser::parse_statement(const std::string& source) {
  return Parser("-", source).parse_statement();
}

ref<Expression> Parser::parse_expression(const std::string& source) {
  return Parser("-", source).parse_expression();
}

ref<Program> Parser::parse_program() {
  ref<Block> body = parse_block_body();
  ref<Program> program = make<Program>(*m_filename, body);
  program->set_location(body);
  return program;
}

ref<Block> Parser::parse_block() {
  eat(TokenType::LeftCurly);
  ref<Block> block = parse_block_body();
  eat(TokenType::RightCurly);
  return block;
}

ref<Block> Parser::parse_block_body() {
  ref<Block> block = make<Block>();

  at(block);

  while (!(type(TokenType::RightCurly) || type(TokenType::Eof))) {
    ref<Statement> stmt = parse_statement();
    block->statements.push_back(stmt);
    block->set_end(stmt);
  }

  return block;
}

ref<Statement> Parser::parse_statement() {
  switch (m_token.type) {
    default: {
      ref<Expression> exp = parse_expression();
      skip(TokenType::Semicolon);
      return exp;
    }
  }
}

ref<Expression> Parser::parse_comma_expression() {
  ref<Expression> exp = parse_expression();
  if (!type(TokenType::Comma))
    return exp;

  ref<Tuple> tuple = make<Tuple>();
  tuple->set_begin(exp);
  tuple->elements.push_back(exp);

  while (type(TokenType::Comma)) {
    eat(TokenType::Comma);
    tuple->elements.push_back(parse_expression());
  }

  return tuple;
}

ref<Expression> Parser::parse_expression() {
  return parse_assignment();
}

ref<Expression> Parser::parse_assignment() {
  ref<Expression> target = parse_ternary();

  if (m_token.type == TokenType::Assignment) {
    TokenType assignment_operator = m_token.assignment_operator;

    eat(TokenType::Assignment);

    if (assignment_operator == TokenType::Assignment) {
      return make<Assignment>(target, parse_expression());
    } else {
      return make<ANDAssignment>(assignment_operator, target, parse_expression());
    }
  } else {
    return target;
  }
}

ref<Expression> Parser::parse_ternary() {
  ref<Expression> condition = parse_binop();

  if (skip(TokenType::QuestionMark)) {
    ref<Expression> then_exp = parse_expression();
    eat(TokenType::Colon);
    ref<Expression> else_exp = parse_expression();
    return make<Ternary>(condition, then_exp, else_exp);
  }

  return condition;
}

ref<Expression> Parser::parse_binop_1(ref<Expression> lhs, uint32_t min_precedence) {
  for (;;) {
    if (kBinopPrecedenceLevels.count(m_token.type) == 0)
      break;

    TokenType operation = m_token.type;
    uint32_t precedence = kBinopPrecedenceLevels.at(operation);
    if (precedence < min_precedence)
      break;

    advance();
    ref<Expression> rhs = parse_literal();

    // higher precedence operators or right associative operators
    for (;;) {
      if (kBinopPrecedenceLevels.count(m_token.type) == 0)
        break;

      uint32_t next_precedence = kBinopPrecedenceLevels.at(m_token.type);
      if ((next_precedence > precedence) ||
          (kRightAssociativeOperators.count(m_token.type) && next_precedence == precedence)) {
        rhs = parse_binop_1(rhs, next_precedence);
      } else {
        break;
      }
    }

    lhs = make<Binop>(operation, lhs, rhs);
  }

  return lhs;
}

ref<Expression> Parser::parse_binop() {
  return parse_binop_1(parse_literal(), 0);
}

ref<Expression> Parser::parse_literal() {
  switch (m_token.type) {
    case TokenType::Int: {
      return parse_int_token();
    }
    case TokenType::Float: {
      return parse_float_token();
    }
    case TokenType::True:
    case TokenType::False: {
      return parse_bool_token();
    }
    case TokenType::Identifier: {
      return parse_identifier_token();
    }
    case TokenType::Character: {
      return parse_char_token();
    }
    case TokenType::String: {
      return parse_string_token();
    }
    case TokenType::FormatString: {
      return parse_format_string();
    }
    case TokenType::LeftParen: {
      return parse_tuple();
    }
    case TokenType::Null: {
      return parse_null_token();
    }
    case TokenType::Self: {
      return parse_self_token();
    }
    case TokenType::Super: {
      return parse_super_token();
    }
    default: {
      unexpected_token("literal");
    }
  }
}

ref<FormatString> Parser::parse_format_string() {
  ref<FormatString> str = make<FormatString>();
  begin(str);

  ref<String> first_element = parse_string_token();
  if (first_element->value.size() > 0) {
    str->elements.push_back(first_element);
  }

  for (;;) {
    str->elements.push_back(parse_expression());
    eat(TokenType::RightCurly);

    // if the expression is followed by another FormatString token the loop
    // repeats and we parse another interpolated expression
    //
    // the format string is only terminated once a regular String token is passed
    if (type(TokenType::FormatString) || type(TokenType::String)) {
      bool last_part = type(TokenType::String);
      ref<String> element = parse_string_token();
      if (element->value.size() > 0)
        str->elements.push_back(element);
      if (last_part) {
        break;
      }
    } else {
      unexpected_token(TokenType::String);
    }
  }

  return str;
}

ref<Expression> Parser::parse_tuple() {
  ref<Tuple> tuple = make<Tuple>();
  begin(tuple);

  eat(TokenType::LeftParen);

  bool force_tuple = false;

  while (!(type(TokenType::RightParen))) {
    tuple->elements.push_back(parse_expression());

    // (<exp>,) becomes a tuple with one element
    if (skip(TokenType::Comma) && type(TokenType::RightParen)) {
      if (tuple->elements.size() == 1) {
        force_tuple = true;
        break;
      }
    }
  }

  end(tuple);
  eat(TokenType::RightParen);

  // (<exp>) becomes just <exp>
  if (!force_tuple && tuple->elements.size() == 1) {
    return tuple->elements.at(0);
  }

  return tuple;
}

ref<Int> Parser::parse_int_token() {
  match(TokenType::Int);
  ref<Int> node = make<Int>(m_token.intval);
  at(node);
  advance();
  return node;
}

ref<Float> Parser::parse_float_token() {
  match(TokenType::Float);
  ref<Float> node = make<Float>(m_token.floatval);
  at(node);
  advance();
  return node;
}

ref<Bool> Parser::parse_bool_token() {
  if (type(TokenType::True) || type(TokenType::False)) {
    ref<Bool> node = make<Bool>(type(TokenType::True));
    at(node);
    advance();
    return node;
  } else {
    unexpected_token("true or false");
  }
}

ref<Id> Parser::parse_identifier_token() {
  match(TokenType::Identifier);
  ref<Id> node = make<Id>(m_token.source);
  at(node);
  advance();
  return node;
}

ref<Char> Parser::parse_char_token() {
  match(TokenType::Character);
  ref<Char> node = make<Char>(m_token.charval);
  at(node);
  advance();
  return node;
}

ref<String> Parser::parse_string_token() {
  if (type(TokenType::String) || type(TokenType::FormatString)) {
    ref<String> node = make<String>(m_token.source);
    at(node);
    advance();
    return node;
  } else {
    unexpected_token(TokenType::String);
  }
}

ref<Null> Parser::parse_null_token() {
  match(TokenType::Null);
  ref<Null> node = make<Null>();
  at(node);
  advance();
  return node;
}

ref<Self> Parser::parse_self_token() {
  match(TokenType::Self);
  ref<Self> node = make<Self>();
  at(node);
  advance();
  return node;
}

ref<Super> Parser::parse_super_token() {
  match(TokenType::Super);
  ref<Super> node = make<Super>();
  at(node);
  advance();
  return node;
}

}  // namespace charly::core::compiler
