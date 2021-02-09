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

#include <list>

#include "charly/core/compiler/passes/class_constructor_check.h"

namespace charly::core::compiler::ast {

void ClassConstructorCheck::inspect_leave(const ref<Class>& node) {
  if (!node->constructor || !node->parent)
    return;

  // depth-first search for super(...)
  std::function<bool(const ref<Node>&)> search_super_call = [&](const ref<Node>& node) {
    // check for super(...)
    if (ref<CallOp> call = cast<CallOp>(node)) {
      if (isa<Super>(call->target)) {
        return true;
      }
    }

    bool super_call_found = false;
    switch (node->type()) {
      case Node::Type::Function:
      case Node::Type::Class:
      case Node::Type::Spawn: {
        break;
      }
      default: {
        node->children([&](const ref<Node>& child) {
          if (super_call_found)
            return;

          super_call_found = search_super_call(child);
        });
        break;
      }
    }

    return super_call_found;
  };

  bool super_called = search_super_call(node->constructor->body);

  if (!super_called) {
    m_console.error(node->constructor->name, "missing call to super inside constructor of class '",
                    node->name->value, "'");
  }
}

}  // namespace charly::core::compiler::ast
