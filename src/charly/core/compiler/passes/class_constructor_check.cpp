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

#include "charly/core/compiler/passes/class_constructor_check.h"

namespace charly::core::compiler::ast {

void ClassConstructorCheck::inspect_leave(const ref<Class>& node) {
  constructor_super_check(node);
  constructor_return_check(node);
}

void ClassConstructorCheck::constructor_super_check(const ref<Class>& node) {
  if (!node->constructor)
    return;

  // search for a call to the super constructor
  auto super_calls = Node::search_all(
    node->constructor->body,
    [&](const ref<Node>& node) {
      // check for super(...)
      if (ref<CallOp> call = cast<CallOp>(node)) {
        return isa<Super>(call->target);
      }

      return false;
    },
    [&](const ref<Node>& node) {
      Node::Type type = node->type();
      return type == Node::Type::Function || type == Node::Type::Class || type == Node::Type::Spawn;
    });

  bool missing_super_call = node->parent && super_calls.empty();
  bool illegal_super_call = !node->parent && !super_calls.empty();
  bool excess_super_calls = node->parent && super_calls.size() > 1;

  // classes that do not inherit from another class are not allowed to call the super constructor
  if (illegal_super_call) {
    m_console.error(node->constructor, "call to super not allowed in constructor of non-inheriting class '",
                    node->name->value, "'");
  } else {
    if (missing_super_call) {
      // classes that inherit from another class must call be super constructor
      m_console.error(node->constructor, "missing super constructor call in constructor of class '", node->name->value,
                      "'");
    } else if (excess_super_calls) {
      // there may only be one call to the super constructor
      m_console.error(node->constructor->name, "constructor of class '", node->name->value,
                      "' may only contain a single call to the super constructor");
    }
  }
}

void ClassConstructorCheck::constructor_return_check(const ref<Class>& node) {
  if (!node->constructor)
    return;

  // search for return statements
  auto return_ops = Node::search_all(
    node->constructor->body,
    [&](const ref<Node>& node) {
      return isa<Return>(node);
    },
    [&](const ref<Node>& node) {
      Node::Type type = node->type();
      return type == Node::Type::Function || type == Node::Type::Class || type == Node::Type::Spawn;
    });

  for (const auto& op : return_ops) {
    auto ret = cast<Return>(op);
    if (ret->value) {
      m_console.error(ret->value, "constructors must not return a value");
    }
  }
}

}  // namespace charly::core::compiler::ast
