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

#include <algorithm>

#include "lvar-alloc.h"
#include "value.h"
#include "ast.h"

namespace Charly::Compilation {

uint32_t FunctionScope::alloc_slot(bool constant) {
  if (this->slots.size() == 0) {
    this->function_node->lvarcount++;
    this->slots.push_back({.constant = constant});
    return 0;
  }

  // Check if we have an unactive slot
  uint32_t allocated_slot = this->slots.size() - 1;
  for (auto slot = this->slots.rbegin(); slot != this->slots.rend(); slot++) {
    // If the slot is inactive, mark it as active and return the index
    if (!slot->active) {
      slot->active = true;
      return allocated_slot;
    }

    allocated_slot--;
  }

  this->function_node->lvarcount++;
  this->slots.push_back({.constant = constant});
  return this->slots.size() - 1;
}

void FunctionScope::mark_as_free(uint32_t index) {
  if (index < this->slots.size()) {
    // We can't reuse this local index if the slot was leaked
    if (!this->slots[index].leaked) {
      this->slots[index].active = false;
      this->slots[index].leaked = false;
      this->slots[index].constant = false;
    }
  }
}

void FunctionScope::mark_as_leaked(uint32_t index) {
  if (index < this->slots.size()) {
    this->slots[index].leaked = true;
  }
}

LocalOffsetInfo LocalScope::alloc_slot(size_t symbol, bool constant) {
  // Check if this symbol is already registered but can be overwritten
  if (this->scope_contains_symbol(symbol)) {
    return LocalOffsetInfo();
  }

  uint32_t allocated_index = this->parent_function->alloc_slot(constant);
  LocalOffsetInfo offset_info(ValueLocation::frame(allocated_index, 0), true, constant);
  this->locals[symbol] = offset_info;
  return offset_info;
}

bool LocalScope::scope_contains_symbol(size_t symbol) {
  return this->locals.count(symbol) != 0;
}

LocalOffsetInfo LocalScope::register_symbol(size_t symbol, LocalOffsetInfo info, bool constant) {
  info.constant = constant;
  this->locals.insert(std::pair<size_t, LocalOffsetInfo>(symbol, info));
  return info;
}

LocalOffsetInfo LocalScope::access_symbol(const std::string& symbol) {

  // Check if this symbol is a special argument index
  if (symbol[0] == '$') {
    // Make sure the remaining characters are all digits
    if (std::all_of(symbol.begin() + 1, symbol.end(), ::isdigit)) {
      uint32_t index = std::atoi(symbol.substr(1, std::string::npos).c_str());
      AST::Function* function_node = this->parent_function->function_node;

      if (function_node == nullptr || index >= function_node->parameters.size()) {
        return LocalOffsetInfo(ValueLocation::arguments(index));
      } else {
        if (function_node == nullptr) {
          return LocalOffsetInfo(ValueLocation::frame(index, 0));
        } else {
          if (function_node->needs_arguments) {
            return LocalOffsetInfo(ValueLocation::frame(index + 1, 0));
          } else {
            return LocalOffsetInfo(ValueLocation::frame(index, 0));
          }
        }
      }
    }
  }

  return LocalScope::access_symbol(charly_create_symbol(symbol));
}

LocalOffsetInfo LocalScope::access_symbol(size_t symbol) {
  LocalScope* search_scope = this;
  FunctionScope* search_function_scope = this->parent_function;

  uint32_t dereferenced_functions = 0;
  bool mark_vars_as_leaked = false;

  // Search for this symbol
  while (search_scope) {
    if (search_scope->scope_contains_symbol(symbol)) {
      LocalOffsetInfo found_offset_info = search_scope->locals[symbol];

      // Register this value in this scope to speed up lookup the next time
      this->locals[symbol] = found_offset_info;

      // Mark the slot as leaked if we passed a function boundary
      // If we don't mark it as leaked, the slot might be allocated to another
      // variable
      if (mark_vars_as_leaked && search_function_scope) {
        if (found_offset_info.location.type == LocationType::LocFrame) {
          search_function_scope->mark_as_leaked(found_offset_info.location.as_frame.index);
        }
      }

      found_offset_info.location = ValueLocation::patch_level(found_offset_info.location, dereferenced_functions);

      return found_offset_info;
    }

    // Update the searching environment
    search_scope = search_scope->parent_scope;
    if (search_scope && search_scope->parent_function != search_function_scope) {
      dereferenced_functions++;
      mark_vars_as_leaked = true;
      search_function_scope = search_scope->parent_function;
    }
  }

  return LocalOffsetInfo();
}

}  // namespace Charly::Compilation
