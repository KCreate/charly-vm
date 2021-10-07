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

#include "charly/charly.h"
#include "charly/core/compiler/compiler.h"

#pragma once

using Catch::Matchers::Equals;

using namespace charly::core::compiler;
using namespace charly::core::compiler::ast;
using namespace charly::core::compiler::ir;
using namespace charly;

#define EXP(S, T)                                              \
  ([&]() {                                                     \
    charly::utils::Buffer buffer(S);                           \
    DiagnosticConsole console("test", buffer);                 \
    return cast<T>(Parser::parse_expression(buffer, console)); \
  }())

#define CHECK_EXP(S)                           \
  {                                            \
    charly::utils::Buffer buffer(S);           \
    DiagnosticConsole console("test", buffer); \
    Parser::parse_expression(buffer, console); \
    CATCH_CHECK(!console.has_errors());        \
  }

#define CHECK_STMT(S)                          \
  {                                            \
    charly::utils::Buffer buffer(S);           \
    DiagnosticConsole console("test", buffer); \
    Parser::parse_statement(buffer, console);  \
    CATCH_CHECK(!console.has_errors());        \
  }

#define CHECK_PROGRAM(S)                       \
  {                                            \
    charly::utils::Buffer buffer(S);           \
    DiagnosticConsole console("test", buffer); \
    Parser::parse_program(buffer, console);    \
    CATCH_CHECK(!console.has_errors());        \
  }

#define CHECK_AST_EXP(S, N)                                          \
  {                                                                  \
    std::stringstream exp_dump;                                      \
    std::stringstream ref_dump;                                      \
    charly::utils::Buffer buffer(S);                                 \
    DiagnosticConsole console("test", buffer);                       \
    ref<Expression> exp = Parser::parse_expression(buffer, console); \
    CATCH_CHECK(!console.has_errors());                              \
    if (!console.has_errors()) {                                     \
      exp->dump(exp_dump);                                           \
      N->dump(ref_dump);                                             \
      CATCH_CHECK_THAT(exp_dump.str(), Equals(ref_dump.str()));            \
    }                                                                \
  }

#define CHECK_AST_STMT(S, N)                                        \
  {                                                                 \
    std::stringstream stmt_dump;                                    \
    std::stringstream ref_dump;                                     \
    charly::utils::Buffer buffer(S);                                \
    DiagnosticConsole console("test", buffer);                      \
    ref<Statement> stmt = Parser::parse_statement(buffer, console); \
    CATCH_CHECK(!console.has_errors());                             \
    if (!console.has_errors()) {                                    \
      stmt->dump(stmt_dump);                                        \
      N->dump(ref_dump);                                            \
      CATCH_CHECK_THAT(stmt_dump.str(), Equals(ref_dump.str()));          \
    }                                                               \
  }

#define CHECK_AST_PROGRAM(S, N)                                 \
  {                                                             \
    std::stringstream prog_dump;                                \
    std::stringstream ref_dump;                                 \
    charly::utils::Buffer buffer(S);                            \
    DiagnosticConsole console("test", buffer);                  \
    ref<Program> prog = Parser::parse_program(buffer, console); \
    CATCH_CHECK(!console.has_errors());                         \
    if (!console.has_errors()) {                                \
      prog->dump(prog_dump);                                    \
      N->dump(ref_dump);                                        \
      CATCH_CHECK_THAT(prog_dump.str(), Equals(ref_dump.str()));      \
    }                                                           \
  }

#define CHECK_ERROR_EXP(S, E)                                    \
  {                                                              \
    charly::utils::Buffer buffer(S);                             \
    DiagnosticConsole console("test", buffer);                   \
    Parser::parse_expression(buffer, console);                   \
    CATCH_CHECK(console.has_errors());                           \
    if (console.has_errors())                                    \
      CATCH_CHECK_THAT(console.messages().front().message, Equals(E)); \
  }

#define CHECK_ERROR_STMT(S, E)                                   \
  {                                                              \
    charly::utils::Buffer buffer(S);                             \
    DiagnosticConsole console("test", buffer);                   \
    Parser::parse_statement(buffer, console);                    \
    CATCH_CHECK(console.has_errors());                           \
    if (console.has_errors())                                    \
      CATCH_CHECK_THAT(console.messages().front().message, Equals(E)); \
  }

#define CHECK_ERROR_PROGRAM(S, E)                                \
  {                                                              \
    charly::utils::Buffer buffer(S);                             \
    DiagnosticConsole console("test", buffer);                   \
    Parser::parse_program(buffer, console);                      \
    CATCH_CHECK(console.has_errors());                           \
    if (console.has_errors())                                    \
      CATCH_CHECK_THAT(console.messages().front().message, Equals(E)); \
  }

#define COMPILE_OK(S)                              \
  {                                                \
    charly::utils::Buffer buffer(S);               \
    auto unit = Compiler::compile("test", buffer); \
    CATCH_CHECK(!unit->console.has_errors());      \
  }

#define COMPILE_ERROR(S, E)                                            \
  {                                                                    \
    charly::utils::Buffer buffer(S);                                   \
    auto unit = Compiler::compile("test", buffer);                     \
    CATCH_CHECK(unit->console.has_errors());                           \
    if (unit->console.has_errors())                                    \
      CATCH_CHECK_THAT(unit->console.messages().front().message, Equals(E)); \
  }
