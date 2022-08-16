/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

#include "charly/utils/argumentparser.h"

#include "charly/core/compiler/codegenerator.h"
#include "charly/core/compiler/compiler.h"
#include "charly/core/compiler/ir/assembler.h"
#include "charly/core/compiler/parser.h"

#include "charly/core/compiler/passes/code_elimination.h"
#include "charly/core/compiler/passes/constant_folding_pass.h"
#include "charly/core/compiler/passes/desugar_pass.h"
#include "charly/core/compiler/passes/duplicates_check.h"
#include "charly/core/compiler/passes/grammar_validation_check.h"
#include "charly/core/compiler/passes/repl_prepare_pass.h"
#include "charly/core/compiler/passes/reserved_identifiers_check.h"
#include "charly/core/compiler/passes/variable_analyzer_pass.h"

namespace charly::core::compiler {

using namespace charly::core::compiler::ast;

ref<CompilationUnit> Compiler::compile(const std::string& filepath, utils::Buffer& source, CompilationUnit::Type type) {
  ref<CompilationUnit> unit = make<CompilationUnit>(type, filepath, source);

  // parse source file
  unit->ast = Parser::parse_program(source, unit->console);

  if (unit->console.has_errors())
    return unit;

  if (utils::ArgumentParser::is_flag_set("ast_raw") &&
      utils::ArgumentParser::flag_has_argument("debug_pattern", filepath, true)) {
    unit->ast->dump(std::cout, true);
  }

#define APPLY_DIAGNOSTIC_PASS(PassName)       \
  {                                           \
    DCHECK(unit->ast.get());                  \
    PassName(unit->console).apply(unit->ast); \
    if (unit->console.has_errors())           \
      return unit;                            \
  }

#define APPLY_TRANSFORM_PASS(PassName)                                 \
  {                                                                    \
    DCHECK(unit->ast.get());                                           \
    unit->ast = cast<Block>(PassName(unit->console).apply(unit->ast)); \
    if (unit->console.has_errors())                                    \
      return unit;                                                     \
  }

  // prepare repl input for compilation
  if (type == CompilationUnit::Type::ReplInput) {
    unit->ast->repl_toplevel_block = true;
    APPLY_TRANSFORM_PASS(ReplPreparePass)
  }

  // wrap in module function
  ref<Function> func = make<Function>(false, make<Name>("main"), unit->ast);
  func->set_location(unit->ast);
  unit->ast = make<Block>(make<Return>(func));

  APPLY_DIAGNOSTIC_PASS(GrammarValidationCheck)
  APPLY_DIAGNOSTIC_PASS(ReservedIdentifiersCheck)
  APPLY_DIAGNOSTIC_PASS(DuplicatesCheck)

  APPLY_TRANSFORM_PASS(DesugarPass)

  {
    VariableAnalyzer analyzer;
    unit->ast = cast<Block>(VariableAnalyzerPass(unit->console, analyzer).apply(unit->ast));
    if (unit->console.has_errors())
      return unit;

    unit->ast = cast<Block>(VariableLocationPatchPass(unit->console, analyzer).apply(unit->ast));
    if (unit->console.has_errors())
      return unit;
  }

  if (!utils::ArgumentParser::is_flag_set("no_ast_opt")) {
    APPLY_TRANSFORM_PASS(ConstantFoldingPass)
    APPLY_TRANSFORM_PASS(CodeEliminationPass)
  }

  if (utils::ArgumentParser::is_flag_set("ast") &&
      utils::ArgumentParser::flag_has_argument("debug_pattern", filepath, true)) {
    unit->ast->dump(std::cout, true);
  }

#undef APPLY_DIAGNOSTIC_PASS
#undef APPLY_TRANSFORM_PASS

  // compile to bytecodes
  CodeGenerator codegenerator(unit);
  unit->ir_module = codegenerator.compile();

  if (utils::ArgumentParser::is_flag_set("ir") &&
      utils::ArgumentParser::flag_has_argument("debug_pattern", filepath, true)) {
    unit->ir_module->dump(std::cout);
  }

  // assemble bytecodes
  unit->compiled_module = ir::Assembler::compile_module(unit->ir_module);

  if (utils::ArgumentParser::is_flag_set("asm") &&
      utils::ArgumentParser::flag_has_argument("debug_pattern", filepath, true)) {
    unit->compiled_module->dump(std::cout);
  }

  return unit;
}

}  // namespace charly::core::compiler
