/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2021 Leonard Schütz
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

#include <cassert>

#include "charly/core/compiler/localvars.h"

namespace charly::core::compiler {

using namespace ir;
using namespace ast;

uint32_t FunctionScope::alloc_slot(bool constant) {
  for (uint32_t index = 0; index < slots.size(); index++) {
    if (slots[index].used == false) {
      slots[index].used = true;
      slots[index].constant = constant;
      return index;
    }
  }

  slots.push_back({ .used = true, .leaked = false, .constant = constant });

  return slots.size() - 1;
}

void FunctionScope::free_slot(uint32_t index) {
  assert(index < slots.size());
  if (!slots[index].leaked) {
    slots[index].used = false;
  }
}

void FunctionScope::leak_slot(uint32_t index) {
  assert(index < slots.size());
  slots[index].leaked = true;
}

const LocalVariable* BlockScope::alloc_slot(const ref<Name>& symbol,
                                            const ref<Node>& declaration,
                                            bool constant,
                                            bool force_local) {
  // toplevel declarations
  if (!force_local && this->ast_block->repl_toplevel_block) {
    ValueLocation location = ValueLocation::Global(symbol->value);
    variables.insert_or_assign(symbol->value, LocalVariable{ .ast_declaration = declaration,
                                                             .value_location = location,
                                                             .declared_locally = true,
                                                             .name = symbol->value,
                                                             .constant = constant });

    return &variables.at(symbol->value);
  }

  uint32_t slot_index = parent_function->alloc_slot(constant);

  ValueLocation location = ValueLocation::LocalFrame(symbol->value, slot_index);

  variables.insert_or_assign(symbol->value, LocalVariable{ .ast_declaration = declaration,
                                                           .value_location = location,
                                                           .declared_locally = true,
                                                           .name = symbol->value,
                                                           .constant = constant });

  return &variables.at(symbol->value);
}

bool BlockScope::symbol_declared(const std::string& symbol) {
  if (variables.count(symbol) == 0) {
    return false;
  }

  return variables.at(symbol).declared_locally;
}

const LocalVariable* BlockScope::lookup_symbol(const std::string& symbol) {

  // traverse block parent chain and search for symbol information
  auto search_block = this;
  uint32_t function_depth = 0;
  LocalVariable* found_variable = nullptr;
  while (search_block) {

    // check if this block contains an entry for this symbol
    if (search_block->variables.count(symbol)) {
      found_variable = &search_block->variables.at(symbol);
      break;
    }

    // load parent block
    auto old_parent_function = search_block->parent_function;
    search_block = search_block->parent_block.get();

    // check if we traversed over a function boundary
    if (search_block && search_block->parent_function != old_parent_function) {
      function_depth++;
    }
  }

  // variable not found, insert a global record into the current block
  if (!found_variable) {
    variables.insert({ symbol, LocalVariable{ .value_location = ValueLocation::Global(symbol),
                                                .declared_locally = false,
                                                .name = symbol,
                                                .constant = false } });

    return &variables.at(symbol);
  }

  // if the found variable was inside the current block, we don't need to promote
  // or change anything about the local variable, we can simply
  // return the found record
  if (search_block == this) {
    return found_variable;
  }

  // insert a cache record into the current block
  // if the result was found in a parent block
  variables.insert({ symbol, *found_variable });
  LocalVariable& variable = variables.at(symbol);
  variable.declared_locally = false;

  if (function_depth > 0) {
    switch (variable.value_location.type) {
      case ValueLocation::Type::LocalFrame: {
        variable.value_location.type = ValueLocation::Type::FarFrame;
        variable.value_location.as.far_frame.depth = function_depth;

        // if we accessed a local variable from another function we need
        // to tell the function that the index got leaked
        search_block->parent_function->leak_slot(variable.value_location.as.far_frame.offset);
        break;
      }
      case ValueLocation::Type::FarFrame: {
        variable.value_location.as.far_frame.depth += function_depth;
        break;
      }
      case ValueLocation::Type::Global: {
        return &variable;
      }
      default: {
        assert(false && "unexpected type");
        break;
      }
    }
  }

  return &variable;
}

}  // namespace charly::core::compiler
