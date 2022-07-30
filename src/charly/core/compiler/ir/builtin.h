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

#include <cstdint>
#include <string>
#include <unordered_map>

#pragma once

namespace charly::core::compiler::ir {

#define FOREACH_BUILTIN(V) \
  V(makefiber, 3)          \
  V(fiberjoin, 1)          \
  V(import, 2)             \
  V(iteratornext, 1)       \
  V(castbool, 1)           \
  V(caststring, 1)         \
  V(castsymbol, 1)         \
  V(castiterator, 1)       \
  V(panic, 0)

// ids of builtin operations
enum class BuiltinId : uint16_t {
#define ID(name, _) name,
  FOREACH_BUILTIN(ID)
#undef ID
};

// number of required arguments for the builtin operations
// -1 indicates that there is no min or max limit
static constexpr int8_t kBuiltinArgumentCount[] = {
#define COUNT(_, count) count,
  FOREACH_BUILTIN(COUNT)
#undef COUNT
};

// names of builtin operations
static std::string kBuiltinNames[] = {
#define NAME(name, _) #name,
  FOREACH_BUILTIN(NAME)
#undef NAME
};

// mapping from name of builtin operation to its Id
static std::unordered_map<std::string, BuiltinId> kBuiltinNameMapping = {
#define PAIR(name, _) { #name, BuiltinId::name },
  FOREACH_BUILTIN(PAIR)
#undef PAIR
};

}  // namespace charly::core::compiler::ir
