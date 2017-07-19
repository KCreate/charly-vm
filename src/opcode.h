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
  namespace Machine {

    // These are the throw types which are used by the throw instruction
    enum ThrowType : uint8_t {
      Exception,
      Break,
      Continue
    };

    // An opcode identifies a single instruction the machine can perform
    // Opcodes can have arguments
    enum Opcode : uint8_t {

      // Do nothing
      Nop = 0x00,

      // Read a value at a given offset from a given frame
      // Pushes null if accessed with an out-of-bounds index
      //
      // args:
      // - index
      // - level
      ReadLocal = 0x01,

      // Resolve symbol inside identifier
      //
      // args:
      // - symbol
      //
      // stack:
      // - identifier
      ReadMemberSymbol = 0x03,

      // Resole value inside identifier
      //
      // stack:
      // - identifier
      // - value
      ReadMemberValue = 0x04,

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
      SetLocal = 0x05,

      // Pop value and write to symbol of identifier
      //
      // args:
      // - symbol
      //
      // stack:
      // - identifier
      // - value
      SetMemberSymbol = 0x07,

      // Pop value and write to member of identifier
      //
      // stack:
      // - identifier
      // - member
      // - value
      SetMemberValue = 0x08,

      // Put the self value from the current frame onto the stack
      PutSelf = 0x09,

      // Put value onto the stack
      //
      // args:
      // - value
      PutValue = 0x0a,

      // Put string onto the stack
      //
      // args:
      // - string (char* data, uint32_t length, uint32_t capacity)
      PutString = 0x0b,

      // Put a function onto the stack
      //
      // args:
      // - symbol
      // - block
      // - anonymous
      // - argc
      PutFunction = 0x0c,

      // Put a function pointer onto the stack
      //
      // args:
      // - symbol
      // - pointer
      // - argc
      PutCFunction = 0x0d,

      // Put an array onto the stack, popping a given amount of values from the stack
      // and inserting them into the array
      //
      // args:
      // - count
      PutArray = 0x0e,

      // Put a hash onto the stack, popping a given amount of key / value pairs from the stack
      // and inserting them into the array
      //
      // args:
      // - count
      PutHash = 0x0f,

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
      PutClass = 0x10,

      // Make a given offset in the current environment a constant
      //
      // args:
      // - offset
      MakeConstant = 0x12,

      // Pop count values off the stack
      //
      // args:
      // - count
      Pop = 0x13,

      // Duplicate the top value of the stack
      Dup = 0x14,

      // Swap the top two values of the stack
      Swap = 0x15,

      // Push the nth value of the stack
      //
      // args:
      // - offset
      Topn = 0x16,

      // Peek value from stack and store it at nth
      //
      // args:
      // - offset
      Setn = 0x17,

      // Call a function with argc arguments
      //
      // args:
      // - argc
      //
      // stack:
      // - function
      // - arguments
      Call = 0x18,

      // Call a function with argc arguments and a target
      //
      // args:
      // - argc
      //
      // stack:
      // - target
      // - function
      // - arguments
      CallMember = 0x19,

      // Return from the current frame
      Return = 0x1a,

      // Throw a value
      //
      // args:
      // - type
      //
      // stack:
      // - value
      Throw = 0x1b,

      // Push a new catch table onto the machine
      // WARNING: Offset is in bytes, no instruction length decoding is done
      //
      // args:
      // - type
      // - offset
      RegisterCatchTable = 0x1c,

      // Pop the current catch table off the catchstack
      PopCatchTable = 0x1d,

      // Apply a given offset to the instruction pointer
      // WARNING: Offset is in bytes, no instruction length decoding is done
      //
      // args:
      // - offset
      Branch = 0x1e,

      // Pop test and apply a given offset to the instruction pointer
      // if test is truthy
      // WARNING: Offset is in bytes, no instruction length decoding is done
      //
      // args:
      // - offset
      //
      // stack:
      // - test
      BranchIf = 0x1f,

      // Pop test and apply a given offset to the instruction pointer
      // if test is falsey
      // WARNING: Offset is in bytes, no instruction length decoding is done
      //
      // args:
      // - offset
      //
      // stack:
      // - test
      BranchUnless = 0x20,

      // Binary operators
      //
      // stack:
      // - left
      // - right
      Add = 0x21,
      Sub = 0x22,
      Mul = 0x23,
      Div = 0x24,
      Mod = 0x25,
      Pow = 0x26,
      Eq  = 0x27,
      Neq = 0x28,
      Lt  = 0x29,
      Gt  = 0x2a,
      Le  = 0x2b,
      Ge  = 0x2c,
      Shr = 0x2d,
      Shl = 0x2e,
      And = 0x2f,
      Or  = 0x30,
      Xor = 0x31,

      // Unary operators
      //
      // stack:
      // - value
      UAdd  = 0x32,
      USub  = 0x33,
      UNot  = 0x34,
      UBNot = 0x35,

      // Machine internals, meant to be used directly by the machine itself
      // not some compiler compiling to be vm
      Halt = 0xff
    };
  }
}
