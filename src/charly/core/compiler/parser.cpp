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

using namespace charly::core::compiler::ast;

namespace charly::core::compiler {

ref<Program> Parser::parse_program(utils::Buffer& source, DiagnosticConsole& console) {
  try {
    return Parser(source, console).parse_program();
  } catch (DiagnosticException&) { return nullptr; }
}

ref<Statement> Parser::parse_statement(utils::Buffer& source, DiagnosticConsole& console) {
  try {
    return Parser(source, console).parse_statement();
  } catch (DiagnosticException&) { return nullptr; }
}

ref<Expression> Parser::parse_expression(utils::Buffer& source, DiagnosticConsole& console) {
  try {
    return Parser(source, console).parse_expression();
  } catch (DiagnosticException&) { return nullptr; }
}

ref<Program> Parser::parse_program() {
  ref<Block> body = make<Block>();
  parse_block_body(body);
  ref<Program> program = make<Program>(body);
  program->set_location(body);

  return program;
}

ref<Block> Parser::parse_block() {
  ref<Block> block = make<Block>();
  begin(block);
  eat(TokenType::LeftCurly);
  parse_block_body(block);
  end(block);
  eat(TokenType::RightCurly);
  return block;
}

void Parser::parse_block_body(const ref<Block>& block) {
  uint32_t parsed_statements = 0;

  while (!(type(TokenType::RightCurly) || type(TokenType::Eof))) {
    ref<Statement> stmt = parse_statement();

    if (parsed_statements == 0 && !block->location().valid) {
      block->set_begin(stmt);
    }

    block->statements.push_back(stmt);
    block->set_end(stmt);

    parsed_statements++;
  }
}

ref<Statement> Parser::parse_statement() {
  ref<Statement> stmt;

  switch (m_token.type) {
    case TokenType::Return: {
      stmt = parse_return();
      break;
    }
    case TokenType::Break: {
      stmt = parse_break();
      break;
    }
    case TokenType::Continue: {
      stmt = parse_continue();
      break;
    }
    case TokenType::Defer: {
      stmt = parse_defer();
      break;
    }
    case TokenType::Throw: {
      stmt = parse_throw();
      break;
    }
    case TokenType::Export: {
      stmt = parse_export();
      break;
    }
    // import form is parsed as expression
    case TokenType::From: {
      return parse_import();
    }
    case TokenType::LeftCurly: {
      stmt = parse_block();
      break;
    }
    default: {
      stmt = parse_expression();
      break;
    }
  }

  skip(TokenType::Semicolon);

  return stmt;
}

ref<Statement> Parser::parse_return() {
  match(TokenType::Return);
  Location begin = m_token.location;
  advance();

  if (m_token.could_start_expression()) {
    ref<Expression> exp = parse_expression();
    ref<Return> ret = make<Return>(exp);
    ret->set_begin(begin);
    return ret;
  }

  ref<Null> null = make<Null>();
  null->set_location(begin);
  ref<Return> ret = make<Return>(null);
  ret->set_begin(begin);
  return ret;
}

ref<Statement> Parser::parse_break() {
  match(TokenType::Break);
  ref<Break> node = make<Break>();
  at(node);
  advance();
  return node;
}

ref<Statement> Parser::parse_continue() {
  match(TokenType::Continue);
  ref<Continue> node = make<Continue>();
  at(node);
  advance();
  return node;
}

ref<Statement> Parser::parse_defer() {
  match(TokenType::Defer);
  Location begin = m_token.location;
  advance();
  ref<Statement> stmt = parse_statement();
  ref<Defer> ret = make<Defer>(stmt);
  ret->set_begin(begin);
  return ret;
}

ref<Statement> Parser::parse_throw() {
  match(TokenType::Throw);
  Location begin = m_token.location;
  advance();
  ref<Expression> exp = parse_expression();
  ref<Throw> ret = make<Throw>(exp);
  ret->set_begin(begin);
  return ret;
}

ref<Statement> Parser::parse_export() {
  match(TokenType::Export);
  Location begin = m_token.location;
  advance();
  ref<Expression> exp = parse_expression();
  ref<Export> ret = make<Export>(exp);
  ret->set_begin(begin);
  return ret;
}

ref<Expression> Parser::parse_import() {
  Location begin_location = m_token.location;

  if (type(TokenType::Import)) {
    eat(TokenType::Import);
    ref<Import> import_node = make<Import>(parse_as_expression());
    import_node->set_begin(begin_location);
    return import_node;
  } else if (type(TokenType::From)) {
    eat(TokenType::From);
    ref<Expression> source_exp = parse_as_expression();

    ref<Import> import_node = make<Import>(source_exp);
    import_node->set_begin(begin_location);

    end(import_node);
    eat(TokenType::Import);
    this->parse_comma_as_expression(import_node->declarations);

    if (import_node->declarations.size() == 0) {
      m_console.error("expected at least one imported symbol", import_node->location());
    } else {
      import_node->set_end(import_node->declarations.back()->location());
    }

    return import_node;
  } else {
    unexpected_token(TokenType::Import);
  }
}

void Parser::parse_comma_expression(std::vector<ref<Expression>>& result) {
  if (!m_token.could_start_expression())
    return;

  ref<Expression> exp = parse_expression();
  result.push_back(exp);

  while (type(TokenType::Comma)) {
    eat(TokenType::Comma);
    result.push_back(parse_expression());
  }
}

void Parser::parse_comma_as_expression(std::vector<ref<Expression>>& result) {
  if (!m_token.could_start_expression())
    return;

  ref<Expression> exp = parse_as_expression();
  result.push_back(exp);

  while (type(TokenType::Comma)) {
    eat(TokenType::Comma);
    result.push_back(parse_as_expression());
  }
}

ref<Expression> Parser::parse_expression() {
  switch (m_token.type) {
    case TokenType::Yield: {
      return parse_yield();
    }
    case TokenType::Import: {
      return parse_import();
    }
    default: {
      return parse_assignment();
    }
  }
}

ref<Expression> Parser::parse_as_expression() {
  ref<Expression> exp = parse_expression();

  if (skip(TokenType::As)) {
    return make<As>(exp, parse_identifier_token());
  }

  return exp;
}

ref<Expression> Parser::parse_yield() {
  if (type(TokenType::Yield)) {
    Location begin_location = m_token.location;
    eat(TokenType::Yield);
    ref<Yield> node = make<Yield>(parse_yield());
    node->set_begin(begin_location);
    return node;
  }

  return parse_assignment();
}

ref<Expression> Parser::parse_assignment() {
  ref<Expression> target = parse_ternary();

  if (type(TokenType::Assignment)) {
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
  ref<Expression> condition = parse_binaryop();

  if (skip(TokenType::QuestionMark)) {
    ref<Expression> then_exp = parse_expression();
    eat(TokenType::Colon);
    ref<Expression> else_exp = parse_expression();
    return make<Ternary>(condition, then_exp, else_exp);
  }

  return condition;
}

ref<Expression> Parser::parse_binaryop_1(ref<Expression> lhs, uint32_t min_precedence) {
  for (;;) {
    if (kBinaryOpPrecedenceLevels.count(m_token.type) == 0)
      break;

    TokenType operation = m_token.type;
    uint32_t precedence = kBinaryOpPrecedenceLevels.at(operation);
    if (precedence < min_precedence)
      break;

    advance();
    ref<Expression> rhs = parse_unaryop();

    // higher precedence operators or right associative operators
    for (;;) {
      if (kBinaryOpPrecedenceLevels.count(m_token.type) == 0)
        break;

      uint32_t next_precedence = kBinaryOpPrecedenceLevels.at(m_token.type);
      if ((next_precedence > precedence) ||
          (kRightAssociativeOperators.count(m_token.type) && next_precedence == precedence)) {
        rhs = parse_binaryop_1(rhs, next_precedence);
      } else {
        break;
      }
    }

    lhs = make<BinaryOp>(operation, lhs, rhs);
  }

  return lhs;
}

ref<Expression> Parser::parse_binaryop() {
  return parse_binaryop_1(parse_unaryop(), 0);
}

ref<Expression> Parser::parse_unaryop() {
  if (m_token.is_unary_operator()) {
    TokenType operation = m_token.type;
    Location start_loc = m_token.location;
    advance();
    ref<UnaryOp> op = make<UnaryOp>(operation, parse_unaryop());
    op->set_begin(start_loc);
    return op;
  }

  return parse_control_expression();
}

ref<Expression> Parser::parse_control_expression() {
  switch (m_token.type) {
    case TokenType::Await: {
      return parse_await();
    }
    case TokenType::Typeof: {
      return parse_typeof();
    }
    default: {
      return parse_literal();
    }
  }
}

ref<Expression> Parser::parse_await() {
  if (type(TokenType::Await)) {
    Location begin_location = m_token.location;
    eat(TokenType::Await);
    ref<Await> node = make<Await>(parse_await());
    node->set_begin(begin_location);
    return node;
  }

  return parse_literal();
}

ref<Expression> Parser::parse_typeof() {
  if (type(TokenType::Typeof)) {
    Location begin_location = m_token.location;
    eat(TokenType::Typeof);
    ref<Typeof> node = make<Typeof>(parse_typeof());
    node->set_begin(begin_location);
    return node;
  }

  return parse_literal();
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
      unexpected_token();
    }
  }
}

