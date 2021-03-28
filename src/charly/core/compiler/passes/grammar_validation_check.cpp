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

#include "charly/core/compiler/passes/grammar_validation_check.h"

namespace charly::core::compiler::ast {

void GrammarValidationCheck::inspect_leave(const ref<Dict>& node) {
  for (ref<DictEntry>& entry : node->elements) {
    ref<Expression>& key = entry->key;
    ref<Expression>& value = entry->value;

    // key-only elements
    if (value.get() == nullptr) {
      if (ref<Name> name = cast<Name>(key)) {
        value = make<Id>(name);
        key = make<Symbol>(name);
        continue;
      }

      if (ref<MemberOp> member = cast<MemberOp>(key)) {
        value = key;
        key = make<Symbol>(member->member);
        key->set_location(member);
        continue;
      }

      // { ...other }
      if (ref<Spread> spread = cast<Spread>(key)) {
        continue;
      }

      m_console.error(key, "expected identifier, member access or spread expression");
      continue;
    }

    if (ref<String> string = cast<String>(key)) {
      key = make<Symbol>(string);
      continue;
    }

    if (ref<Name> name = cast<Name>(key)) {
      key = make<Symbol>(name);
      continue;
    }

    if (isa<FormatString>(key)) {
      key = make<BuiltinOperation>(ir::BuiltinId::castsymbol, key);
      continue;
    }

    m_console.error(key, "expected identifier or string literal");
  }
}

void GrammarValidationCheck::inspect_leave(const ref<Function>& node) {
  bool default_argument_passed = false;
  bool spread_argument_passed = false;
  for (const ref<FunctionArgument>& argument : node->arguments) {

    // no parameters allowed after spread argument (...x)
    if (spread_argument_passed) {
      Location excess_arguments_location = argument->location();
      excess_arguments_location.set_end(node->arguments.back()->location());
      m_console.error(excess_arguments_location, "excess parameter(s)");
      break;
    }

    if (argument->spread_initializer) {
      spread_argument_passed = true;

      // spread arguments cannot have default values
      if (argument->default_value) {
        m_console.error(argument, "spread argument cannot have a default value");
      }

      continue;
    }

    // missing default value
    if (argument->default_value) {
      default_argument_passed = true;
    } else if (default_argument_passed) {
      m_console.error(argument, "argument '", argument->name->value , "' is missing a default value");
    }
  }
}

}  // namespace charly::core::compiler::ast
