/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

#include <unordered_map>
#include <unordered_set>

#include "charly/core/compiler/parser.h"

using namespace charly::core::compiler::ast;

namespace charly::core::compiler {

ref<Block> Parser::parse_program(utils::Buffer& source, DiagnosticConsole& console) {
  try {
    Parser parser = Parser(source, console);

    parser.m_keyword_context._return = true;
    parser.m_keyword_context._break = false;
    parser.m_keyword_context._continue = false;
    parser.m_keyword_context._yield = false;
    parser.m_keyword_context._export = true;
    parser.m_keyword_context._super = false;

    return parser.parse_program();
  } catch (DiagnosticException&) { return nullptr; }
}

ref<Statement> Parser::parse_statement(utils::Buffer& source, DiagnosticConsole& console) {
  try {
    Parser parser = Parser(source, console);

    parser.m_keyword_context._return = true;
    parser.m_keyword_context._break = true;
    parser.m_keyword_context._continue = true;
    parser.m_keyword_context._yield = true;
    parser.m_keyword_context._export = true;
    parser.m_keyword_context._super = true;

    return parser.parse_statement();
  } catch (DiagnosticException&) { return nullptr; }
}

ref<Expression> Parser::parse_expression(utils::Buffer& source, DiagnosticConsole& console) {
  try {
    Parser parser = Parser(source, console);

    parser.m_keyword_context._return = true;
    parser.m_keyword_context._break = true;
    parser.m_keyword_context._continue = true;
    parser.m_keyword_context._yield = true;
    parser.m_keyword_context._export = true;
    parser.m_keyword_context._super = true;

    return parser.parse_expression();
  } catch (DiagnosticException&) { return nullptr; }
}

ref<Block> Parser::parse_program() {
  ref<Block> body = make<Block>();
  parse_block_body(body);
  return body;
}

ref<Block> Parser::parse_block() {
  ref<Block> block = make<Block>();
  begin(block);
  eat(TokenType::LeftCurly);

  auto kwcontext = m_keyword_context;
  m_keyword_context._export = false;
  parse_block_body(block);
  m_keyword_context = kwcontext;

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

    if (auto stmt_list = cast<StatementList>(stmt)) {
      for (auto entry : stmt_list->statements) {
        block->statements.push_back(entry);
      }
    } else {
      block->statements.push_back(stmt);
    }

    block->set_end(stmt);

    parsed_statements++;
  }
}

ref<Statement> Parser::parse_block_or_statement() {
  auto kwcontext = m_keyword_context;
  m_keyword_context._export = false;

  ref<Statement> stmt;
  if (type(TokenType::LeftCurly)) {
    stmt = parse_block();
  } else {
    stmt = parse_jump_statement();
  }

  m_keyword_context = kwcontext;

  return stmt;
}

ref<Statement> Parser::parse_statement() {
  ref<Statement> stmt;

  switch (m_token.type) {
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
    case TokenType::Try: {
      stmt = parse_try();
      break;
    }
    case TokenType::Switch: {
      stmt = parse_switch();
      break;
    }
    case TokenType::For: {
      stmt = parse_for();
      break;
    }
    case TokenType::Defer: {
      stmt = parse_defer();
      break;
    }
    case TokenType::Let:
    case TokenType::Const: {
      stmt = parse_declaration();
      break;
    }
    case TokenType::Export: {
      return parse_export();
    }
    case TokenType::Import: {
      return parse_import();
    }
    default: {
      stmt = parse_jump_statement();
      break;
    }
  }

  // wrap function and class literals inside const declaration nodes
  switch (stmt->type()) {
    case Node::Type::Class:
    case Node::Type::Function: {
      ref<Name> variable_name;

      if (ref<Function> func = cast<Function>(stmt)) {
        if (func->arrow_function) {
          break;
        }
        variable_name = func->name;
      }

      if (ref<Class> klass = cast<Class>(stmt)) {
        variable_name = klass->name;
      }

      ref<Declaration> declaration = make<Declaration>(variable_name, cast<Expression>(stmt), true, true);
      declaration->set_location(stmt);
      stmt = declaration;

      break;
    }
    default: {
      break;
    }
  }

  return stmt;
}

ref<Statement> Parser::parse_jump_statement() {
  switch (m_token.type) {
    case TokenType::Return: {
      return parse_return();
    }
    case TokenType::Break: {
      return parse_break();
    }
    case TokenType::Continue: {
      return parse_continue();
    }
    default: {
      return parse_throw_statement();
    }
  }
}

ref<Statement> Parser::parse_throw_statement() {
  switch (m_token.type) {
    case TokenType::Throw: {
      return parse_throw();
    }
    default: {
      return parse_expression();
    }
  }
}

