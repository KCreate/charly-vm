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

#include "irinfo.h"

#pragma once

/*
 * This file contains the local variable allocation algorithm the Charly compiler uses
 * to assign each variable a slot in the frame's environment. It tries to reuse different slots
 * for variables that are not active at the same time.
 * */

namespace Charly::Compilation {

// Holds information about a given slot in a functions local variables table
struct SlotInfo {
  bool active = true;
  bool leaked = false;
  bool constant = false;
};

struct FunctionScope {
  std::vector<SlotInfo> active_slots;
  AST::Function* function_node = nullptr;
  FunctionScope* parent_scope = nullptr;

  FunctionScope(AST::Function* fn, FunctionScope* ps) : function_node(fn), parent_scope(ps) {
  }

  ~FunctionScope() {
    if (this->function_node) {
      this->function_node->lvarcount = this->active_slots.size();
    }
  }

  // Allocate a slot
  inline uint32_t alloc_slot(bool constant) {
    if (this->active_slots.size() == 0) {
      this->active_slots.push_back({ .active = true, .constant = constant });
      return 0;
    }

    // Check if we have an unactive slot
    uint32_t allocated_slot = this->active_slots.size() - 1;
    for (auto slot = this->active_slots.rbegin(); slot != this->active_slots.rend(); slot++) {

      // If the slot is inactive, mark it as active and return the index
      if (!slot->active) {
        slot->active = true;
        return allocated_slot;
      }

      allocated_slot--;
    }

    this->active_slots.push_back({ .active = true, .constant = constant });
    return this->active_slots.size() - 1;
  }

  // Mark a specific index in the active slots as free
  inline void mark_as_free(uint32_t index) {
    if (index < this->active_slots.size()) {

      // We can't reuse this local index if the slot was leaked
      if (!this->active_slots[index].leaked) {
        this->active_slots[index].active = false;
        this->active_slots[index].leaked = false;
        this->active_slots[index].constant = false;
      }
    }
  }

  inline void mark_as_leaked(uint32_t index) {
    if (index < this->active_slots.size()) {
      this->active_slots[index].leaked = true;
    }
  }
};

// Information used by the code generator to read/write from/to local variables
struct LocalOffsetInfo {
  uint32_t level = 0xffffffff;
  uint32_t offset = 0xffffffff;
  bool valid = true;
  bool constant = false;

  inline IRVarOffsetInfo to_offset_info() {
    return { .level = this->level, .index = this->offset };
  }
};

struct LocalScope {
  FunctionScope* contained_function = nullptr;
  LocalScope* parent_scope = nullptr;
  std::unordered_map<size_t, LocalOffsetInfo> local_indices;

  LocalScope(FunctionScope* cf, LocalScope* ps) : contained_function(cf), parent_scope(ps) {
  }

  ~LocalScope() {
    for (auto& offset_info : this->local_indices) {
      this->contained_function->mark_as_free(offset_info.second.offset);
    }
  }

  // Allocate a slot in this scope
  inline LocalOffsetInfo alloc_slot(size_t symbol, bool constant = false) {
    uint32_t allocated_index = this->contained_function->alloc_slot(constant);
    LocalOffsetInfo offset_info = { .level = 0, .offset = allocated_index, .constant = constant };
    this->local_indices[symbol] = offset_info;
    return offset_info;
  }

  inline LocalOffsetInfo declare_slot(size_t symbol, bool constant = false) {
    uint32_t allocated_index = this->local_indices.size();
    LocalOffsetInfo offset_info = { .level = 0, .offset = allocated_index, .constant = constant };
    this->local_indices[symbol] = offset_info;
    return offset_info;
  }

  // Checks if this scope contains a symbol
  inline bool scope_contains_symbol(size_t symbol) {
    return this->local_indices.count(symbol) != 0;
  }

  // Return the offset info for a given symbol
  inline LocalOffsetInfo resolve_symbol(size_t symbol, bool ignore_parents = false) {
    LocalScope* search_scope = this;
    FunctionScope* search_function_scope = this->contained_function;

    uint32_t dereferenced_functions = 0;
    bool mark_vars_as_leaked = false;

    // Search for this symbol
    while (search_scope) {

      // Check if this scope contains the symbol
      if (search_scope->scope_contains_symbol(symbol)) {
        LocalOffsetInfo found_offset_info = search_scope->local_indices[symbol];

        // Mark the slot as leaked if we passed a function boundary
        // If we don't mark it as leaked, the slot might be allocated to another
        // variable
        if (dereferenced_functions && mark_vars_as_leaked && search_function_scope) {
          search_function_scope->mark_as_leaked(found_offset_info.offset);
        }

        // The level is always relative to the function we come from
        found_offset_info.level = dereferenced_functions;
        return found_offset_info;
      }

      if (ignore_parents) break;

      // Update the searching environment
      search_scope = search_scope->parent_scope;
      if (search_scope && search_scope->contained_function != search_function_scope) {
        dereferenced_functions++;
        mark_vars_as_leaked = true;
        search_function_scope = search_scope->contained_function;
      }
    }

    return { .valid = false };
  }
};
}  // namespace Charly::Compilation
