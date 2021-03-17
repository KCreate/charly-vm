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
#include <vector>

#include "charly/core/compiler/token.h"
#include "charly/utils/macros.h"
#include "charly/value.h"

#pragma once

namespace charly::core::compiler::ir {

using Label = uint32_t;

#define FOREACH_OPERANDTYPE(V)   \
                                 \
  /* count operands */           \
  V(Count8, uint8_t)             \
  V(Count16, uint16_t)           \
                                 \
  /* hashed symbol value */      \
  V(Symbol, std::string)         \
                                 \
  /* relative offset to label */ \
  V(Offset, Label)               \
                                 \
  /* immediate value */          \
  V(Immediate, VALUE)

enum class OperandType : uint16_t {
#define ID(name, _) name,
  FOREACH_OPERANDTYPE(ID)
#undef ID
};

// FIXME: add comment why this is needed
//        has to do with macro shizzle below
static const size_t Size = 0;

#define OPTYPE(name, type) static const size_t Op##name##Size = sizeof(type);
FOREACH_OPERANDTYPE(OPTYPE)
#undef OPTYPE

#define OPTYPE(name, type) using Op##name = type;
FOREACH_OPERANDTYPE(OPTYPE)
#undef OPTYPE

// opcode listing
//  - name
//  - size of operands
#define FOREACH_OPCODE(V)                          \
  /* machine control */                            \
  V(nop)                                           \
  V(halt)                                          \
                                                   \
  /* misc. instructions */                         \
  V(import)                                        \
  V(stringconcat, OpCount8)                        \
  V(declareglobal, OpSymbol)                       \
  V(type)                                          \
                                                   \
  /* stack management */                           \
  V(pop)                                           \
  V(dup)                                           \
  V(dup2)                                          \
                                                   \
  /* control flow */                               \
  V(jmp, OpOffset)                                 \
  V(jmpf, OpOffset)                                \
  V(jmpt, OpOffset)                                \
  V(throwex)                                       \
                                                   \
  /* function control flow */                      \
  V(call, OpCount8)                                \
  V(callspread, OpCount8)                          \
  V(ret)                                           \
                                                   \
  /* load operations */                            \
  V(load, OpImmediate)                             \
  V(loadsymbol, OpSymbol) /* pseudo instruction */ \
  V(loadcontextself)                               \
  V(loadglobal, OpSymbol)                          \
  V(loadlocal, OpCount8)                           \
  V(loadfar, OpCount8, OpCount8)                   \
  V(loadattr, OpSymbol)                            \
  V(loadattrvalue)                                 \
  V(loadsuper)                                     \
  V(loadsuperattr, OpSymbol)                       \
                                                   \
  /* write operations */                           \
  V(setglobal, OpSymbol)                           \
  V(setlocal, OpCount8)                            \
  V(setfar, OpCount8, OpCount8)                    \
  V(setattr, OpSymbol)                             \
  V(setattrvalue)                                  \
                                                   \
  /* value destructuring operations */             \
  V(unpacksequence, OpCount8)                      \
  V(unpacksequencespread, OpCount8, OpCount8)      \
  V(unpackobject, OpCount8)                        \
  V(unpackobjectspread, OpCount8)                  \
                                                   \
  /* value allocation */                           \
  V(makefunc, OpOffset)                            \
  V(makeclass, OpCount8, OpCount8, OpCount8)       \
  V(makesubclass, OpCount8, OpCount8, OpCount8)    \
  V(makestr, OpOffset)                             \
  V(makelist, OpCount16)                           \
  V(makelistspread, OpCount16)                     \
  V(makedict, OpCount16)                           \
  V(makedictspread, OpCount16)                     \
  V(maketuple, OpCount16)                          \
  V(maketuplespread, OpCount16)                    \
                                                   \
  /* fiber management */                           \
  V(fibercreate)                                   \
  V(fiberspawn)                                    \
  V(fiberyield)                                    \
  V(fiberawait)                                    \
                                                   \
  /* cast operations */                            \
  V(caststring)                                    \
  V(castsymbol)                                    \
  V(castiterator)                                  \
                                                   \
  /* iterator operations */                        \
  V(iteratornext)                                  \
                                                   \
  /* arithmetic operations */                      \
  V(add)                                           \
  V(sub)                                           \
  V(mul)                                           \
  V(div)                                           \
  V(mod)                                           \
  V(pow)                                           \
  V(usub)                                          \
                                                   \
  /* comparison operations */                      \
  V(eq)                                            \
  V(neq)                                           \
  V(lt)                                            \
  V(gt)                                            \
  V(le)                                            \
  V(ge)                                            \
  V(unot)                                          \
                                                   \
  /* bitwise operations */                         \
  V(shl)                                           \
  V(shr)                                           \
  V(shru)                                          \
  V(band)                                          \
  V(bor)                                           \
  V(bxor)                                          \
  V(ubnot)

enum class Opcode : uint16_t {
#define ID(name, ...) name,
  FOREACH_OPCODE(ID)
#undef ID
};

/*
 * total length of instruction in bytes (including opcode)
 * */
static constexpr size_t kOpcodeLength[] = {
#define SUM_ELEMENT(type) + type##Size
#define MAP(name, ...) 1 + CHARLY_VA_FOR_EACH(SUM_ELEMENT, __VA_ARGS__),
  FOREACH_OPCODE(MAP)
#undef MAP
#undef SUM_ELEMENT
};

static std::string kOpcodeNames[] = {
#define NAME(name, ...) #name,
  FOREACH_OPCODE(NAME)
#undef NAME
};

static std::unordered_map<std::string, Opcode> kOpcodeNameMapping = {
#define PAIR(name, ...) { #name, Opcode::name },
  FOREACH_OPCODE(PAIR)
#undef PAIR
};

// clang-format off
static std::unordered_map<TokenType, Opcode> kBinopOpcodeMapping = {

  // arithmetic
  { TokenType::Plus, Opcode::add, },
  { TokenType::Minus, Opcode::sub, },
  { TokenType::Mul, Opcode::mul, },
  { TokenType::Div, Opcode::div, },
  { TokenType::Mod, Opcode::mod, },
  { TokenType::Pow, Opcode::pow, },

  // bitwise
  { TokenType::BitAND, Opcode::band, },
  { TokenType::BitOR, Opcode::bor, },
  { TokenType::BitXOR, Opcode::bxor, },
  { TokenType::BitLeftShift, Opcode::shl, },
  { TokenType::BitRightShift, Opcode::shr, },
  { TokenType::BitUnsignedRightShift, Opcode::shru, },

  // comparison
  { TokenType::Equal, Opcode::eq, },
  { TokenType::NotEqual, Opcode::neq, },
  { TokenType::LessThan, Opcode::lt, },
  { TokenType::GreaterThan, Opcode::gt, },
  { TokenType::LessEqual, Opcode::le, },
  { TokenType::GreaterEqual, Opcode::band, }
};

static std::unordered_map<TokenType, Opcode> kUnaryopOpcodeMapping = {
  { TokenType::Minus, Opcode::usub, },
  { TokenType::UnaryNot, Opcode::unot, },
  { TokenType::BitNOT, Opcode::ubnot, },
};

static std::unordered_map<BuiltinId, Opcode> kBuiltinOperationOpcodeMapping = {
  { BuiltinId::fibercreate,  Opcode::fibercreate,  },
  { BuiltinId::fiberspawn,   Opcode::fiberspawn,   },
  { BuiltinId::fiberyield,   Opcode::fiberyield,   },
  { BuiltinId::importmodule, Opcode::import,       },
  { BuiltinId::iteratornext, Opcode::iteratornext, },
  { BuiltinId::stringconcat, Opcode::stringconcat, },
  { BuiltinId::caststring,   Opcode::caststring,   },
  { BuiltinId::castsymbol,   Opcode::castsymbol,   },
  { BuiltinId::castiterator, Opcode::castiterator, },
};
// clang-format on

}  // namespace charly::core::compiler::ir