ref<Return> Parser::parse_return() {
  Location begin = m_token.location;
  eat(TokenType::Return);

  bool newline_passed_since_base = begin.end_row != m_token.location.row;

  ref<Return> node;
  if (!newline_passed_since_base && m_token.could_start_expression()) {
    auto exp = parse_expression();
    node = make<Return>(exp);
    node->set_begin(begin);
    node->set_end(exp);
  } else {
    node = make<Return>();
    node->set_location(begin);
  }

  if (!m_keyword_context._return)
    m_console.error(node, "return statement not allowed at this point");

  return node;
}

ref<Break> Parser::parse_break() {
  ref<Break> node = make<Break>();
  at(node);
  eat(TokenType::Break);

  if (!m_keyword_context._break)
    m_console.error(node, "break statement not allowed at this point");

  return node;
}

ref<Continue> Parser::parse_continue() {
  ref<Continue> node = make<Continue>();
  at(node);
  eat(TokenType::Continue);

  if (!m_keyword_context._continue)
    m_console.error(node, "continue statement not allowed at this point");

  return node;
}

ref<TryFinally> Parser::parse_defer() {
  Location begin = m_token.location;
  eat(TokenType::Defer);

  auto kwcontext = m_keyword_context;
  m_keyword_context._return = false;
  m_keyword_context._break = false;
  m_keyword_context._continue = false;
  m_keyword_context._yield = false;
  ref<Block> defer_block = wrap_statement_in_block(parse_block_or_statement());
  m_keyword_context = kwcontext;

  ref<Block> remaining_statement = make<Block>();
  parse_block_body(remaining_statement);

  ref<TryFinally> node = make<TryFinally>(remaining_statement, defer_block);
  node->set_begin(begin);
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

  if (!m_keyword_context._export)
    m_console.error(node, "export statement not allowed at this point");

  return node;
}

ref<If> Parser::parse_if() {
  Location begin = m_token.location;
  eat(TokenType::If);

  ref<Expression> condition = parse_expression();
  ref<Block> then_block = wrap_statement_in_block(parse_block_or_statement());
  ref<Block> else_block = nullptr;

  if (skip(TokenType::Else)) {
    if (type(TokenType::If)) {
      else_block = wrap_statement_in_block(parse_if());
    } else {
      else_block = wrap_statement_in_block(parse_block_or_statement());
    }
  }

  ref<If> node = make<If>(condition, then_block, else_block);
  node->set_begin(begin);
  return node;
}

ref<While> Parser::parse_while() {
  Location begin = m_token.location;
  eat(TokenType::While);

  ref<Expression> condition = parse_expression();

  auto kwcontext = m_keyword_context;
  m_keyword_context._break = true;
  m_keyword_context._continue = true;
  ref<Block> then_block = wrap_statement_in_block(parse_block_or_statement());
  m_keyword_context = kwcontext;

  ref<While> node = make<While>(condition, then_block);
  node->set_begin(begin);
  return node;
}

ref<Loop> Parser::parse_loop() {
  Location begin = m_token.location;
  eat(TokenType::Loop);

  auto kwcontext = m_keyword_context;
  m_keyword_context._break = true;
  m_keyword_context._continue = true;
  ref<Block> then_block = wrap_statement_in_block(parse_block_or_statement());
  m_keyword_context = kwcontext;

  ref<Loop> node = make<Loop>(then_block);
  node->set_begin(begin);
  return node;
}

ref<Statement> Parser::parse_try() {
  Location begin = m_token.location;
  eat(TokenType::Try);

  ref<Statement> stmt = parse_block_or_statement();

  bool no_catch = false;
  bool no_finally = false;

  if (skip(TokenType::Catch)) {
    ref<Name> exception_name;
    if (skip(TokenType::LeftParen)) {
      exception_name = make<Name>(parse_identifier_token());
      eat(TokenType::RightParen);
    } else {
      exception_name = make<Name>("exception");
      exception_name->set_location(begin);
    }

    ref<Block> try_block = wrap_statement_in_block(stmt);
    ref<Block> catch_block = wrap_statement_in_block(parse_block_or_statement());
    stmt = make<Try>(try_block, exception_name, catch_block);
  } else {
    no_catch = true;
  }

  if (skip(TokenType::Finally)) {
    auto kwcontext = m_keyword_context;
    m_keyword_context._return = false;
    m_keyword_context._break = false;
    m_keyword_context._continue = false;
    m_keyword_context._yield = false;
    ref<Block> finally_block = wrap_statement_in_block(parse_block_or_statement());
    m_keyword_context = kwcontext;

    stmt = make<TryFinally>(wrap_statement_in_block(stmt), finally_block);
  } else {
    no_finally = true;
  }

  if (no_catch && no_finally) {
    unexpected_token(TokenType::Catch);
  }

  stmt->set_begin(begin);
  return stmt;
}

