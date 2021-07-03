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

#include <string>

#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/diagnostic.h"
#include "charly/core/compiler/ir/builder.h"
#include "charly/core/runtime/compiled_module.h"
#include "charly/utils/buffer.h"

#pragma once

namespace charly::core::compiler {

struct CompilationUnit {
  enum class Type : uint8_t { Module, ReplInput };

  CompilationUnit(Type type, const std::string& filepath, utils::Buffer& source) :
    type(type),
    console(filepath, source),
    filepath(filepath),
    ast(nullptr),
    ir_module(nullptr),
    compiled_module(nullptr) {}

  Type type;
  DiagnosticConsole console;
  std::string filepath;
  ref<ast::Block> ast;
  ref<ir::IRModule> ir_module;
  ref<runtime::CompiledModule> compiled_module;
};

class Compiler {
public:

  // compile source code into a compilation unit
  static ref<CompilationUnit> compile(const std::string& filepath,
                                      utils::Buffer& source,
                                      CompilationUnit::Type type = CompilationUnit::Type::ReplInput);
};

}  // namespace charly::core::compiler
