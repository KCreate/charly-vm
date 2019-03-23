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

namespace Charly::Compilation {

uint32_t FunctionScope::alloc_slot(bool constant) {
  if (this->active_slots.size() == 0) {
    this->active_slots.push_back({.constant = constant});
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

  this->active_slots.push_back({.constant = constant});
  return this->active_slots.size() - 1;
}

void FunctionScope::mark_as_free(uint32_t index) {
  if (index < this->active_slots.size()) {
    // We can't reuse this local index if the slot was leaked
    if (!this->active_slots[index].leaked) {
      this->active_slots[index].active = false;
      this->active_slots[index].leaked = false;
      this->active_slots[index].constant = false;
    }
  }
}

void FunctionScope::mark_as_leaked(uint32_t index) {
  if (index < this->active_slots.size()) {
    this->active_slots[index].leaked = true;
  }
}

LocalOffsetInfo LocalScope::alloc_slot(size_t symbol, bool constant, bool overwriteable) {
  // Check if this symbol is already registered but can be overwritten
  if (this->scope_contains_symbol(symbol) && this->locals[symbol].overwriteable) {
    return this->locals[symbol];
  }

  uint32_t allocated_index = this->contained_function->alloc_slot(constant);
  LocalOffsetInfo offset_info = {
    .location = ValueLocation::frame(0, allocated_index),
    .constant = constant,
    .overwriteable = overwriteable
  };
  this->locals[symbol] = offset_info;
  return offset_info;
}

bool LocalScope::scope_contains_symbol(size_t symbol) {
  return this->locals.count(symbol) != 0;
}

bool LocalScope::symbol_is_free(size_t symbol) {
  return !this->scope_contains_symbol() || this->locals[symbol].overwriteable;
}

LocalOffsetInfo LocalScope::register_symbol(size_t symbol, LocationOffsetInfo info) {
  this->locals[symbol] = info;
  return info;
}

LocalOffsetInfo LocalScope::access_symbol(const std::string& symbol) {

  // Check if this symbol is a special argument index
  if (symbol[0] == '$') {
    // Make sure the remaining characters are all digits
    if (std::all_of(node->name.begin() + 1, node->name.end(), ::isdigit)) {
      uint32_t index = std::atoi(node->name.substr(1, std::string::npos).c_str());
      AST::Function* function_node = this->scope->contained_function->function_node;

      if (function_node == nullptr || index >= function_node->parameters.size()) {
        return ValueLocation::arguments(index);
      } else {
        if (function_node == nullptr) {
          return ValueLocation::frame(0, index)
        } else {
          if (function_node->needs_arguments) {
            return ValueLocation::frame(0, index + 1)
          } else {
            return ValueLocation::frame(0, index)
          }
        }
      }
    }
  }

  return LocalScope::access_symbol(charly_create_symbol(symbol));
}

LocalOffsetInfo LocalScope::access_symbol(size_t symbol) {
  LocalScope* search_scope = this;
  FunctionScope* search_function_scope = this->contained_function;

  uint32_t dereferenced_functions = 0;
  bool mark_vars_as_leaked = false;

  // Search for this symbol
  while (search_scope) {
    if (search_scope->scope_contains_symbol(symbol)) {
      LocalOffsetInfo found_offset_info = search_scope->locals[symbol];

      // Register this value in this scope to speed up lookup the next time
      this->scope->locals[symbol] = found_offset_info;

      // Mark the slot as leaked if we passed a function boundary
      // If we don't mark it as leaked, the slot might be allocated to another
      // variable
      if (dereferenced_functions && mark_vars_as_leaked && search_function_scope) {
        search_function_scope->mark_as_leaked(found_offset_info.offset);
      }

      return ValueLocation::patch_level(found_offset_info);
    }

    // Check wether this symbol is a known self var
    if (search_function_scope->known_self_vars) {
      auto begin_it = search_function_scope->known_self_vars->begin();
      auto end_it = search_function_scope->known_self_vars->end();

      if (std::count_if(begin_it, end_it, [symbol] (auto name) {
        return symbol == charly_create_symbol(name);
      })) {
        return ValueLocation::self(symbol, dereferenced_functions);
      }
    }

    // Update the searching environment
    search_scope = search_scope->parent_scope;
    if (search_scope && search_scope->contained_function != search_function_scope) {
      dereferenced_functions++;
      mark_vars_as_leaked = true;
      search_function_scope = search_scope->contained_function;
    }
  }

  return {.valid = false};
}

}  // namespace Charly::Compilation
