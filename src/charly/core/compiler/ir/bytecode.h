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
#include "charly/value.h"

#pragma once

namespace charly::core::compiler::ir {

#define FOREACH_OPERANDTYPE(V)                              \
                                                            \
  /* count operands */                                      \
  V(Count8, uint8_t)                                        \
  V(Count16, uint16_t)                                      \
  V(Count32, uint32_t)                                      \
  V(Count64, uint64_t)                                      \
                                                            \
  /* index into the local variables of the current frame */ \
  V(Local, uint8_t)                                         \
                                                            \
  /* hashed symbol value */                                 \
  V(Symbol, SYMBOL)                                         \
                                                            \
  /* relative offset to label */                            \
  V(Label, uint32_t)                                        \
                                                            \
  /* immediate value */                                     \
  V(Immediate, VALUE)

enum class OperandType : uint16_t {
#define ID(name, _) name,
  FOREACH_OPERANDTYPE(ID)
#undef ID
};

#define OPTYPE(name, type) static const size_t Op##name = sizeof(type);
FOREACH_OPERANDTYPE(OPTYPE)
#undef OPTYPE

// opcode listing
//  - name
//  - size of operands
#define FOREACH_OPCODE(V)                      \
  /* machine control */                        \
  V(nop, 0)                                    \
  V(halt, 0)                                   \
                                               \
  /* misc. instructions */                     \
  V(register_symbol, OpSymbol + OpLabel)       \
  V(import, 0)                                 \
  V(stringconcat, OpCount8)                    \
  V(type, 0)                                   \
                                               \
  /* stack management */                       \
  V(pop, 0)                                    \
  V(dup, 0)                                    \
                                               \
  /* control flow */                           \
  V(jmp, OpLabel)                              \
  V(jmpf, OpLabel)                             \
  V(jmpt, OpLabel)                             \
  V(throwex, 0)                                \
                                               \
  /* function control flow */                  \
  V(call, OpCount8)                            \
  V(callmember, OpSymbol + OpCount8)           \
  V(callspread, OpCount8)                      \
  V(callmemberspread, OpSymbol + OpCount8)     \
  V(ret, 0)                                    \
                                               \
  /* load operations */                        \
  V(load, OpImmediate)                         \
  V(loadself, 0)                               \
  V(loadglobal, OpSymbol)                      \
  V(loadlocal, OpCount8)                       \
  V(loadfar, OpCount8 + OpCount8)              \
  V(loadattr, OpSymbol)                        \
  V(loadattrvalue, 0)                          \
  V(loadsuper, 0)                              \
  V(loadsuperattr, OpSymbol)                   \
                                               \
  /* write operations */                       \
  V(setglobal, OpSymbol)                       \
  V(setlocal, OpCount8)                        \
  V(setfar, OpCount8 + OpCount8)               \
  V(setattr, OpSymbol)                         \
  V(setattrvalue, 0)                           \
                                               \
  /* value destructuring operations */         \
  V(unpacksequence, OpCount8)                  \
  V(unpacksequencespread, OpCount8 + OpCount8) \
  V(unpackobject, OpCount8)                    \
  V(unpackobjectspread, OpCount8)              \
                                               \
  /* value allocation */                       \
  V(makefunc, OpLabel)                         \
  V(makeclass, OpLabel)                        \
  V(makestr, OpLabel)                          \
  V(makearr, OpCount16)                        \
  V(makedict, OpCount16)                       \
  V(maketuple, OpCount16)                      \
                                               \
  /* fiber management */                       \
  V(fibercreate, 0)                            \
  V(fiberspawn, 0)                             \
  V(fiberyield, 0)                             \
  V(fiberawait, 0)                             \
                                               \
  /* cast instructions */                      \
  V(caststring, 0)                             \
  V(castsymbol, 0)                             \
  V(castiterator, 0)                           \
                                               \
  /* arithmetic operations */                  \
  V(add, 0)                                    \
  V(sub, 0)                                    \
  V(mul, 0)                                    \
  V(div, 0)                                    \
  V(mod, 0)                                    \
  V(pow, 0)                                    \
  V(usub, 0)                                   \
                                               \
  /* comparison operations */                  \
  V(eq, 0)                                     \
  V(neq, 0)                                    \
  V(lt, 0)                                     \
  V(gt, 0)                                     \
  V(le, 0)                                     \
  V(ge, 0)                                     \
  V(unot, 0)                                   \
                                               \
  /* bitwise operations */                     \
  V(shl, 0)                                    \
  V(shr, 0)                                    \
  V(shru, 0)                                   \
  V(band, 0)                                   \
  V(bor, 0)                                    \
  V(bxor, 0)                                   \
  V(ubnot, 0)

enum class Opcode : uint16_t {
#define ID(name, _) name,
  FOREACH_OPCODE(ID)
#undef ID
};

/*
 * total length of instruction in bytes (including opcode)
 * */
static constexpr size_t kOpcodeLength[] = {
#define LENGTH(_, length) length + 1,
  FOREACH_OPCODE(LENGTH)
#undef LENGTH
};

static std::string kOpcodeNames[] = {
#define NAME(name, _) #name,
  FOREACH_OPCODE(NAME)
#undef NAME
};

static std::unordered_map<std::string, Opcode> kOpcodeNameMapping = {
#define PAIR(name, _) { #name, Opcode::name },
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
// clang-format on

}  // namespace charly::core::compiler::ir
