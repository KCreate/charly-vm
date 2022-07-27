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

#include "charly/core/compiler/passes/duplicates_check.h"

namespace charly::core::compiler::ast {

void DuplicatesCheck::inspect_leave(const ref<UnpackTarget>& node) {
  std::unordered_set<std::string> names;
  bool spread_passed = false;

  for (const ref<UnpackTargetElement>& element : node->elements) {
    // duplicate spread check
    if (element->spread) {
      if (spread_passed) {
        m_console.error(element, "excess spread");
      }

      spread_passed = true;
    }
  }
}

void DuplicatesCheck::inspect_leave(const ref<Dict>& node) {
  std::unordered_set<std::string> dict_keys;

  for (ref<DictEntry>& entry : node->elements) {
    switch (entry->key->type()) {
      case Node::Type::Symbol: {
        ref<Symbol> name = cast<Symbol>(entry->key);

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
  for (const ref<FunctionArgument>& argument : node->arguments) {
    if (argument_names.count(argument->name->value)) {
      m_console.error(argument->name, "duplicate argument '", argument->name->value, "'");
    } else {
      argument_names.insert(argument->name->value);
    }
  }
}

void DuplicatesCheck::inspect_leave(const ref<Class>& node) {
  std::unordered_map<std::string, ref<ClassProperty>> class_member_properties;
  std::unordered_map<SYMBOL, std::list<ref<Function>>> class_member_functions;
  std::unordered_map<std::string, ref<ClassProperty>> class_static_properties;
  std::unordered_map<SYMBOL, std::list<ref<Function>>> class_static_functions;

  // check for duplicate member properties
  for (const ref<ClassProperty>& prop : node->member_properties) {
    if (class_member_properties.count(prop->name->value)) {
      m_console.error(prop->name, "duplicate declaration of member property '", prop->name->value, "'");
      m_console.info(class_member_properties.at(prop->name->value)->name, "first declared here");
      continue;
    }

    class_member_properties.insert({ prop->name->value, prop });
  }

  // check for duplicate static properties
  for (const ref<ClassProperty>& prop : node->static_properties) {
    if (class_static_properties.count(prop->name->value)) {
      m_console.error(prop->name, "duplicate declaration of static property '", prop->name->value, "'");
      m_console.info(class_static_properties.at(prop->name->value)->name, "first declared here");
      continue;
    }

    class_static_properties[prop->name->value] = prop;
  }

  auto check_overload_conflicts = [&](auto& functions, auto& target_properties, auto& target_functions) {
    for (const ref<Function>& func : functions) {
      const auto& name = func->name->value;
      auto minargc = func->minimum_argc();
      auto argc = func->argc();
      bool has_spread = func->has_spread();

      // check for shadowed properties
      if (target_properties.count(name)) {
        m_console.error(func->name, "redeclaration of property '", name, "' as function");
        m_console.info(target_properties.at(name)->name, "first declared as property here");
        continue;
      }

      // initialize function overload group
      auto& overload_group = target_functions[crc32::hash_string(name)];

      // make sure the function doesn't collide with a previous overload
      bool duplicate_found = false;
      for (auto overload : overload_group) {
        uint8_t overload_argc = overload->argc();
        uint8_t overload_minargc = overload->minimum_argc();
        bool overload_has_spread = overload->has_spread();

        // 1. overload has spread argument that would capture other overload arguments
        // func foo(a, b, c...)
        // func foo(a, b, c = 1, d = 2)
        //
        // unclear wether foo(1, 2) should call first or second overload -> conflict!
        bool spread_conflict =
          (overload_has_spread && overload_argc <= minargc) || (has_spread && argc <= overload_minargc);

        // 2. previous overload range overlaps with new range
        // func foo(a, b, c = 1, d = 2)
        // func foo(a, b, d, c = 1)
        //
        // unclear wether foo(1, 2, 3) should call first or second overload -> conflict!
        bool overlap_conflict = (argc >= overload_minargc && minargc <= overload_argc) ||
                                (overload_argc >= minargc && overload_minargc <= argc);

        // check if the overloads overlap_hides_old_overload
        if (spread_conflict || overlap_conflict) {
          m_console.error(func->name, "function overload shadows previous overload");
          m_console.info(overload->name, "first declared here");
          duplicate_found = true;
          break;
        }
      }

      if (!duplicate_found) {
        overload_group.push_back(func);
      } else {
        break;
      }
    }
  };

  // check for duplicate member function overloads
  check_overload_conflicts(node->member_functions, class_member_properties, class_member_functions);

  // sort individual overload sets by minargc
  for (auto overload_group : class_member_functions) {
    auto& vec = overload_group.second;
    vec.sort([](const ref<Function>& left, const ref<Function>& right) {
      return left->minimum_argc() < right->minimum_argc();
    });
  }
  node->member_function_overloads = class_member_functions;

  // check for duplicate static functions or properties
  check_overload_conflicts(node->static_functions, class_static_properties, class_static_functions);

  // sort individual overload sets by minargc
  for (auto overload_group : class_static_functions) {
    auto& vec = overload_group.second;
    vec.sort([](const ref<Function>& left, const ref<Function>& right) {
      return left->minimum_argc() < right->minimum_argc();
    });
  }
  node->static_function_overloads = class_static_functions;
}

}  // namespace charly::core::compiler::ast
