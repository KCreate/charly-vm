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

#include "charly/core/compiler/codegenerator.h"

using namespace charly::core::compiler::ast;
using namespace charly::core::compiler::ir;

namespace charly::core::compiler {

void CodeGenerator::compile() {
  m_builder.label();

  apply(m_unit->ast);

  while (m_function_queue.size()) {
    compile_function(m_function_queue.front());
    m_function_queue.pop();
  }

  assert(m_builder.has_pending_labels() == false);
}

void CodeGenerator::compile_function(const QueuedFunction& function) {
  m_builder.begin_function(function.head_label, function.ast);
  m_builder.emit_nop();
  apply(function.ast->body);
  m_builder.end_function(function.head_label);
}

bool CodeGenerator::inspect_enter(const ref<Function>& node) {
  Label begin_label = m_builder.reserve_label();
  m_function_queue.push({ .head_label = begin_label, .ast = node });
  m_builder.emit_load(0xDEADBEEF);
  return false;
}

void CodeGenerator::inspect_leave(const ref<Int>& node) {
  m_builder.emit_load(node->value);
}

void CodeGenerator::inspect_leave(const ref<BinaryOp>& node) {
  switch (node->operation) {
    default: {
      assert(kBinopOpcodeMapping.count(node->operation));
      m_builder.emit(kBinopOpcodeMapping.at(node->operation));
      break;
    }
    case TokenType::And:
    case TokenType::Or: {
      assert(false && "not implemented");
      break;
    }
  }
}

void CodeGenerator::inspect_leave(const ref<UnaryOp>& node) {
  assert(kUnaryopOpcodeMapping.count(node->operation));
  m_builder.emit(kUnaryopOpcodeMapping.at(node->operation));
}

}  // namespace charly::core::compiler
