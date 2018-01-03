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

#include <exception>
#include <iostream>
#include <vector>

#include "ast.h"
#include "compiler.h"
#include "symboltable.h"
#include "tree-walker.h"

#pragma once

namespace Charly::Compilation {

typedef std::pair<AST::AbstractNode*, std::string> CompilerError;

class FatalError : public std::exception {};

class CompilerPass : public TreeWalker {
public:
  CompilerPass(CompilerContext& s) : context(s) {
  }

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

  void inline dump_errors(std::ostream& stream) {
    for (auto[node, message] : this->errors) {
      stream << "Error";
      auto& location_start = node->location_start;
      if (location_start.has_value()) {
        stream << ' ';
        location_start->write_to_stream(stream);
        stream << ": ";
      } else {
        stream << ": ";
      }
      stream << message << '\n';
    }
  }

protected:
  CompilerContext& context;
  std::vector<CompilerError> errors;
};
}  // namespace Charly::Compilation