ref<FormatString> Parser::parse_format_string() {
  ref<FormatString> format_string = make<FormatString>();

  match(TokenType::FormatString);
  begin(format_string);
  end(format_string);

  ref<String> element = parse_string_token();
  if (element->value.size() > 0)
    format_string->elements.push_back(element);

  do {
    // parse interpolated expression
    ref<Expression> exp = parse_expression();
    format_string->elements.push_back(exp);
    format_string->set_end(exp);

    eat(TokenType::RightCurly);

    // lexer should only generate string or formatstring tokens at this point
    if (!(type(TokenType::FormatString) || type(TokenType::String))) {
      unexpected_token(TokenType::String);
    }

    // if the expression is followed by another FormatString token the loop
    // repeats and we parse another interpolated expression
    //
    // a regular string token signals the end of the format string
    bool final_element = type(TokenType::String);

    ref<String> element = parse_string_token();
    format_string->set_end(element);

    if (element->value.size() > 0)
      format_string->elements.push_back(element);

    if (final_element)
      return format_string;
  } while (true);
}

ref<Expression> Parser::parse_tuple() {
  ref<Tuple> tuple = make<Tuple>();
  begin(tuple);

  eat(TokenType::LeftParen);

  if (!type(TokenType::RightParen)) {
    ref<Expression> exp = parse_expression();

    // (x) is treated as parentheses, not a tuple
    if (type(TokenType::RightParen)) {
      advance();
      return exp;
    }

    tuple->elements.push_back(exp);

    while (skip(TokenType::Comma)) {
      // (x,) is treated as a tuple with one value
      if (tuple->elements.size() == 1 && type(TokenType::RightParen)) {
        break;
      }

      tuple->elements.push_back(parse_expression());
    }
  }

  end(tuple);
  eat(TokenType::RightParen);

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

void Parser::unexpected_token() {
  std::string& real_type = kTokenTypeStrings[static_cast<uint8_t>(m_token.type)];

  utils::Buffer formatbuf;
  if (m_token.type == TokenType::Eof) {
    formatbuf.append_string("unexpected end of file");
  } else {
    formatbuf.append_string("unexpected token '");
    formatbuf.append_string(real_type);
    formatbuf.append_string("'");
  }

  m_console.fatal(formatbuf.buffer_string(), m_token.location);
}

void Parser::unexpected_token(const std::string& message) {
  m_console.fatal(message, m_token.location);
}

void Parser::unexpected_token(TokenType expected) {
  std::string& real_type = kTokenTypeStrings[static_cast<uint8_t>(m_token.type)];
  std::string& expected_type = kTokenTypeStrings[static_cast<uint8_t>(expected)];

  utils::Buffer formatbuf;

  switch (m_token.type) {
    case TokenType::Eof: {
      formatbuf.append_string("unexpected end of file");
      formatbuf.append_string(", expected a '");
      formatbuf.append_string(expected_type);
      formatbuf.append_string("' token");
      break;
    }
    case TokenType::Int:
    case TokenType::Float: {
      formatbuf.append_string("unexpected numerical constant");
      formatbuf.append_string(", expected a '");
      formatbuf.append_string(expected_type);
      formatbuf.append_string("' token");
      break;
    }
    case TokenType::String: {
      formatbuf.append_string("unexpected string literal");
      formatbuf.append_string(", expected a '");
      formatbuf.append_string(expected_type);
      formatbuf.append_string("' token");
      break;
    }
    case TokenType::FormatString: {
      formatbuf.append_string("unexpected format string");
      formatbuf.append_string(", expected a '");
      formatbuf.append_string(expected_type);
      formatbuf.append_string("' token");
      break;
    }
    default: {
      formatbuf.append_string("unexpected '");
      formatbuf.append_string(real_type);
      formatbuf.append_string("' token, expected a '");
      formatbuf.append_string(expected_type);
      formatbuf.append_string("' token");
      break;
    }
  }

  m_console.fatal(formatbuf.buffer_string(), m_token.location);
}

}  // namespace charly::core::compiler
