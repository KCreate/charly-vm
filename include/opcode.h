/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include "defines.h"

#pragma once

namespace Charly {
// An opcode identifies a single instruction the machine can perform
// Opcodes can have arguments
enum Opcode : uint8_t {

  // Do nothing
  Nop,

  // Read a value at a given offset from a given frame
  // Pushes null if accessed with an out-of-bounds index
  //
  // args:
  // - index
  // - level
  ReadLocal,

  // Resolve symbol inside identifier
  //
  // args:
  // - symbol
  //
  // stack:
  // - identifier
  ReadMemberSymbol,

  // Resolve member inside identifier
  //
  // stack:
  // - identifier
  // - member
  ReadMemberValue,

  // Reads a value at a given index inside an array
  // The value popped from the stack needs to be an array, else this instruction does nothing
  // and the first stack argument will be pushed back onto the stack
  //
  // args:
  // - index
  //
  // stack:
  // - array
  ReadArrayIndex,

  // Set a value at a given offset inside a given frame
  // Will write null if the stack is empty
  // Will pop, but not write, if the index is out-of-bounds
  //
  // args:
  // - index
  // - level
  //
  // stack:
  // - value
  SetLocal,

  // Pop value and write to symbol of identifier
  //
  // args:
  // - symbol
  //
  // stack:
  // - identifier
  // - value
  SetMemberSymbol,

  // Pop value and write to member of identifier
  //
  // stack:
  // - identifier
  // - member
  // - value
  SetMemberValue,

  // Sets the value at a given index inside an array
  // The value popped from the stack needs to be an array, else this instruction does nothing
  //
  // The array is pushed back onto the stack in any case
  //
  // args:
  // - index
  //
  // stack:
  // - array
  // - value
  SetArrayIndex,

  // Put the self value from the current frame onto the stack
  PutSelf,

  // Put value onto the stack
  //
  // args:
  // - value
  PutValue,

  // Put a double onto the stack
  //
  // args:
  // - value
  PutFloat,

  // Put string onto the stack
  //
  // The actual string data lives in the TEXT segment of the compiled instructionblock
  //
  // args:
  // - offset
  // - length
  PutString,

  // Put a function onto the stack
  //
  // args:
  // - symbol
  // - block_offset
  // - anonymous
  // - argc
  // - lvarcount
  PutFunction,

  // Put a function pointer onto the stack
  //
  // args:
  // - symbol
  // - pointer
  // - argc
  PutCFunction,

  // Put an array onto the stack, popping a given amount of values from the stack
  // and inserting them into the array
  //
  // args:
  // - count
  PutArray,

  // Put a hash onto the stack, popping a given amount of key / value pairs from the stack
  // and inserting them into the array
  //
  // args:
  // - count
  PutHash,

  // Put a new class onto the stack
  // Properties, methods, static properties, static methods and parent classes are popped
  // off of the stack
  //
  // args:
  // - symbol
  // - propertycount
  // - staticpropertycount
  // - methodcount
  // - staticmethodcount
  // - parentclasscount
  // - has_constructor
  //
  // stack:
  // - properties
  // - static properties
  // - methods
  // - static methods
  // - parent classes
  // - constructor?
  PutClass,

  // Pop a value off the stack
  Pop,

  // Duplicate the top value of the stack
  Dup,

  // Swap the top two values of the stack
  Swap,

  // Push the nth value of the stack
  //
  // args:
  // - offset
  Topn,

  // Peek value from stack and store it at nth
  //
  // args:
  // - offset
  Setn,

  // Call a function with argc arguments
  //
  // args:
  // - argc
  //
  // stack:
  // - function
  // - arguments
  Call,

  // Call a function with argc arguments and a target
  //
  // args:
  // - argc
  //
  // stack:
  // - target
  // - function
  // - arguments
  CallMember,

  // Return from the current frame
  Return,

  // Throw a value
  //
  // stack:
  // - value
  Throw,

  // Push a new catch table onto the machine
  // WARNING: Offset is in bytes, no instruction length decoding is done
  //
  // args:
  // - offset
  RegisterCatchTable,

  // Pop the current catch table off the catchstack
  PopCatchTable,

  // Apply a given offset to the instruction pointer
  // WARNING: Offset is in bytes, no instruction length decoding is done
  //
  // args:
  // - offset
  Branch,

  // Pop test and apply a given offset to the instruction pointer
  // if test is truthy
  // WARNING: Offset is in bytes, no instruction length decoding is done
  //
  // args:
  // - offset
  //
  // stack:
  // - test
  BranchIf,

