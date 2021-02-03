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

#include "charly/utils/colorwriter.h"
#include "charly/core/compiler/pass.h"

#pragma once

namespace charly::core::compiler::ast {

class DumpPass : public Pass {
public:
  DumpPass(std::ostream& out = std::cout, bool print_location = true) :
    m_writer(out), m_print_location(print_location) {}

private:
  utils::ColorWriter m_writer;
  bool m_print_location;

  virtual void enter(const ref<Node>& node) override;

  void dump(const ref<Node>&);
  void dump(const ref<Assignment>& node);
  void dump(const ref<BinaryOp>& node);
  void dump(const ref<UnaryOp>& node);
  void dump(const ref<As>& node);
  void dump(const ref<Id>& node);
  void dump(const ref<Int>& node);
  void dump(const ref<Float>& node);
  void dump(const ref<Bool>& node);
  void dump(const ref<Char>& node);
  void dump(const ref<String>& node);
  void dump(const ref<Function>& node);
  void dump(const ref<Class>& node);
  void dump(const ref<ClassProperty>& node);
  void dump(const ref<MemberOp>& node);
  void dump(const ref<Declaration>& node);
  void dump(const ref<Try>& node);
  void dump(const ref<For>& node);
};

}  // namespace charly::core::compiler::ast
