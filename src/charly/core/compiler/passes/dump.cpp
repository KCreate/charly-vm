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

#include "charly/core/compiler/passes/dump.h"

using Color = charly::utils::Color;

namespace charly::core::compiler::ast {

void DumpPass::enter(const ref<Node>& node) {
  for (uint16_t i = 0; i < m_depth; i++) {
    m_writer << "  ";
  }

  m_writer << "- ";
  m_writer.fg(Color::Blue, node->node_name());

  AST_TYPESWITCH(node, { dump(node); });

  if (m_print_location) {
    m_writer << " " << '<' << node->location() << '>';
  }
  m_writer << '\n';
}

void DumpPass::dump(const ref<Node>&) {}

void DumpPass::dump(const ref<Assignment>& node) {
  if (node->operation != TokenType::Assignment) {
    m_writer << ' ';
    m_writer.fg(Color::Yellow, '\'', kTokenTypeStrings[static_cast<int>(node->operation)], '\'');
  }
}

void DumpPass::dump(const ref<BinaryOp>& node) {
  m_writer << ' ';
  m_writer.fg(Color::Yellow, '\'', kTokenTypeStrings[static_cast<int>(node->operation)], '\'');
}

void DumpPass::dump(const ref<UnaryOp>& node) {
  m_writer << ' ';
  m_writer.fg(Color::Blue, '\'', kTokenTypeStrings[static_cast<int>(node->operation)], '\'');
}

void DumpPass::dump(const ref<As>& node) {
  m_writer << ' ';
  m_writer.fg(Color::Yellow, '\"', node->name, '\"');
}

void DumpPass::dump(const ref<Id>& node) {
  m_writer << ' ';
  m_writer.fg(Color::Yellow, node->value);
}

void DumpPass::dump(const ref<Int>& node) {
  m_writer << ' ';
  m_writer.fg(Color::Red, node->value);
}

void DumpPass::dump(const ref<Float>& node) {
  m_writer << ' ';
  m_writer.fg(Color::Red, node->value);
}

void DumpPass::dump(const ref<Bool>& node) {
  m_writer << ' ';
  m_writer.fg(Color::Red, node->value ? "true" : "false");
}

void DumpPass::dump(const ref<Char>& node) {
  m_writer << ' ';
  m_writer.fg(Color::Yellow, '\'');
  char buf[4] = { 0 };
  char* buf_ptr = buf;
  char* end = utf8::append(node->value, buf_ptr);
  m_writer.fg(Color::Yellow, std::string(buf_ptr, end - buf_ptr), '\'');
}

void DumpPass::dump(const ref<String>& node) {
  m_writer << ' ';
  m_writer.fg(Color::Yellow, '\"', node->value, '\"');
}

void DumpPass::dump(const ref<Function>& node) {
  m_writer << ' ';

  if (node->arrow_function) {
    m_writer.fg(Color::Cyan, "anonymous");
  } else {
    m_writer.fg(Color::Yellow, node->name);
  }
}

void DumpPass::dump(const ref<Class>& node) {
  m_writer << ' ';
  m_writer.fg(Color::Yellow, node->name);
}

void DumpPass::dump(const ref<ClassProperty>& node) {
  if (node->is_static) {
    m_writer << ' ';
    m_writer.fg(Color::Red, "static");
  }
  m_writer << ' ';
  m_writer.fg(Color::Yellow, node->name);
}

void DumpPass::dump(const ref<MemberOp>& node) {
  m_writer << ' ';
  m_writer.fg(Color::Yellow, '\"', node->member, '\"');
}

void DumpPass::dump(const ref<Declaration>& node) {
  if (isa<Tuple>(node->target) || isa<Dict>(node->target)) {
    m_writer << ' ';
    m_writer.fg(Color::Cyan, "unpack");
  }
  if (node->constant) {
    m_writer << ' ';
    m_writer.fg(Color::Red, "const");
  }
}

void DumpPass::dump(const ref<Try>& node) {
  m_writer << ' ';
  m_writer.fg(Color::Yellow, node->exception_name);
}

void DumpPass::dump(const ref<For>& node) {
  if (node->constant_value) {
    m_writer << ' ';
    m_writer.fg(Color::Red, "const");
  }
}

}  // namespace charly::core::compiler::ast
