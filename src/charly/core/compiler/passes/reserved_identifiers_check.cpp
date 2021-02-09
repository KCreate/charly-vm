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

// list of identifiers which cannot be used as member fields
// (member properties and functions) of classes
static std::unordered_set<std::string> kIllegalMemberFieldNames = {
  "klass",
  "object_id"
};

// list of identifiers which cannot be used as static fields
// (static properties and functions) of classes
static std::unordered_set<std::string> kIllegalStaticFieldNames = {
  "constructor",
  "name",
  "parent"
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
    m_console.error(node->name, "'", node->name->value, "' is a reserved variable name");
  }
}

void ReservedIdentifiersCheck::inspect_leave(const ref<UnpackDeclaration>& node) {
  if (ref<Tuple> tuple = cast<Tuple>(node->target)) {
    for (ref<Expression>& element : tuple->elements) {
      switch (element->type()) {
        case Node::Type::Name: {
          ref<Name> name = cast<Name>(element);

          if (is_reserved_identifier(name->value)) {
            m_console.error(name, "'", name->value, "' is a reserved variable name");
          }

          break;
        }
        case Node::Type::Spread: {
          ref<Spread> spread = cast<Spread>(element);
          assert(isa<Name>(spread->expression));
          ref<Name> name = cast<Name>(spread->expression);

          if (is_reserved_identifier(name->value)) {
            m_console.error(name, "'", name->value, "' is a reserved variable name");
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
          ref<Name> name = cast<Name>(entry->key);

          if (is_reserved_identifier(name->value)) {
            m_console.error(name, "'", name->value, "' is a reserved variable name");
          }
          break;
        }
        case Node::Type::Spread: {
          ref<Spread> spread = cast<Spread>(entry->key);
          assert(isa<Name>(spread->expression));
          ref<Name> name = cast<Name>(spread->expression);

          if (is_reserved_identifier(name->value)) {
            m_console.error(name, "'", name->value, "' is a reserved variable name");
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
  for (const ref<FunctionArgument>& argument : node->arguments) {
    if (is_reserved_identifier(argument->name->value)) {
      m_console.error(argument->name, "'", argument->name->value, "' is a reserved variable name");
    }
  }
}

void ReservedIdentifiersCheck::inspect_leave(const ref<Class>& node) {
  for (const ref<ClassProperty>& prop : node->member_properties) {
    if (is_reserved_identifier(prop->name->value) || kIllegalMemberFieldNames.count(prop->name->value)) {
      m_console.error(prop->name, "'", prop->name->value, "' cannot be the name of a property");
      continue;
    }
  }

  for (const ref<Function>& func : node->member_functions) {
    if (is_reserved_identifier(func->name->value) || kIllegalMemberFieldNames.count(func->name->value)) {
      m_console.error(func->name, "'", func->name->value, "' cannot be the name of a member function");
    }
  }

  // check for duplicate static properties
  for (const ref<ClassProperty>& prop : node->static_properties) {
    if (is_reserved_identifier(prop->name->value) || kIllegalStaticFieldNames.count(prop->name->value)) {
      m_console.error(prop->name, "'", prop->name->value, "' cannot be the name of a static property");
    }
  }
}

}  // namespace charly::core::compiler::ast
