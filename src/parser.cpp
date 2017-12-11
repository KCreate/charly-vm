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
      error_message = "Unexpected ";
      error_message.append(kTokenTypeStrings[this->token.type]);
    } else {
      error_message = "Unexpected end of file";
    }

    throw SyntaxError(this->token.location, error_message);
  }

  void Parser::unexpected_token(TokenType expected) {
    std::string error_message;

    if (this->token.type == TokenType::Eof) {
      error_message = "Expected ";
      error_message.append(kTokenTypeStrings[expected]);
      error_message.append(", got ");
      error_message.append(kTokenTypeStrings[this->token.type]);
    } else {
      error_message = "Unexpected end of file, expected ";
      error_message.append(kTokenTypeStrings[expected]);
    }

    throw SyntaxError(this->token.location, error_message);
  }

  void Parser::unexpected_token(std::string&& expected_value) {
    std::string error_message;

    if (this->token.type == TokenType::Eof) {
      error_message = "Expected ";
      error_message.append(expected_value);
      error_message.append(", got ");
      error_message.append(kTokenTypeStrings[this->token.type]);
    } else {
      error_message = "Unexpected end of file, expected ";
      error_message.append(expected_value);
    }

    throw SyntaxError(this->token.location, error_message);
  }

  void Parser::illegal_token() {
    throw SyntaxError(this->token.location, "This token is not allowed at this location");
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
    if (this->token.type == type) this->advance();
  }

  void Parser::if_token(TokenType type, ParseFunc func) {
    if (this->token.type == type) func();
  }

  AST::AbstractNode* Parser::parse_program() {
    AST::Block* block = new AST::Block();

    while (this->token.type != TokenType::Eof) {
      block->append_node(this->parse_statement());
    }

    return block;
  }

  AST::Block* Parser::parse_block() {
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

    return static_cast<AST::Block*>(block->at(start_location, end_location));
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
    }

    this->unexpected_token();
    return nullptr;
  }

  AST::AbstractNode* Parser::parse_expression() {
    AST::Identifier* id;

    this->expect_token(TokenType::Identifier, [&](){
      id = new AST::Identifier(this->token.value);
      id->at(this->token.location);
    });

    return id;
  }
}















