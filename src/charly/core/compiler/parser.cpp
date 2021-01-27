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

    skip(TokenType::Semicolon);

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
    case TokenType::Import: {
      stmt = parse_import();
      break;
    }
    case TokenType::LeftCurly: {
      stmt = parse_block();
      break;
    }
    case TokenType::If: {
      stmt = parse_if();
      break;
    }
    case TokenType::While: {
      stmt = parse_while();
      break;
    }
    case TokenType::Loop: {
      stmt = parse_loop();
      break;
    }
    case TokenType::Let:
    case TokenType::Const: {
      stmt = parse_declaration();
      break;
    }
    default: {
      if (m_token.could_start_expression()) {
        stmt = parse_expression();
        break;
      }

      unexpected_token("expected a statement");
      break;
    }
  }

  return stmt;
}

ref<Return> Parser::parse_return() {
  Location begin = m_token.location;
  eat(TokenType::Return);

  if (m_token.could_start_expression()) {
    ref<Expression> exp = parse_expression();
    ref<Return> ret = make<Return>(exp);
    ret->set_begin(begin);
    return ret;
  }

  ref<Null> null = make<Null>();
  null->set_location(begin);
  ref<Return> node = make<Return>(null);
  node->set_begin(begin);
  return node;
}

ref<Break> Parser::parse_break() {
  ref<Break> node = make<Break>();
  at(node);
  eat(TokenType::Break);
  return node;
}

ref<Continue> Parser::parse_continue() {
  ref<Continue> node = make<Continue>();
  at(node);
  eat(TokenType::Continue);
  return node;
}

ref<Defer> Parser::parse_defer() {
  Location begin = m_token.location;
  eat(TokenType::Defer);
  ref<Statement> stmt = parse_statement();
  ref<Defer> node = make<Defer>(stmt);
  node->set_begin(begin);
  validate_defer(node);
  return node;
}

ref<Throw> Parser::parse_throw() {
  Location begin = m_token.location;
  eat(TokenType::Throw);
  ref<Expression> exp = parse_expression();
  ref<Throw> node = make<Throw>(exp);
  node->set_begin(begin);
  return node;
}

ref<Export> Parser::parse_export() {
  Location begin = m_token.location;
  eat(TokenType::Export);
  ref<Expression> exp = parse_expression();
  ref<Export> node = make<Export>(exp);
  node->set_begin(begin);
  return node;
}

ref<Import> Parser::parse_import() {
  Location begin = m_token.location;
  eat(TokenType::Import);
  ref<Expression> source_exp = parse_as_expression();
  ref<Import> import_node = make<Import>(source_exp);
  import_node->set_begin(begin);
  validate_import(import_node);
  return import_node;
}

ref<If> Parser::parse_if() {
  Location begin = m_token.location;
  eat(TokenType::If);

  ref<Expression> condition = parse_expression();
  ref<Statement> then_stmt = parse_statement();
  ref<Statement> else_stmt = make<Nop>();

  if (skip(TokenType::Else)) {
    else_stmt = parse_statement();
  }

  ref<If> node = make<If>(condition, then_stmt, else_stmt);
  node->set_begin(begin);
  return node;
}

ref<While> Parser::parse_while() {
  Location begin = m_token.location;
  eat(TokenType::While);

  ref<Expression> condition = parse_expression();
  ref<Statement> then_stmt = parse_statement();

  ref<While> node = make<While>(condition, then_stmt);
  node->set_begin(begin);
  return node;
}

ref<While> Parser::parse_loop() {
  Location begin = m_token.location;
  eat(TokenType::Loop);
  ref<Expression> condition = make<Bool>(true);
  condition->set_location(begin);
  ref<While> node = make<While>(condition, parse_statement());
  node->set_begin(begin);
  return node;
}

