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
#include <exception>

#include "ast.h"
#include "tree-walker.h"
#include "symboltable.h"

#pragma once

namespace Charly::Compilation {

typedef std::pair<AST::AbstractNode*, std::string> CompilerError;

class FatalError : public std::exception {};

class CompilerPass : public TreeWalker {
public:
  CompilerPass(SymbolTable& s) : symtable(s) {
  }

  // Error handling
  void inline push_error(AST::AbstractNode* node, const std::string& error) {
    this->errors.emplace_back(node, error);
  }
  bool inline has_errors() {
    return this->errors.size() > 0;
  }
  void inline fatal_error(AST::AbstractNode* node, const std::string& error) {
    this->errors.emplace_back(node, error);
    throw FatalError();
  }

  template <class T>
  void inline dump_errors(T& stream) {
    for (auto& err : this->errors) {
      stream << "Error: ";
      stream << err.second;
      auto& location_start = err.first->location_start;
      if (location_start.has_value()) {
        stream << ' ';
        location_start->write_to_stream(stream);
      }
      stream << '\n';
    }
  }

protected:
  SymbolTable& symtable;
  std::vector<CompilerError> errors;
};
}
