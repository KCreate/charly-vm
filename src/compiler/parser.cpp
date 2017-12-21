/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include <stack>

#include "parser.h"

namespace Charly::Compilation {
ParseResult* Parser::parse() {
  this->advance();
  AST::AbstractNode* tree = this->parse_program();
  ParseResult* program = new ParseResult(this->source.filename, this->tokens, tree);
  return program;
}

Token& Parser::advance() {
  this->read_token();
  return this->token;
}

void Parser::advance_to_token(TokenType type) {
  std::stack<TokenType> token_stack;

  while ((this->token.type == type && token_stack.size() == 0)) {
    switch (this->token.type) {
      case TokenType::LeftCurly:
      case TokenType::LeftParen:
      case TokenType::LeftBracket: {
        token_stack.push(this->token.type);
        break;
      }
      case TokenType::RightCurly: {
        if (token_stack.size() > 0 && token_stack.top() == TokenType::LeftCurly) {
          token_stack.pop();
        } else {
          this->unexpected_token(this->token.type);
        }
        break;
      }
      case TokenType::RightParen: {
        if (token_stack.size() > 0 && token_stack.top() == TokenType::LeftParen) {
          token_stack.pop();
        } else {
          this->unexpected_token(this->token.type);
        }
        break;
      }
      case TokenType::RightBracket: {
        if (token_stack.size() > 0 && token_stack.top() == TokenType::LeftBracket) {
          token_stack.pop();
        } else {
          this->unexpected_token(this->token.type);
        }
        break;
      }
      default: { break; }
    }

    this->advance();
  }
}

void Parser::unexpected_token() {
  std::string error_message;

  if (this->token.type == TokenType::Eof) {
    error_message = "Unexpected end of file";
  } else {
    error_message = "Unexpected ";
    error_message.append(kTokenTypeStrings[this->token.type]);
  }

  throw SyntaxError(this->token.location, error_message);
}

void Parser::unexpected_token(TokenType expected) {
  std::string error_message;

  if (this->token.type == TokenType::Eof) {
    error_message = "Unexpected end of file, expected ";
    error_message.append(kTokenTypeStrings[expected]);
  } else {
    error_message = "Unexpected ";
    error_message.append(kTokenTypeStrings[this->token.type]);
    error_message.append(", expected ");
    error_message.append(kTokenTypeStrings[expected]);
  }

  throw SyntaxError(this->token.location, error_message);
}

void Parser::unexpected_token(const std::string& expected_value) {
  std::string error_message;

  if (this->token.type == TokenType::Eof) {
    error_message = "Unexpected end of file, expected ";
    error_message.append(expected_value);
  } else {
    error_message = "Expected ";
    error_message.append(expected_value);
    error_message.append(", got ");
    error_message.append(kTokenTypeStrings[this->token.type]);
  }

  throw SyntaxError(this->token.location, error_message);
}

void Parser::illegal_token() {
  throw SyntaxError(this->token.location, "This token is not allowed at this location");
}

void Parser::illegal_token(const std::string& message) {
  throw SyntaxError(this->token.location, message);
}

void Parser::illegal_node(AST::AbstractNode* node, const std::string& message) {
  throw SyntaxError(node->location_start.value_or(Location(0, 0, 0, 0, "<unknown>")), message);
}

void Parser::assert_token(TokenType type) {
  if (this->token.type != type) {
    this->unexpected_token(type);
  }
}

void Parser::expect_token(TokenType type) {
  if (this->token.type != type) {
    this->unexpected_token(type);
  }

  this->advance();
}

void Parser::expect_token(TokenType type, ParseFunc func) {
  if (this->token.type != type) {
    this->unexpected_token(type);
  }

  func();
  this->advance();
}

void Parser::skip_token(TokenType type) {
  if (this->token.type == type)
    this->advance();
}

void Parser::if_token(TokenType type, ParseFunc func) {
  if (this->token.type == type)
    func();
}

AST::AbstractNode* Parser::parse_program() {
  AST::Block* block = new AST::Block();

  while (this->token.type != TokenType::Eof) {
    block->append_node(this->parse_statement());
  }

  return block;
}

AST::AbstractNode* Parser::parse_block() {
  AST::Block* block = new AST::Block();

  std::optional<Location> location_start;
  std::optional<Location> end_location;

  this->expect_token(TokenType::LeftCurly, [&]() { location_start = this->token.location; });

  bool add_to_block = true;
  while (this->token.type != TokenType::RightCurly) {
    AST::AbstractNode* node = this->parse_statement();

    if (add_to_block && !node->is_literal()) {
      block->append_node(node);

      if (node->type() == AST::kTypeReturn || node->type() == AST::kTypeBreak || node->type() == AST::kTypeContinue) {
        add_to_block = false;
      }
    } else {
      delete node;
    }
  }

  this->expect_token(TokenType::RightCurly, [&]() { end_location = this->token.location; });

  return block->at(location_start, end_location);
}

AST::AbstractNode* Parser::parse_statement() {
  Location location_start = this->token.location;

  switch (this->token.type) {
    case TokenType::Let: {
      this->advance();
      switch (this->token.type) {
        case TokenType::Identifier: {
          std::string name = this->token.value;
          Location ident_location = this->token.location;

          this->advance();
          switch (this->token.type) {
            case TokenType::Semicolon: {
              AST::Null* exp = new AST::Null();
              exp->at(location_start, ident_location);

              this->advance();
              return (new AST::LocalInitialisation(name, exp, false))->at(location_start, ident_location);
              break;
            }
            case TokenType::Assignment: {
              this->advance();
              AST::AbstractNode* exp = this->parse_expression();
              this->assign_default_name(exp, name);

              this->skip_token(TokenType::Semicolon);
              return (new AST::LocalInitialisation(name, exp, false))->at(location_start, exp->location_end);
              break;
            }
            default: {
              AST::Null* exp = new AST::Null();
              return (new AST::LocalInitialisation(name, exp, false))->at(location_start, ident_location);
            }
          }
          break;
        }
      }
      break;
    }
    case TokenType::Const: {
      this->advance();
      this->assert_token(TokenType::Identifier);

      std::string identifier = this->token.value;

      this->advance();
      this->expect_token(TokenType::Assignment);

      AST::AbstractNode* exp = this->parse_expression();
      this->assign_default_name(exp, identifier);

      this->skip_token(TokenType::Semicolon);
      return (new AST::LocalInitialisation(identifier, exp, true))->at(location_start, exp->location_end);
    }
    case TokenType::If: {
      return this->parse_if_statement();
    }
    case TokenType::Unless: {
      return this->parse_unless_statement();
    }
    case TokenType::Guard: {
      return this->parse_guard_statement();
    }
    case TokenType::While: {
      return this->parse_while_statement();
    }
    case TokenType::Until: {
      return this->parse_until_statement();
    }
    case TokenType::Loop: {
      return this->parse_loop_statement();
    }
    case TokenType::Try: {
      return this->parse_try_statement();
    }
    case TokenType::Switch: {
      return this->parse_switch_statement();
    }
    default: { return this->parse_control_statement(); }
  }

  this->unexpected_token();
  return nullptr;
}

AST::AbstractNode* Parser::parse_control_statement() {
  Location location_start = this->token.location;

  switch (this->token.type) {
    case TokenType::Return: {
      Location location_start = this->token.location;

      // Check if return is allowed at this position
      if (!this->keyword_context.return_allowed) {
        this->illegal_token();
      }

      this->advance();

      AST::AbstractNode* exp;
      std::optional<Location> location_end;

      if ((this->token.type != TokenType::Semicolon) && (this->token.type != TokenType::RightCurly) &&
          (this->token.type != TokenType::Eof)) {
        exp = this->parse_expression();
        location_end = exp->location_end;
      } else {
        exp = (new AST::Null())->at(location_start);
        location_end = location_start;
      }

      this->skip_token(TokenType::Semicolon);
      return (new AST::Return(exp))->at(location_start, location_end);
    }
    case TokenType::Break: {
      Location location_start = this->token.location;

      // Check if break is allowed at this position
      if (!this->keyword_context.break_allowed) {
        this->illegal_token();
      }

      this->advance();
      this->skip_token(TokenType::Semicolon);
      return (new AST::Break())->at(location_start);
    }
    case TokenType::Continue: {
      Location location_start = this->token.location;

      // Check if continue is allowed at this position
      if (!this->keyword_context.continue_allowed) {
        this->illegal_token();
      }

      this->advance();
      this->skip_token(TokenType::Semicolon);
      return (new AST::Continue())->at(location_start);
    }
    case TokenType::Throw: {
      Location location_start = this->token.location;
      this->advance();
      AST::AbstractNode* exp = this->parse_expression();
      this->skip_token(TokenType::Semicolon);
      return (new AST::Throw(exp))->at(location_start, exp->location_end);
    }
    default: {
      AST::AbstractNode* exp = this->parse_expression();

      // Generate local initialisations for function and class nodes
      if (exp->type() == AST::kTypeFunction || exp->type() == AST::kTypeClass) {
        // Read the name of the node
        std::string name;
        if (exp->type() == AST::kTypeFunction) {
          name = AST::cast<AST::Function>(exp)->name;
        }
        if (exp->type() == AST::kTypeClass) {
          name = AST::cast<AST::Class>(exp)->name;
        }

        // Wrap the node in a local initialisation if it has a name
        if (name.size() > 0) {
          exp = (new AST::LocalInitialisation(name, exp, true))->at(exp);
        }
      }

      this->skip_token(TokenType::Semicolon);
      return exp;
    }
  }
}

AST::AbstractNode* Parser::parse_if_statement() {
  Location location_start = this->token.location;
  this->expect_token(TokenType::If);

  AST::AbstractNode* test;

  // The parens around the test are optional
  if (this->token.type == TokenType::LeftParen) {
    this->advance();
    test = this->parse_expression();
    this->expect_token(TokenType::RightParen);
  } else {
    test = this->parse_expression();
  }

  AST::AbstractNode* then_node;

  if (this->token.type == TokenType::LeftCurly) {
    then_node = this->parse_block();
  } else {
    then_node = this->parse_control_statement();
    this->skip_token(TokenType::Semicolon);
    return (new AST::If(test, then_node))->at(location_start, then_node->location_end);
  }

  AST::AbstractNode* else_node;

  if (this->token.type == TokenType::Else) {
    this->advance();

    if (this->token.type == TokenType::If) {
      else_node = this->parse_if_statement();
    } else {
      if (this->token.type == TokenType::LeftCurly) {
        else_node = this->parse_block();
      } else {
        else_node = this->parse_control_statement();
        this->skip_token(TokenType::Semicolon);
      }
    }

    return (new AST::IfElse(test, then_node, else_node))->at(location_start, else_node->location_end);
  } else {
    return (new AST::If(test, then_node))->at(location_start, then_node->location_end);
  }
}

AST::AbstractNode* Parser::parse_unless_statement() {
  Location location_start = this->token.location;
  this->expect_token(TokenType::Unless);

  AST::AbstractNode* test;

  // The parens around the test are optional
  if (this->token.type == TokenType::LeftParen) {
    this->advance();
    test = this->parse_expression();
    this->expect_token(TokenType::RightParen);
  } else {
    test = this->parse_expression();
  }

  AST::AbstractNode* then_node;

  if (this->token.type == TokenType::LeftCurly) {
    then_node = this->parse_block();
  } else {
    then_node = this->parse_control_statement();
    this->skip_token(TokenType::Semicolon);
    return (new AST::Unless(test, then_node))->at(location_start, then_node->location_end);
  }

  AST::AbstractNode* else_node;

  // Unless nodes are not allowed to have else if alternative blocks
  // as that would be a way to create really messy code
  if (this->token.type == TokenType::Else) {
    this->advance();

    if (this->token.type == TokenType::LeftCurly) {
      else_node = this->parse_block();
    } else {
      else_node = this->parse_control_statement();
      this->skip_token(TokenType::Semicolon);
    }

    return (new AST::UnlessElse(test, then_node, else_node))->at(location_start, else_node->location_end);
  } else {
    return (new AST::Unless(test, then_node))->at(location_start, then_node->location_end);
  }
}

AST::AbstractNode* Parser::parse_guard_statement() {
  Location location_start = this->token.location;
  this->expect_token(TokenType::Guard);

  AST::AbstractNode* test;

  // The parens around the test are optional
  if (this->token.type == TokenType::LeftParen) {
    this->advance();
    test = this->parse_expression();
    this->expect_token(TokenType::RightParen);
  } else {
    test = this->parse_expression();
  }

  AST::AbstractNode* block;

  if (this->token.type == TokenType::LeftCurly) {
    block = this->parse_block();
  } else {
    block = this->parse_control_statement();
    this->skip_token(TokenType::Semicolon);
  }

  return (new AST::Guard(test, block))->at(location_start, block->location_end);
}

AST::AbstractNode* Parser::parse_switch_statement() {
  Location location_start = this->token.location;
  this->expect_token(TokenType::Switch);

  AST::AbstractNode* condition = nullptr;
  AST::NodeList* cases = new AST::NodeList();
  AST::AbstractNode* default_block = nullptr;

  if (this->token.type == TokenType::LeftParen) {
    this->advance();
    condition = this->parse_expression();
    this->expect_token(TokenType::RightParen);
  } else {
    condition = this->parse_expression();
  }

  std::optional<Location> location_end;
  auto backup_context = this->keyword_context;
  this->keyword_context.break_allowed = true;

  this->expect_token(TokenType::LeftCurly);
  while (this->token.type != TokenType::RightCurly) {
    AST::AbstractNode* node = this->parse_switch_node();

    if (node->type() == AST::kTypeSwitchNode) {
      cases->append_node(node);
    } else {
      // Check if we already got a default block
      if (default_block) {
        this->illegal_node(node, "Duplicate default block");
      }

      default_block = node;
    }
  }
  this->expect_token(TokenType::RightCurly);

  this->keyword_context = backup_context;

  if (default_block == nullptr) {
    default_block = new AST::Empty();
  }

  return (new AST::Switch(condition, cases, default_block))->at(location_start, location_end);
}

AST::AbstractNode* Parser::parse_switch_node() {
  Location location_start = this->token.location;

  switch (this->token.type) {
    case TokenType::Case: {
      this->advance();

      AST::NodeList* cases = new AST::NodeList();
      AST::AbstractNode* block;

      if (this->token.type == TokenType::LeftParen) {
        this->advance();

        // There has to be at least one expression
        cases->append_node(this->parse_expression());

        // Parse more expressions if there are any
        while (this->token.type == TokenType::Comma) {
          this->advance();
          cases->append_node(this->parse_expression());
        }

        this->expect_token(TokenType::RightParen);
      } else {
        // There has to be at least one expression
        cases->append_node(this->parse_expression());

        // Parse more expressions if there are any
        while (this->token.type == TokenType::Comma) {
          this->advance();
          cases->append_node(this->parse_expression());
        }
      }

      if (this->token.type == TokenType::LeftCurly) {
        block = this->parse_block();
      } else {
        block = this->parse_control_statement();
        this->skip_token(TokenType::Semicolon);
      }

      return (new AST::SwitchNode(cases, block))->at(location_start, block->location_end);
    }
    case TokenType::Default: {
      this->advance();

      AST::AbstractNode* block;

      if (this->token.type == TokenType::LeftCurly) {
        block = this->parse_block();
      } else {
        block = this->parse_control_statement();
        this->skip_token(TokenType::Semicolon);
      }

      return block;
    }
    default: {
      this->unexpected_token("case or default");
      return nullptr;
    }
  }
}

AST::AbstractNode* Parser::parse_while_statement() {
  Location location_start = this->token.location;
  this->expect_token(TokenType::While);

  AST::AbstractNode* test;

  if (this->token.type == TokenType::LeftParen) {
    this->advance();
    test = this->parse_expression();
    this->expect_token(TokenType::RightParen);
  } else {
    test = this->parse_expression();
  }

  auto context_backup = this->keyword_context;
  this->keyword_context.break_allowed = true;
  this->keyword_context.continue_allowed = true;

  AST::AbstractNode* then_block;

  if (this->token.type == TokenType::LeftCurly) {
    then_block = this->parse_block();
  } else {
    then_block = this->parse_control_statement();
    this->skip_token(TokenType::Semicolon);
  }

  this->keyword_context = context_backup;
  return (new AST::While(test, then_block))->at(location_start, then_block->location_end);
}

AST::AbstractNode* Parser::parse_until_statement() {
  Location location_start = this->token.location;
  this->expect_token(TokenType::Until);

  AST::AbstractNode* test;

  if (this->token.type == TokenType::LeftParen) {
    this->advance();
    test = this->parse_expression();
    this->expect_token(TokenType::RightParen);
  } else {
    test = this->parse_expression();
  }

  auto context_backup = this->keyword_context;
  this->keyword_context.break_allowed = true;
  this->keyword_context.continue_allowed = true;

  AST::AbstractNode* then_block;

  if (this->token.type == TokenType::LeftCurly) {
    then_block = this->parse_block();
  } else {
    then_block = this->parse_control_statement();
    this->skip_token(TokenType::Semicolon);
  }

  this->keyword_context = context_backup;
  return (new AST::Until(test, then_block))->at(location_start, then_block->location_end);
}

AST::AbstractNode* Parser::parse_loop_statement() {
  Location location_start = this->token.location;
  this->expect_token(TokenType::Loop);

  auto context_backup = this->keyword_context;
  this->keyword_context.break_allowed = true;
  this->keyword_context.continue_allowed = true;

  AST::AbstractNode* block;

  if (this->token.type == TokenType::LeftCurly) {
    block = this->parse_block();
  } else {
    block = this->parse_control_statement();
    this->skip_token(TokenType::Semicolon);
  }

  this->keyword_context = context_backup;
  return (new AST::Loop(block))->at(location_start, block->location_end);
}

AST::AbstractNode* Parser::parse_try_statement() {
  Location location_start = this->token.location;
  this->expect_token(TokenType::Try);

  AST::AbstractNode* try_block;
  std::string exception_name = "__CHARLY_INTERNAL_EXCEPTION_NAME";
  AST::AbstractNode* catch_block;
  AST::AbstractNode* finally_block;

  try_block = this->parse_block();

  if (this->token.type == TokenType::Catch) {
    this->advance();
    this->expect_token(TokenType::LeftParen);
    this->expect_token(TokenType::Identifier, [&]() { exception_name = this->token.value; });
    this->expect_token(TokenType::RightParen);

    catch_block = this->parse_block();

    if (this->token.type == TokenType::Finally) {
      this->advance();
      finally_block = this->parse_block();
    } else {
      finally_block = new AST::Empty();
    }
  } else {
    this->expect_token(TokenType::Finally);
    finally_block = this->parse_block();
    catch_block = new AST::Empty();
  }

  std::optional<Location> end_location;

  if (finally_block) {
    end_location = finally_block->location_end;
  } else {
    end_location = catch_block->location_end;
  }

  return (new AST::TryCatch(try_block, exception_name, catch_block, finally_block))->at(location_start, end_location);
}

AST::AbstractNode* Parser::parse_expression() {
  return parse_assignment();
}

AST::AbstractNode* Parser::parse_assignment() {
  AST::AbstractNode* left = this->parse_ternary_if();

  // We generate specific node for the AND assignment to make sure
  // we don't generate duplicate code in case there is a call expression
  // inside the target node somewhere
  bool generate_and_assignment = false;
  TokenType and_operator = TokenType::Unknown;

  // Check if this is an AND assignment
  AST::AbstractNode* right = nullptr;
  if (this->token.is_and_assignment()) {
    and_operator = kTokenAndAssignmentOperators[this->token.type];
    this->advance();
    right = this->parse_assignment();
    generate_and_assignment = true;
  } else if (this->token.type == TokenType::Assignment) {
    this->advance();
    right = this->parse_assignment();
  } else {
    return left;
  }

  // Generate different assignment nodes for different targets
  if (left->type() == AST::kTypeIdentifier) {
    if (generate_and_assignment) {
      AST::Identifier* id = AST::cast<AST::Identifier>(left);
      right = (new AST::Binary(and_operator, (new AST::Identifier(id->name))->at(id), right))->at(left, right);
      AST::AbstractNode* node = (new AST::Assignment(id->name, right))->at(left, right);
      delete left;
      return node;
    } else {
      AST::Identifier* id = AST::cast<AST::Identifier>(left);
      AST::AbstractNode* node = (new AST::Assignment(id->name, right))->at(left, right);
      delete left;
      return node;
    }
  } else if (left->type() == AST::kTypeMember) {
    if (generate_and_assignment) {
      AST::Member* member = AST::cast<AST::Member>(left);
      AST::AbstractNode* node =
          (new AST::ANDMemberAssignment(member->target, member->symbol, and_operator, right))->at(left, right);
      member->target = nullptr;
      delete left;
      return node;
    } else {
      AST::Member* member = AST::cast<AST::Member>(left);
      AST::AbstractNode* node =
          (new AST::MemberAssignment(member->target, member->symbol, right))->at(left, right);
      member->target = nullptr;
      delete left;
      return node;
    }
  } else if (left->type() == AST::kTypeIndex) {
    if (generate_and_assignment) {
      AST::Index* index = AST::cast<AST::Index>(left);
      AST::AbstractNode* node =
          (new AST::ANDIndexAssignment(index->target, index->argument, and_operator, right))->at(left, right);
      index->target = nullptr;
      index->argument = nullptr;
      delete left;
      return node;
    } else {
      AST::Index* index = AST::cast<AST::Index>(left);
      AST::AbstractNode* node =
          (new AST::IndexAssignment(index->target, index->argument, right))->at(left, right);
      index->target = nullptr;
      index->argument = nullptr;
      delete left;
      return node;
    }
  } else {
    this->illegal_node(left, "Invalid left-hand side of assignment.");
    return nullptr;
  }
}

AST::AbstractNode* Parser::parse_ternary_if() {
  AST::AbstractNode* test = this->parse_or();

  if (this->token.type == TokenType::QuestionMark) {
    this->advance();

    AST::AbstractNode* left = this->parse_ternary_if();
    this->expect_token(TokenType::Colon);
    AST::AbstractNode* right = this->parse_ternary_if();

    return (new AST::IfElse(test, left, right))->at(test, right);
  } else {
    return test;
  }
}

AST::AbstractNode* Parser::parse_or() {
  AST::AbstractNode* left = this->parse_and();

  while (this->token.type == TokenType::OR) {
    this->advance();
    AST::AbstractNode* right = this->parse_and();
    left = (new AST::Or(left, right))->at(left, right);
  }

  return left;
}

AST::AbstractNode* Parser::parse_and() {
  AST::AbstractNode* left = this->parse_bitwise_or();

  while (this->token.type == TokenType::AND) {
    this->advance();
    AST::AbstractNode* right = this->parse_bitwise_or();
    left = (new AST::And(left, right))->at(left, right);
  }

  return left;
}

AST::AbstractNode* Parser::parse_bitwise_or() {
  AST::AbstractNode* left = this->parse_bitwise_xor();

  while (this->token.type == TokenType::BitOR) {
    TokenType op = this->token.type;
    this->advance();
    AST::AbstractNode* right = this->parse_bitwise_xor();
    left = (new AST::Binary(op, left, right))->at(left, right);
  }

  return left;
}

AST::AbstractNode* Parser::parse_bitwise_xor() {
  AST::AbstractNode* left = this->parse_bitwise_and();

  while (this->token.type == TokenType::BitXOR) {
    TokenType op = this->token.type;
    this->advance();
    AST::AbstractNode* right = this->parse_bitwise_and();
    left = (new AST::Binary(op, left, right))->at(left, right);
  }

  return left;
}

AST::AbstractNode* Parser::parse_bitwise_and() {
  AST::AbstractNode* left = this->parse_equal_not();

  while (this->token.type == TokenType::BitAND) {
    TokenType op = this->token.type;
    this->advance();
    AST::AbstractNode* right = this->parse_equal_not();
    left = (new AST::Binary(op, left, right))->at(left, right);
  }

  return left;
}

AST::AbstractNode* Parser::parse_equal_not() {
  AST::AbstractNode* left = this->parse_less_greater();

  while (true) {
    if (this->token.type == TokenType::Equal || this->token.type == TokenType::Not) {
      TokenType op = this->token.type;
      this->advance();
      AST::AbstractNode* right = this->parse_less_greater();
      left = (new AST::Binary(op, left, right))->at(left, right);
    } else {
      break;
    }
  }

  return left;
}

AST::AbstractNode* Parser::parse_less_greater() {
  AST::AbstractNode* left = this->parse_bitwise_shift();

  while (true) {
    if (this->token.type == TokenType::Less || this->token.type == TokenType::Greater ||
        this->token.type == TokenType::LessEqual || this->token.type == TokenType::GreaterEqual) {
      TokenType op = this->token.type;
      this->advance();
      AST::AbstractNode* right = this->parse_bitwise_shift();
      left = (new AST::Binary(op, left, right))->at(left, right);
    } else {
      break;
    }
  }

  return left;
}

AST::AbstractNode* Parser::parse_bitwise_shift() {
  AST::AbstractNode* left = this->parse_add_sub();

  while (true) {
    if (this->token.type == TokenType::LeftShift || this->token.type == TokenType::RightShift) {
      TokenType op = this->token.type;
      this->advance();
      AST::AbstractNode* right = this->parse_add_sub();
      left = (new AST::Binary(op, left, right))->at(left, right);
    } else {
      break;
    }
  }

  return left;
}

AST::AbstractNode* Parser::parse_add_sub() {
  AST::AbstractNode* left = this->parse_mul_div();

  while (true) {
    if (this->token.type == TokenType::Plus || this->token.type == TokenType::Minus) {
      TokenType op = this->token.type;
      this->advance();
      AST::AbstractNode* right = this->parse_mul_div();
      left = (new AST::Binary(op, left, right))->at(left, right);
    } else {
      break;
    }
  }

  return left;
}

AST::AbstractNode* Parser::parse_mul_div() {
  AST::AbstractNode* left = this->parse_mod();

  while (true) {
    if (this->token.type == TokenType::Mul || this->token.type == TokenType::Div) {
      TokenType op = this->token.type;
      this->advance();
      AST::AbstractNode* right = this->parse_mod();
      left = (new AST::Binary(op, left, right))->at(left, right);
    } else {
      break;
    }
  }

  return left;
}

AST::AbstractNode* Parser::parse_mod() {
  AST::AbstractNode* left = this->parse_unary();

  while (this->token.type == TokenType::Mod) {
    TokenType op = this->token.type;
    this->advance();
    AST::AbstractNode* right = this->parse_unary();
    left = (new AST::Binary(op, left, right))->at(left, right);
  }

  return left;
}

AST::AbstractNode* Parser::parse_unary() {
  Location location_start = this->token.location;

  switch (this->token.type) {
    case TokenType::Plus: {
      TokenType op = TokenType::UPlus;
      this->advance();
      AST::AbstractNode* value = this->parse_unary();
      return (new AST::Unary(op, value))->at(location_start, value->location_end);
    }
    case TokenType::Minus: {
      TokenType op = TokenType::UMinus;
      this->advance();
      AST::AbstractNode* value = this->parse_unary();
      return (new AST::Unary(op, value))->at(location_start, value->location_end);
    }
    case TokenType::Not: {
      TokenType op = TokenType::UNot;
      this->advance();
      AST::AbstractNode* value = this->parse_unary();
      return (new AST::Unary(op, value))->at(location_start, value->location_end);
    }
    case TokenType::BitNOT: {
      TokenType op = TokenType::BitNOT;
      this->advance();
      AST::AbstractNode* value = this->parse_unary();
      return (new AST::Unary(op, value))->at(location_start, value->location_end);
    }
    default: { return this->parse_pow(); }
  }
}

AST::AbstractNode* Parser::parse_pow() {
  AST::AbstractNode* left = this->parse_typeof();

  if (this->token.type == TokenType::Pow) {
    this->advance();
    AST::AbstractNode* right = this->parse_pow();
    left = (new AST::Binary(TokenType::Pow, left, right))->at(left, right);
  }

  return left;
}

AST::AbstractNode* Parser::parse_typeof() {
  Location location_start;

  if (this->token.type == TokenType::Typeof) {
    this->advance();
    AST::AbstractNode* exp = this->parse_typeof();
    return (new AST::Typeof(exp))->at(location_start, exp->location_end);
  }

  return this->parse_member_call();
}

AST::AbstractNode* Parser::parse_member_call() {
  AST::AbstractNode* target = this->parse_literal();

  while (true) {
    switch (this->token.type) {
      case TokenType::LeftParen: {
        Location location_start = this->token.location;
        this->advance();
        Location location_end = this->token.location;

        AST::NodeList* args = new AST::NodeList();

        // Parse arguments to call
        if (this->token.type != TokenType::RightParen) {
          args->append_node(this->parse_expression());

          // Parse all remaining arguments
          while (this->token.type == TokenType::Comma) {
            this->advance();
            args->append_node(this->parse_expression());
          }

          location_end = this->token.location;
          this->expect_token(TokenType::RightParen);
        } else {
          this->advance();
        }

        // Specialize the call node in case we are calling a member access node
        // or an index access
        if (target->type() == AST::kTypeMember) {
          // Rip out the stuff we need from the member node
          AST::Member* member = AST::cast<AST::Member>(target);
          AST::AbstractNode* context = member->target;
          member->target = nullptr;

          // Create the new call node
          target = (new AST::CallMember(context, member->symbol, args))->at(target->location_start, location_end);

          // Dispose of the old node
          delete member;
        } else if (target->type() == AST::kTypeIndex) {
          // Rip out the stuff we need from the index node
          AST::Index* index_exp = AST::cast<AST::Index>(target);
          AST::AbstractNode* context = index_exp->target;
          AST::AbstractNode* index_argument = index_exp->argument;
          index_exp->target = nullptr;
          index_exp->argument = nullptr;

          // Create the new call node
          target = (new AST::CallIndex(context, index_argument, args))->at(target->location_start, location_end);

          // Dispose of the old node
          delete index_exp;
        } else {
          target = (new AST::Call(target, args))->at(target->location_start, location_end);
        }

        break;
      }
      case TokenType::LeftBracket: {
        this->advance();

        AST::AbstractNode* exp = this->parse_expression();
        Location location_end = this->token.location;
        this->expect_token(TokenType::RightBracket);

        // Rewrite to target.exp in case exp if a string literal
        if (exp->type() == AST::kTypeString) {
          AST::String* str = AST::cast<AST::String>(exp);
          target = (new AST::Member(target, str->value))->at(target->location_start, location_end);
          delete str;
        } else {
          target = (new AST::Index(target, exp))->at(target->location_start, location_end);
        }

        break;
      }
      case TokenType::Point: {
        this->advance();

        std::string symbol;

        Location location_end;

        this->expect_token(TokenType::Identifier, [&]() {
          symbol = this->token.value;
          location_end = this->token.location;
        });

        target = (new AST::Member(target, symbol))->at(target->location_end, location_end);
        break;
      }
      default: { return target; }
    }
  }
}

AST::AbstractNode* Parser::parse_literal() {
  switch (this->token.type) {
    case TokenType::AtSign: {
      Location location_start = this->token.location;
      this->advance();

      if (this->token.type == TokenType::Identifier) {
        AST::AbstractNode* exp = new AST::Member((new AST::Self())->at(location_start), this->token.value);
        exp->at(location_start, this->token.location);
        this->advance();
        return exp;
      } else {
        this->unexpected_token("identifier");
        return nullptr;
      }
      break;
    }
    case TokenType::Self: {
      AST::AbstractNode* node = new AST::Self();
      node->at(this->token.location);
      this->advance();
      return node;
    }
    case TokenType::Identifier: {
      AST::AbstractNode* id = new AST::Identifier(this->token.value);
      id->at(this->token.location);
      this->advance();
      return id;
    }
    case TokenType::LeftParen: {
      this->advance();
      AST::AbstractNode* exp = this->parse_expression();
      this->expect_token(TokenType::RightParen);
      return exp;
    }
    case TokenType::Integer: {
      AST::AbstractNode* val = new AST::Integer(this->token.numeric_value.i64_value);
      val->at(this->token.location);
      this->advance();
      return val;
    }
    case TokenType::Float: {
      AST::AbstractNode* val = new AST::Float(this->token.numeric_value.dbl_value);
      val->at(this->token.location);
      this->advance();
      return val;
    }
    case TokenType::String: {
      AST::AbstractNode* val = new AST::String(this->token.value);
      val->at(this->token.location);
      this->advance();
      return val;
    }
    case TokenType::BooleanTrue:
    case TokenType::BooleanFalse: {
      AST::AbstractNode* val = new AST::Boolean(this->token.type == TokenType::BooleanTrue);
      val->at(this->token.location);
      this->advance();
      return val;
    }
    case TokenType::Null: {
      AST::AbstractNode* val = new AST::Null();
      val->at(this->token.location);
      this->advance();
      return val;
    }
    case TokenType::Nan: {
      AST::AbstractNode* val = new AST::Nan();
      val->at(this->token.location);
      this->advance();
      return val;
    }
    case TokenType::LeftBracket: {
      return this->parse_array();
    }
    case TokenType::LeftCurly: {
      return this->parse_hash();
    }
    case TokenType::RightArrow: {
      return this->parse_arrowfunc();
    }
    case TokenType::Func: {
      return this->parse_func();
    }
    case TokenType::Class: {
      return this->parse_class();
    }
  }

  this->unexpected_token("expression");
  return nullptr;
}

AST::AbstractNode* Parser::parse_array() {
  Location location_start = this->token.location;

  this->expect_token(TokenType::LeftBracket);

  AST::NodeList* items = new AST::NodeList();

  // Check if there are any items
  if (this->token.type != TokenType::RightBracket) {
    items->append_node(this->parse_expression());

    while (this->token.type == TokenType::Comma) {
      this->advance();
      items->append_node(this->parse_expression());
    }
  }

  Location location_end = this->token.location;
  this->expect_token(TokenType::RightBracket);

  return (new AST::Array(items))->at(location_start, location_end);
}

AST::AbstractNode* Parser::parse_hash() {
  Location location_start = this->token.location;
  this->expect_token(TokenType::LeftCurly);

  AST::Hash* hash = new AST::Hash();

  // Check if there are any entries
  if (this->token.type != TokenType::RightCurly) {
    hash->append_pair(this->parse_hash_entry());

    while (this->token.type == TokenType::Comma) {
      this->advance();
      hash->append_pair(this->parse_hash_entry());
    }
  }

  Location location_end = this->token.location;
  this->expect_token(TokenType::RightCurly);

  return hash->at(location_start, location_end);
}

std::pair<std::string, AST::AbstractNode*> Parser::parse_hash_entry() {
  std::string key;
  Location key_location;
  AST::AbstractNode* value;

  this->expect_token(TokenType::Identifier, [&]() {
    key = this->token.value;
    key_location = this->token.location;
  });

  if (this->token.type == TokenType::Colon) {
    this->advance();
    value = this->parse_expression();
  } else {
    value = (new AST::Identifier(key))->at(key_location);
  }

  return {key, value};
}

AST::AbstractNode* Parser::parse_func() {
  Location location_start = this->token.location;
  this->expect_token(TokenType::Func);

  std::string name;
  bool has_symbol = false;

  if (this->token.type == TokenType::Identifier) {
    name = this->token.value;
    has_symbol = true;
    this->advance();
  }

  std::vector<std::string> params;
  if (this->token.type == TokenType::LeftParen) {
    this->advance();

    if (this->token.type != TokenType::RightParen) {
      this->expect_token(TokenType::Identifier, [&]() { params.push_back(this->token.value); });

      while (this->token.type == TokenType::Comma) {
        this->advance();
        this->expect_token(TokenType::Identifier, [&]() {

          // Check if this argument already exists
          auto search = std::find(params.begin(), params.end(), this->token.value);
          if (search == params.end()) {
            params.push_back(this->token.value);
          } else {
            this->illegal_token("Duplicate function parameter");
          }
        });
      }
    }

    this->expect_token(TokenType::RightParen);
  }

  // Parse any of the two block body syntaxes
  //
  // func foo(a, b) { a * b }
  // func foo(a, b) = a * b
  AST::AbstractNode* body = nullptr;
  if (this->token.type == TokenType::LeftCurly) {
    auto backup_context = this->keyword_context;
    this->keyword_context.return_allowed = true;
    this->keyword_context.break_allowed = false;
    this->keyword_context.continue_allowed = false;
    body = this->parse_block();
    this->keyword_context = backup_context;

    // Insert a return null statement into the block in case the last statement in the
    // functions body is not already a return statement
    if (AST::cast<AST::Block>(body)->statements.size() > 0) {
      if (AST::cast<AST::Block>(body)->statements.back()->type() != AST::kTypeReturn) {
        AST::cast<AST::Block>(body)->append_node((new AST::Return(new AST::Null()))->at(body));
      }
    }
  } else if (has_symbol && this->token.type == TokenType::Assignment) {
    this->advance();
    AST::AbstractNode* expr = this->parse_control_statement();
    AST::Block* wrapper_body = AST::cast<AST::Block>((new AST::Block())->at(expr));
    wrapper_body->append_node((new AST::Return(expr))->at(expr));
    body = wrapper_body;
  } else {
    this->unexpected_token("block");
    return nullptr;
  }

  return (new AST::Function(name, params, body, false))->at(location_start, body->location_end);
}

AST::AbstractNode* Parser::parse_arrowfunc() {
  Location location_start = this->token.location;
  this->expect_token(TokenType::RightArrow);

  std::vector<std::string> params;
  if (this->token.type == TokenType::LeftParen) {
    this->advance();

    if (this->token.type != TokenType::RightParen) {
      this->expect_token(TokenType::Identifier, [&]() { params.push_back(this->token.value); });

      while (this->token.type == TokenType::Comma) {
        this->advance();
        this->expect_token(TokenType::Identifier, [&]() {

          // Check if this argument already exists
          auto search = std::find(params.begin(), params.end(), this->token.value);
          if (search == params.end()) {
            params.push_back(this->token.value);
          } else {
            this->illegal_token("Duplicate function parameter");
          }
        });
      }
    }

    this->expect_token(TokenType::RightParen);
  }

  AST::AbstractNode* block;

  if (this->token.type == TokenType::LeftCurly) {
    auto backup_context = this->keyword_context;
    this->keyword_context.return_allowed = true;
    this->keyword_context.break_allowed = false;
    this->keyword_context.continue_allowed = false;
    block = this->parse_block();
    this->keyword_context = backup_context;

    // Insert a return null statement into the block in case the last statement in the
    // functions body is not already a return statement
    if (AST::cast<AST::Block>(block)->statements.size() > 0) {
      if (AST::cast<AST::Block>(block)->statements.back()->type() != AST::kTypeReturn) {
        AST::cast<AST::Block>(block)->append_node((new AST::Return(new AST::Null()))->at(block));
      }
    }
  } else {
    AST::AbstractNode* exp = this->parse_control_statement();
    AST::Block* exp_wrapper = AST::cast<AST::Block>((new AST::Block())->at(exp));
    exp_wrapper->append_node((new AST::Return(exp))->at(exp));
    block = exp_wrapper;
  }

  return (new AST::Function("", params, block, true))->at(location_start, block->location_end);
}

AST::AbstractNode* Parser::parse_class() {
  Location location_start = this->token.location;
  this->expect_token(TokenType::Class);

  // Parse an optional class name
  std::string name;
  if (this->token.type == TokenType::Identifier) {
    name = this->token.value;
    this->advance();
  }

  AST::NodeList* parents = new AST::NodeList();
  AST::AbstractNode* constructor = nullptr;
  std::vector<std::string> member_properties;
  std::vector<std::string> static_properties;
  AST::NodeList* member_functions = new AST::NodeList();
  AST::NodeList* static_functions = new AST::NodeList();

  // Parse parent classes
  if (this->token.type == TokenType::Extends) {
    this->advance();

    parents->append_node(this->parse_expression());
    while (this->token.type == TokenType::Comma) {
      this->advance();
      parents->append_node(this->parse_expression());
    }
  }

  // Parse the class body
  this->expect_token(TokenType::LeftCurly);
  if (this->token.type != TokenType::RightCurly) {
    // Parse all class statements
    while (true) {
      Location statement_location_start = this->token.location;

      bool static_declaration = false;

      // Check if this is a static declaration
      if (this->token.type == TokenType::Static) {
        static_declaration = true;
        this->advance();
      }

      if (this->token.type == TokenType::Func) {
        if (static_declaration) {
          static_functions->append_node(this->parse_func());
        } else {
          AST::Function* func = AST::cast<AST::Function>(this->parse_func());

          if (func->name == "constructor") {
            constructor = func;
          } else {
            member_functions->append_node(func);
          }
        }
      } else if (this->token.type == TokenType::Property) {
        this->advance();
        this->expect_token(TokenType::Identifier, [&]() {
          if (static_declaration) {
            static_properties.push_back(this->token.value);
          } else {
            member_properties.emplace_back(this->token.value);
          }
        });
        this->skip_token(TokenType::Semicolon);
      } else {
        this->unexpected_token("func or property");
      }

      if (this->token.type == TokenType::RightCurly)
        break;
    }
  }
  Location location_end = this->token.location;
  this->expect_token(TokenType::RightCurly);

  if (constructor == nullptr) {
    constructor = new AST::Empty();
  }

  return (new AST::Class(name, constructor, member_properties, member_functions, static_properties, static_functions,
                         parents))
      ->at(location_start, location_end);
}

void Parser::assign_default_name(AST::AbstractNode* node, const std::string& name) {
  if (node->type() == AST::kTypeFunction) {
    AST::Function* func = AST::cast<AST::Function>(node);
    if (func->name.size() == 0) {
      func->name = name;
    }
  } else if (node->type() == AST::kTypeClass) {
    AST::Class* klass = AST::cast<AST::Class>(node);
    if (klass->name.size() == 0) {
      klass->name = name;
    }
  }
}

}  // namespace Charly::Compilation
