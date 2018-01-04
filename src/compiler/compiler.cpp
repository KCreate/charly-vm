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

#include <sstream>

#include "compiler.h"
#include "codegenerator.h"
#include "irinfo.h"
#include "lvar-rewrite.h"
#include "normalizer.h"
#include "parser.h"

namespace Charly::Compilation {

CompilerResult Compiler::compile(AST::AbstractNode* tree, const CompilationConfig& config) {
  CompilerResult result;

  // Generate a module inclusion function if one is requested
  // The module inclusion function wraps a program into a single function that can be
  // called by the runtime
  if (config.wrap_inclusion_function) {
    // Append a return export node to the end of the parsed block
    AST::Block* block = tree->as<AST::Block>();
    AST::Identifier* ret_id = new AST::Identifier(config.inclusion_function_return_identifier);
    AST::Return* ret_stmt = new AST::Return(ret_id->at(block));
    block->statements.push_back(ret_stmt->at(block));

    // Wrap the whole program in a function which handles the exporting interface
    // to other programs
    tree = new AST::Function("", config.inclusion_function_arguments, block, true);
    tree->at(block);

    // Add the known vars which are known to be inserted via the require call
    IRKnownSelfVars* known_self_vars = new IRKnownSelfVars(config.known_top_level_vars);
    tree->as<AST::Function>()->known_self_vars = known_self_vars;
  }

  try {
    // Clean up the code a little bit and add or remove some nodes
    Normalizer normalizer(this->context, result);
    tree = normalizer.visit_node(tree);

    if (result.has_errors) {
      return result;
    }

    // Calculate all offsets of all variables, assignments and declarations
    LVarRewriter lvar_rewriter(this->context, result);
    tree = lvar_rewriter.visit_node(tree);

    if (result.has_errors) {
      return result;
    }

    // Optionally skip code generation
    if (config.codegen) {
      CodeGenerator codegenerator(this->context, result);
      InstructionBlock* compiled_block = codegenerator.compile(tree);
      result.instructionblock = compiled_block;
    }
  } catch (CompilerMessage msg) {
    result.has_errors = true;
  }

  return result;
}
}  // namespace Charly::Compilation
