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

#include <unordered_map>
#include <functional>
#include <optional>

#include "block.h"
#include "opcode.h"
#include "ast.h"

#pragma once

namespace Charly::Compiler {

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
  std::unordered_map<size_t, std::vector<LVarRecord>> table;
  uint32_t next_frame_index = 0;
  uint32_t max_depth = 0;

  // Returns the hash of a string
  static inline size_t hash_symbol(const std::string& symbol) {
    return std::hash<std::string>{}(symbol);
  }

  // Declare a new symbol inside this scope
  inline void declare(const std::string& symbol, uint32_t depth, uint64_t blockid, bool is_constant = false) {
    size_t hash = LVarScope::hash_symbol(symbol);
    this->table.try_emplace(hash);
    this->table[hash].push_back(LVarRecord({depth, blockid, this->next_frame_index++, is_constant}));
    if (depth > max_depth)
      max_depth = depth;
  }

  inline std::optional<LVarRecord> resolve(const std::string& symbol,
                                                  uint32_t depth,
                                                  uint64_t blockid,
                                                  bool noparentblocks) {
    size_t hash = LVarScope::hash_symbol(symbol);
    std::optional<LVarRecord> result;
    LVarScope* search_table = this;

    // Iterate over this and all parent tables
    while (search_table) {
      // Check if this table contains this symbol
      if (this->table.count(hash)) {
        auto& recordlist = this->table[hash];

        // Reverse search for a match
        auto rit = recordlist.rbegin();
        for (; rit != recordlist.rend(); rit++) {
          LVarRecord& record = *rit;

          // Check if both the depth and the blockid match
          if (record.depth == depth && record.blockid == blockid) {
            result = record;
            break;
          }

          // Check if this record would be reachable
          if (record.depth < depth && !noparentblocks) {
            result = record;
            break;
          }
        }
      }

      if (result.has_value())
        break;
      if (noparentblocks)
        break;
      search_table = search_table->parent;
    }

    return result;
  }
};

// Rewrites all variable declarations and identifier
//class LocalVarRewriter {
  //uint32_t depth = 0;
  //LVarScope* current_scope = nullptr;
  //std::vector<std::pair<AST::AbstractNode*, std::string>> errors;
//};

// Responsible for generating Charly bytecodes
class CodeGenerator {
};
}
