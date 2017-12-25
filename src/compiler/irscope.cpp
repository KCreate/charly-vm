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

#include "irinfo.h"

namespace Charly::Compilation {
IRVarRecord IRScope::declare(size_t symbol, uint32_t depth, uint64_t blockid, bool is_constant) {
  this->table.try_emplace(symbol);

  IRVarRecord record({depth, blockid, this->next_frame_index++, is_constant});
  std::vector<IRVarRecord>& records = this->table[symbol];
  records.push_back(record);

  return record;
}

void IRScope::pop_blockid(uint64_t blockid) {
  for (auto& recordlist : this->table) {
    if (recordlist.second.size() == 0)
      continue;
    if (recordlist.second.back().blockid == blockid)
      recordlist.second.pop_back();
  }
}

std::optional<IRVarRecord> IRScope::resolve(size_t symbol, uint32_t depth, uint64_t blockid, bool noparentblocks) {
  std::optional<IRVarRecord> result;
  IRScope* search_table = this;
  uint32_t dereferenced_tables = 0;

  // Iterate over this and all parent tables
  while (search_table) {
    // Check if this table contains this symbol
    if (search_table->table.count(symbol) > 0) {
      auto& recordlist = search_table->table[symbol];

      // Reverse search for a match
      for (auto rit = recordlist.rbegin(); rit != recordlist.rend(); rit++) {
        IRVarRecord& record = *rit;

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
    dereferenced_tables++;
  }

  // How many frames need to be dereference to reach this record
  if (result.has_value()) {
    result->depth = dereferenced_tables;
  }

  return result;
}
}  // namespace Charly::Compilation
