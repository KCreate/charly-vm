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
class CompilerPass : public TreeWalker {
protected:
  CompilerContext& context;
  CompilerConfig& config;
  CompilerResult& result;

  inline void push_info(AST::AbstractNode* node, const std::string& message) {
    this->result.messages.emplace_back(CompilerMessage::Severity::Info, node, message);
  }

  inline void push_warning(AST::AbstractNode* node, const std::string& message) {
    this->result.messages.emplace_back(CompilerMessage::Severity::Warning, node, message);
  }

  inline void push_error(AST::AbstractNode* node, const std::string& message) {
    this->result.messages.emplace_back(CompilerMessage::Severity::Error, node, message);
    this->result.has_errors = true;
  }

  inline void push_fatal_error(AST::AbstractNode* node, const std::string& message) {
    this->result.messages.emplace_back(CompilerMessage::Severity::Error, node, message);
    this->result.has_errors = true;
    throw this->result.messages.back();
  }

public:
  CompilerPass(CompilerContext& c, CompilerConfig& cfg, CompilerResult& r)
      : TreeWalker(), context(c), config(cfg), result(r) {
  }
};
}  // namespace Charly::Compilation
