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

#include "ast.h"
#include "compiler-pass.h"

#pragma once

namespace Charly::Compilation {

// Represents a single record in an LVarScope
struct LVarRecord {
  uint32_t depth = 0;
  uint64_t blockid = 0;
  uint32_t frame_index = 0;
  bool is_constant = false;
};

// Represents a new level of scope as introduced by a function
struct LVarScope {
  LVarScope* parent = nullptr;
  AST::Function* function_node;
  std::unordered_map<size_t, std::vector<LVarRecord>> table;
  uint32_t next_frame_index = 0;

  LVarScope(LVarScope* p, AST::Function* f) : parent(p), function_node(f) {
  }

  // Declare a new symbol inside this scope
  LVarRecord declare(size_t symbol, uint32_t depth, uint64_t blockid, bool is_constant = false);
  void pop_blockid(uint64_t blockid);
  std::optional<LVarRecord> resolve(size_t symbol, uint32_t depth, uint64_t blockid, bool noparentblocks);
};

class LVarRewriter : public CompilerPass {
  using CompilerPass::CompilerPass;

public:
  AST::AbstractNode* visit_function(AST::Function* node, VisitContinue cont);
  AST::AbstractNode* visit_block(AST::Block* node, VisitContinue cont);
  AST::AbstractNode* visit_localinitialisation(AST::LocalInitialisation* node, VisitContinue cont);
  AST::AbstractNode* visit_identifier(AST::Identifier* node, VisitContinue cont);
  AST::AbstractNode* visit_assignment(AST::Assignment* node, VisitContinue cont);

private:
  uint32_t depth = 0;
  uint64_t blockid = 0;
  LVarScope* scope = nullptr;
};

}  // namespace Charly::Compilation
