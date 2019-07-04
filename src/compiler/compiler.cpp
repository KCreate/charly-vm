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

#include "compiler.h"
#include "codegenerator.h"
#include "lvar-rewrite.h"
#include "normalizer.h"

namespace Charly::Compilation {

CompilerResult Compiler::compile(AST::AbstractNode* tree) {
  CompilerResult result = {.abstract_syntax_tree = tree};

  // Generate a module inclusion function if one is requested
  // The module inclusion function wraps a program into a single function that can be
  // called by the runtime
  AST::Function* inclusion_function = nullptr;
  if (this->config.wrap_inclusion_function) {
    // Append a return export node to the end of the parsed block
    AST::Block* block = result.abstract_syntax_tree->as<AST::Block>();
    AST::Identifier* ret_id = new AST::Identifier(this->config.inclusion_function_return_identifier);
    AST::Return* ret_stmt = new AST::Return(ret_id->at(block));
    block->statements.push_back(ret_stmt->at(block));

    // Wrap the whole program in a function which handles the exporting interface
    // to other programs
    inclusion_function = new AST::Function(this->config.inclusion_function_name,
                                           this->config.inclusion_function_arguments, {}, block, true);
    inclusion_function->at(block);
    inclusion_function->lvarcount = this->config.known_top_level_constants.size();
    result.abstract_syntax_tree = inclusion_function;

    // Push the function onto the stack and wrap it in a block node
    // The PushStack node prevents the optimizer from removing the function literal
    result.abstract_syntax_tree = new AST::PushStack(result.abstract_syntax_tree);
    result.abstract_syntax_tree = new AST::Block(result.abstract_syntax_tree);
  }

  try {
    // Clean up the code a little bit and add or remove some nodes
    Normalizer normalizer(this->context, this->config, result);
    result.abstract_syntax_tree = normalizer.visit_node(result.abstract_syntax_tree);

    if (result.has_errors) {
      return result;
    }

    // Calculate all offsets of all variables, assignments and declarations
    LVarRewriter lvar_rewriter(this->context, this->config, result);
    lvar_rewriter.push_local_scope();

    // Register the known local variables in the top level
    uint32_t i = 0;
    for (const auto& varname : this->config.known_top_level_constants) {
      lvar_rewriter.scope->register_symbol(
          this->context.symtable(varname), LocalOffsetInfo(ValueLocation::frame(i, 1), true, true), true);
      i++;
    }

    result.abstract_syntax_tree = lvar_rewriter.visit_node(result.abstract_syntax_tree);

    if (result.has_errors) {
      return result;
    }

    // Optionally skip code generation
    if (this->config.codegen) {
      CodeGenerator codegenerator(this->context, this->config, result);
      InstructionBlock* compiled_block = codegenerator.compile(result.abstract_syntax_tree);
      result.instructionblock = compiled_block;
    }
  } catch (CompilerMessage msg) {
    result.has_errors = true;
  }

  return result;
}
}  // namespace Charly::Compilation
