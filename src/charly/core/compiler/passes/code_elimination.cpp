/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

#include "charly/core/compiler/passes/code_elimination.h"

namespace charly::core::compiler::ast {

ref<Statement> CodeEliminationPass::transform(const ref<Block>& node) {
  auto it = node->statements.begin();
  while (it != node->statements.end()) {
    const ref<Statement>& stmt = *it;

    // erase statements that have no effects
    if (!stmt->has_side_effects()) {
      it = node->statements.erase(it);
      continue;
    }

    // remove all statements after a terminating statement (return, break, continue, throw, export)
    if (stmt->terminates_block()) {
      node->statements.erase(std::next(it), node->statements.end());
      break;
    }

    // unwrap blocks
    if (auto block = cast<Block>(stmt)) {
      // cannot unwrap blocks that carry break-information
      // switch statements use this type of block to be able to jump to the end of the statement
      if (!block->allows_break) {
        it = node->statements.erase(it);
        it = node->statements.insert(it, block->statements.begin(), block->statements.end());
        continue;
      }
    }

    if (auto binop = cast<BinaryOp>(stmt)) {
      it = node->statements.erase(it);
      it = node->statements.insert(it, binop->rhs);
      it = node->statements.insert(it, binop->lhs);
      continue;
    }

    if (auto unaryop = cast<UnaryOp>(stmt)) {
      it = node->statements.erase(it);
      it = node->statements.insert(it, unaryop->expression);
      continue;
    }

    if (auto typeofop = cast<Typeof>(stmt)) {
      it = node->statements.erase(it);
      it = node->statements.insert(it, typeofop->expression);
      continue;
    }

    if (auto formatstring = cast<FormatString>(stmt)) {
      it = node->statements.erase(it);
      it = node->statements.insert(it, formatstring->elements.begin(), formatstring->elements.end());
      continue;
    }

    if (auto tuple = cast<Tuple>(stmt)) {
      if (!tuple->has_spread_elements()) {
        it = node->statements.erase(it);
        it = node->statements.insert(it, tuple->elements.begin(), tuple->elements.end());
        continue;
      }
    }

    if (auto list = cast<List>(stmt)) {
      if (!list->has_spread_elements()) {
        it = node->statements.erase(it);
        it = node->statements.insert(it, list->elements.begin(), list->elements.end());
        continue;
      }
    }

    if (auto dict = cast<Dict>(stmt)) {
      if (!dict->has_spread_elements()) {
        it = node->statements.erase(it);

        for (auto& entry : dict->elements) {
          it = node->statements.insert(it, entry->value);
          it = node->statements.insert(it, entry->key);

          // need to move forward again to place all the elements in the correct order
          it = std::next(it, 2);
        }

        // go back to the first element pair that was inserted
        it = std::prev(it, dict->elements.size() * 2);
        continue;
      }
    }

    if (auto exp = cast<ExpressionWithSideEffects>(stmt)) {
      if (exp->block->has_side_effects() && !exp->expression->has_side_effects()) {
        *it = exp->block;
        continue;
      }
    }

    it++;
  }

  return node;
}

ref<Expression> CodeEliminationPass::transform(const ref<ExpressionWithSideEffects>& node) {
  if (!node->block->has_side_effects()) {
    return node->expression;
  }
  return node;
}

}  // namespace charly::core::compiler::ast
