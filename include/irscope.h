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

/*
 * This file contains the local variable allocation algorithm the Charly compiler uses
 * to assign each variable a slot in the frame's environment. It tries to reuse different slots
 * for variables that are not active at the same time.
 * */

namespace Charly::Compilation {

// Forward declare the Function AST node for use inside the FunctionScope struct
namespace AST {
  class Function;
};

// Holds information about a given slot
//
// The level and offset fields are relative to the LocalScope object which emitted this object
struct LocalSlotInfo {
  uint32_t level = 0xffffffff;
  uint32_t offset = 0xffffffff;
  bool leaked = false;
  bool valid = true;
};

struct FunctionScope {
  std::vector<bool> active_slots;
  AST::Function* function_node = nullptr;

  // Allocate a slot
  //
  // Reuses an un-active slot if one is found or adds another one to the list
  inline uint32_t alloc_slot() {

  }

  // Mark a specific index in the active slots as free
  inline void mark_as_free(uint32_t index) {

  }
};

struct LocalScope {
  FunctionScope* contained_function = nullptr;
  LocalScope* parent_scope = nullptr;
  std::unordered_map<size_t, LocalSlotInfo> local_indices;

  LocalScope(FunctionScope* cf, LocalScope* ps) : contained_function(cf), parent_scope(ps) {
  }

  ~LocalScope() {
    for (auto& slot_info : this->local_indices) {
      if (!slot_info.leaked) {
        this->contained_function->mark_as_free(slot.offset);
      }
    }
  }

  // Allocate a slot in this scope
  inline LocalSlotInfo alloc_slot() {
    uint32_t allocated_index = this->contained_function->alloc_slot();
    return { .level = 0, .offset = allocated_index };
  }

  // Return the offset of a symbol
  inline LocalSlotInfo resolve_slot(size_t symbol) {
    if (this->local_indices.count(symbol) != 0) {
      return this->local_indices[symbol];
    }

    return { .valid = false }
  }
};
}  // namespace Charly::Compilation
