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
#include <unordered_set>
#include <vector>

#include "charly/charly.h"
#include "charly/value.h"

#include "charly/core/compiler/ir/builtin.h"
#include "charly/core/compiler/token.h"

#pragma once

namespace charly::core::compiler::ir {

// TODO: reserve space in constant table for bytecodes whose quickened version has to store a value
// TODO: update usages of FOREACH_OPCODE

static const size_t kInstructionLength = 4;

// opcode types
#define FOREACH_OPCODE_TYPE(V) \
  V(IXXX, ixxx)                \
  V(IABX, iabx)                \
  V(IABC, iabc)                \
  V(IABB, iabb)                \
  V(IAAX, iaax)                \
  V(IAAA, iaaa)

// opcode listing
//  - name
//  - instruction type
//  - popped stack values
//  - pushed stack values
#define FOREACH_OPCODE(V)                                                              \
  /* nop - no operation                                                                \
   * */                                                                                \
  V(nop, IXXX, 0, 0)                                                                   \
  /* panic - abort machine                                                             \
   * */                                                                                \
  V(panic, IXXX, 0, 0)                                                                 \
  /* import - import module or file                                                    \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - modulename (filepath)                                                           \
   * - sourcepath (path of current file)                                               \
   *                                                                                   \
   * stack results:                                                                    \
   * - module (value returned by included file)                                        \
   * */                                                                                \
  V(import, IXXX, 2, 1)                                                                \
  /* stringconcat - concatenate multiple values into a string                          \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - amount of values                                                                \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - values...                                                                       \
   *                                                                                   \
   * stack results:                                                                    \
   * - result string                                                                   \
   * */                                                                                \
  V(stringconcat, IAXX, arg, 1)                                                        \
  /* declareglobal - declare a new global variable                                     \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - string table offset                                                             \
   * */                                                                                \
  V(declareglobal, IAAX, 0, 0)                                                         \
  /* declareglobalconst - declare a new constant global variable                       \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - string table offset                                                             \
   * */                                                                                \
  V(declareglobalconst, IAAX, 0, 0)                                                    \
  /* type - load the class of a value                                                  \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - value                                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - class of value                                                                  \
   * */                                                                                \
  V(type, IXXX, 1, 1)                                                                  \
  /* swap - swap top two values on stack                                               \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - v1                                                                              \
   * - v2                                                                              \
   *                                                                                   \
   * stack results:                                                                    \
   * - v2                                                                              \
   * - v1                                                                              \
   *                                                                                   \
   * */                                                                                \
  V(swap, IXXX, 2, 2)                                                                  \
  /* pop - pop a value from the stack                                                  \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - value                                                                           \
   * */                                                                                \
  V(pop, IXXX, 1, 0)                                                                   \
  /* dup - duplicate top value on the stack                                            \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - value                                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - value                                                                           \
   * - copy of value                                                                   \
   * */                                                                                \
  V(dup, IXXX, 1, 2)                                                                   \
  /* dup2 - duplicate the top two values on the stack                                  \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - value1                                                                          \
   * - value2                                                                          \
   *                                                                                   \
   * stack results:                                                                    \
   * - value1                                                                          \
   * - value2                                                                          \
   * - value1 copy                                                                     \
   * - value2 copy                                                                     \
   * */                                                                                \
  V(dup2, IXXX, 2, 4)                                                                  \
  /* jmp - jump to label                                                               \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - relative offset                                                                 \
   * */                                                                                \
  V(jmp, IAAX, 0, 0)                                                                   \
  /* jmpf - jump to label if TOS is falsey                                             \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - relative offset                                                                 \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - test                                                                            \
   * */                                                                                \
  V(jmpf, IAAX, 1, 0)                                                                  \
  /* jmpt - jump to label if TOS is truthy                                             \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - relative offset                                                                 \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - test                                                                            \
   * */                                                                                \
  V(jmpt, IAAX, 1, 0)                                                                  \
  /* testintjmp - compare TOS with an immediate value and jump if they compare equal   \
   *                                                                                   \
   *           the test value gets pushed back onto the stack if the comparison failed \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - integer value                                                                   \
   * - relative offset                                                                 \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - test                                                                            \
   *                                                                                   \
   * stack results:                                                                    \
   * - test                                                                            \
   * */                                                                                \
  V(testintjmp, IABB, 1, 1)                                                            \
  /* throwex - throw TOS as exception                                                  \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - value                                                                           \
   * */                                                                                \
  V(throwex, IXXX, 1, 0)                                                               \
  /* getexception - push the last thrown exception onto the stack                      \
   *                                                                                   \
   * stack results:                                                                    \
   * - exception                                                                       \
   * */                                                                                \
  V(getexception, IXXX, 0, 1)                                                          \
  /* call - call a function                                                            \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - amount of arguments                                                             \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - self value argument                                                             \
   * - function                                                                        \
   * - arguments...                                                                    \
   *                                                                                   \
   * stack results:                                                                    \
   * - return value                                                                    \
   * */                                                                                \
  V(call, IAXX, 2 + arg, 1)                                                            \
  /* callspread - call a function with spreaded arguments                              \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - amount of segments                                                              \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - self value argument                                                             \
   * - function                                                                        \
   * - segments...                                                                     \
   *                                                                                   \
   * stack results:                                                                    \
   * - return value                                                                    \
   * */                                                                                \
  V(callspread, IAXX, 2 + arg, 1)                                                      \
  /* ret - return from current function                                                \
   * */                                                                                \
  V(ret, IXXX, 0, 0)                                                                   \
  /* load - load immediate value onto the stack                                        \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - constant table offset                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - immediate value                                                                 \
   * */                                                                                \
  V(load, IAAX, 0, 1)                                                                  \
  /* loadsmi - load small immediate value onto stack                                   \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - immediate value                                                                 \
   *                                                                                   \
   * stack results:                                                                    \
   * - immediate value                                                                 \
   * */                                                                                \
  V(loadsmi, IAAA, 0, 1)                                                               \
  /* loadself - load the self value in this frame                                      \
   *                                                                                   \
   * stack results:                                                                    \
   * - self                                                                            \
   * */                                                                                \
  V(loadself, IXXX, 0, 1)                                                              \
  /* loadfarself - load the self value in this frame                                   \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - function depth                                                                  \
   *                                                                                   \
   * stack results:                                                                    \
   * - self                                                                            \
   * */                                                                                \
  V(loadfarself, IAXX, 0, 1)                                                           \
  /* loadargc - load the amount of arguments passed to the function                    \
   *                                                                                   \
   * stack results:                                                                    \
   * - argc                                                                            \
   * */                                                                                \
  V(loadargc, IXXX, 0, 1)                                                              \
  /* loadglobal - load a global value                                                  \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - string table offset                                                             \
   *                                                                                   \
   * stack results:                                                                    \
   * - result value                                                                    \
   * */                                                                                \
  V(loadglobal, IAAX, 0, 1)                                                            \
  /* loadlocal - load frame local variable                                             \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - slot index                                                                      \
   *                                                                                   \
   * stack results:                                                                    \
   * - value                                                                           \
   * */                                                                                \
  V(loadlocal, IAXX, 0, 1)                                                             \
  /* loadfar - load variable from surrounding function                                 \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - function depth                                                                  \
   * - slot index                                                                      \
   *                                                                                   \
   * stack results:                                                                    \
   * - value                                                                           \
   * */                                                                                \
  V(loadfar, IABX, 0, 1)                                                               \
  /* loadattr - load property                                                          \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - value                                                                           \
   * - index                                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - property                                                                        \
   * */                                                                                \
  V(loadattr, IXXX, 2, 1)                                                              \
  /* loadattrsym - load property via symbol                                            \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - string table offset                                                             \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - value                                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - property                                                                        \
   * */                                                                                \
  V(loadattrsym, IAAX, 1, 1)                                                           \
  /* loadsuperconstructor - load the super constructor                                 \
   *                                                                                   \
   * stack results:                                                                    \
   * - constructor function of the parent class                                        \
   *                        */                                                         \
  V(loadsuperconstructor, IXXX, 0, 1)                                                  \
  /* loadsuperattr - load function from parent type                                    \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - string table offset                                                             \
   *                                                                                   \
   * stack results:                                                                    \
   * - function from parent type                                                       \
   * */                                                                                \
  V(loadsuperattr, IAAX, 0, 1)                                                         \
  /* setglobal - write to global variable                                              \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - symbol                                                                          \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - value                                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - value                                                                           \
   * */                                                                                \
  V(setglobal, IAAX, 1, 1)                                                             \
  /* setlocal - write to frame local variable                                          \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - slot index                                                                      \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - value                                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - value                                                                           \
   * */                                                                                \
  V(setlocal, IAXX, 1, 1)                                                              \
  /* setreturn - set the function return value                                         \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - value                                                                           \
   * */                                                                                \
  V(setreturn, IXXX, 1, 0)                                                             \
  /* setfar - write to variable from surrounding function                              \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - function depth                                                                  \
   * - slot index                                                                      \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - value                                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - value                                                                           \
   * */                                                                                \
  V(setfar, IABX, 1, 1)                                                                \
  /* setattr - write to property                                                       \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - target                                                                          \
   * - index                                                                           \
   * - value                                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - value                                                                           \
   * */                                                                                \
  V(setattr, IXXX, 3, 1)                                                               \
  /* setattrsym - write to property via symbol                                         \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - symbol                                                                          \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - target                                                                          \
   * - value                                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - value                                                                           \
   * */                                                                                \
  V(setattrsym, IAAX, 2, 1)                                                            \
  /* unpacksequence - unpack into sequence of values                                   \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - amount of values in sequence                                                    \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - source                                                                          \
   *                                                                                   \
   * stack results:                                                                    \
   * - values...                                                                       \
   * */                                                                                \
  V(unpacksequence, IAXX, 1, arg)                                                      \
  /* unpacksequencespread - unpack into sequence of values with                        \
   *                        a spread somewhere in between                              \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - amount of values before the spread                                              \
   * - amount of values after the spread                                               \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - source                                                                          \
   *                                                                                   \
   * stack results:                                                                    \
   * - values...                                                                       \
   * */                                                                                \
  V(unpacksequencespread, IABX, 1, 1 + arg1 + arg2)                                    \
  /* unpackobject- unpack specific keys from value                                     \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - amount of keys                                                                  \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - source                                                                          \
   * - key-symbols...                                                                  \
   *                                                                                   \
   * stack results:                                                                    \
   * - values...                                                                       \
   *                                                                                   \
   * */                                                                                \
  V(unpackobject, IAXX, 1 + arg, arg)                                                  \
  /* unpackobjectspread - unpack specific keys from value, put                         \
   *                      remaining keys into rest value                               \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - amount of keys                                                                  \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - source                                                                          \
   * - key-symbols...                                                                  \
   *                                                                                   \
   * stack results:                                                                    \
   * - values...                                                                       \
   * - rest value                                                                      \
   * */                                                                                \
  V(unpackobjectspread, IAXX, 1 + arg, 1 + arg)                                        \
  /* makestr - allocate new string                                                     \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - string table index                                                              \
   *                                                                                   \
   * stack results:                                                                    \
   * - string                                                                          \
   * */                                                                                \
  V(makestr, IAAX, 0, 1)                                                               \
  /* makefunc - allocate new function                                                  \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - body label                                                                      \
   *                                                                                   \
   * stack results:                                                                    \
   * - function                                                                        \
   * */                                                                                \
  V(makefunc, IAAX, 0, 1)                                                              \
  /* makeclass - allocate new class                                                    \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - flags                                                                           \
   * - class name symbol                                                               \
   * - parent class                                                                    \
   * - constructor function                                                            \
   * - member function overload tuples                                                 \
   * - member property tuple                                                           \
   * - static function overload tuples                                                 \
   * - static property keys tuple                                                      \
   * - static property values tuple                                                    \
   *                                                                                   \
   * stack results:                                                                    \
   * - class                                                                           \
   * */                                                                                \
  V(makeclass, IXXX, 9, 1)                                                             \
  /* makelist - allocate new list                                                      \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - amount of values                                                                \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - values...                                                                       \
   *                                                                                   \
   * stack results:                                                                    \
   * - list                                                                            \
   * */                                                                                \
  V(makelist, IAAX, arg, 1)                                                            \
  /* makelistspread - allocate new list with spreaded arguments                        \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - amount of segments                                                              \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - segments...                                                                     \
   *                                                                                   \
   * stack results:                                                                    \
   * - list                                                                            \
   * */                                                                                \
  V(makelistspread, IAAX, arg, 1)                                                      \
  /* makedict - allocate new dict                                                      \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - amount of key/value pairs                                                       \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - key symbol                                                                      \
   *   value...                                                                        \
   *                                                                                   \
   * stack results:                                                                    \
   * - dict                                                                            \
   * */                                                                                \
  V(makedict, IAAX, arg * 2, 1)                                                        \
  /* makedictspread - allocate new dict with spreaded arguments                        \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - amount of source values                                                         \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - values...                                                                       \
   *                                                                                   \
   * stack results:                                                                    \
   * - dict                                                                            \
   * */                                                                                \
  V(makedictspread, IAAX, arg, 1)                                                      \
  /* maketuple - allocate new tuple                                                    \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - amount of values                                                                \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - values...                                                                       \
   *                                                                                   \
   * stack results:                                                                    \
   * - tuple                                                                           \
   * */                                                                                \
  V(maketuple, IAAX, arg, 1)                                                           \
  /* maketuplespread - allocate new tuple with spreaded arguments                      \
   *                                                                                   \
   * opcode operands:                                                                  \
   * - amount of segments                                                              \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - segments...                                                                     \
   *                                                                                   \
   * stack results:                                                                    \
   * - tuple                                                                           \
   * */                                                                                \
  V(maketuplespread, IAAX, arg, 1)                                                     \
  /* makefiber - allocate new fiber                                                    \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - context                                                                         \
   * - function                                                                        \
   * - arguments tuple                                                                 \
   *                                                                                   \
   * stack results:                                                                    \
   * - fiber                                                                           \
   * */                                                                                \
  V(makefiber, IXXX, 3, 1)                                                             \
  /* fiberjoin - wait for a fiber to finish                                            \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - fiber                                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - fiber return value                                                              \
   * */                                                                                \
  V(fiberjoin, IXXX, 1, 1)                                                             \
  /* castXYZ - cast source value to new type                                           \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - value                                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - casted value                                                                    \
   * */                                                                                \
  V(caststring, IXXX, 1, 1)                                                            \
  V(casttuple, IXXX, 1, 1)                                                            \
  V(castsymbol, IXXX, 1, 1)                                                            \
  V(castiterator, IXXX, 1, 1)                                                          \
  /* iteratornext - load the next result tuple from an iterator                        \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - iterator                                                                        \
   *                                                                                   \
   * stack results:                                                                    \
   * - result tuple (value, done)                                                      \
   * */                                                                                \
  V(iteratornext, IXXX, 1, 1)                                                          \
  /* binary operators                                                                  \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - left                                                                            \
   * - right                                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - result                                                                          \
   * */                                                                                \
  V(add, IXXX, 2, 1)                                                                   \
  V(sub, IXXX, 2, 1)                                                                   \
  V(mul, IXXX, 2, 1)                                                                   \
  V(div, IXXX, 2, 1)                                                                   \
  V(mod, IXXX, 2, 1)                                                                   \
  V(pow, IXXX, 2, 1)                                                                   \
  V(eq, IXXX, 2, 1)                                                                    \
  V(neq, IXXX, 2, 1)                                                                   \
  V(lt, IXXX, 2, 1)                                                                    \
  V(gt, IXXX, 2, 1)                                                                    \
  V(le, IXXX, 2, 1)                                                                    \
  V(ge, IXXX, 2, 1)                                                                    \
  V(shl, IXXX, 2, 1)                                                                   \
  V(shr, IXXX, 2, 1)  /* sign-retaining right shift */                                 \
  V(shru, IXXX, 2, 1) /* zero-filling right shift */                                   \
  V(band, IXXX, 2, 1)                                                                  \
  V(bor, IXXX, 2, 1)                                                                   \
  V(bxor, IXXX, 2, 1)                                                                  \
  /* unary operators                                                                   \
   *                                                                                   \
   * stack arguments:                                                                  \
   * - value                                                                           \
   *                                                                                   \
   * stack results:                                                                    \
   * - result                                                                          \
   * */                                                                                \
  V(usub, IXXX, 1, 1)                                                                  \
  V(unot, IXXX, 1, 1)                                                                  \
  V(ubnot, IXXX, 1, 1)

enum Opcode : uint8_t
{
#define ID(name, ...) name,
  FOREACH_OPCODE(ID)
#undef ID
  OpcodeCount  // amount of opcodes
};

#define DEF_FORWARD(T, _) struct Instruction##T;
FOREACH_OPCODE_TYPE(DEF_FORWARD)
#undef DEF_FORWARD

#define DEF_FORWARD(N, ...) struct Instruction_##N;
FOREACH_OPCODE(DEF_FORWARD)
#undef DEF_FORWARD

struct Instruction {
  static const uint32_t kOpcodeMask = 0x000000ff;
  static const uint32_t kArg1Mask = 0x0000ff00;
  static const uint32_t kArg2Mask = 0x00ff0000;
  static const uint32_t kArg3Mask = 0xff000000;
  static const uint32_t kArgBBMask = 0xffff0000;
  static const uint32_t kArgAAMask = 0x00ffff00;
  static const uint32_t kArgAAAMask = 0xffffff00;
  static const uint32_t kOpcodeShift = 0;
  static const uint32_t kArg1Shift = 8;
  static const uint32_t kArg2Shift = 16;
  static const uint32_t kArg3Shift = 24;
  static const uint32_t kArgBBShift = 16;
  static const uint32_t kArgAAShift = 8;
  static const uint32_t kArgAAAShift = 8;