  // Pop test and apply a given offset to the instruction pointer
  // if test is falsey
  // WARNING: Offset is in bytes, no instruction length decoding is done
  //
  // args:
  // - offset
  //
  // stack:
  // - test
  BranchUnless,

  // Binary operators
  //
  // stack:
  // - left
  // - right
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Pow,
  Eq,
  Neq,
  Lt,
  Gt,
  Le,
  Ge,
  Shr,
  Shl,
  And,
  Or,
  Xor,

  // Unary operators
  //
  // stack:
  // - value
  UAdd,
  USub,
  UNot,
  UBNot,

  // Halt execution of the machine and return to the caller
  Halt,

  // Manually instruct the VM to perform a garbage collection right now
  GCCollect,

  // Push the type of the uppermost value on the stack as a string
  Typeof
};

// clang-format off
// Constant lengths of all instructions
static constexpr uint32_t kInstructionLengths[]{
  /* Nop */                1,
  /* ReadLocal */          1 + sizeof(uint32_t) + sizeof(uint32_t),
  /* ReadMemberSymbol */   1 + sizeof(VALUE),
  /* ReadMemberValue */    1,
  /* ReadArrayIndex */     1 + sizeof(uint32_t),
  /* SetLocal */           1 + sizeof(uint32_t) + sizeof(uint32_t),
  /* SetMemberSymbol */    1 + sizeof(VALUE),
  /* SetMemberValue */     1,
  /* SetArrayIndex */      1 + sizeof(uint32_t),
  /* PutSelf */            1,
  /* PutValue */           1 + sizeof(VALUE),
  /* PutFloat */           1 + sizeof(double),
  /* PutString */          1 + sizeof(uint32_t) + sizeof(uint32_t),
  /* PutFunction */        1 + sizeof(VALUE) + sizeof(int32_t) + sizeof(bool) + sizeof(uint32_t) + sizeof(uint32_t),
  /* PutCFunction */       1 + sizeof(VALUE) + sizeof(FPOINTER) + sizeof(uint32_t),
  /* PutArray */           1 + sizeof(uint32_t),
  /* PutHash */            1 + sizeof(uint32_t),
  /* PutClass */           1 + sizeof(VALUE) + sizeof(uint32_t) * 5 + sizeof(bool),
  /* Pop */                1,
  /* Dup */                1,
  /* Swap */               1,
  /* Topn */               1 + sizeof(uint32_t),
  /* Setn */               1 + sizeof(uint32_t),
  /* Call */               1 + sizeof(uint32_t),
  /* CallMember */         1 + sizeof(uint32_t),
  /* Return */             1,
  /* Throw */              1,
  /* RegisterCatchTable */ 1 + sizeof(int32_t),
  /* PopCatchTable */      1,
  /* Branch */             1 + sizeof(uint32_t),
  /* BranchIf */           1 + sizeof(uint32_t),
  /* BranchUnless */       1 + sizeof(uint32_t),
  /* Add, */               1,
  /* Sub, */               1,
  /* Mul, */               1,
  /* Div, */               1,
  /* Mod, */               1,
  /* Pow, */               1,
  /* Eq, */                1,
  /* Neq, */               1,
  /* Lt, */                1,
  /* Gt, */                1,
  /* Le, */                1,
  /* Ge, */                1,
  /* Shr, */               1,
  /* Shl, */               1,
  /* And, */               1,
  /* Or, */                1,
  /* Xor, */               1,
  /* UAdd */               1,
  /* USub */               1,
  /* UNot */               1,
  /* UBNot */              1,
  /* Halt */               1,
  /* GCCollect */          1,
  /* Typeof */             1
};

// String representations of instruction opcodes
static std::string kOpcodeMnemonics[]{
  "nop",
  "readlocal",
  "readmembersymbol",
  "readmembervalue",
  "readarrayindex",
  "setlocal",
  "setmembersymbol",
  "setmembervalue",
  "setarrayindex",
  "putself",
  "putvalue",
  "putfloat",
  "putstring",
  "putfunction",
  "putcfunction",
  "putarray",
  "puthash",
  "putclass",
  "pop",
  "dup",
  "swap",
  "topn",
  "setn",
  "call",
  "callmember",
  "return",
  "throw",
  "registercatchtable",
  "popcatchtable",
  "branch",
  "branchif",
  "branchunless",
  "add",
  "sub",
  "mul",
  "div",
  "mod",
  "pow",
  "eq",
  "neq",
  "lt",
  "gt",
  "le",
  "ge",
  "shr",
  "shl",
  "and",
  "or",
  "xor",
  "uadd",
  "usub",
  "unot",
  "ubnot",
  "halt",
  "gccollect",
  "typeof"
};
// clang-format on

}  // namespace Charly
