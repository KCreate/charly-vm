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

#include <algorithm>

#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/astpass.h"

namespace charly::core::compiler::ast {

void Block::visit_children(ASTPass* pass) {
  for (ref<Statement>& node : this->statements) {
    node = cast<Statement>(pass->visit(node));
  }

  auto begin = this->statements.begin();
  auto end = this->statements.end();
  this->statements.erase(std::remove(begin, end, nullptr), end);
}

void Program::visit_children(ASTPass* pass) {
  this->block = cast<Block>(pass->visit(this->block));
}

void FormatString::visit_children(ASTPass* pass) {
  for (ref<Expression>& node : this->elements) {
    node = cast<Expression>(pass->visit(node));
  }

  auto begin = this->elements.begin();
  auto end = this->elements.end();
  this->elements.erase(std::remove(begin, end, nullptr), end);
}

void Tuple::visit_children(ASTPass* pass) {
  for (ref<Expression>& node : this->elements) {
    node = cast<Expression>(pass->visit(node));
  }

  auto begin = this->elements.begin();
  auto end = this->elements.end();
  this->elements.erase(std::remove(begin, end, nullptr), end);
}

}  // namespace charly::core::compiler
