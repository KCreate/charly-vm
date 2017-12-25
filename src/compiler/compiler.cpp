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

#include <vector>

#include "codegenerator.h"
#include "compiler.h"
#include "disassembler.h"
#include "irinfo.h"
#include "lvar-rewrite.h"
#include "normalizer.h"

namespace Charly::Compilation {
InstructionBlock* Compiler::compile(ParseResult& result) {
  // Append a return export node to the end of the parsed block
  AST::Block* block = AST::cast<AST::Block>(result.parse_tree);
  block->statements.push_back((new AST::Return((new AST::Identifier("export"))->at(block)))->at(block));

  // Wrap the whole program in a function which handles the exporting interface
  // to other programs
  result.parse_tree = new AST::Function("", {"export", "Charly"}, block, true);
  result.parse_tree->at(block);

  // Add the known vars which are known to be inserted via the require call
  IRKnownSelfVars* known_self_vars = new IRKnownSelfVars(std::vector<std::string>(
      {"require_no_exec", "require", "stdin", "stdout", "stderr", "print", "write", "gets", "getc", "exit", "sleep"}));
  result.parse_tree->as<AST::Function>()->known_self_vars = known_self_vars;

  // Clean up the code a little bit and add or remove some nodes
  Normalizer normalizer(this->symtable);
  result.parse_tree = normalizer.visit_node(result.parse_tree);

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
}  // namespace Charly::Compilation