ref<Statement> Parser::parse_declaration() {
  if (!(type(TokenType::Let) || type(TokenType::Const)))
    unexpected_token("let or const");

  Location begin = m_token.location;

  bool const_declaration = type(TokenType::Const);
  advance();

  // parse the left-hand side of the declaration
  ref<Expression> target;
  bool requires_assignment = const_declaration;
  switch (m_token.type) {

    // regular local variable
    case TokenType::Identifier: {
      target = parse_identifier_token();
      break;
    }

    // sequence unpack declaration
    case TokenType::LeftParen: {
      target = parse_tuple(false); // disable paren conversion
      requires_assignment = true;
      break;
    }

    // object unpack declaration
    case TokenType::LeftCurly: {
      target = parse_dict();
      requires_assignment = true;
      break;
    }

    default: {
      unexpected_token("expected variable declaration");
    }
  }

  if (requires_assignment)
    match(TokenType::Assignment);

  ref<Declaration> declaration_node;

  if (skip(TokenType::Assignment)) {
    ref<Expression> value = parse_expression();
    declaration_node = make<Declaration>(target, value, const_declaration);
  } else {
    ref<Null> null = make<Null>();
    null->set_location(target);
    declaration_node = make<Declaration>(target, null, const_declaration);
  }

  declaration_node->set_begin(begin);

  validate_declaration(declaration_node);

  return declaration_node;
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

ref<Expression> Parser::parse_as_expression() {
  ref<Expression> exp = parse_expression();

  if (skip(TokenType::As)) {
    return make<As>(exp, parse_identifier_token());
  }

  return exp;
}

ref<Expression> Parser::parse_expression() {
  switch (m_token.type) {
    case TokenType::Yield: {
      return parse_yield();
    }
    case TokenType::Import: {
      return parse_import_expression();
    }
    default: {
      return parse_assignment();
    }
  }
}

ref<Yield> Parser::parse_yield() {
  Location begin = m_token.location;
  eat(TokenType::Yield);
  ref<Yield> node = make<Yield>(parse_expression());
  node->set_begin(begin);
  return node;
}

ref<ImportExpression> Parser::parse_import_expression() {
  Location begin = m_token.location;
  eat(TokenType::Import);
  ref<ImportExpression> import_node = make<ImportExpression>(parse_expression());
  import_node->set_begin(begin);
  return import_node;
}

ref<Expression> Parser::parse_assignment() {
  ref<Expression> target = parse_ternary();

  if (type(TokenType::Assignment)) {
    TokenType assignment_operator = m_token.assignment_operator;
    eat(TokenType::Assignment);
    ref<Assignment> node = make<Assignment>(assignment_operator, target, parse_expression());
    validate_assignment(node);
    return node;
  }

  return target;
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
    case TokenType::Spawn: {
      return parse_spawn();
    }
    case TokenType::Await: {
      return parse_await();
    }
    case TokenType::Typeof: {
      return parse_typeof();
    }
    default: {
      return parse_call_member_index();
    }
  }
}

ref<Expression> Parser::parse_spawn() {
  Location begin = m_token.location;
  eat(TokenType::Spawn);
  ref<Spawn> node = make<Spawn>(parse_statement());
  node->set_begin(begin);
  validate_spawn(node);
  return node;
}

ref<Expression> Parser::parse_await() {
  Location begin = m_token.location;
  eat(TokenType::Await);
  ref<Await> node = make<Await>(parse_control_expression());
  node->set_begin(begin);
  return node;
}

ref<Expression> Parser::parse_typeof() {
  Location begin = m_token.location;
  eat(TokenType::Typeof);
  ref<Typeof> node = make<Typeof>(parse_control_expression());
  node->set_begin(begin);
  return node;
}

ref<Expression> Parser::parse_call_member_index() {
  ref<Expression> target = parse_literal();

  for (;;) {
    bool newline_passed_since_base = target->location().end_row != m_token.location.row;

    switch (m_token.type) {
      case TokenType::LeftParen: {
        if (newline_passed_since_base)
          return target;
        target = parse_call(target);
        break;
      }
      case TokenType::LeftBracket: {
        if (newline_passed_since_base)
          return target;
        target = parse_index(target);
        break;
      }
      case TokenType::Point: {
        target = parse_member(target);
        break;
      }
      default: {
        return target;
      }
    }
  }
}

ref<CallOp> Parser::parse_call(ref<Expression> target) {
  eat(TokenType::LeftParen);
  ref<CallOp> callop = make<CallOp>(target);
  parse_comma_expression(callop->arguments);
  end(callop);
  eat(TokenType::RightParen);
  return callop;
}

ref<MemberOp> Parser::parse_member(ref<Expression> target) {
  eat(TokenType::Point);
  return make<MemberOp>(target, parse_identifier_token());
}