ref<Switch> Parser::parse_switch() {
  Location begin = m_token.location;
  eat(TokenType::Switch);

  ref<Switch> node = make<Switch>(parse_expression());
  node->set_begin(begin);

  eat(TokenType::LeftCurly);

  while (!type(TokenType::RightCurly)) {
    Location case_begin = m_token.location;

    switch (m_token.type) {
      case TokenType::Case: {
        advance();
        ref<Expression> case_test = parse_expression();

        auto kwcontext = m_keyword_context;
        m_keyword_context._break = true;
        ref<Block> case_block = wrap_statement_in_block(parse_block_or_statement());
        m_keyword_context = kwcontext;

        ref<SwitchCase> case_node = make<SwitchCase>(case_test, case_block);
        case_node->set_begin(case_begin);
        node->cases.push_back(make<SwitchCase>(case_test, case_block));
        break;
      }
      case TokenType::Default: {
        advance();

        auto kwcontext = m_keyword_context;
        m_keyword_context._break = true;
        ref<Block> block = wrap_statement_in_block(parse_block_or_statement());
        m_keyword_context = kwcontext;
        block->set_begin(case_begin);
        node->default_block = block;
        break;
      }
      default: {
        unexpected_token("case or default");
      }
    }
  }

  end(node);
  eat(TokenType::RightCurly);

  return node;
}

ref<For> Parser::parse_for() {
  Location begin = m_token.location;
  eat(TokenType::For);

  bool constant_value = false;
  if (skip(TokenType::Const)) {
    constant_value = true;
  } else {
    skip(TokenType::Let);
  }

  ref<Expression> target;
  switch (m_token.type) {
    case TokenType::Identifier: {
      target = parse_identifier_token();
      break;
    }
    case TokenType::LeftParen: {
      target = parse_tuple(false);
      break;
    }
    case TokenType::LeftCurly: {
      target = parse_dict();
      break;
    }
    default: {
      unexpected_token("expected identifier or unpack expression");
    }
  }

  eat(TokenType::In);

  ref<Statement> declaration = create_declaration(target, parse_expression(), constant_value);

  auto kwcontext = m_keyword_context;
  m_keyword_context._break = true;
  m_keyword_context._continue = true;
  ref<Block> then_block = wrap_statement_in_block(parse_block_or_statement());
  m_keyword_context = kwcontext;

  ref<For> node = make<For>(declaration, then_block);
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
      target = parse_tuple(false);  // disable paren conversion
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

  // constant declarations require a value
  ref<Expression> value;
  if (requires_assignment) {
    match(TokenType::Assignment);
  }
  if (type(TokenType::Assignment) && m_token.assignment_operator != TokenType::Assignment) {
    m_console.error(m_token.location, "operator assignment is not allowed in this place");
  }
  if (skip(TokenType::Assignment)) {
    value = parse_expression();
  } else {
    value = make<Null>();
    value->set_location(target);
  }

  ref<Statement> node = create_declaration(target, value, const_declaration);
  node->set_begin(begin);
  return node;
}

void Parser::parse_call_arguments(std::list<ref<Expression>>& result) {
  if (!m_token.could_start_expression())
    return;

  do {
    result.push_back(parse_possible_spread_expression());
  } while (skip(TokenType::Comma));
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

  if (!m_keyword_context._yield)
    m_console.error(node, "yield expression not allowed at this point");

  return node;
}

