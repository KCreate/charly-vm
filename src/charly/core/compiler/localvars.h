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

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <memory>

#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/ir/valuelocation.h"

#pragma once

namespace charly::core::compiler {

// state of function local variable slots during
// the allocation phase
struct SlotInfo {
  bool used;
  bool leaked;
  bool constant;
};

// keeps track of frame slots and which frame slots need to be
// stored on the heap
struct BlockScope;
struct FunctionScope {
  FunctionScope(ast::ref<ast::Function> ast_function,
                std::shared_ptr<FunctionScope> parent_function,
                std::shared_ptr<BlockScope> parent_block) :
    ast_function(ast_function), parent_function(parent_function), parent_block(parent_block) {}

  // get a free slot
  uint32_t alloc_slot(bool constant);

  // free a slot and let it be reused again
  void free_slot(uint32_t index);

  // mark a slot as leaked, preventing it from being assigned to
  // other variables in the future
  void leak_slot(uint32_t index);

  ast::ref<ast::Function> ast_function;
  std::vector<SlotInfo> slots;
  std::shared_ptr<FunctionScope> parent_function;
  std::shared_ptr<BlockScope> parent_block;
};

struct LocalVariable {

  // the node that declared this variable
  ast::ref<ast::Node> ast_declaration;

  // the runtime location where this value can be found
  ir::ValueLocation value_location;

  // wether this value is declared in the current block
  bool declared_locally;

  // name of the variable
  std::string name;

  // constant value or not
  bool constant;
};

// keeps track of the variables declared inside blocks
// and serves as a lookup cache for variable lookups into
// parent blocks
struct BlockScope {
  BlockScope(ast::ref<ast::Block> block,
             std::shared_ptr<FunctionScope> parent_function,
             std::shared_ptr<BlockScope> parent_block) :
    ast_block(block), parent_function(parent_function), parent_block(parent_block) {}

  ~BlockScope() {
    for (const auto& entry : variables) {
      if (entry.second.declared_locally) {
        const auto& location = entry.second.value_location;
        switch (location.type) {
          case ir::ValueLocation::Type::LocalFrame: {
            parent_function->free_slot(entry.second.value_location.as.local_frame.offset);
            break;
          }
          case ir::ValueLocation::Type::FarFrame: {
            parent_function->free_slot(entry.second.value_location.as.far_frame.offset);
            break;
          }
          default: {
            break;
          }
        }
      }
    }
  }

  // register a new variable inside this block
  const LocalVariable* alloc_slot(const ast::ref<ast::Name>& symbol,
                                  const ast::ref<ast::Node>& declaration,
                                  bool constant = false);

  // check wether a given symbol was already declared inside this block
  bool symbol_declared(const std::string& symbol);

  // lookup the location of a symbol
  const LocalVariable* lookup_symbol(const std::string& symbol);

  ast::ref<ast::Block> ast_block;
  std::unordered_map<std::string, LocalVariable> variables;
  std::shared_ptr<FunctionScope> parent_function;
  std::shared_ptr<BlockScope> parent_block;
};

}  // namespace charly::core::compiler
