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
#include <unordered_set>

#include "charly/core/compiler/passes/reserved_identifiers_check.h"

namespace charly::core::compiler::ast {

// list of reserved identifiers that are
// used by the compiler itself
static const std::unordered_set<std::string> kReservedIdentifiers = {
  "$",
  "$$"
};

bool is_reserved_identifier(const std::string& str) {
  if (kReservedIdentifiers.count(str)) {
    return true;
  }

  // argument index shorthand identifiers ($0, $1, etc)
  if (str.size() > 1 && str[0] == '$') {
    if (std::all_of(str.begin() + 1, str.end(), ::isdigit)) {
      return true;
    }
  }

  return false;
}

void ReservedIdentifiersCheck::inspect_leave(const ref<Declaration>& node) {
  if (is_reserved_identifier(node->name->value)) {
    m_console.error("reserved variable name", node->name);
  }
}

void ReservedIdentifiersCheck::inspect_leave(const ref<UnpackDeclaration>& node) {
  if (ref<Tuple> tuple = cast<Tuple>(node->target)) {
    for (ref<Expression>& element : tuple->elements) {
      switch (element->type()) {
        case Node::Type::Name: {
          if (is_reserved_identifier(cast<Name>(element)->value)) {
            m_console.error("reserved variable name", element);
          }
          break;
        }
        case Node::Type::Spread: {
          ref<Spread> spread = cast<Spread>(element);

          assert(isa<Name>(spread->expression));
          if (is_reserved_identifier(cast<Name>(spread->expression)->value)) {
            m_console.error("reserved variable name", spread->expression);
          }

          break;
        }
        default: {
          assert(false && "unexpected node");
        }
      }
    }
  } else if (ref<Dict> dict = cast<Dict>(node->target)) {
    for (ref<DictEntry>& entry : dict->elements) {
      switch (entry->key->type()) {
        case Node::Type::Name: {
          if (is_reserved_identifier(cast<Name>(entry->key)->value)) {
            m_console.error("reserved variable name", entry->key);
          }
          break;
        }
        case Node::Type::Spread: {
          ref<Spread> spread = cast<Spread>(entry->key);

          assert(isa<Name>(spread->expression));
          if (is_reserved_identifier(cast<Name>(spread->expression)->value)) {
            m_console.error("reserved variable name", spread->expression);
          }

          break;
        }
        default: {
          assert(false && "unexpected node");
        }
      }
    }
  } else {
    assert(false && "unexpected node");
  }
}

void ReservedIdentifiersCheck::inspect_leave(const ref<Function>& node) {
  for (const ref<Expression>& argument : node->arguments) {
    switch (argument->type()) {
      case Node::Type::Name: {
        if (is_reserved_identifier(cast<Name>(argument)->value)) {
          m_console.error("reserved variable name", argument);
        }
        break;
      }
      case Node::Type::Assignment: {
        ref<Assignment> assignment = cast<Assignment>(argument);

        assert(isa<Name>(assignment->target));
        if (is_reserved_identifier(cast<Name>(assignment->target)->value)) {
          m_console.error("reserved variable name", assignment->target);
        }
        break;
      }
      case Node::Type::Spread: {
        ref<Spread> spread = cast<Spread>(argument);

        assert(isa<Name>(spread->expression));
        if (is_reserved_identifier(cast<Name>(spread->expression)->value)) {
          m_console.error("reserved variable name", spread->expression);
        }

        break;
      }
      default: {
        assert(false && "unexpected node");
      }
    }
  }
}

void ReservedIdentifiersCheck::inspect_leave(const ref<Class>& node) {
  for (const ref<ClassProperty>& prop : node->member_properties) {
    if (is_reserved_identifier(prop->name->value)) {
      m_console.error("reserved variable name", prop->name);
    }
  }

  for (const ref<Function>& prop : node->member_functions) {
    if (is_reserved_identifier(prop->name->value)) {
      m_console.error("reserved variable name", prop->name);
    }
  }
}

}  // namespace charly::core::compiler::ast
