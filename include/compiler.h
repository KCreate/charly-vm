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

#include <iostream>
#include <optional>

#include "instructionblock.h"
#include "sourcefile.h"
#include "ast.h"
#include "symboltable.h"
#include "stringpool.h"
#include "runflags.h"

#pragma once

namespace Charly::Compilation {

// Context for multiple compilations
struct CompilerContext {
  SymbolTable symtable;
  StringPool stringpool;
};

// Configuration passed to the compiler
struct CompilationConfig {
  // Known variables in the toplevel
  //
  // If a module inclusion function is generated, these are the known variables
  // inside the scope the function is created in
  //
  // If no module inclusion function is generated, these are ignored
  std::vector<std::string> known_top_level_vars;

  // Module inclusion function
  bool wrap_inclusion_function = true;
  std::string inclusion_function_name = "__CHARLY_MODULE_FUNC";
  std::vector<std::string> inclusion_function_arguments = {"export", "Charly"};
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
  Compiler(CompilerContext& ctx) : context(ctx) {
  }

  InstructionBlock* compile(SourceFile& source, const CompilationConfig& config);
  InstructionBlock* compile(AST::AbstractNode* tree, const CompilationConfig& config);

private:
  CompilerContext& context;
};
}  // namespace Charly::Compilation
