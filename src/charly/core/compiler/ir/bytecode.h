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
#include <unordered_set>
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
//  - amount of popped values
//  - amount of pushed values
//  - size of operands
#define FOREACH_OPCODE(V)                                                                    \
  /* nop - no operation                                                                      \
   * */                                                                                      \
  V(nop, 0, 0)                                                                               \
  /* panic - abort machine                                                                   \
   * */                                                                                      \
  V(panic, 0, 0)                                                                             \
                                                                                             \
  /* import - import module or file                                                          \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - modulename (filepath)                                                                 \
   * - sourcepath (path of current file)                                                     \
   *                                                                                         \
   * stack results:                                                                          \
   * - module (value returned by included file)                                              \
   * */                                                                                      \
  V(import, 2, 1)                                                                            \
  /* stringconcat - concatenate multiple values into a string                                \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - amount of values                                                                      \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - values...                                                                             \
   *                                                                                         \
   * stack results:                                                                          \
   * - result string                                                                         \
   * */                                                                                      \
  V(stringconcat, arg0, 1, OpCount8)                                                         \
  /* declareglobal - declare a new global variable                                           \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - symbol                                                                                \
   * */                                                                                      \
  V(declareglobal, 0, 0, OpSymbol)                                                           \
  /* declareglobalconst - declare a new constant global variable                             \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - symbol                                                                                \
   * */                                                                                      \
  V(declareglobalconst, 0, 0, OpSymbol)                                                      \
  /* type - load the class of a value                                                        \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - class of value                                                                        \
   * */                                                                                      \
  V(type, 1, 1)                                                                              \
                                                                                             \
  /* pop - pop a value from the stack                                                        \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   * */                                                                                      \
  V(pop, 1, 0)                                                                               \
  /* dup - duplicate top value on the stack                                                  \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - value                                                                                 \
   * - copy of value                                                                         \
   * */                                                                                      \
  V(dup, 1, 2)                                                                               \
  /* dup2 - duplicate the top two values on the stack                                        \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value1                                                                                \
   * - value2                                                                                \
   *                                                                                         \
   * stack results:                                                                          \
   * - value1                                                                                \
   * - value2                                                                                \
   * - value1 copy                                                                           \
   * - value2 copy                                                                           \
   * */                                                                                      \
  V(dup2, 2, 4)                                                                              \
                                                                                             \
  /* jmp - jump to label                                                                     \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - branch target                                                                         \
   * */                                                                                      \
  V(jmp, 0, 0, OpOffset)                                                                     \
  /* jmpf - jump to label if TOS is falsey                                                   \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - branch target                                                                         \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - test                                                                                  \
   * */                                                                                      \
  V(jmpf, 1, 0, OpOffset)                                                                    \
  /* jmpt - jump to label if TOS is truthy                                                   \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - branch target                                                                         \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - test                                                                                  \
   * */                                                                                      \
  V(jmpt, 1, 0, OpOffset)                                                                    \
  /* testjmp - compare TOS with an immediate value and jump if they compare equal            \
   *           performs a non-strict comparison (true == 25 would branch)                    \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - immediate test                                                                        \
   * - branch target                                                                         \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - test                                                                                  \
   *                                                                                         \
   * stack results:                                                                          \
   * - test                                                                                  \
   * */                                                                                      \
  V(testjmp, 1, 1, OpImmediate, OpOffset)                                                    \
  /* testjmpstrict - compare TOS with an immediate value and jump if they are the same value \
   *                 performs a strict comparison (true == 25 would not branch)              \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - immediate test                                                                        \
   * - branch target                                                                         \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - test                                                                                  \
   *                                                                                         \
   * stack results:                                                                          \
   * - test                                                                                  \
   * */                                                                                      \
  V(testjmpstrict, 1, 1, OpImmediate, OpOffset)                                              \
  /* throwex - throw TOS as exception                                                        \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   * */                                                                                      \
  V(throwex, 1, 0)                                                                           \
  /* getexception - push the last thrown exception onto the stack                            \
   *                                                                                         \
   * stack results:                                                                          \
   * - exception                                                                             \
   * */                                                                                      \
  V(getexception, 0, 1) /* pseudo-instruction */                                             \
                                                                                             \
  /* call - call a function                                                                  \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - amount of arguments                                                                   \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - self value argument                                                                   \
   * - function                                                                              \
   * - arguments...                                                                          \
   *                                                                                         \
   * stack results:                                                                          \
   * - return value                                                                          \
   * */                                                                                      \
  V(call, 2 + arg0, 1, OpCount8)                                                             \
  /* callspread - call a function with spreaded arguments                                    \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - amount of segments                                                                    \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - self value argument                                                                   \
   * - function                                                                              \
   * - segments...                                                                           \
   *                                                                                         \
   * stack results:                                                                          \
   * - return value                                                                          \
   * */                                                                                      \
  V(callspread, 2 + arg0, 1, OpCount8)                                                       \
  /* ret - return from current function                                                      \
   * */                                                                                      \
  V(ret, 0, 0)                                                                               \
                                                                                             \
  /* load - load immediate value onto the stack                                              \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - immediate value                                                                       \
   *                                                                                         \
   * stack results:                                                                          \
   * - immediate value                                                                       \
   * */                                                                                      \
  V(load, 0, 1, OpImmediate)                                                                 \
  /* loadsymbol - load symbol onto the stack                                                 \
   * gets emitted as a load instruction, exists for                                          \
   * more readable disassemblies                                                             \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - symbol                                                                                \
   *                                                                                         \
   * stack results:                                                                          \
   * - symbol                                                                                \
   * */                                                                                      \
  V(loadsymbol, 0, 1, OpSymbol) /* pseudo instruction */                                     \
  /* loadself - load the self value in this frame                                            \
   *                                                                                         \
   * stack results:                                                                          \
   * - self                                                                                  \
   * */                                                                                      \
  V(loadself, 0, 1) /* pseudo instruction */                                                 \
  /* loadglobal - load a global value                                                        \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - symbol                                                                                \
   *                                                                                         \
   * stack results:                                                                          \
   * - result value                                                                          \
   * */                                                                                      \
  V(loadglobal, 0, 1, OpSymbol)                                                              \
  /* loadlocal - load frame local variable                                                   \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - slot offset                                                                           \
   *                                                                                         \
   * stack results:                                                                          \
   * - value                                                                                 \
   * */                                                                                      \
  V(loadlocal, 0, 1, OpCount8)                                                               \
  /* loadfar - load variable from surrounding function                                       \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - function depth                                                                        \
   * - slot offset                                                                           \
   *                                                                                         \
   * stack results:                                                                          \
   * - value                                                                                 \
   * */                                                                                      \
  V(loadfar, 0, 1, OpCount8, OpCount8)                                                       \
  /* loadattr - load property                                                                \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - symbol                                                                                \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - property                                                                              \
   * */                                                                                      \
  V(loadattr, 1, 1, OpSymbol)                                                                \
  /* loadattrvalue - load indexed property                                                   \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   * - index                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - property                                                                              \
   * */                                                                                      \
  V(loadattrvalue, 2, 1)                                                                     \
  /* loadsuperconstructor - load the super constructor of a given                            \
   *                        value                                                            \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - type                                                                                  \
   *                                                                                         \
   * stack results:                                                                          \
   * - constructor function of the parent class                                              \
   *                        */                                                               \
  V(loadsuperconstructor, 1, 1)                                                              \
  /* loadsuperattr - load function from parent type                                          \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - symbol                                                                                \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - type                                                                                  \
   *                                                                                         \
   * stack results:                                                                          \
   * - function from parent type                                                             \
   * */                                                                                      \
  V(loadsuperattr, 1, 1, OpSymbol)                                                           \
                                                                                             \
  /* setglobal - write to global variable                                                    \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - symbol                                                                                \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - value                                                                                 \
   * */                                                                                      \
  V(setglobal, 1, 1, OpSymbol)                                                               \
  /* setlocal - write to frame local variable                                                \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - slot offset                                                                           \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - value                                                                                 \
   * */                                                                                      \
  V(setlocal, 1, 1, OpCount8)                                                                \
  /* setreturn - set the function return value                                               \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   * */                                                                                      \
  V(setreturn, 1, 0) /* pseudo-instruction */                                                \
  /* setfar - write to variable from surrounding function                                    \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - function depth                                                                        \
   * - slot offset                                                                           \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - value                                                                                 \
   * */                                                                                      \
  V(setfar, 1, 1, OpCount8, OpCount8)                                                        \
  /* setattr - write to property                                                             \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - symbol                                                                                \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - target                                                                                \
   * - value                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - value                                                                                 \
   * */                                                                                      \
  V(setattr, 2, 1, OpSymbol)                                                                 \
  /* setattrvalue - write to computed property                                               \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - target                                                                                \
   * - index                                                                                 \
   * - value                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - value                                                                                 \
   * */                                                                                      \
  V(setattrvalue, 3, 1)                                                                      \
                                                                                             \
  /* unpacksequence - unpack into sequence of values                                         \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - amount of values in sequence                                                          \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - source                                                                                \
   *                                                                                         \
   * stack results:                                                                          \
   * - values...                                                                             \
   * */                                                                                      \
  V(unpacksequence, 1, arg0, OpCount8)                                                       \
  /* unpacksequencespread - unpack into sequence of values with                              \
   *                        a spread somewhere in between                                    \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - amount of values before the spread                                                    \
   * - amount of values after the spread                                                     \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - source                                                                                \
   *                                                                                         \
   * stack results:                                                                          \
   * - values...                                                                             \
   * */                                                                                      \
  V(unpacksequencespread, 1, arg0 + arg1 + 1, OpCount8, OpCount8)                            \
  /* unpackobject- unpack specific keys from value                                           \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - amount of keys                                                                        \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - source                                                                                \
   * - key-symbols...                                                                        \
   *                                                                                         \
   * stack results:                                                                          \
   * - values...                                                                             \
   *                                                                                         \
   * */                                                                                      \
  V(unpackobject, 1 + arg0, arg0, OpCount8)                                                  \
  /* unpackobjectspread - unpack specific keys from value, put                               \
   *                      remaining keys into rest value                                     \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - amount of keys                                                                        \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - source                                                                                \
   * - key-symbols...                                                                        \
   *                                                                                         \
   * stack results:                                                                          \
   * - values...                                                                             \
   * - rest value                                                                            \
   * */                                                                                      \
  V(unpackobjectspread, 1 + arg0, arg0 + 1, OpCount8)                                        \
                                                                                             \
  /* makefunc - allocate new function                                                        \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - body label                                                                            \
   *                                                                                         \
   * stack results:                                                                          \
   * - function                                                                              \
   * */                                                                                      \
  V(makefunc, 0, 1, OpOffset)                                                                \
  /* makeclass - allocate new class                                                          \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - class name                                                                            \
   * - amount of member functions                                                            \
   * - amount of member properties                                                           \
   * - amount of static properties                                                           \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - constructor function                                                                  \
   * - member functions...                                                                   \
   * - member property symbols...                                                            \
   * - static property key                                                                   \
   *   static property value...                                                              \
   *                                                                                         \
   * stack results:                                                                          \
   * - class                                                                                 \
   * */                                                                                      \
  V(makeclass, 1 + arg1 + arg2 + arg3 * 2, 1, OpSymbol, OpCount8, OpCount8, OpCount8)        \
  /* makesubclass - allocate new subclass                                                    \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - class name                                                                            \
   * - amount of member functions                                                            \
   * - amount of member properties                                                           \
   * - amount of static properties                                                           \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - parent class                                                                          \
   * - constructor function                                                                  \
   * - member functions...                                                                   \
   * - member property symbols...                                                            \
   * - static property key                                                                   \
   *   static property value...                                                              \
   *                                                                                         \
   * stack results:                                                                          \
   * - class                                                                                 \
   * */                                                                                      \
  V(makesubclass, 2 + arg1 + arg2 + arg3 * 2, 1, OpSymbol, OpCount8, OpCount8, OpCount8)     \
  /* makestr - allocate new string                                                           \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - string table index                                                                    \
   *                                                                                         \
   * stack results:                                                                          \
   * - string                                                                                \
   * */                                                                                      \
  V(makestr, 0, 1, OpCount16)                                                                \
  /* makelist - allocate new list                                                            \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - amount of values                                                                      \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - values...                                                                             \
   *                                                                                         \
   * stack results:                                                                          \
   * - list                                                                                  \
   * */                                                                                      \
  V(makelist, arg0, 1, OpCount16)                                                            \
  /* makelistspread - allocate new list with spreaded arguments                              \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - amount of segments                                                                    \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - segments...                                                                           \
   *                                                                                         \
   * stack results:                                                                          \
   * - list                                                                                  \
   * */                                                                                      \
  V(makelistspread, arg0, 1, OpCount16)                                                      \
  /* makedict - allocate new dict                                                            \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - amount of key/value pairs                                                             \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - key symbol                                                                            \
   *   value...                                                                              \
   *                                                                                         \
   * stack results:                                                                          \
   * - dict                                                                                  \
   * */                                                                                      \
  V(makedict, arg0 * 2, 1, OpCount16)                                                        \
  /* makedictspread - allocate new dict with spreaded arguments                              \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - amount of source values                                                               \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - values...                                                                             \
   *                                                                                         \
   * stack results:                                                                          \
   * - dict                                                                                  \
   * */                                                                                      \
  V(makedictspread, arg0, 1, OpCount16)                                                      \
  /* maketuple - allocate new tuple                                                          \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - amount of values                                                                      \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - values...                                                                             \
   *                                                                                         \
   * stack results:                                                                          \
   * - tuple                                                                                 \
   * */                                                                                      \
  V(maketuple, arg0, 1, OpCount16)                                                           \
  /* maketuplespread - allocate new tuple with spreaded arguments                            \
   *                                                                                         \
   * opcode operands:                                                                        \
   * - amount of segments                                                                    \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - segments...                                                                           \
   *                                                                                         \
   * stack results:                                                                          \
   * - tuple                                                                                 \
   * */                                                                                      \
  V(maketuplespread, arg0, 1, OpCount16)                                                     \
                                                                                             \
  /* fiberspawn - create new fiber and start running it                                      \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - context                                                                               \
   * - function                                                                              \
   * - tuple containing arguments                                                            \
   *                                                                                         \
   * stack results:                                                                          \
   * - fiber                                                                                 \
   * */                                                                                      \
  V(fiberspawn, 3, 1)                                                                        \
  /* fiberyield - yield from the current fiber                                               \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - argument from next fiber invocation                                                   \
   * */                                                                                      \
  V(fiberyield, 1, 1)                                                                        \
  /* fibercall - call a fiber                                                                \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - next yielded value or return value                                                    \
   * */                                                                                      \
  V(fibercall, 1, 1)                                                                         \
  /* fiberpause - pause current fiber                                                        \
   *                                                                                         \
   * stack results:                                                                          \
   * - value passed to fiberresume                                                           \
   * */                                                                                      \
  V(fiberpause, 0, 1)                                                                        \
  /* fiberresume - pause current fiber                                                       \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value (return value of corresponding fiberpause)                                      \
   * */                                                                                      \
  V(fiberresume, 1, 0)                                                                       \
  /* fiberawait - wait for a fiber to finish                                                 \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - fiber                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - fiber return value                                                                    \
   * */                                                                                      \
  V(fiberawait, 1, 1)                                                                        \
                                                                                             \
  /* castXYZ - cast source value to new type                                                 \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - casted value                                                                          \
   * */                                                                                      \
  V(caststring, 1, 1)                                                                        \
  V(castsymbol, 1, 1)                                                                        \
  V(castiterator, 1, 1)                                                                      \
                                                                                             \
  /* iteratornext - load the next result tuple from an iterator                              \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - iterator                                                                              \
   *                                                                                         \
   * stack results:                                                                          \
   * - result tuple (value, done)                                                            \
   * */                                                                                      \
  V(iteratornext, 1, 1)                                                                      \
                                                                                             \
  /* binary operators                                                                        \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - left                                                                                  \
   * - right                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - result                                                                                \
   * */                                                                                      \
  V(add, 2, 1)                                                                               \
  V(sub, 2, 1)                                                                               \
  V(mul, 2, 1)                                                                               \
  V(div, 2, 1)                                                                               \
  V(mod, 2, 1)                                                                               \
  V(pow, 2, 1)                                                                               \
  V(eq, 2, 1)                                                                                \
  V(neq, 2, 1)                                                                               \
  V(lt, 2, 1)                                                                                \
  V(gt, 2, 1)                                                                                \
  V(le, 2, 1)                                                                                \
  V(ge, 2, 1)                                                                                \
  V(shl, 2, 1)                                                                               \
  V(shr, 2, 1)  /* sign-retaining right shift */                                             \
  V(shru, 2, 1) /* zero-filling right shift */                                               \
  V(band, 2, 1)                                                                              \
  V(bor, 2, 1)                                                                               \
  V(bxor, 2, 1)                                                                              \
                                                                                             \
  /* unary operators                                                                         \
   *                                                                                         \
   * stack arguments:                                                                        \
   * - value                                                                                 \
   *                                                                                         \
   * stack results:                                                                          \
   * - result                                                                                \
   * */                                                                                      \
  V(usub, 1, 1)                                                                              \
  V(unot, 1, 1)                                                                              \
  V(ubnot, 1, 1)

