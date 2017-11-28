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

#include "defines.h"

#pragma once

namespace Charly {
// These are the throw types which are used by the throw instruction
enum ThrowType : uint8_t { Exception, Break, Continue };

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
  // - block
  // - anonymous
  // - argc
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
  PutClass,

  // Make a given offset in the current environment a constant
  //
  // args:
  // - offset
  MakeConstant,

  // Pop count values off the stack
  //
  // args:
  // - count
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
  // args:
  // - type
  //
  // stack:
  // - value
  Throw,

  // Push a new catch table onto the machine
  // WARNING: Offset is in bytes, no instruction length decoding is done
  //
  // args:
  // - type
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

  // Machine internals, meant to be used directly by the machine itself
  // not some compiler compiling to be vm
  Halt
};
}  // namespace Charly
