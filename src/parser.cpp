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

namespace Charly::Compiler {
  ParseResult* Parser::parse() {
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
        default: {
          break;
        }
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
      error_message = "Expected ";
      error_message.append(kTokenTypeStrings[expected]);
      error_message.append(", got ");
      error_message.append(kTokenTypeStrings[this->token.type]);
    }

    throw SyntaxError(this->token.location, error_message);
  }

  void Parser::unexpected_token(std::string&& expected_value) {
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

    std::optional<Location> start_location;
    std::optional<Location> end_location;

    this->expect_token(TokenType::LeftCurly, [&]() {
      start_location = this->token.location;
    });

    while (this->token.type != TokenType::RightCurly) {
      block->append_node(this->parse_statement());
    }

    this->expect_token(TokenType::RightCurly, [&]() {
      end_location = this->token.location;
    });

    return block->at(start_location, end_location);
  }

  AST::AbstractNode* Parser::parse_statement() {
    Location start_location = this->token.location;

    switch (this->token.type) {
      case TokenType::Let: {

        this->advance();
        switch (this->token.type) {
          case TokenType::Identifier: {
            std::string name = this->token.value;
            Location ident_location = this->token.location;

            this->advance();
            switch(this->token.type) {
              case TokenType::Semicolon: {
                AST::Null* exp = new AST::Null();
                exp->at(start_location, ident_location);

                this->advance();
                return (new AST::LocalInitialisation(name, exp, false))->at(start_location, ident_location);
                break;
              }
              case TokenType::Assignment: {
                this->advance();
                AST::AbstractNode* exp = this->parse_expression();

                // We copy the name of the identifier into an unnamed function for better
                // stack traces
                //
                // let foo = func() {}
                //
                // Would become:
                //
                // let foo = func foo() {}
                if (exp->type() == AST::kTypeFunction) {
                  AST::Function* func = reinterpret_cast<AST::Function*>(exp);
                  if (func->name.size() == 0) {
                    func->name = name;
                  }
                }

                this->skip_token(TokenType::Semicolon);
                return (new AST::LocalInitialisation(name, exp, false))->at(start_location, exp->location_end);
                break;
              }
              default: {
                AST::Null* exp = new AST::Null();
                return (new AST::LocalInitialisation(name, exp, false))->at(start_location, ident_location);
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

        std::optional<Location> end_location = exp->location_end;
        this->skip_token(TokenType::Semicolon);
        return (new AST::LocalInitialisation(identifier, exp, true))->at(start_location, end_location);
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
      default: {
        AST::AbstractNode* exp = this->parse_expression();
        this->skip_token(TokenType::Semicolon);
        return exp;
      }
    }

    this->unexpected_token();
    return nullptr;
  }

  AST::AbstractNode* Parser::parse_if_statement() {
    Location start_location = this->token.location;
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
    AST::AbstractNode* else_node;

    if (this->token.type == TokenType::LeftCurly) {
      then_node = this->parse_block();
    } else {
      then_node = this->parse_statement();
      else_node = new AST::Empty();
      return (new AST::If(test, then_node, else_node))->at(start_location, then_node->location_end);
    }

    if (this->token.type == TokenType::Else) {
      this->advance();

      if (this->token.type == TokenType::If) {
        else_node = this->parse_if_statement();
      } else {
        else_node = this->parse_block();
      }

      return (new AST::If(test, then_node, else_node))->at(start_location, else_node->location_end);
    } else {
      else_node = new AST::Empty();
      return (new AST::If(test, then_node, else_node))->at(start_location, then_node->location_end);
    }
  }

  AST::AbstractNode* Parser::parse_unless_statement() {
    Location start_location = this->token.location;
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
    AST::AbstractNode* else_node;

    if (this->token.type == TokenType::LeftCurly) {
      then_node = this->parse_block();
    } else {
      then_node = this->parse_statement();
      else_node = new AST::Empty();
      return (new AST::Unless(test, then_node, else_node))->at(start_location, then_node->location_end);
    }

    // Unless nodes are not allowed to have else if alternative blocks
    // as that would be a way to create really messy code
    if (this->token.type == TokenType::Else) {
      this->advance();

      if (this->token.type == TokenType::LeftCurly) {
        else_node = this->parse_block();
      } else {
        else_node = this->parse_statement();
      }
    } else {
      else_node = new AST::Empty();
    }

    return (new AST::Unless(test, then_node, else_node))->at(start_location, then_node->location_end);
  }

  AST::AbstractNode* Parser::parse_guard_statement() {
    Location start_location = this->token.location;
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
      block = this->parse_statement();
    }

    return (new AST::Guard(test, block))->at(start_location, block->location_end);
  }

  AST::AbstractNode* Parser::parse_while_statement() {
    Location start_location = this->token.location;
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
      then_block = this->parse_statement();
    }

    this->keyword_context = context_backup;
    return (new AST::While(test, then_block))->at(start_location, then_block->location_end);
  }

  AST::AbstractNode* Parser::parse_until_statement() {
    Location start_location = this->token.location;
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
      then_block = this->parse_statement();
    }

    this->keyword_context = context_backup;
    return (new AST::Until(test, then_block))->at(start_location, then_block->location_end);
  }

  AST::AbstractNode* Parser::parse_loop_statement() {
    Location start_location = this->token.location;
    this->expect_token(TokenType::Loop);

    auto context_backup = this->keyword_context;
    this->keyword_context.break_allowed = true;
    this->keyword_context.continue_allowed = true;

    AST::AbstractNode* block;

    if (this->token.type == TokenType::LeftCurly) {
      block = this->parse_block();
    } else {
      block = this->parse_statement();
    }

    this->keyword_context = context_backup;
    return (new AST::Loop(block))->at(start_location, block->location_end);
  }

  AST::AbstractNode* Parser::parse_try_statement() {
    Location start_location = this->token.location;
    this->expect_token(TokenType::Try);

    AST::AbstractNode* try_block;
    std::string exception_name;
    AST::AbstractNode* catch_block;
    AST::AbstractNode* finally_block;

    try_block = this->parse_block();

    if (this->token.type == TokenType::Catch) {
      this->advance();
      this->expect_token(TokenType::LeftParen);
      this->expect_token(TokenType::Identifier, [&](){
        exception_name = this->token.value;
      });
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

    return (new AST::TryCatch(try_block, exception_name, catch_block, finally_block))->at(start_location, end_location);
  }

  AST::AbstractNode* Parser::parse_expression() {
    return parse_literal();
  }

  AST::AbstractNode* Parser::parse_literal() {
    switch (this->token.type) {
      case TokenType::Identifier: {
        AST::Identifier* id = new AST::Identifier(this->token.value);
        id->at(this->token.location);
        this->advance();
        return id;
      }
    }

    this->unexpected_token("expression");
    return nullptr;
  }
}