ref<Statement> Parser::parse_import() {
  Location begin = m_token.location;
  eat(TokenType::Import);

  if (skip(TokenType::LeftCurly)) {
    // unpack import statement
    // import { foo [as f] } from libfoo [as foolib]
    std::vector<std::tuple<ref<Name>, ref<Name>>> unpack_elements;
    do {
      auto field = make<Name>(parse_identifier_token());
      ref<Name> field_name;

      if (skip(TokenType::As)) {
        field_name = make<Name>(parse_identifier_token());
      }

      unpack_elements.emplace_back(field, field_name);
    } while (skip(TokenType::Comma));

    eat(TokenType::RightCurly);
    eat(TokenType::From);

    ref<Expression> source;
    ref<Name> name;
    if (skip(TokenType::LeftParen)) {
      source = parse_expression();
      eat(TokenType::RightParen);
    } else {
      source = parse_expression();
      if (auto id = cast<Id>(source)) {
        source = make<String>(id);
        name = make<Name>(id);
      }
    }

    ref<Name> renamed_name;
    if (skip(TokenType::As)) {
      renamed_name = make<Name>(parse_identifier_token());
    }

    // generate unpack assignments
    CHECK(unpack_elements.size());
    auto unpack_dict = make<Dict>();
    for (const auto& entry : unpack_elements) {
      ref<Name> field = std::get<0>(entry);
      unpack_dict->elements.push_back(make<DictEntry>(field));
    }

    auto unpack_target = create_unpack_target(unpack_dict, true);

    auto import = make<Import>(source);
    import->set_begin(begin);

    ref<StatementList> statements;
    if (name && renamed_name) {
      // import {...} from foo as bar
      statements = make<StatementList>(
        make<Declaration>(name, import, true, false),
        make<Declaration>(renamed_name, make<Id>(name), true, false),
        make<UnpackDeclaration>(unpack_target, make<Id>(name), true)
      );
    } else if (name) {
      // import {...} from foo
      statements = make<StatementList>(
        make<Declaration>(name, import, true, false),
        make<UnpackDeclaration>(unpack_target, make<Id>(name), true)
      );
    } else if (renamed_name) {
      // import {...} from "foo" as bar
      statements = make<StatementList>(
        make<Declaration>(renamed_name, import, true, false),
        make<UnpackDeclaration>(unpack_target, make<Id>(renamed_name), true)
      );
    } else {
      // import {...} from "foo"
      statements = make<StatementList>(
        make<UnpackDeclaration>(unpack_target, import, true)
      );
    }

    // emit renamed unpack arguments
    for (auto& entry : unpack_elements) {
      ref<Name> field = std::get<0>(entry);
      ref<Name> field_name = std::get<1>(entry);

      if (field_name) {
        auto rename_decl = make<Declaration>(field_name, make<Id>(field), true, false);
        statements->append_statement(rename_decl);
      }
    }

    return statements;
  } else {
    // regular import statement
    // import foo
    // import "foo"
    // import foo as bar
    // import "foo" as bar
    ref<Expression> source;
    ref<Name> name;
    if (skip(TokenType::LeftParen)) {
      source = parse_expression();
      eat(TokenType::RightParen);
    } else {
      source = parse_expression();
      if (auto id = cast<Id>(source)) {
        source = make<String>(id);
        name = make<Name>(id);
      }
    }

    ref<Name> renamed_name;
    if (skip(TokenType::As)) {
      renamed_name = make<Name>(parse_identifier_token());
    }

    auto import = make<Import>(source);
    import->set_begin(begin);

    if (name && renamed_name) {
      return make<StatementList>(
        make<Declaration>(name, import, true, false),
        make<Declaration>(renamed_name, make<Id>(name), true, false)
      );
    } else if (name) {
      return make<Declaration>(name, import, true, false);
    } else if (renamed_name) {
      return make<Declaration>(renamed_name, import, true, false);
    } else {
      return import;
    }
  }
}

ref<Expression> Parser::parse_import_expression() {
  Location begin = m_token.location;
  eat(TokenType::Import);
  auto source = parse_expression();
  auto import = make<Import>(source);
  import->set_begin(begin);
  return import;
}

