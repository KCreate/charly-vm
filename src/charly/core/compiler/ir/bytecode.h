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

#include "charly/charly.h"
#include "charly/value.h"

#include "charly/core/compiler/token.h"
#include "charly/core/compiler/ir/builtin.h"

#pragma once

namespace charly::core::compiler::ir {

using Label = uint32_t;

// the datatype used inside the emitted bytecode to
// store an index into the functions inline cache table
using ICIndexType = uint16_t;

// - name of operand type
// - type used to represent it inside the IR
// - type thats actually encoded into the instruction stream
#define FOREACH_OPERANDTYPE(V)     \
                                   \
  /* count / index operands */     \
  V(Count8, uint8_t, uint8_t)      \
  V(Count16, uint16_t, uint16_t)   \
  V(Index8, uint8_t, uint8_t)      \
  V(Index16, uint16_t, uint16_t)   \
                                   \
  /* hashed symbol value */        \
  V(Symbol, std::string, uint32_t) \
                                   \
  /* relative offset to label */   \
  V(Offset, Label, int32_t)        \
                                   \
  /* immediate value */            \
  V(Immediate, runtime::RawValue, runtime::RawValue)       \
                                   \
  /* inline cache index */         \
  V(ICIndex, ICIndexType, ICIndexType)

enum class OperandType : uint16_t {
#define ID(name, _, __) name,
  FOREACH_OPERANDTYPE(ID)
#undef ID
};

static const size_t Size = 0;

#define OPTYPE(name, type, encoded_type) static const size_t Op##name##Size = sizeof(encoded_type);
FOREACH_OPERANDTYPE(OPTYPE)
#undef OPTYPE

#define OPTYPE(name, type, encoded_type) using name = encoded_type;
FOREACH_OPERANDTYPE(OPTYPE)
#undef OPTYPE

#define OPTYPE(name, type, encoded_type) using IROp##name = type;
FOREACH_OPERANDTYPE(OPTYPE)
#undef OPTYPE

enum ICType : uint8_t {
  ICNone = 0,
  ICSimpleValue,
  ICPropertyOffset,
  ICGlobalOffset,
  ICBinaryOp,
  ICUnaryOp
};

static std::string kInlineCacheTypeNames[] = {
  "ICNone",
  "ICSimpleValue",
  "ICPropertyOffset",
  "ICGlobalOffset",
  "ICBinaryOp",
  "ICUnaryOp"
};

// opcode listing
//  - name
//  - type of inline cache
//  - amount of popped values
//  - amount of pushed values
//  - size of operands
#define FOREACH_OPCODE(V)                                                                                            \
  /* nop - no operation                                                                                              \
   * */                                                                                                              \
  V(nop, ICNone, 0, 0)                                                                                               \
  /* panic - abort machine                                                                                           \
   * */                                                                                                              \
  V(panic, ICNone, 0, 0)                                                                                             \
                                                                                                                     \
  /* import - import module or file                                                                                  \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - modulename (filepath)                                                                                         \
   * - sourcepath (path of current file)                                                                             \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - module (value returned by included file)                                                                      \
   * */                                                                                                              \
  V(import, ICNone, 2, 1)                                                                                            \
  /* stringconcat - concatenate multiple values into a string                                                        \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - amount of values                                                                                              \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - values...                                                                                                     \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - result string                                                                                                 \
   * */                                                                                                              \
  V(stringconcat, ICNone, arg0, 1, Count8 count)                                                                     \
  /* declareglobal - declare a new global variable                                                                   \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - symbol                                                                                                        \
   * */                                                                                                              \
  V(declareglobal, ICNone, 0, 0, Symbol name)                                                                        \
  /* declareglobalconst - declare a new constant global variable                                                     \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - symbol                                                                                                        \
   * */                                                                                                              \
  V(declareglobalconst, ICNone, 0, 0, Symbol name)                                                                   \
  /* type - load the class of a value                                                                                \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - class of value                                                                                                \
   * */                                                                                                              \
  V(type, ICNone, 1, 1)                                                                                              \
                                                                                                                     \
  /* pop - pop a value from the stack                                                                                \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   * */                                                                                                              \
  V(pop, ICNone, 1, 0)                                                                                               \
  /* dup - duplicate top value on the stack                                                                          \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - value                                                                                                         \
   * - copy of value                                                                                                 \
   * */                                                                                                              \
  V(dup, ICNone, 1, 2)                                                                                               \
  /* dup2 - duplicate the top two values on the stack                                                                \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value1                                                                                                        \
   * - value2                                                                                                        \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - value1                                                                                                        \
   * - value2                                                                                                        \
   * - value1 copy                                                                                                   \
   * - value2 copy                                                                                                   \
   * */                                                                                                              \
  V(dup2, ICNone, 2, 4)                                                                                              \
                                                                                                                     \
  /* jmp - jump to label                                                                                             \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - branch target                                                                                                 \
   * */                                                                                                              \
  V(jmp, ICNone, 0, 0, Offset offset)                                                                                \
  /* jmpf - jump to label if TOS is falsey                                                                           \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - branch target                                                                                                 \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - test                                                                                                          \
   * */                                                                                                              \
  V(jmpf, ICNone, 1, 0, Offset offset)                                                                               \
  /* jmpt - jump to label if TOS is truthy                                                                           \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - branch target                                                                                                 \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - test                                                                                                          \
   * */                                                                                                              \
  V(jmpt, ICNone, 1, 0, Offset offset)                                                                               \
  /* testjmp - compare TOS with an immediate value and jump if they compare equal                                    \
   *           performs a non-strict comparison (true == 25 would branch)                                            \
   *                                                                                                                 \
   *           the test value gets pushed back onto the stack if the comparison failed                               \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - immediate test                                                                                                \
   * - branch target                                                                                                 \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - test                                                                                                          \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - test                                                                                                          \
   * */                                                                                                              \
  V(testjmp, ICNone, 1, 1, Immediate value, Offset offset)                                                           \
  /* throwex - throw TOS as exception                                                                                \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   * */                                                                                                              \
  V(throwex, ICNone, 1, 0)                                                                                           \
  /* getexception - push the last thrown exception onto the stack                                                    \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - exception                                                                                                     \
   * */                                                                                                              \
  V(getexception, ICNone, 0, 1)                                                                                      \
                                                                                                                     \
  /* call - call a function                                                                                          \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - amount of arguments                                                                                           \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - self value argument                                                                                           \
   * - function                                                                                                      \
   * - arguments...                                                                                                  \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - return value                                                                                                  \
   * */                                                                                                              \
  V(call, ICNone, 2 + arg0, 1, Count8 count)                                                                         \
  /* callspread - call a function with spreaded arguments                                                            \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - amount of segments                                                                                            \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - self value argument                                                                                           \
   * - function                                                                                                      \
   * - segments...                                                                                                   \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - return value                                                                                                  \
   * */                                                                                                              \
  V(callspread, ICNone, 2 + arg0, 1, Count8 count)                                                                   \
  /* ret - return from current function                                                                              \
   * */                                                                                                              \
  V(ret, ICNone, 0, 0)                                                                                               \
                                                                                                                     \
  /* load - load immediate value onto the stack                                                                      \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - immediate value                                                                                               \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - immediate value                                                                                               \
   * */                                                                                                              \
  V(load, ICNone, 0, 1, Immediate value)                                                                             \
  /* loadsymbol - load symbol onto the stack                                                                         \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - symbol                                                                                                        \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - symbol                                                                                                        \
   * */                                                                                                              \
  V(loadsymbol, ICNone, 0, 1, Symbol name)                                                                           \
  /* loadself - load the self value in this frame                                                                    \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - self                                                                                                          \
   * */                                                                                                              \
  V(loadself, ICNone, 0, 1)                                                                                          \
  /* loadargc - load the amount of arguments passed to the function                                                  \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - argc                                                                                                          \
   * */                                                                                                              \
  V(loadargc, ICNone, 0, 1)                                                                                          \
  /* loadglobal - load a global value                                                                                \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - symbol                                                                                                        \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - result value                                                                                                  \
   * */                                                                                                              \
  V(loadglobal, ICGlobalOffset, 0, 1, Symbol name, ICIndex ic_index)                                                 \
  /* loadlocal - load frame local variable                                                                           \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - slot index                                                                                                    \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - value                                                                                                         \
   * */                                                                                                              \
  V(loadlocal, ICNone, 0, 1, Index8 index)                                                                           \
  /* loadfar - load variable from surrounding function                                                               \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - function depth                                                                                                \
   * - slot index                                                                                                    \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - value                                                                                                         \
   * */                                                                                                              \
  V(loadfar, ICNone, 0, 1, Count8 depth, Index8 index)                                                               \
  /* loadattr - load property                                                                                        \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - symbol                                                                                                        \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - property                                                                                                      \
   * */                                                                                                              \
  V(loadattr, ICPropertyOffset, 1, 1, Symbol name, ICIndex ic_index)                                                 \
  /* loadattrsym - load property via symbol                                                                          \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   * - symbol                                                                                                        \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - property                                                                                                      \
   * */                                                                                                              \
  V(loadattrsym, ICPropertyOffset, 2, 1, ICIndex ic_index)                                                           \
  /* loadsuperconstructor - load the super constructor of a given                                                    \
   *                        value                                                                                    \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - type                                                                                                          \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - constructor function of the parent class                                                                      \
   *                        */                                                                                       \
  V(loadsuperconstructor, ICNone, 1, 1)                                                                              \
  /* loadsuperattr - load function from parent type                                                                  \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - symbol                                                                                                        \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - type                                                                                                          \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - function from parent type                                                                                     \
   * */                                                                                                              \
  V(loadsuperattr, ICNone, 1, 1, Symbol name)                                                                        \
                                                                                                                     \
  /* setglobal - write to global variable                                                                            \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - symbol                                                                                                        \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - value                                                                                                         \
   * */                                                                                                              \
  V(setglobal, ICGlobalOffset, 1, 1, Symbol name, ICIndex ic_index)                                                  \
  /* setlocal - write to frame local variable                                                                        \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - slot index                                                                                                    \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - value                                                                                                         \
   * */                                                                                                              \
  V(setlocal, ICNone, 1, 1, Index8 index)                                                                            \
  /* setreturn - set the function return value                                                                       \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   * */                                                                                                              \
  V(setreturn, ICNone, 1, 0)                                                                                         \
  /* setfar - write to variable from surrounding function                                                            \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - function depth                                                                                                \
   * - slot index                                                                                                    \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - value                                                                                                         \
   * */                                                                                                              \
  V(setfar, ICNone, 1, 1, Count8 depth, Index8 index)                                                                \
  /* setattr - write to property                                                                                     \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - symbol                                                                                                        \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - target                                                                                                        \
   * - value                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - value                                                                                                         \
   * */                                                                                                              \
  V(setattr, ICPropertyOffset, 2, 1, Symbol name, ICIndex ic_index)                                                  \
  /* setattrsym - write to property via symbol                                                                       \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - target                                                                                                        \
   * - symbol                                                                                                        \
   * - value                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - value                                                                                                         \
   * */                                                                                                              \
  V(setattrsym, ICPropertyOffset, 3, 1, ICIndex ic_index)                                                            \
                                                                                                                     \
  /* unpacksequence - unpack into sequence of values                                                                 \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - amount of values in sequence                                                                                  \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - source                                                                                                        \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - values...                                                                                                     \
   * */                                                                                                              \
  V(unpacksequence, ICNone, 1, arg0, Count8 count)                                                                   \
  /* unpacksequencespread - unpack into sequence of values with                                                      \
   *                        a spread somewhere in between                                                            \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - amount of values before the spread                                                                            \
   * - amount of values after the spread                                                                             \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - source                                                                                                        \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - values...                                                                                                     \
   * */                                                                                                              \
  V(unpacksequencespread, ICNone, 1, arg0 + arg1 + 1, Count8 before, Count8 after)                                   \
  /* unpackobject- unpack specific keys from value                                                                   \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - amount of keys                                                                                                \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - source                                                                                                        \
   * - key-symbols...                                                                                                \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - values...                                                                                                     \
   *                                                                                                                 \
   * */                                                                                                              \
  V(unpackobject, ICNone, 1 + arg0, arg0, Count8 count)                                                              \
  /* unpackobjectspread - unpack specific keys from value, put                                                       \
   *                      remaining keys into rest value                                                             \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - amount of keys                                                                                                \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - source                                                                                                        \
   * - key-symbols...                                                                                                \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - values...                                                                                                     \
   * - rest value                                                                                                    \
   * */                                                                                                              \
  V(unpackobjectspread, ICNone, 1 + arg0, arg0 + 1, Count8 count)                                                    \
                                                                                                                     \
  /* makefunc - allocate new function                                                                                \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - body label                                                                                                    \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - function                                                                                                      \
   * */                                                                                                              \
  V(makefunc, ICNone, 0, 1, Offset offset)                                                                           \
  /* makeclass - allocate new class                                                                                  \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - class name                                                                                                    \
   * - amount of member functions                                                                                    \
   * - amount of member properties                                                                                   \
   * - amount of static properties                                                                                   \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - constructor function                                                                                          \
   * - member functions...                                                                                           \
   * - member property symbols...                                                                                    \
   * - static property key                                                                                           \
   *   static property value...                                                                                      \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - class                                                                                                         \
   * */                                                                                                              \
  V(makeclass, ICSimpleValue, 1 + arg1 + arg2 + arg3 * 2, 1, Symbol name, Count8 memberfuncs, Count8 memberprops,    \
    Count8 staticprops, ICIndex ic_index)                                                                            \
  /* makesubclass - allocate new subclass                                                                            \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - class name                                                                                                    \
   * - amount of member functions                                                                                    \
   * - amount of member properties                                                                                   \
   * - amount of static properties                                                                                   \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - parent class                                                                                                  \
   * - constructor function                                                                                          \
   * - member functions...                                                                                           \
   * - member property symbols...                                                                                    \
   * - static property key                                                                                           \
   *   static property value...                                                                                      \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - class                                                                                                         \
   * */                                                                                                              \
  V(makesubclass, ICSimpleValue, 2 + arg1 + arg2 + arg3 * 2, 1, Symbol name, Count8 memberfuncs, Count8 memberprops, \
    Count8 staticprops, ICIndex ic_index)                                                                            \
  /* makestr - allocate new string                                                                                   \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - index into function string table                                                                              \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - string                                                                                                        \
   * */                                                                                                              \
  V(makestr, ICNone, 0, 1, Index16 index)                                                                            \
  /* makelist - allocate new list                                                                                    \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - amount of values                                                                                              \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - values...                                                                                                     \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - list                                                                                                          \
   * */                                                                                                              \
  V(makelist, ICNone, arg0, 1, Count16 count)                                                                        \
  /* makelistspread - allocate new list with spreaded arguments                                                      \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - amount of segments                                                                                            \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - segments...                                                                                                   \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - list                                                                                                          \
   * */                                                                                                              \
  V(makelistspread, ICNone, arg0, 1, Count16 count)                                                                  \
  /* makedict - allocate new dict                                                                                    \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - amount of key/value pairs                                                                                     \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - key symbol                                                                                                    \
   *   value...                                                                                                      \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - dict                                                                                                          \
   * */                                                                                                              \
  V(makedict, ICNone, arg0 * 2, 1, Count16 count)                                                                    \
  /* makedictspread - allocate new dict with spreaded arguments                                                      \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - amount of source values                                                                                       \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - values...                                                                                                     \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - dict                                                                                                          \
   * */                                                                                                              \
  V(makedictspread, ICNone, arg0, 1, Count16 count)                                                                  \
  /* maketuple - allocate new tuple                                                                                  \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - amount of values                                                                                              \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - values...                                                                                                     \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - tuple                                                                                                         \
   * */                                                                                                              \
  V(maketuple, ICNone, arg0, 1, Count16 count)                                                                       \
  /* maketuplespread - allocate new tuple with spreaded arguments                                                    \
   *                                                                                                                 \
   * opcode operands:                                                                                                \
   * - amount of segments                                                                                            \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - segments...                                                                                                   \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - tuple                                                                                                         \
   * */                                                                                                              \
  V(maketuplespread, ICNone, arg0, 1, Count16 count)                                                                 \
                                                                                                                     \
  /* fiberspawn - create new fiber and start running it                                                              \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - context                                                                                                       \
   * - function                                                                                                      \
   * - tuple containing arguments                                                                                    \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - fiber                                                                                                         \
   * */                                                                                                              \
  V(fiberspawn, ICNone, 3, 1)                                                                                        \
  /* fiberyield - yield from the current fiber                                                                       \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - argument from next fiber invocation                                                                           \
   * */                                                                                                              \
  V(fiberyield, ICNone, 1, 1)                                                                                        \
  /* fibercall - call a fiber                                                                                        \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - next yielded value or return value                                                                            \
   * */                                                                                                              \
  V(fibercall, ICNone, 1, 1)                                                                                         \
  /* fiberpause - pause current fiber                                                                                \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - value passed to fiberresume                                                                                   \
   * */                                                                                                              \
  V(fiberpause, ICNone, 0, 1)                                                                                        \
  /* fiberresume - pause current fiber                                                                               \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value (return value of corresponding fiberpause)                                                              \
   * */                                                                                                              \
  V(fiberresume, ICNone, 1, 0)                                                                                       \
  /* fiberawait - wait for a fiber to finish                                                                         \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - fiber                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - fiber return value                                                                                            \
   * */                                                                                                              \
  V(fiberawait, ICNone, 1, 1)                                                                                        \
                                                                                                                     \
  /* castXYZ - cast source value to new type                                                                         \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - casted value                                                                                                  \
   * */                                                                                                              \
  V(caststring, ICNone, 1, 1)                                                                                        \
  V(castsymbol, ICNone, 1, 1)                                                                                        \
  V(castiterator, ICNone, 1, 1)                                                                                      \
                                                                                                                     \
  /* iteratornext - load the next result tuple from an iterator                                                      \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - iterator                                                                                                      \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - result tuple (value, done)                                                                                    \
   * */                                                                                                              \
  V(iteratornext, ICNone, 1, 1)                                                                                      \
                                                                                                                     \
  /* binary operators                                                                                                \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - left                                                                                                          \
   * - right                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - result                                                                                                        \
   * */                                                                                                              \
  V(add, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                         \
  V(sub, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                         \
  V(mul, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                         \
  V(div, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                         \
  V(mod, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                         \
  V(pow, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                         \
  V(eq, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                          \
  V(neq, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                         \
  V(lt, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                          \
  V(gt, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                          \
  V(le, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                          \
  V(ge, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                          \
  V(shl, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                         \
  V(shr, ICBinaryOp, 2, 1, ICIndex ic_index)  /* sign-retaining right shift */                                       \
  V(shru, ICBinaryOp, 2, 1, ICIndex ic_index) /* zero-filling right shift */                                         \
  V(band, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                        \
  V(bor, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                         \
  V(bxor, ICBinaryOp, 2, 1, ICIndex ic_index)                                                                        \
                                                                                                                     \
  /* unary operators                                                                                                 \
   *                                                                                                                 \
   * stack arguments:                                                                                                \
   * - value                                                                                                         \
   *                                                                                                                 \
   * stack results:                                                                                                  \
   * - result                                                                                                        \
   * */                                                                                                              \
  V(usub, ICUnaryOp, 1, 1, ICIndex ic_index)                                                                         \
  V(unot, ICUnaryOp, 1, 1, ICIndex ic_index)                                                                         \
  V(ubnot, ICUnaryOp, 1, 1, ICIndex ic_index)

enum Opcode : uint8_t {
#define ID(name, ictype, stackpop, stackpush, ...) name,
  FOREACH_OPCODE(ID)
#undef ID
  __Count // amount of opcodes
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
  Opcode::testjmp
};

/*
 * declare a packed struct for each opcode for easy decoding
 * */
#define STRUCT_PROP(arg) arg;
#define STRUCT(name, ictype, stackpop, stackpush, ...) \
  struct __attribute__((packed)) Opcode_##name {       \
    Opcode opcode;                                     \
    CHARLY_VA_FOR_EACH(STRUCT_PROP, __VA_ARGS__)       \
  };
FOREACH_OPCODE(STRUCT)
#undef STRUCT
#undef STRUCT_PROP

struct __attribute__((packed)) InstructionDecoder {
  union {
    Opcode opcode;
#define STRUCT(name, ictype, stackpop, stackpush, ...) Opcode_##name name;
    FOREACH_OPCODE(STRUCT)
#undef STRUCT
  };

  uintptr_t ip() const {
    return bitcast<uintptr_t>(this);
  }
};

/*
 * total length of instruction in bytes (including opcode)
 * */
static constexpr size_t kOpcodeLength[] = {
#define MAP(name, ictype, stackpop, stackpush, ...) sizeof(Opcode_##name),
  FOREACH_OPCODE(MAP)
#undef MAP
};

/*
 * types of inline caches for each opcode
 * */
static ICType kOpcodeInlineCaches[] = {
#define TYPE(name, ictype, stackpop, stackpush, ...) ICType::ictype,
  FOREACH_OPCODE(TYPE)
#undef TYPE
};

static std::string kOpcodeNames[] = {
#define NAME(name, ictype, stackpop, stackpush, ...) #name,
  FOREACH_OPCODE(NAME)
#undef NAME
};

static std::unordered_map<std::string, Opcode> kOpcodeNameMapping = {
#define PAIR(name, ictype, stackpop, stackpush, ...) { #name, Opcode::name },
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
