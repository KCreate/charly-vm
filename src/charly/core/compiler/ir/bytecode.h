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
#define FOREACH_OPCODE(V)                                          \
  /* nop - no operation                                            \
   * */                                                            \
  V(nop)                                                           \
  /* panic - abort machine                                         \
   * */                                                            \
  V(panic)                                                         \
                                                                   \
  /* import - import module or file                                \
   *                                                               \
   * stack arguments:                                              \
   * - modulename (filepath)                                       \
   * - sourcepath (path of current file)                           \
   *                                                               \
   * stack results:                                                \
   * - module (value returned by included file)                    \
   * */                                                            \
  V(import)                                                        \
  /* stringconcat - concatenate multiple values into a string      \
   *                                                               \
   * opcode operands:                                              \
   * - amount of values                                            \
   *                                                               \
   * stack arguments:                                              \
   * - values...                                                   \
   *                                                               \
   * stack results:                                                \
   * - result string                                               \
   * */                                                            \
  V(stringconcat, OpCount8)                                        \
  /* declareglobal - declare a new global variable                 \
   *                                                               \
   * opcode operands:                                              \
   * - symbol                                                      \
   * */                                                            \
  V(declareglobal, OpSymbol)                                       \
  /* declareglobalconst - declare a new constant global variable   \
   *                                                               \
   * opcode operands:                                              \
   * - symbol                                                      \
   * */                                                            \
  V(declareglobalconst, OpSymbol)                                  \
  /* type - load the class of a value                              \
   *                                                               \
   * stack arguments:                                              \
   * - value                                                       \
   *                                                               \
   * stack results:                                                \
   * - class of value                                              \
   * */                                                            \
  V(type)                                                          \
                                                                   \
  /* pop - pop a value from the stack                              \
   *                                                               \
   * stack arguments:                                              \
   * - value                                                       \
   * */                                                            \
  V(pop)                                                           \
  /* dup - duplicate top value on the stack                        \
   *                                                               \
   * stack arguments:                                              \
   * - value                                                       \
   *                                                               \
   * stack results:                                                \
   * - value                                                       \
   * - copy of value                                               \
   * */                                                            \
  V(dup)                                                           \
  /* dup2 - duplicate the top two values on the stack              \
   *                                                               \
   * stack arguments:                                              \
   * - value1                                                      \
   * - value2                                                      \
   *                                                               \
   * stack results:                                                \
   * - value1                                                      \
   * - value2                                                      \
   * - value1 copy                                                 \
   * - value2 copy                                                 \
   * */                                                            \
  V(dup2)                                                          \
                                                                   \
  /* jmp - jump to label                                           \
   *                                                               \
   * opcode operands:                                              \
   * - branch target                                               \
   * */                                                            \
  V(jmp, OpOffset)                                                 \
  /* jmpf - jump to label if TOS is falsey                         \
   *                                                               \
   * opcode operands:                                              \
   * - branch target                                               \
   *                                                               \
   * stack arguments:                                              \
   * - test                                                        \
   * */                                                            \
  V(jmpf, OpOffset)                                                \
  /* jmpt - jump to label if TOS is truthy                         \
   *                                                               \
   * opcode operands:                                              \
   * - branch target                                               \
   *                                                               \
   * stack arguments:                                              \
   * - test                                                        \
   * */                                                            \
  V(jmpt, OpOffset)                                                \
  /* argswitch - switch to correct default value initializer label \
   * */                                                            \
  V(argswitch)                                                     \
  /* throwex - throw TOS as exception                              \
   *                                                               \
   * stack arguments:                                              \
   * - value                                                       \
   * */                                                            \
  V(throwex)                                                       \
                                                                   \
  /* call - call a function                                        \
   *                                                               \
   * opcode operands:                                              \
   * - amount of arguments                                         \
   *                                                               \
   * stack arguments:                                              \
   * - self value argument                                         \
   * - function                                                    \
   * - arguments...                                                \
   *                                                               \
   * stack results:                                                \
   * - return value                                                \
   * */                                                            \
  V(call, OpCount8)                                                \
  /* callspread - call a function with spreaded arguments          \
   *                                                               \
   * opcode operands:                                              \
   * - amount of segments                                          \
   *                                                               \
   * stack arguments:                                              \
   * - self value argument                                         \
   * - function                                                    \
   * - segments...                                                 \
   *                                                               \
   * stack results:                                                \
   * - return value                                                \
   * */                                                            \
  V(callspread, OpCount8)                                          \
  /* ret - return from current function                            \
   * */                                                            \
  V(ret)                                                           \
                                                                   \
  /* load - load immediate value onto the stack                    \
   *                                                               \
   * opcode operands:                                              \
   * - immediate value                                             \
   *                                                               \
   * stack results:                                                \
   * - immediate value                                             \
   * */                                                            \
  V(load, OpImmediate)                                             \
  /* loadsymbol - load symbol onto the stack                       \
   * gets emitted as a load instruction, exists for                \
   * more readable disassemblies                                   \
   *                                                               \
   * opcode operands:                                              \
   * - symbol                                                      \
   *                                                               \
   * stack results:                                                \
   * - symbol                                                      \
   * */                                                            \
  V(loadsymbol, OpSymbol) /* pseudo instruction */                 \
  /* loadglobal - load a global value                              \
   *                                                               \
   * opcode operands:                                              \
   * - symbol                                                      \
   *                                                               \
   * stack results:                                                \
   * - result value                                                \
   * */                                                            \
  V(loadglobal, OpSymbol)                                          \
  /* loadlocal - load frame local variable                         \
   *                                                               \
   * opcode operands:                                              \
   * - slot offset                                                 \
   *                                                               \
   * stack results:                                                \
   * - value                                                       \
   * */                                                            \
  V(loadlocal, OpCount8)                                           \
  /* loadfar - load variable from surrounding function             \
   *                                                               \
   * opcode operands:                                              \
   * - function depth                                              \
   * - slot offset                                                 \
   *                                                               \
   * stack results:                                                \
   * - value                                                       \
   * */                                                            \
  V(loadfar, OpCount8, OpCount8)                                   \
  /* loadattr - load property                                      \
   *                                                               \
   * opcode operands:                                              \
   * - symbol                                                      \
   *                                                               \
   * stack arguments:                                              \
   * - value                                                       \
   *                                                               \
   * stack results:                                                \
   * - property                                                    \
   * */                                                            \
  V(loadattr, OpSymbol)                                            \
  /* loadattrvalue - load indexed property                         \
   *                                                               \
   * stack arguments:                                              \
   * - value                                                       \
   * - index                                                       \
   *                                                               \
   * stack results:                                                \
   * - property                                                    \
   * */                                                            \
  V(loadattrvalue)                                                 \
  /* loadsuperconstructor - load the super constructor of a given  \
   *                        value                                  \
   *                                                               \
   * stack arguments:                                              \
   * - type                                                        \
   *                                                               \
   * stack results:                                                \
   * - constructor function of the parent class                    \
   *                        */                                     \
  V(loadsuperconstructor)                                          \
  /* loadsuperattr - load function from parent type                \
   *                                                               \
   * opcode operands:                                              \
   * - symbol                                                      \
   *                                                               \
   * stack arguments:                                              \
   * - type                                                        \
   *                                                               \
   * stack results:                                                \
   * - function from parent type                                   \
   * */                                                            \
  V(loadsuperattr, OpSymbol)                                       \
                                                                   \
  /* setglobal - write to global variable                          \
   *                                                               \
   * opcode operands:                                              \
   * - symbol                                                      \
   *                                                               \
   * stack arguments:                                              \
   * - value                                                       \
   *                                                               \
   * stack results:                                                \
   * - value                                                       \
   * */                                                            \
  V(setglobal, OpSymbol)                                           \
  /* setlocal - write to frame local variable                      \
   *                                                               \
   * opcode operands:                                              \
   * - slot offset                                                 \
   *                                                               \
   * stack arguments:                                              \
   * - value                                                       \
   *                                                               \
   * stack results:                                                \
   * - value                                                       \
   * */                                                            \
  V(setlocal, OpCount8)                                            \
  /* setfar - write to variable from surrounding function          \
   *                                                               \
   * opcode operands:                                              \
   * - function depth                                              \
   * - slot offset                                                 \
   *                                                               \
   * stack arguments:                                              \
   * - value                                                       \
   *                                                               \
   * stack results:                                                \
   * - value                                                       \
   * */                                                            \
  V(setfar, OpCount8, OpCount8)                                    \
  /* setattr - write to property                                   \
   *                                                               \
   * opcode operands:                                              \
   * - symbol                                                      \
   *                                                               \
   * stack arguments:                                              \
   * - target                                                      \
   * - value                                                       \
   *                                                               \
   * stack results:                                                \
   * - value                                                       \
   * */                                                            \
  V(setattr, OpSymbol)                                             \
  /* setattrvalue - write to computed property                     \
   *                                                               \
   * stack arguments:                                              \
   * - target                                                      \
   * - index                                                       \
   * - value                                                       \
   *                                                               \
   * stack results:                                                \
   * - value                                                       \
   * */                                                            \
  V(setattrvalue)                                                  \
                                                                   \
  /* unpacksequence - unpack into sequence of values               \
   *                                                               \
   * opcode operands:                                              \
   * - amount of values in sequence                                \
   *                                                               \
   * stack arguments:                                              \
   * - source                                                      \
   *                                                               \
   * stack results:                                                \
   * - values...                                                   \
   * */                                                            \
  V(unpacksequence, OpCount8)                                      \
  /* unpacksequencespread - unpack into sequence of values with    \
   *                        a spread somewhere in between          \
   *                                                               \
   * opcode operands:                                              \
   * - amount of values before the spread                          \
   * - amount of values after the spread                           \
   *                                                               \
   * stack arguments:                                              \
   * - source                                                      \
   *                                                               \
   * stack results:                                                \
   * - values...                                                   \
   * */                                                            \
  V(unpacksequencespread, OpCount8, OpCount8)                      \
  /* unpackobject- unpack specific keys from value                 \
   *                                                               \
   * opcode operands:                                              \
   * - amount of keys                                              \
   *                                                               \
   * stack arguments:                                              \
   * - source                                                      \
   * - key-symbols...                                              \
   *                                                               \
   * stack results:                                                \
   * - values...                                                   \
   *                                                               \
   * */                                                            \
  V(unpackobject, OpCount8)                                        \
  /* unpackobjectspread - unpack specific keys from value, put     \
   *                      remaining keys into rest value           \
   *                                                               \
   * opcode operands:                                              \
   * - amount of keys                                              \
   *                                                               \
   * stack arguments:                                              \
   * - source                                                      \
   * - key-symbols...                                              \
   *                                                               \
   * stack results:                                                \
   * - values...                                                   \
   * - rest value                                                  \
   * */                                                            \
  V(unpackobjectspread, OpCount8)                                  \
                                                                   \
  /* makefunc - allocate new function                              \
   *                                                               \
   * opcode operands:                                              \
   * - body label                                                  \
   *                                                               \
   * stack results:                                                \
   * - function                                                    \
   * */                                                            \
  V(makefunc, OpOffset)                                            \
  /* makeclass - allocate new class                                \
   *                                                               \
   * opcode operands:                                              \
   * - amount of member functions                                  \
   * - amount of member properties                                 \
   * - amount of static properties                                 \
   *                                                               \
   * stack arguments:                                              \
   * - constructor function                                        \
   * - member functions...                                         \
   * - member property symbols...                                  \
   * - static property key                                         \
   *   static property value...                                    \
   *                                                               \
   * stack results:                                                \
   * - class                                                       \
   * */                                                            \
  V(makeclass, OpCount8, OpCount8, OpCount8)                       \
  /* makesubclass - allocate new subclass                          \
   *                                                               \
   * opcode operands:                                              \
   * - amount of member functions                                  \
   * - amount of member properties                                 \
   * - amount of static properties                                 \
   *                                                               \
   * stack arguments:                                              \
   * - parent class                                                \
   * - constructor function                                        \
   * - member functions...                                         \
   * - member property symbols...                                  \
   * - static property key                                         \
   *   static property value...                                    \
   *                                                               \
   * stack results:                                                \
   * - class                                                       \
   * */                                                            \
  V(makesubclass, OpCount8, OpCount8, OpCount8)                    \
  /* makestr - allocate new string                                 \
   *                                                               \
   * opcode operands:                                              \
   * - string label                                                \
   *                                                               \
   * stack results:                                                \
   * - string                                                      \
   * */                                                            \
  V(makestr, OpOffset)                                             \
  /* makelist - allocate new list                                  \
   *                                                               \
   * opcode operands:                                              \
   * - amount of values                                            \
   *                                                               \
   * stack arguments:                                              \
   * - values...                                                   \
   *                                                               \
   * stack results:                                                \
   * - list                                                        \
   * */                                                            \
  V(makelist, OpCount16)                                           \
  /* makelistspread - allocate new list with spreaded arguments    \
   *                                                               \
   * opcode operands:                                              \
   * - amount of segments                                          \
   *                                                               \
   * stack arguments:                                              \
   * - segments...                                                 \
   *                                                               \
   * stack results:                                                \
   * - list                                                        \
   * */                                                            \
  V(makelistspread, OpCount16)                                     \
  /* makedict - allocate new dict                                  \
   *                                                               \
   * opcode operands:                                              \
   * - amount of key/value pairs                                   \
   *                                                               \
   * stack arguments:                                              \
   * - key symbol                                                  \
   *   value...                                                    \
   *                                                               \
   * stack results:                                                \
   * - dict                                                        \
   * */                                                            \
  V(makedict, OpCount16)                                           \
  /* makedictspread - allocate new dict with spreaded arguments    \
   *                                                               \
   * opcode operands:                                              \
   * - amount of source values                                     \
   *                                                               \
   * stack arguments:                                              \
   * - values...                                                   \
   *                                                               \
   * stack results:                                                \
   * - dict                                                        \
   * */                                                            \
  V(makedictspread, OpCount16)                                     \
  /* maketuple - allocate new tuple                                \
   *                                                               \
   * opcode operands:                                              \
   * - amount of values                                            \
   *                                                               \
   * stack arguments:                                              \
   * - values...                                                   \
   *                                                               \
   * stack results:                                                \
   * - tuple                                                       \
   * */                                                            \
  V(maketuple, OpCount16)                                          \
  /* maketuplespread - allocate new tuple with spreaded arguments  \
   *                                                               \
   * opcode operands:                                              \
   * - amount of segments                                          \
   *                                                               \
   * stack arguments:                                              \
   * - segments...                                                 \
   *                                                               \
   * stack results:                                                \
   * - tuple                                                       \
   * */                                                            \
  V(maketuplespread, OpCount16)                                    \
                                                                   \
  /* fiberspawn - create new fiber and start running it            \
   *                                                               \
   * stack arguments:                                              \
   * - context                                                     \
   * - function                                                    \
   * - tuple containing arguments                                  \
   *                                                               \
   * stack results:                                                \
   * - fiber                                                       \
   * */                                                            \
  V(fiberspawn)                                                    \
  /* fiberyield - yield from the current fiber                     \
   *                                                               \
   * stack arguments:                                              \
   * - value                                                       \
   *                                                               \
   * stack results:                                                \
   * - argument from next fiber invocation                         \
   * */                                                            \
  V(fiberyield)                                                    \
  /* fibercall - call a fiber                                      \
   *                                                               \
   * stack arguments:                                              \
   * - value                                                       \
   *                                                               \
   * stack results:                                                \
   * - next yielded value or return value                          \
   * */                                                            \
  V(fibercall)                                                     \
  /* fiberpause - pause current fiber                              \
   *                                                               \
   * stack results:                                                \
   * - value passed to fiberresume                                 \
   * */                                                            \
  V(fiberpause)                                                    \
  /* fiberresume - pause current fiber                             \
   *                                                               \
   * stack arguments:                                              \
   * - value (return value of corresponding fiberpause)            \
   * */                                                            \
  V(fiberresume)                                                   \
  /* fiberawait - wait for a fiber to finish                       \
   *                                                               \
   * stack arguments:                                              \
   * - fiber                                                       \
   *                                                               \
   * stack results:                                                \
   * - fiber return value                                          \
   * */                                                            \
  V(fiberawait)                                                    \
                                                                   \
  /* castXYZ - cast source value to new type                       \
   *                                                               \
   * stack arguments:                                              \
   * - value                                                       \
   *                                                               \
   * stack results:                                                \
   * - casted value                                                \
   * */                                                            \
  V(caststring)                                                    \
  V(castsymbol)                                                    \
  V(castiterator)                                                  \
                                                                   \
  /* iteratornext - load the next result tuple from an iterator    \
   *                                                               \
   * stack arguments:                                              \
   * - iterator                                                    \
   *                                                               \
   * stack results:                                                \
   * - result tuple (value, done)                                  \
   * */                                                            \
  V(iteratornext)                                                  \
                                                                   \
  /* binary operators                                              \
   *                                                               \
   * stack arguments:                                              \
   * - left                                                        \
   * - right                                                       \
   *                                                               \
   * stack results:                                                \
   * - result                                                      \
   * */                                                            \
  V(add)                                                           \
  V(sub)                                                           \
  V(mul)                                                           \
  V(div)                                                           \
  V(mod)                                                           \
  V(pow)                                                           \
  V(eq)                                                            \
  V(neq)                                                           \
  V(lt)                                                            \
  V(gt)                                                            \
  V(le)                                                            \
  V(ge)                                                            \
  V(shl)                                                           \
  V(shr)  /* sign-retaining right shift */                         \
  V(shru) /* zero-filling right shift */                           \
  V(band)                                                          \
  V(bor)                                                           \
  V(bxor)                                                          \
                                                                   \
  /* unary operators                                               \
   *                                                               \
   * stack arguments:                                              \
   * - value                                                       \
   *                                                               \
   * stack results:                                                \
   * - result                                                      \
   * */                                                            \
  V(usub)                                                          \
  V(unot)                                                          \
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
#define ID(name, _) { BuiltinId::name, Opcode::name, },
  FOREACH_BUILTIN(ID)
#undef ID
};
// clang-format on

}  // namespace charly::core::compiler::ir
