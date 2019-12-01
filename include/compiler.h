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

#include <iostream>
#include <optional>

#include "ast.h"
#include "instructionblock.h"
#include "runflags.h"
#include "sourcefile.h"
#include "stringpool.h"
#include "symboltable.h"

#pragma once

namespace Charly::Compilation {

// Holds a single compiler error
struct CompilerMessage {
  // The severity of the message
  enum class Severity : uint8_t { Info, Warning, Error };

  Severity severity = Severity::Info;
  std::optional<AST::AbstractNode*> node;
  std::string message;

  CompilerMessage(Severity s, AST::AbstractNode* n, const std::string& m) : severity(s), message(m) {
    if (n != nullptr) {
      node = n;
    }
  }
};

// Holds the result of a compilation
struct CompilerResult {
  std::optional<InstructionBlock*> instructionblock;
  AST::AbstractNode* abstract_syntax_tree;
  std::vector<CompilerMessage> messages;
  bool has_errors = false;
};

// Context for multiple compilations
struct CompilerContext {
  SymbolTable& symtable;
  StringPool& stringpool;

  CompilerContext(SymbolTable& st, StringPool& sp) : symtable(st), stringpool(sp) {
  }
};

// Configuration passed to the compiler
struct CompilerConfig {
  typedef std::tuple<std::string, bool> TopLevelConstant;

  // Known constants in the toplevel
  std::vector<TopLevelConstant> known_top_level_constants = {
    {"__charly_internal_import",  true},    // 00
    {"Object",                    true},    // 01
    {"Class",                     true},    // 02
    {"Array",                     true},    // 03
    {"String",                    true},    // 04
    {"Number",                    true},    // 05
    {"Function",                  true},    // 06
    {"Generator",                 true},    // 07
    {"Boolean",                   true},    // 08
    {"Null",                      true},    // 09
    {"stdin",                     false},   // 10
    {"stdout",                    false},   // 11
    {"stderr",                    false},   // 12
    {"print",                     false},   // 13
    {"write",                     false},   // 14
    {"gets",                      false},   // 15
    {"getc",                      false},   // 16
    {"exit",                      false},   // 17
    {"defer",                     false},   // 18
    {"Charly",                    false}    // 19
  };

  // Module inclusion function
  bool wrap_inclusion_function = true;
  std::string inclusion_function_name = "__CHARLY_MODULE_FUNC";
  std::vector<std::string> inclusion_function_arguments = {"export"};
  std::string inclusion_function_return_identifier = "export";

  // Wether to run the codegen phase at all
  bool codegen = true;

  // Errors are written to this stream
  std::ostream& err_stream = std::cout;
  std::ostream& log_stream = std::cerr;

  // Various other flags
  RunFlags& flags;
};

class Compiler {
public:
  Compiler(CompilerContext& ctx, CompilerConfig& cfg) : context(ctx), config(cfg) {
  }

  CompilerResult compile(AST::AbstractNode* tree);

private:
  CompilerContext& context;
  CompilerConfig& config;
};
}  // namespace Charly::Compilation