ref<IndexOp> Parser::parse_index(ref<Expression> target) {
  eat(TokenType::LeftBracket);
  ref<IndexOp> indexop = make<IndexOp>(target, parse_expression());
  end(indexop);
  eat(TokenType::RightBracket);
  return indexop;
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
    case TokenType::LeftCurly: {
      return parse_dict();
    }
    case TokenType::LeftBracket: {
      return parse_list();
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
      unexpected_token("expected an expression");
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

  for (;;) {
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
  };
}

ref<Expression> Parser::parse_tuple(bool paren_conversion) {
  ref<Tuple> tuple = make<Tuple>();
  begin(tuple);

  eat(TokenType::LeftParen);

  if (!type(TokenType::RightParen)) {
    ref<Expression> exp = parse_expression();

    // (x) is treated as parentheses, not a tuple
    if (type(TokenType::RightParen) && paren_conversion) {
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

ref<List> Parser::parse_list() {
  ref<List> list = make<List>();
  begin(list);

  eat(TokenType::LeftBracket);

  if (!type(TokenType::RightBracket)) {
    list->elements.push_back(parse_expression());
    while (skip(TokenType::Comma)) {
      list->elements.push_back(parse_expression());
    }
  }

  end(list);
  eat(TokenType::RightBracket);

  return list;
}

ref<Dict> Parser::parse_dict() {
  ref<Dict> dict = make<Dict>();
  begin(dict);

  eat(TokenType::LeftCurly);

  if (!type(TokenType::RightCurly)) {
    do {
      ref<Expression> key = parse_expression();
      ref<Expression> value = nullptr;

      if (skip(TokenType::Colon)) {
        value = parse_expression();
      }

      dict->elements.push_back(make<DictEntry>(key, value));
    } while (skip(TokenType::Comma));
  }

  end(dict);
  eat(TokenType::RightCurly);

  validate_dict(dict);

  return dict;
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

void Parser::validate_defer(const ref<Defer>& node) {
  if (!isa<Block>(node->statement) && !isa<CallOp>(node->statement)) {
    m_console.error("expected a call expression", node->statement->location());
  }
}

void Parser::validate_import(const ref<Import>& node) {
  if (ref<As> as_node = cast<As>(node->source)) {
    const ref<Expression>& exp = as_node->expression;
    if (!(isa<Id>(exp) || isa<String>(exp) || isa<FormatString>(exp))) {
      m_console.error("expected an identifier or a string literal", exp->location());
    }
  } else {
    if (!isa<Id>(node->source)) {
      m_console.error("expected an identifier", node->source->location());
    }
  }
}

void Parser::validate_declaration(const ref<Declaration>& node) {
  switch (node->target->type()) {
    case Node::Type::Id: {
      return;
    }
    case Node::Type::Tuple:
    case Node::Type::Dict: {
      if (!node->target->assignable())
        m_console.error("left-hand side of declaration is not assignable", node->target->location());
      return;
    }
    default: {
      assert(false && "unexpected node");
      break;
    }
  }
}

void Parser::validate_assignment(const ref<Assignment>& node) {
  // tuple or dict assignment not allowed if the assignment
  // operator is anything else than regular assignment
  if (node->operation != TokenType::Assignment) {
    switch (node->target->type()) {
      case Node::Type::Tuple:
      case Node::Type::Dict: {
        m_console.error("this type of expression cannot be used as the left-hand side of an operator assignment",
                        node->target->location());
        return;
      }
      default: {
        break;
      }
    }
  }

  if (!node->target->assignable()) {
    m_console.error("left-hand side of assignment is not assignable", node->target->location());
  }
}

void Parser::validate_spawn(const ref<Spawn>& node) {
  if (!isa<Block>(node->statement) && !isa<CallOp>(node->statement)) {
    m_console.error("expected a call expression", node->statement->location());
  }
}

void Parser::validate_dict(const ref<Dict>& node) {
  for (ref<DictEntry>& entry : node->elements) {
    ref<Expression>& key = entry->key;
    ref<Expression>& value = entry->value;

    // key-only elements
    if (value.get() == nullptr) {
      if (isa<Id>(key))
        continue;

      if (isa<MemberOp>(key))
        continue;

      // { ...other }
      if (ref<UnaryOp> unaryop = cast<UnaryOp>(key)) {
        if (unaryop->operation != TokenType::TriplePoint) {
          m_console.error("unexpected operation", key->location());
        }
        continue;
      }

      m_console.error("expected identifier, member access or spread expression", key->location());
      continue;
    }

    if (ref<String> string = cast<String>(key)) {
      key = make<Id>(string->value);
      continue;
    }

    // check key expression
    if (isa<Id>(key) || isa<FormatString>(key))
      continue;

    // check valid '{ [2 + 2]: foo }' key syntax
    if (ref<List> list = cast<List>(key)) {
      if (list->elements.size() != 1) {
        m_console.error("list can only contain a single element", list->location());
      }

      key = make<FormatString>(list->elements.at(0));
      continue;
    }

    m_console.error("expected identifier, string literal, formatstring or '[x]: y' expression", key->location());
  }
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
  std::string& real_type = kTokenTypeStrings[static_cast<uint8_t>(m_token.type)];

  utils::Buffer formatbuf;

  switch (m_token.type) {
    case TokenType::Eof: {
      formatbuf.append_string("unexpected end of file");
      formatbuf.append_string(", ");
      formatbuf.append_string(message);
      break;
    }
    case TokenType::Int:
    case TokenType::Float: {
      formatbuf.append_string("unexpected numerical constant");
      formatbuf.append_string(", ");
      formatbuf.append_string(message);
      break;
    }
    case TokenType::String: {
      formatbuf.append_string("unexpected string literal");
      formatbuf.append_string(", ");
      formatbuf.append_string(message);
      break;
    }
    case TokenType::FormatString: {
      formatbuf.append_string("unexpected format string");
      formatbuf.append_string(", ");
      formatbuf.append_string(message);
      break;
    }
    default: {
      formatbuf.append_string("unexpected '");
      formatbuf.append_string(real_type);
      formatbuf.append_string("' token, ");
      formatbuf.append_string(message);
      break;
    }
  }

  m_console.fatal(formatbuf.buffer_string(), m_token.location);
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
