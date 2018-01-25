/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
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

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#pragma once

namespace Charly::Compilation {

// Forward declaration
namespace AST {
struct Function;
};

// Represents a single record in an IRScope
struct IRVarRecord {
  uint32_t depth = 0;
  uint64_t blockid = 0;
  uint32_t frame_index = 0;
  bool is_constant = false;
};

// Represents a level of scope introduced by a function
struct IRScope {
  IRScope* parent = nullptr;
  AST::Function* function_node;
  std::unordered_map<size_t, std::vector<IRVarRecord>> table;
  uint32_t next_frame_index = 0;

  IRScope(IRScope* p, AST::Function* f) : parent(p), function_node(f) {
  }

  // Declare a new symbol inside this scope
  IRVarRecord declare(size_t symbol, uint32_t depth, uint64_t blockid, bool is_constant = false);
  void pop_blockid(uint64_t blockid);
  std::optional<IRVarRecord> resolve(size_t symbol, uint32_t depth, uint64_t blockid, bool noparentblocks);
};

// Contains the amount of environments which need to be dereferenced
// and the index of a local variable to read
struct IRVarOffsetInfo {
  uint32_t level;
  uint32_t index;
};

// Contains a list of variables which, unless defined explicitly,
// should be rewritten to @<name> inside a particular function
struct IRKnownSelfVars {
  std::unordered_set<std::string> names;

  IRKnownSelfVars(const std::vector<std::string>& n) {
    for (auto& name : n) {
      this->names.insert(name);
    }
  }

  IRKnownSelfVars(std::vector<std::string>&& n) {
    for (auto& name : n) {
      this->names.insert(std::move(name));
    }
  }
};

// Contains information about this value on the stack
struct IRAssignmentInfo {
  bool assignment_value_required = true;

  IRAssignmentInfo(bool avr) : assignment_value_required(avr) {
  }
};

}  // namespace Charly::Compilation
