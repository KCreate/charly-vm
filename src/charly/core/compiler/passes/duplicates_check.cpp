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

#include "charly/core/compiler/passes/duplicates_check.h"

namespace charly::core::compiler::ast {

void DuplicatesCheck::inspect_leave(const ref<UnpackDeclaration>& node) {
  std::unordered_set<std::string> names;

  if (ref<Tuple> tuple = cast<Tuple>(node->target)) {
    for (ref<Expression>& element : tuple->elements) {
      switch (element->type()) {
        case Node::Type::Name: {
          ref<Name> name = cast<Name>(element);
          if (names.count(name->value)) {
            m_console.error(element, "duplicate unpack target");
          }
          names.insert(name->value);
          break;
        }
        case Node::Type::Spread: {
          ref<Spread> spread = cast<Spread>(element);
          ref<Name> name = cast<Name>(spread->expression);
          if (names.count(name->value)) {
            m_console.error(spread->expression, "duplicate unpack target");
          }
          names.insert(name->value);
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
          if (names.count(name->value)) {
            m_console.error(entry->key, "duplicate unpack target");
          }
          names.insert(name->value);
          break;
        }
        case Node::Type::Spread: {
          ref<Spread> spread = cast<Spread>(entry->key);
          ref<Name> name = cast<Name>(spread->expression);
          if (names.count(name->value)) {
            m_console.error(spread->expression, "duplicate unpack target");
          }
          names.insert(name->value);
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

void DuplicatesCheck::inspect_leave(const ref<Dict>& node) {
  std::unordered_set<std::string> dict_keys;

  for (ref<DictEntry>& entry : node->elements) {
    switch (entry->key->type()) {
      case Node::Type::Name: {
        ref<Name> name = cast<Name>(entry->key);

        if (dict_keys.count(name->value)) {
          m_console.error(entry->key, "duplicate key '", name->value, "'");
        }

        dict_keys.insert(name->value);
        break;
      }
      default: {
        continue;
      }
    }
  }
}

void DuplicatesCheck::inspect_leave(const ref<Function>& node) {
  std::unordered_set<std::string> argument_names;

  for (const ref<Expression>& argument : node->arguments) {
    switch (argument->type()) {
      case Node::Type::Name: {
        ref<Name> name = cast<Name>(argument);

        if (argument_names.count(name->value)) {
          m_console.error(name, "duplicate argument '", name->value, "'");
        } else {
          argument_names.insert(name->value);
        }

        break;
      }
      case Node::Type::Assignment: {
        ref<Assignment> assignment = cast<Assignment>(argument);
        ref<Name> name = cast<Name>(assignment->target);

        if (argument_names.count(name->value)) {
          m_console.error(assignment->target, "duplicate argument '", name->value, "'");
        } else {
          argument_names.insert(name->value);
        }

        break;
      }
      case Node::Type::Spread: {
        ref<Spread> spread = cast<Spread>(argument);
        ref<Name> name = cast<Name>(spread->expression);

        if (argument_names.count(name->value)) {
          m_console.error(spread->expression, "duplicate argument '", name->value, "'");
        } else {
          argument_names.insert(name->value);
        }

        break;
      }
      default: {
        assert(false && "unexpected node");
      }
    }
  }
}

void DuplicatesCheck::inspect_leave(const ref<Class>& node) {
  std::unordered_map<std::string, ref<ClassProperty>> class_member_properties;
  std::unordered_map<std::string, ref<Function>> class_member_functions;
  std::unordered_map<std::string, ref<ClassProperty>> class_static_properties;

  // check for duplicate properties
  for (const ref<ClassProperty>& prop : node->member_properties) {
    if (class_member_properties.count(prop->name->value)) {
      m_console.error(prop->name, "duplicate declaration of '", prop->name->value, "'");
      m_console.info(class_member_properties.at(prop->name->value)->name, "first declared here");
      continue;
    }

    class_member_properties.insert({prop->name->value, prop});
  }

  for (auto it = node->member_functions.begin(); it != node->member_functions.end();) {
    const ref<Function>& func = *it;
    const std::string& name = func->name->value;

    if (class_member_properties.count(name)) {
      m_console.error(func->name, "redeclaration of property '", func->name->value, "' as function");
      m_console.info(class_member_properties.at(name)->name, "declared as property here");
      ++it;
      continue;
    }

    if (name.compare("constructor") == 0) {
      if (node->constructor) {
        m_console.error(func->name, "duplicate class constructor");
        m_console.info(node->constructor->name, "first constructor declared here");
        ++it;
        continue;
      }

      node->constructor = func;
      it = node->member_functions.erase(it);
      continue;
    }

    if (class_member_functions.count(name)) {
      m_console.error(func->name, "duplicate declaration of member function '", func->name->value, "'");
      m_console.info(class_member_functions.at(name)->name, "first declared here");
      ++it;
      continue;
    }

    class_member_functions.insert({name, func});
    ++it;
  }

  // check for duplicate static properties
  for (const ref<ClassProperty>& prop : node->static_properties) {
    if (class_static_properties.count(prop->name->value)) {
      m_console.error(prop->name, "duplicate declaration of static property '", prop->name->value, "'");
      m_console.info(class_static_properties.at(prop->name->value)->name, "declared here");
      continue;
    }

    class_static_properties[prop->name->value] = prop;
  }
}

}  // namespace charly::core::compiler::ast