ref<Expression> Parser::parse_assignment() {
  ref<Expression> target = parse_ternary();

  if (type(TokenType::Assignment)) {
    TokenType assignment_operator = m_token.assignment_operator;
    eat(TokenType::Assignment);
    ref<Expression> source = parse_expression();

    switch (target->type()) {
      case Node::Type::Tuple:
      case Node::Type::Dict: {
        ref<UnpackTarget> unpack_target = create_unpack_target(target);

        if (assignment_operator != TokenType::Assignment) {
          m_console.error(target, "cannot use operator assignment when assigning to an unpack target");
          break;
        }

        return make<Assignment>(TokenType::Assignment, unpack_target, source);
      }
      default: {
        if (is_assignable(target)) {
          return make<Assignment>(assignment_operator, target, source);
        } else {
          m_console.error(target, "left-hand side of assignment cannot be assigned to");
        }
      }
    }
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

ref<Expression> Parser::parse_binaryop() {
  return parse_binaryop_1(parse_unaryop(), 0);
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

ref<Expression> Parser::parse_possible_spread_expression() {
  if (type(TokenType::TriplePoint)) {
    Location start_loc = m_token.location;
    advance();
    ref<Spread> op = make<Spread>(parse_call_member_index());
    op->set_begin(start_loc);
    return op;
  }

  return parse_expression();
}

ref<Expression> Parser::parse_unaryop() {
  if (m_token.is_unary_operator()) {
    TokenType operation = m_token.type;
    Location start_loc = m_token.location;
    advance();

    // +exp is always just exp
    if (operation == TokenType::Plus) {
      return parse_unaryop();
    }

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
  auto kwcontext = m_keyword_context;
  m_keyword_context._return = true;
  m_keyword_context._break = false;
  m_keyword_context._continue = false;
  m_keyword_context._yield = true;
  ref<Statement> stmt = parse_block_or_statement();
  m_keyword_context = kwcontext;

  // wrap non-call statements in a block
  if (!isa<Block>(stmt) && !isa<CallOp>(stmt)) {
    m_console.fatal(stmt, "expected block or call operation");
  }

  ref<Spawn> node = make<Spawn>(stmt);
  node->set_begin(begin);
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

ref<CallOp> Parser::parse_call(const ref<Expression>& target) {
  eat(TokenType::LeftParen);
  ref<CallOp> callop = make<CallOp>(target);
  parse_call_arguments(callop->arguments);
  end(callop);
  eat(TokenType::RightParen);
  return callop;
}

ref<MemberOp> Parser::parse_member(const ref<Expression>& target) {
  eat(TokenType::Point);
  return make<MemberOp>(target, make<Name>(parse_identifier_token()));
}

ref<IndexOp> Parser::parse_index(const ref<Expression>& target) {
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
    case TokenType::AtSign: {
      ref<Self> self = make<Self>();
      self->set_location(m_token.location);
      advance();
      return make<MemberOp>(self, make<Name>(parse_identifier_token()));
    }
    case TokenType::Identifier: {
      return parse_identifier_token();
    }
    case TokenType::String: {
      return parse_string_token();
    }
    case TokenType::FormatString: {
      return parse_format_string();
    }
    case TokenType::RightArrow: {
      return parse_arrow_function();
    }
    case TokenType::Func: {
      return parse_function();
    }
    case TokenType::Final:
    case TokenType::Class: {
      return parse_class();
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
      return parse_super();
    }
    case TokenType::Builtin: {
      return parse_builtin();
    }
    default: {
      unexpected_token("expected an expression");
    }
  }
}

ref<Expression> Parser::parse_builtin() {
  Location begin = m_token.location;
  eat(TokenType::Builtin);
  eat(TokenType::LeftParen);

  ref<String> name_str = parse_string_token();
  ref<Name> name = make<Name>(name_str->value);

  // check if this is a valid builtin name
  if (ir::kBuiltinNameMapping.count(name->value) == 0) {
    // eat remaining closing paren
    eat(TokenType::RightParen);

    m_console.error(name_str, "unknown builtin operation '", name->value, "'");
    return make<Null>();
  }

  ref<BuiltinOperation> builtin = make<BuiltinOperation>(name);
  builtin->set_begin(begin);
  while (skip(TokenType::Comma)) {
    ref<Expression> exp = parse_expression();
    builtin->arguments.push_back(exp);
  }

  // eat remaining closing paren
  builtin->set_end(m_token.location);
  eat(TokenType::RightParen);

  // check for correct amount of arguments
  int32_t got = builtin->arguments.size();
  int32_t expected = ir::kBuiltinArgumentCount[(uint32_t)(builtin->operation)];
  if (expected != -1 && got != expected) {
    m_console.error(builtin, "incorrect amount of arguments. expected ", expected, ", got ", got);
    return make<Null>();
  }

  return builtin;
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

    ref<String> string_element = parse_string_token();
    format_string->set_end(string_element);

    if (string_element->value.size() > 0)
      format_string->elements.push_back(string_element);

    if (final_element)
      return format_string;
  }
}

ref<Expression> Parser::parse_tuple(bool paren_conversion) {
  ref<Tuple> tuple = make<Tuple>();
  begin(tuple);

  eat(TokenType::LeftParen);

  do {
    // parse the empty tuple ()
    if (type(TokenType::RightParen)) {
      if (tuple->elements.size() <= 1) {
        break;
      }
    }

    ref<Expression> exp = parse_possible_spread_expression();

    // (x) is parsed as just x
    // if x is a spread expression (...x) then the tuple
    // shouldn't be omitted
    if (paren_conversion && tuple->elements.size() == 0 && type(TokenType::RightParen)) {
      if (!isa<Spread>(exp)) {
        advance();
        return exp;
      }
    }

    tuple->elements.push_back(exp);
  } while (skip(TokenType::Comma));

  end(tuple);
  eat(TokenType::RightParen);

  return tuple;
}

ref<List> Parser::parse_list() {
  ref<List> list = make<List>();
  begin(list);

  eat(TokenType::LeftBracket);

  if (!type(TokenType::RightBracket)) {
    do {
      list->elements.push_back(parse_possible_spread_expression());
    } while (skip(TokenType::Comma));
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
      ref<Expression> key = parse_possible_spread_expression();
      ref<Expression> value = nullptr;

      if (ref<Id> id = cast<Id>(key)) {
        key = make<Name>(id);
      }

      if (skip(TokenType::Colon)) {
        value = parse_expression();
      }

      dict->elements.push_back(make<DictEntry>(key, value));
    } while (skip(TokenType::Comma));
  }

  end(dict);
  eat(TokenType::RightCurly);

  return dict;
}

ref<Function> Parser::parse_function(FunctionFlags flags) {
  if (!flags.class_function && type(TokenType::RightArrow)) {
    return parse_arrow_function();
  }

  Location begin = m_token.location;
  eat(TokenType::Func);

  // function name
  ref<Name> function_name = make<Name>(parse_identifier_token());

  // argument list
  std::list<ref<FunctionArgument>> argument_list;
  parse_function_arguments(argument_list, flags);

  ref<Statement> body;

  auto kwcontext = m_keyword_context;
  m_keyword_context._return = true;
  m_keyword_context._break = false;
  m_keyword_context._continue = false;

  // only class functions may contain super statements
  // yield statements are allowed in all functions except class constructors
  if (flags.class_function) {
    // constructors cannot yield
    if (function_name->value.compare("constructor") == 0) {
      m_keyword_context._yield = false;
    } else {
      m_keyword_context._yield = true;
    }

    // static class functions cannot contain super references
    if (flags.static_function) {
      m_keyword_context._super = false;
    } else {
      m_keyword_context._super = true;
    }
  } else {
    m_keyword_context._yield = true;
    m_keyword_context._super = false;
  }

  m_keyword_context._export = false;
  if (type(TokenType::Assignment)) {
    if (m_token.assignment_operator != TokenType::Assignment) {
      m_console.error(m_token.location, "operator assignment is not allowed in this place");
    }
    eat(TokenType::Assignment);
    body = parse_throw_statement();
  } else if (type(TokenType::LeftCurly)) {
    body = parse_block();
  } else {
    // allow foo(@x, @y) declarations inside classes
    if (flags.class_function) {
      body = make<Return>();
    } else {
      unexpected_token(TokenType::LeftCurly);
    }
  }
  m_keyword_context = kwcontext;

  if (isa<Expression>(body)) {
    body = make<Block>(cast<Expression>(body));
  }

  ref<Function> node = make<Function>(false, function_name, wrap_statement_in_block(body), argument_list);
  node->set_begin(begin);
  return node;
}

ref<Function> Parser::parse_arrow_function() {
  Location begin = m_token.location;
  eat(TokenType::RightArrow);

  // argument list
  std::list<ref<FunctionArgument>> argument_list;
  parse_function_arguments(argument_list);

  ref<Statement> body;

  auto kwcontext = m_keyword_context;
  m_keyword_context._return = true;
  m_keyword_context._break = false;
  m_keyword_context._continue = false;
  m_keyword_context._export = false;
  if (type(TokenType::LeftCurly)) {
    body = parse_block();
  } else {
    body = parse_jump_statement();
  }
  m_keyword_context = kwcontext;

  if (isa<Expression>(body)) {
    body = make<Return>(cast<Expression>(body));
  }

  ref<Function> node = make<Function>(true, make<Name>("anonymous"), wrap_statement_in_block(body), argument_list);
  node->set_begin(begin);
  return node;
}

void Parser::parse_function_arguments(std::list<ref<FunctionArgument>>& result, FunctionFlags flags) {
  if (skip(TokenType::LeftParen)) {
    if (!type(TokenType::RightParen)) {
      do {
        bool self_initializer = false;    // func foo(@a)
        bool spread_initializer = false;  // func foo(...a)

        // store the location of a potential accessor token
        // and set the correct flag
        Location accessor_token_location = m_token.location;

        if (m_token.type == TokenType::TriplePoint) {
          advance();
          spread_initializer = true;
        }

        if (m_token.type == TokenType::AtSign) {
          // the (@x, @y) syntax is only allowed inside
          // class constructors or member functions
          if (!(flags.class_function && !flags.static_function)) {
            unexpected_token(
              "self initializer arguments are only allowed inside class constructors or member functions");
          }

          advance();
          self_initializer = true;
        }

        ref<Name> name = make<Name>(parse_identifier_token());

        Location argument_location = name->location();
        argument_location.set_begin(accessor_token_location);

        ref<Expression> default_value = nullptr;
        if (type(TokenType::Assignment) && m_token.assignment_operator != TokenType::Assignment) {
          m_console.error(m_token.location, "operator assignment is not allowed in this place");
        }
        if (skip(TokenType::Assignment)) {
          default_value = parse_expression();
          argument_location.set_end(default_value->location());
        }

        ref<FunctionArgument> argument =
          make<FunctionArgument>(self_initializer, spread_initializer, name, default_value);
        argument->set_begin(argument_location);

        result.emplace_back(argument);
      } while (skip(TokenType::Comma));
    }

    eat(TokenType::RightParen);
  }
}

ref<Class> Parser::parse_class() {
  Location begin = m_token.location;

  bool is_final = skip(TokenType::Final);
  eat(TokenType::Class);

  // class name
  ref<Name> class_name = make<Name>(parse_identifier_token());

  ref<Expression> parent;
  if (skip(TokenType::Extends)) {
    parent = parse_expression();
  }

  ref<Class> node = make<Class>(class_name, parent);
  node->is_final = is_final;
  node->set_begin(begin);

  // parse class body
  eat(TokenType::LeftCurly);

  std::unordered_set<std::string> class_static_fields;
  std::unordered_set<std::string> class_member_fields;

  while (!type(TokenType::RightCurly)) {
    bool is_static = false;
    bool is_private = false;

    if (skip(TokenType::Private))
      is_private = true;

    if (skip(TokenType::Static))
      is_static = true;

    if (type(TokenType::Property)) {
      Location property_token_location = m_token.location;
      advance();

      ref<Name> name = make<Name>(parse_identifier_token());

      ref<Expression> value;
      if (type(TokenType::Assignment) && m_token.assignment_operator != TokenType::Assignment) {
        m_console.error(m_token.location, "operator assignment is not allowed in this place");
      }
      if (skip(TokenType::Assignment)) {
        value = parse_expression();
      } else {
        value = make<Null>();
        value->set_location(name);
      }

      auto prop = make<ClassProperty>(is_static, is_private, name, value);
      prop->set_begin(property_token_location);
      if (is_static) {
        node->static_properties.push_back(prop);
      } else {
        node->member_properties.push_back(prop);
      }
    } else {
      FunctionFlags flags;
      flags.class_function = true;
      flags.static_function = is_static;
      ref<Function> function = parse_function(flags);
      function->class_private_function = is_private;
      function->host_class = node;

      if (is_static) {
        function->class_static_function = true;
        node->static_functions.push_back(function);
      } else {
        if (function->name->value.compare("constructor") == 0) {
          if (is_private) {
            m_console.error(function, "class constructors cannot be private");
          }

          if (node->constructor) {
            m_console.error(function, "duplicate declaration of class constructor");
            m_console.info(node->constructor, "previously declared here");
          } else {
            node->constructor = function;
            function->class_constructor = true;
          }
        } else {
          function->class_member_function = true;
          node->member_functions.push_back(function);
        }
      }
    }
  }

  // subclasses that define new properties need
  // a user-defined constructor
  if (node->parent && node->member_properties.size() && !node->constructor) {
    m_console.error(node->name, "class '", node->name->value, "' is missing a constructor");
  }

  end(node);
  eat(TokenType::RightCurly);

  return node;
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

ref<Expression> Parser::parse_super() {
  match(TokenType::Super);
  ref<Super> node = make<Super>();
  at(node);

  if (!m_keyword_context._super)
    m_console.error(node, "super is not allowed at this point");

  advance();

  // parse either a call or callmember expression
  if (type(TokenType::LeftParen)) {
    return parse_call(node);
  } else if (type(TokenType::Point)) {
    advance();
    auto id = parse_identifier_token();
    auto memberop = make<MemberOp>(node, id->value);
    memberop->set_begin(node);
    memberop->set_end(id);

    if (type(TokenType::LeftParen)) {
      return parse_call(memberop);
    }
  }

  m_console.error(node, "super must be used as part of a call operation");
  return node;
}

ref<Statement> Parser::create_declaration(const ref<Expression>& target, const ref<Expression>& value, bool constant) {
  switch (target->type()) {
    case Node::Type::Id: {
      return make<Declaration>(make<Name>(cast<Id>(target)), value, constant);
    }
    case Node::Type::Tuple:
    case Node::Type::Dict: {
      return make<UnpackDeclaration>(create_unpack_target(target, true), value, constant);
    }
    default: {
      FAIL("unexpected node type");
    }
  }
}

ref<UnpackTarget> Parser::create_unpack_target(const ref<Expression>& node, bool declaration) {
  switch (node->type()) {
    case Node::Type::Tuple: {
      ref<Tuple> tuple = cast<Tuple>(node);
      ref<UnpackTarget> target = make<UnpackTarget>(false);

      if (tuple->elements.size() == 0) {
        m_console.error(tuple, "empty unpack target");
        break;
      }

      for (const ref<Expression>& member : tuple->elements) {
        auto expression = member;
        bool is_spread = false;
        if (auto spread = cast<Spread>(member)) {
          expression = spread->expression;
          is_spread = true;
        }

        if (declaration) {
          if (auto id = cast<Id>(expression)) {
            target->elements.push_back(make<UnpackTargetElement>(id, is_spread));
          } else {
            m_console.error(expression, "expected an identifier or spread");
          }
        } else {
          if (is_valid_unpack_target_element(expression)) {
            target->elements.push_back(make<UnpackTargetElement>(expression, is_spread));
          } else {
            m_console.error(expression, "expected a valid unpack target expression");
          }
        }
      }

      target->set_location(node);
      return target;
    }
    case Node::Type::Dict: {
      ref<Dict> dict = cast<Dict>(node);
      ref<UnpackTarget> target = make<UnpackTarget>(true);

      if (dict->elements.size() == 0) {
        m_console.error(dict, "empty unpack target");
        break;
      }

      for (const ref<DictEntry>& entry : dict->elements) {
        if (entry->value) {
          m_console.error(entry->value, "dict used as unpack target must not contain any values");
          break;
        }

        if (ref<Name> key_name = cast<Name>(entry->key)) {
          target->elements.push_back(make<UnpackTargetElement>(make<Id>(key_name), false));
        } else if (ref<Spread> spread = cast<Spread>(entry->key)) {
          if (ref<Id> id = cast<Id>(spread->expression)) {
            auto element = make<UnpackTargetElement>(id, true);
            element->set_begin(spread);
            target->elements.push_back(element);
          } else {
            m_console.error(spread->expression, "expected an identifier");
          }
        } else {
          m_console.error(entry->key, "expected an identifier or spread");
          break;
        }
      }

      target->set_location(node);
      return target;
    }
    default: {
      FAIL("unexpected node type");
    }
  }

  // a dummy UnpackTarget node can be returned here since
  // we don't care about the AST if any errors have been generated
  return make<UnpackTarget>(false);
}

bool Parser::is_assignable(const ref<Expression>& expression) {
  switch (expression->type()) {
    case Node::Type::Id:
    case Node::Type::MemberOp:
    case Node::Type::IndexOp:
    case Node::Type::UnpackTarget: return true;
    default: return false;
  }
}

bool Parser::is_valid_unpack_target_element(const ref<Expression>& expression) {
  switch (expression->type()) {
    case Node::Type::Id:
    case Node::Type::MemberOp: return true;
    default: return false;
  }
}

ref<Block> Parser::wrap_statement_in_block(const ref<Statement>& node) {
  if (isa<Block>(node)) {
    return cast<Block>(node);
  }

  return make<Block>(node);
}

void Parser::unexpected_token(const std::string& message) {
  std::string& real_type = kTokenTypeStrings[static_cast<uint8_t>(m_token.type)];

  switch (m_token.type) {
    case TokenType::Eof: {
      m_console.fatal(m_token.location, "unexpected end of file, ", message);
    }
    case TokenType::Int:
    case TokenType::Float: {
      m_console.fatal(m_token.location, "unexpected numerical constant, ", message);
    }
    case TokenType::String: {
      m_console.fatal(m_token.location, "unexpected string literal, ", message);
    }
    case TokenType::FormatString: {
      m_console.fatal(m_token.location, "unexpected format string, ", message);
    }
    default: {
      m_console.fatal(m_token.location, "unexpected '", real_type, "' token, ", message);
    }
  }
}

void Parser::unexpected_token(TokenType expected) {
  std::string& real_type = kTokenTypeStrings[static_cast<uint8_t>(m_token.type)];
  std::string& expected_type = kTokenTypeStrings[static_cast<uint8_t>(expected)];

  switch (m_token.type) {
    case TokenType::Eof: {
      m_console.fatal(m_token.location, "unexpected end of file, expected a '", expected_type, "' token");
    }
    case TokenType::Int:
    case TokenType::Float: {
      m_console.fatal(m_token.location, "unexpected numerical constant, expected a '", expected_type, "' token");
    }
    case TokenType::String: {
      m_console.fatal(m_token.location, "unexpected string literal, expected a '", expected_type, "' token");
    }
    case TokenType::FormatString: {
      m_console.fatal(m_token.location, "unexpected format string, expected a '", expected_type, "' token");
    }
    default: {
      m_console.fatal(m_token.location, "unexpected '", real_type, "' token, expected a '", expected_type, "' token");
    }
  }
}

}  // namespace charly::core::compiler