  uint32_t raw;

  __attribute__((always_inline)) Opcode opcode() const {
    return bitcast<Opcode>((raw & kOpcodeMask) >> kOpcodeShift);
  }

  __attribute__((always_inline)) uintptr_t ip() const {
    return bitcast<uintptr_t>(this);
  }

#define DEF_CAST(N, ...)                                      \
  __attribute__((always_inline)) Instruction_##N* N() const { \
    DCHECK(opcode() == Opcode::N);                            \
    return (Instruction_##N*)this;                            \
  }
  FOREACH_OPCODE(DEF_CAST)
#undef DEF_CAST

  friend std::ostream& operator<<(std::ostream& out, const Instruction& instruction) {
    out << (void*)(uintptr_t)instruction.raw;
    return out;
  }

  operator uint32_t() const {
    return raw;
  }
};

struct Instruction_IXXX : public Instruction {};

struct Instruction_IAXX : public Instruction {
  __attribute__((always_inline)) uint8_t arg() const {
    return (raw & kArg1Mask) >> kArg1Shift;
  }
};

struct Instruction_IABX : public Instruction {
  __attribute__((always_inline)) uint8_t arg1() const {
    return (raw & kArg1Mask) >> kArg1Shift;
  }
  __attribute__((always_inline)) uint8_t arg2() const {
    return (raw & kArg2Mask) >> kArg2Shift;
  }
};

struct Instruction_IABC : public Instruction {
  __attribute__((always_inline)) uint8_t arg1() const {
    return (raw & kArg1Mask) >> kArg1Shift;
  }
  __attribute__((always_inline)) uint8_t arg2() const {
    return (raw & kArg2Mask) >> kArg2Shift;
  }
  __attribute__((always_inline)) uint8_t arg3() const {
    return (raw & kArg3Mask) >> kArg3Shift;
  }
};

struct Instruction_IABB : public Instruction {
  __attribute__((always_inline)) uint8_t arg1() const {
    return (raw & kArg1Mask) >> kArg1Shift;
  }
  __attribute__((always_inline)) uint16_t arg2() const {
    return (raw & kArgBBMask) >> kArgBBShift;
  }
};

struct Instruction_IAAX : public Instruction {
  __attribute__((always_inline)) uint16_t arg() const {
    return (raw & kArgAAMask) >> kArgAAShift;
  }
};

struct Instruction_IAAA : public Instruction {
  __attribute__((always_inline)) uint32_t arg() const {
    return (raw & kArgAAAMask) >> kArgAAAShift;
  }
};

#define DEF_OPSTRUCT(N, T, ...) \
  struct Instruction_##N : public Instruction_##T {};
FOREACH_OPCODE(DEF_OPSTRUCT)
#undef DEF_OPSTRUCT

inline Instruction encode_ixxx(Opcode opcode) {
  return Instruction{ opcode };
}

inline Instruction encode_iaxx(Opcode opcode, uint8_t arg) {
  return Instruction{ uint32_t{ opcode } | ((uint32_t)arg << Instruction::kArg1Shift) };
}

inline Instruction encode_iabx(Opcode opcode, uint8_t arg1, uint8_t arg2) {
  return Instruction{ uint32_t{ opcode } | ((uint32_t)arg1 << Instruction::kArg1Shift) |
                      ((uint32_t)arg2 << Instruction::kArg2Shift) };
}

inline Instruction encode_iabc(Opcode opcode, uint8_t arg1, uint8_t arg2, uint8_t arg3) {
  return Instruction{ uint32_t{ opcode } | ((uint32_t)arg1 << Instruction::kArg1Shift) |
                      ((uint32_t)arg2 << Instruction::kArg2Shift) | ((uint32_t)arg3 << Instruction::kArg3Shift) };
}

inline Instruction encode_iabb(Opcode opcode, uint8_t arg1, uint16_t arg2) {
  return Instruction{ uint32_t{ opcode } | ((uint32_t)arg1 << Instruction::kArg1Shift) |
                      ((uint32_t)arg2 << Instruction::kArgBBShift) };
}

inline Instruction encode_iaax(Opcode opcode, uint16_t arg) {
  return Instruction{ uint32_t{ opcode } | ((uint32_t)arg << Instruction::kArgAAShift) };
}

inline Instruction encode_iaaa(Opcode opcode, uint32_t arg) {
  DCHECK((arg & 0xff000000) == 0);
  return Instruction{ uint32_t{ opcode } | ((uint32_t)arg << Instruction::kArgAAAShift) };
}

/*
 * opcodes which terminate their basic block
 * */
static const std::unordered_set<Opcode> kTerminatingOpcodes = {
  Opcode::panic,
  Opcode::jmp,
  Opcode::throwex,
  Opcode::ret,
};

/*
 * opcodes that can perform branches (call-like excluded)
 * */
static const std::unordered_set<Opcode> kBranchingOpcodes = {
  Opcode::jmp,
  Opcode::jmpf,
  Opcode::jmpt,
  Opcode::testintjmp,
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
  { TokenType::GreaterEqual, Opcode::ge }
};

static std::unordered_map<TokenType, Opcode> kUnaryopOpcodeMapping = {
  { TokenType::Minus, Opcode::usub, },
  { TokenType::UnaryNot, Opcode::unot, },
  { TokenType::BitNOT, Opcode::ubnot, },
};

static std::unordered_map<BuiltinId, Opcode> kBuiltinOperationOpcodeMapping = {
#define ID(name, ...) { BuiltinId::name, Opcode::name, },
  FOREACH_BUILTIN(ID)
#undef ID
};
// clang-format on

}  // namespace charly::core::compiler::ir
