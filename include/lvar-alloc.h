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

#include <vector>
#include <unordered_map>

#include "lvar-location.h"

#pragma once

namespace Charly::Compilation {
  // Holds information about a given slot in a function's lvar table
  //
  // active     - Wether this slot is currently in use
  // leaked     - Wether this slot has ever been leaked to another context
  // constant   - Wether this slot has been marked as constant
  struct SlotInfo {
    bool active = true;
    bool leaked = false;
    bool constant = false;
  };

  // These frames are introduced into the list every time a new function is being analyzed.
  // The variable slots inside stack frames are managed inside this class
  class FunctionScope {
  public:
    std::vector<SlotInfo> slots;
    AST::Function* function_node = nullptr;
    FunctionScope* parent_scope = nullptr;

    FunctionScope(AST::Function* fn, FunctionScope* ps) : function_node(fn), parent_scope(ps) {
    }

    ~FunctionScope() {
      if (this->function_node) {
        this->function_node->lvarcount = this->slots.size();
      }
    }

    uint32_t alloc_slot(bool constant);
    void mark_as_free(uint32_t index);
    inline void mark_as_leaked(uint32_t index);
  };

  // Manages the locations of variables on a block basis during the compilation process
  // Whenever there's a reference to a variable location this class manages the lookup process
  class LocalScope {
  public:
    FunctionScope* parent_function;
    LocalScope* parent_scope;
    std::unordered_map<size_t, LocalOffsetInfo> locals;

    LocalScope(FunctionScope* cf, LocalScope* ps) : parent_function(cf), parent_scope(ps) {
    }

    ~LocalScope() {
      for (const auto& offset_info : this->locals) {

        // If this variable was stored inside the frame, we have to mark it as free
        if (offset_info.second.location.type == LocationType::LocFrame) {
          this->parent_function->mark_as_free(offset_info.second.location.as_frame.index);
        }
      }
    }

    // Allocate a new variable slot for a given symbol
    LocalOffsetInfo alloc_slot(size_t symbol, bool constant = false, bool overwriteable = false);
    bool scope_contains_symbol(size_t symbol);
    bool symbol_is_free(size_t symbol);
    LocalOffsetInfo register_symbol(size_t symbol, LocalOffsetInfo info, bool constant = false);
    LocalOffsetInfo access_symbol(const std::string& symbol);
    LocalOffsetInfo access_symbol(size_t symbol);
  };

}
