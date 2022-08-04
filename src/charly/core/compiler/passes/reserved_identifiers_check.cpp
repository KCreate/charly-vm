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

#include <algorithm>
#include <unordered_set>

#include "charly/core/compiler/passes/reserved_identifiers_check.h"

namespace charly::core::compiler::ast {

// list of identifiers which cannot be used as member fields
// (member properties and functions) of classes
static std::unordered_set<std::string> kIllegalMemberNames = { "klass", "constructor" };

// list of identifiers which cannot be used as static fields
// (static properties and functions) of classes
static std::unordered_set<std::string> kIllegalStaticNames = { "klass", "name", "parent", "constructor" };

bool is_reserved_identifier(const std::string& str) {
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
  for (const ref<UnpackTargetElement>& element : node->target->elements) {
    CHECK(isa<Id>(element->target));
    auto id = cast<Id>(element->target);
    if (is_reserved_identifier(id->value)) {
      m_console.error(id, "'", id->value, "' is a reserved variable id");
    }
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
  // check for reserved member property names
  for (const ref<ClassProperty>& prop : node->member_properties) {
    if (is_reserved_identifier(prop->name->value) || kIllegalMemberNames.count(prop->name->value)) {
      m_console.error(prop->name, "'", prop->name->value, "' cannot be the name of a property");
      continue;
    }
  }

  // check for reserved member function names
  for (const ref<Function>& func : node->member_functions) {
    if (is_reserved_identifier(func->name->value) || kIllegalMemberNames.count(func->name->value)) {
      m_console.error(func->name, "'", func->name->value, "' cannot be the name of a member function");
    }
  }

  // check for reserved static property names
  for (const ref<ClassProperty>& prop : node->static_properties) {
    if (is_reserved_identifier(prop->name->value) || kIllegalStaticNames.count(prop->name->value)) {
      m_console.error(prop->name, "'", prop->name->value, "' cannot be the name of a static property");
    }
  }

  // check for reserved static function names
  for (const ref<Function>& func : node->static_functions) {
    if (is_reserved_identifier(func->name->value) || kIllegalStaticNames.count(func->name->value)) {
      m_console.error(func->name, "'", func->name->value, "' cannot be the name of a static function");
    }
  }
}

}  // namespace charly::core::compiler::ast
