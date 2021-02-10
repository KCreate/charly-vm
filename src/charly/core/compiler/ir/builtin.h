/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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

#include <cstdint>
#include <string>
#include <unordered_map>

#pragma once

namespace charly::core::compiler::ir {

// ids of builtin operations
enum BuiltinId : uint16_t {

  // create a fiber, but don't start it
  // - function
  // - tuple containing arguments
  fibercreate,

  // create a fiber and start running it
  // - function
  // - tuple containing arguments
  fiberspawn,

  // yield from the current fiber
  // pauses the current fiber and resumes the callee fiber
  // - argument
  fiberyield,

  // import a builtin module or file from the file system
  // - value
  // - source path
  importmodule,

  // get the next result tuple from the iterator
  // - iterator
  iteratornext,

  // concatenate values together
  stringconcat,

  // cast value to a specific type
  // - value
  caststring,
  castsymbol,
  castgenerator,
  castiterator
};

// number of required arguments for the builtin operations
// -1 indicates that there is no min or max limit
static constexpr int8_t kBuiltinArgumentCount[] = {
  2,   // fibercreate
  2,   // fiberspawn
  1,   // fiberyield
  2,   // importmodule
  1,   // iteratornext
  -1,  // stringconcat
  1,   // caststring
  1,   // castsymbol
  1,   // castgenerator
  1    // castiterator
};

// clang-format off

// names of builtin operations
static std::string kBuiltinNames[] = {
  "fibercreate",
  "fiberspawn",
  "fiberyield",
  "importmodule",
  "iteratornext",
  "stringconcat",
  "caststring",
  "castsymbol",
  "castgenerator",
  "castiterator"
};

// mapping from name of builtin operation to its Id
static std::unordered_map<std::string, BuiltinId> kBuiltinNameMapping = {
  { "fibercreate",   BuiltinId::fibercreate },
  { "fiberspawn",    BuiltinId::fiberspawn },
  { "fiberyield",  BuiltinId::fiberyield },
  { "importmodule",  BuiltinId::importmodule },
  { "iteratornext",  BuiltinId::iteratornext },
  { "stringconcat",  BuiltinId::stringconcat },
  { "caststring",    BuiltinId::caststring },
  { "castsymbol",    BuiltinId::castsymbol },
  { "castgenerator", BuiltinId::castgenerator },
  { "castiterator",  BuiltinId::castiterator }
};

// clang-format on

}  // namespace charly::core::compiler::ir
