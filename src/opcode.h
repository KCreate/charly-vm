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

    enum ThrowType : uint8_t {
      Return,
      Exception,
      Break,
      Continue
    };

    enum Opcode : uint8_t {

      // Do nothing
      Nop = 0x00,

      // Read a value at a given index in the current environment
      // Pushes Null if accessed with an out-of-bounds index
      //
      // args:
      // - index
      ReadLocal = 0x01,

      // Resolve symbol
      //
      // args:
      // - symbol
      ReadSymbol = 0x02,

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

      // Set a value at a given index in the current environment
      // Will write Null if the stack is empty
      // Will pop, but not write, if the index is out-of-bounds
      //
      // args:
      // - index
      //
      // stack:
      // - value
      SetLocal = 0x05,

      // Pop value and write to symbol
      //
      // args:
      // - symbol
      //
      // stack:
      // - value
      SetSymbol = 0x06,

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

      // Put floatval onto the stack
      //
      // args:
      // - floatval
      PutFloat = 0x0c,

      // Put a function onto the stack
      //
      // args:
      // - symbol
      // - block
      // - anonymous
      // - argc
      PutFunction = 0x0d,

      // Put a function pointer onto the stack
      //
      // args:
      // - symbol
      // - pointer
      // - argc
      PutCFunction = 0x0e,

      // Put an array onto the stack, reserve a given size
      //
      // args:
      // - size
      PutArray = 0x0f,

      // Put a hash onto the stack, reserve a given size
      //
      // args:
      // - size
      PutHash = 0x10,

      // Put a new class onto the stack
      // Pop parent classes of the stack
      //
      // args:
      // - symbol
      // - parentclasscount
      PutClass = 0x11,

      // Create a new entry in the offset-table of the current environment
      //
      // args:
      // - symbol
      // - offset
      RegisterLocal = 0x12,

      // Make a given offset in the current environment a constant
      //
      // args:
      // - offset
      MakeConstant = 0x13,

      // Pop count values off the stack
      //
      // args:
      // - count
      Pop = 0x14,

      // Duplicate the top value of the stack
      Dup = 0x15,

      // Swap the top two values of the stack
      Swap = 0x16,

      // Push the nth value of the stack
      //
      // args:
      // - offset
      Topn = 0x17,

      // Peek value from stack and store it at nth
      //
      // args:
      // - offset
      Setn = 0x18,

      // Call a function with argc arguments
      //
      // args:
      // - argc
      //
      // stack:
      // - function
      // - arguments
      Call = 0x19,

      // Call a function with argc arguments and a target
      //
      // args:
      // - argc
      //
      // stack:
      // - context
      // - function
      // - arguments
      CallMember = 0x1a,

      // Throw a value
      //
      // args:
      // - type
      //
      // stack:
      // - value
      Throw = 0x1b,

      // Apply a given offset to the instruction pointer
      //
      // args:
      // - offset
      Branch = 0x1c,

      // Pop test and apply a given offset to the instruction pointer
      // if test is truthy
      //
      // args:
      // - offset
      //
      // stack:
      // - test
      BranchIf = 0x1d,

      // Pop test and apply a given offset to the instruction pointer
      // if test is falsey
      //
      // args:
      // - offset
      //
      // stack:
      // - test
      BranchUnless = 0x1e,

      // Binary operators
      //
      // stack:
      // - left
      // - right
      Add = 0x1f,
      Sub = 0x20,
      Mul = 0x21,
      Div = 0x22,
      Mod = 0x23,
      Pow = 0x24,
      Eq  = 0x25,
      Neq = 0x26,
      Lt  = 0x27,
      Gt  = 0x28,
      Le  = 0x29,
      Ge  = 0x2a,
      Shr = 0x2b,
      Shl = 0x2c,
      And = 0x2d,
      Or  = 0x2e,
      Xor = 0x2f,

      // Unary operators
      //
      // stack:
      // - value
      UAdd  = 0x30,
      USub  = 0x31,
      UNot  = 0x32,
      UBNot = 0x33,

      // Machine internals, meant to be used directly by the machine itself
      // not some compiler compiling to be vm
      Halt = 0xff
    };
  }
}
