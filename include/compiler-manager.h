/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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

#include "address-mapping.h"
#include "ast.h"
#include "compiler.h"
#include "disassembler.h"
#include "parser.h"
#include "sourcefile.h"

#pragma once

namespace Charly::Compilation {

// Interface to the parser and compiler
class CompilerManager {
public:
  CompilerManager() {
    SymbolTable::encode("__charly_internal_stale_thread_exception");
    SymbolTable::encode("argc");
    SymbolTable::encode("constructor");
    SymbolTable::encode("function");
    SymbolTable::encode("halt_after_return");
    SymbolTable::encode("host_class");
    SymbolTable::encode("klass");
    SymbolTable::encode("length");
    SymbolTable::encode("main");
    SymbolTable::encode("name");
    SymbolTable::encode("parent");
    SymbolTable::encode("parent_environment");
    SymbolTable::encode("prototype");
    SymbolTable::encode("push_return_value");
    SymbolTable::encode("self_value");
    SymbolTable::encode("source_location");
    SymbolTable::encode("<dead>");
    SymbolTable::encode("<class>");
    SymbolTable::encode("<object>");
    SymbolTable::encode("<array>");
    SymbolTable::encode("<string>");
    SymbolTable::encode("<function>");
    SymbolTable::encode("<cfunction>");
    SymbolTable::encode("<frame>");
    SymbolTable::encode("<catchtable>");
    SymbolTable::encode("<cpointer>");
    SymbolTable::encode("<number>");
    SymbolTable::encode("<boolean>");
    SymbolTable::encode("<null>");
    SymbolTable::encode("<symbol>");
    SymbolTable::encode("<unknown>");
    SymbolTable::encode("<anonymous>");
  };
  CompilerManager(const CompilerManager&) = delete;
  CompilerManager(CompilerManager&&) = delete;

  std::optional<ParserResult> parse(const std::string& filename, const std::string& source);
  std::optional<CompilerResult> compile(const std::string& filename, const std::string& source);

public:
  AddressMapping address_mapping;
};
}  // namespace Charly::Compilation
