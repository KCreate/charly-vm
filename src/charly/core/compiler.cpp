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
#include "charly/core/compiler/parser.h"

#include "charly/core/compiler/passes/grammar_validation_check.h"
#include "charly/core/compiler/passes/class_constructor_check.h"
#include "charly/core/compiler/passes/desugar_pass.h"
#include "charly/core/compiler/passes/duplicates_check.h"
#include "charly/core/compiler/passes/reserved_identifiers_check.h"

using namespace charly::core::compiler::ast;

namespace charly::core::compiler {

std::shared_ptr<CompilationUnit> Compiler::compile(CompilationUnit::Type type,
                                                   const std::string& filepath,
                                                   utils::Buffer& source) {
  std::shared_ptr<CompilationUnit> unit = std::make_shared<CompilationUnit>(type, filepath, source);

  // parse source file
  unit->ast = Parser::parse_program(source, unit->console);

  if (unit->console.has_errors())
    return unit;

#define APPLY_DIAGNOSTIC_PASS(PassName)       \
  {                                           \
    assert(unit->ast.get());                  \
    PassName(unit->console).apply(unit->ast); \
    if (unit->console.has_errors())           \
      return unit;                            \
  }

#define APPLY_TRANSFORM_PASS(PassName)                    \
  {                                                       \
    assert(unit->ast.get());                              \
    unit->ast = PassName(unit->console).apply(unit->ast); \
    if (unit->console.has_errors())                       \
      return unit;                                        \
  }

  if (type == CompilationUnit::Type::Module) {
    unit->ast = Compiler::wrap_module(unit->ast);
  }

  APPLY_DIAGNOSTIC_PASS(GrammarValidationCheck);

  APPLY_DIAGNOSTIC_PASS(ReservedIdentifiersCheck);
  APPLY_DIAGNOSTIC_PASS(DuplicatesCheck);
  APPLY_DIAGNOSTIC_PASS(ClassConstructorCheck);

  APPLY_TRANSFORM_PASS(DesugarPass);

#undef APPLY_DIAGNOSTIC_PASS
#undef APPLY_TRANSFORM_PASS

  return unit;
}

ast::ref<ast::Statement> Compiler::wrap_module(const ref<Statement>& program) {
  ref<Function> func = make<Function>(false, make<Name>("main"), program);
  func->set_location(program);
  return func;
}

}  // namespace charly::core::compiler
