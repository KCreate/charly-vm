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

#include "charly/core/compiler.h"

#pragma once

using Catch::Matchers::Equals;

using namespace charly::core::compiler;
using namespace charly::core::compiler::ast;

#define EXP(S, T)                                              \
  ([&]() {                                                     \
    charly::utils::Buffer buffer(S);                           \
    DiagnosticConsole console("test", buffer);                 \
    return cast<T>(Parser::parse_expression(buffer, console)); \
  }())

#define CHECK_AST_EXP(S, N)                                                     \
  {                                                                             \
    std::stringstream exp_dump;                                                 \
    std::stringstream ref_dump;                                                 \
    charly::utils::Buffer buffer(S);                                            \
    DiagnosticConsole console("test", buffer);                                  \
    DumpPass(exp_dump, false).visit(Parser::parse_expression(buffer, console)); \
    REQUIRE(!console.has_errors());                                             \
    DumpPass(ref_dump, false).visit(N);                                         \
    CHECK_THAT(exp_dump.str(), Equals(ref_dump.str()));                         \
  }

#define CHECK_AST_STMT(S, N)                                                    \
  {                                                                             \
    std::stringstream stmt_dump;                                                \
    std::stringstream ref_dump;                                                 \
    charly::utils::Buffer buffer(S);                                            \
    DiagnosticConsole console("test", buffer);                                  \
    DumpPass(stmt_dump, false).visit(Parser::parse_statement(buffer, console)); \
    REQUIRE(!console.has_errors());                                             \
    DumpPass(ref_dump, false).visit(N);                                         \
    CHECK_THAT(stmt_dump.str(), Equals(ref_dump.str()));                        \
  }

#define CHECK_ERROR_EXP(S, E)                                  \
  {                                                            \
    charly::utils::Buffer buffer(S);                           \
    DiagnosticConsole console("test", buffer);                 \
    Parser::parse_expression(buffer, console);                 \
    REQUIRE(console.has_errors());                             \
    CHECK_THAT(console.messages().front().message, Equals(E)); \
  }

#define CHECK_ERROR_STMT(S, E)                                 \
  {                                                            \
    charly::utils::Buffer buffer(S);                           \
    DiagnosticConsole console("test", buffer);                 \
    Parser::parse_statement(buffer, console);                  \
    REQUIRE(console.has_errors());                             \
    CHECK_THAT(console.messages().front().message, Equals(E)); \
  }
