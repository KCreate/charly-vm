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

#include "ast.h"
#include "block.h"
#include "codegenerator.h"
#include "disassembler.h"
#include "lvar-rewrite.h"
#include "parseresult.h"
#include "symboltable.h"

namespace Charly::Compilation {
class Compiler {
public:
  inline InstructionBlock* compile(ParseResult& result) {
    // Wrap the whole program in a function which handles the exporting interface
    // to other programs
    result.parse_tree = (new AST::Function("", {"export", "Charly"}, result.parse_tree, true))->at(result.parse_tree);

    // Calculate all offsets of all variables, assignments and declarations
    LVarRewriter lvar_rewriter(this->symtable);
    result.parse_tree = lvar_rewriter.visit_node(result.parse_tree);
    if (lvar_rewriter.has_errors()) {
      lvar_rewriter.dump_errors(std::cout);
      return nullptr;
    }

    // Generate code for the created AST
    CodeGenerator codegenerator(this->symtable);
    InstructionBlock* compiled_block = codegenerator.compile(result.parse_tree);
    if (codegenerator.has_errors()) {
      codegenerator.dump_errors(std::cout);
      return nullptr;
    }

    return compiled_block;
  }

private:
  SymbolTable symtable;
};
}  // namespace Charly::Compilation
