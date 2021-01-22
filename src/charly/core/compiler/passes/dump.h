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

#include <sstream>

#include "charly/core/compiler/astpass.h"
#include "charly/utils/colorwriter.h"

#pragma once

namespace charly::core::compiler::ast {

class DumpPass : public ASTPass {
  virtual void before_enter_any(const ref<Node>& node) override {
    for (uint16_t i = 0; i < m_depth; i++) {
      m_writer.write("  ");
    }

    m_writer.write("- ");
    m_writer.blue(node->name());
  }

  virtual void after_enter_any(const ref<Node>& node) override {
    if (m_print_location) {
      m_writer.write(" ");
      m_writer.write('<');
      m_writer.write(node->location());
      m_writer.write('>');
    }
    m_writer.write('\n');
    m_depth++;
  }

  virtual void before_leave_any(const ref<Node>&) override {
    m_depth--;
  }

  virtual bool enter(const ref<ANDAssignment>& node) override {
    m_writer.write(' ');
    m_writer.yellow('\'');
    m_writer.yellow(kTokenTypeStrings[static_cast<int>(node->operation)]);
    m_writer.yellow('\'');
    return true;
  }

  virtual bool enter(const ref<BinaryOp>& node) override {
    m_writer.write(' ');
    m_writer.yellow('\'');
    m_writer.yellow(kTokenTypeStrings[static_cast<int>(node->operation)]);
    m_writer.yellow('\'');
    return true;
  }

  virtual bool enter(const ref<UnaryOp>& node) override {
    m_writer.write(' ');
    m_writer.blue('\'');
    m_writer.blue(kTokenTypeStrings[static_cast<int>(node->operation)]);
    m_writer.blue('\'');
    return true;
  }

  virtual bool enter(const ref<As>& node) override {
    m_writer.write(' ');
    m_writer.yellow('\"');
    m_writer.yellow(node->name);
    m_writer.yellow('\"');
    return true;
  }

  virtual bool enter(const ref<Id>& node) override {
    m_writer.write(' ');
    m_writer.yellow(node->value);
    return true;
  }

  virtual bool enter(const ref<Int>& node) override {
    m_writer.write(' ');
    m_writer.red(node->value);
    return true;
  }

  virtual bool enter(const ref<Float>& node) override {
    m_writer.write(' ');
    m_writer.red(node->value);
    return true;
  }

  virtual bool enter(const ref<Bool>& node) override {
    m_writer.write(' ');
    m_writer.red(node->value ? "true" : "false");
    return true;
  }

  virtual bool enter(const ref<Char>& node) override {
    m_writer.write(' ');
    m_writer.yellow('\'');
    char buf[4] = { 0 };
    char* buf_ptr = buf;
    char* end = utf8::append(node->value, buf_ptr);
    m_writer.yellow(std::string(buf_ptr, end - buf_ptr));
    m_writer.yellow('\'');
    return true;
  }

  virtual bool enter(const ref<String>& node) override {
    m_writer.write(' ');
    m_writer.yellow('\"');
    m_writer.yellow(node->value);
    m_writer.yellow('\"');
    return true;
  }

public:
  DumpPass(std::ostream& stream = std::cout, bool print_location = true) :
    m_writer(stream), m_depth(0), m_print_location(print_location) {}

  utils::ColorWriter m_writer;
  uint16_t m_depth;
  bool m_print_location;
};

}  // namespace charly::core::compiler::ast