enum class Opcode : uint16_t {
#define ID(name, stackpop, stackpush, ...) name,
  FOREACH_OPCODE(ID)
#undef ID
};

/*
 * opcodes which terminate their basic block
 * */
static const std::unordered_set<Opcode> kTerminatingOpcodes = {
  Opcode::panic,
  Opcode::jmp,
  Opcode::throwex,
  Opcode::ret
};

/*
 * opcodes that can perform branches (call-like excluded)
 * */
static const std::unordered_set<Opcode> kBranchingOpcodes = {
  Opcode::jmp,
  Opcode::jmpf,
  Opcode::jmpt,
  Opcode::testjmp,
  Opcode::testjmpstrict
};

/*
 * total length of instruction in bytes (including opcode)
 * */
static constexpr size_t kOpcodeLength[] = {
#define SUM_ELEMENT(type) + type##Size
#define MAP(name, stackpop, stackpush, ...) 1 + CHARLY_VA_FOR_EACH(SUM_ELEMENT, __VA_ARGS__),
  FOREACH_OPCODE(MAP)
#undef MAP
#undef SUM_ELEMENT
};

static std::string kOpcodeNames[] = {
#define NAME(name, stackpop, stackpush, ...) #name,
  FOREACH_OPCODE(NAME)
#undef NAME
};

static std::unordered_map<std::string, Opcode> kOpcodeNameMapping = {
#define PAIR(name, stackpop, stackpush, ...) { #name, Opcode::name },
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
