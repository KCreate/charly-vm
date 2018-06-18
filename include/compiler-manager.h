/*
 * This file is part of the Charly Virtual Machine (https://code.leonardschuetz.ch/r/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2018 Leonard Schütz
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
#include <unordered_map>
#include <vector>

#include "ast.h"
#include "compiler.h"
#include "disassembler.h"
#include "parser.h"
#include "runflags.h"
#include "sourcefile.h"

#pragma once

namespace Charly::Compilation {

// Interface to the parser and compiler
class CompilerManager {
public:
  CompilerManager(const RunFlags& f) : flags(f){};
  CompilerManager(const CompilerManager&) = delete;
  CompilerManager(CompilerManager&&) = delete;

  std::optional<ParserResult> parse(const std::string& filename, const std::string& source);
  std::optional<CompilerResult> compile(const std::string& filename, const std::string& source);

private:
  RunFlags flags;
  std::ostream& out_stream = std::cout;
  std::ostream& err_stream = std::cerr;

public:
  SymbolTable symtable;
  StringPool stringpool;
};
}  // namespace Charly::Compilation
