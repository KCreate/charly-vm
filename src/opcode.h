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

    enum class Opcode {

      // Do nothing
      Nop = 0x00,

      // Resolve symbol
      //
      // args:
      // - symbol
      ReadSymbol = 0x01,

      // Resolve symbol inside identifier
      //
      // args:
      // - symbol
      //
      // stack:
      // - identifier
      ReadMemberSymbol = 0x02,

      // Resole value inside identifier
      //
      // stack:
      // - identifier
      // - value
      ReadMemberValue = 0x03,

      // Pop value and write to symbol
      //
      // args:
      // - symbol
      //
      // stack:
      // - value
      SetSymbol = 0x04,

      // Pop value and write to symbol of identifier
      //
      // args:
      // - symbol
      //
      // stack:
      // - identifier
      // - value
      SetMemberSymbol = 0x05,

      // Pop value and write to member of identifier
      //
      // stack:
      // - identifier
      // - member
      // - value
      SetMemberValue = 0x06,

      // Put the self value from the current frame onto the stack
      PutSelf = 0x07,

      // Put value onto the stack
      //
      // args:
      // - value
      PutValue = 0x08,

      // Put string onto the stack
      //
      // args:
      // - string
      PutString = 0x09,

      // Put floatval onto the stack
      //
      // args:
      // - floatval
      PutFloat = 0x0a,

      // Put a function onto the stack
      //
      // args:
      // - id
      // - block
      // - anonymous
      // - argc
      PutFunction = 0x0b,

      // Put a function pointer onto the stack
      //
      // args:
      // - id
      // - pointer
      // - argc
      PutCFunction = 0x0c,

      // Put an array onto the stack, reserve a given size
      //
      // args:
      // - size
      PutArray = 0x0d,

      // Put a hash onto the stack, reserve a given size
      //
      // args:
      // - size
      PutHash = 0x0e,

      // Put a new class onto the stack
      // Pop parent classes of the stack
      //
      // args:
      // - id
      // - parentclasscount
      PutClass = 0x0f,

      // Create a new entry in the offset-table of the current environment
      //
      // args:
      // - id
      // - offset
      RegisterLocal = 0x10,

      // Make a given offset in the current environment a constant
      //
      // args:
      // - offset
      MakeConstant = 0x11,

      // Pop count values off the stack
      //
      // args:
      // - count
      Pop = 0x12,

      // Duplicate the top value of the stack
      Dup = 0x13,

      // Swap the top two values of the stack
      Swap = 0x14,

      // Push the nth value of the stack
      //
      // args:
      // - offset
      Topn = 0x15,

      // Peek value from stack and store it at nth
      //
      // args:
      // - offset
      Setn = 0x16,

      // Pop count values off the stack
      //
      // args:
      // - count
      AdjustStack = 0x17,

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
      // - context
      // - function
      // - arguments
      CallMember = 0x19,

      // Throw a value
      //
      // args:
      // - type
      //
      // stack:
      // - value
      Throw = 0x1a,

      // Apply a given offset to the instruction pointer
      //
      // args:
      // - offset
      Branch = 0x1b,

      // Pop test and apply a given offset to the instruction pointer
      // if test is truthy
      //
      // args:
      // - offset
      //
      // stack:
      // - test
      BranchIf = 0x1c,

      // Pop test and apply a given offset to the instruction pointer
      // if test is falsey
      //
      // args:
      // - offset
      //
      // stack:
      // - test
      BranchUnless = 0x1d,

      // Binary operators
      //
      // stack:
      // - left
      // - right
      Add = 0x1e,
      Sub = 0x1f,
      Mul = 0x20,
      Div = 0x21,
      Mod = 0x22,
      Pow = 0x23,
      Eq  = 0x24,
      Neq = 0x25,
      Lt  = 0x26,
      Gt  = 0x27,
      Le  = 0x28,
      Ge  = 0x29,
      Shr = 0x2a,
      Shl = 0x2b,
      And = 0x2c,
      Or  = 0x2d,
      Xor = 0x2e,

      // Unary operators
      //
      // stack:
      // - value
      UAdd  = 0x2f,
      USub  = 0x30,
      UNot  = 0x31,
      UBNot = 0x32
    };

    enum class ThrowType {
      Return,
      Throw,
      Break,
      Continue
    };

    struct Instruction {
      uint8_t opcode;

      union {
        struct ReadSymbol {
          VALUE symbol;
        };

        struct ReadMemberSymbol {
          VALUE symbol;
        };

        struct SetSymbol {
          VALUE symbol;
        };

        struct SetMemberSymbol {
          VALUE symbol;
        };

        struct PutValue {
          VALUE value;
        };

        struct PutString {
          char* data;
          uint32_t size;
          uint32_t capacity;
        };

        struct PutFloat {
          double value;
        };

        struct PutFunction {
          VALUE id;
          // TODO: Block pointer
          bool anonymous;
          uint32_t argc;
        };

        struct PutCFunction {
          VALUE id;
          VALUE (* fpointer)();
          uint32_t argc;
        };

        struct PutArray {
          uint32_t size;
        };

        struct PutHash {
          uint32_t size;
        };

        struct PutClass {
          VALUE id;
          uint32_t parent_class_count;
        };

        struct RegisterLocal {
          VALUE id;
          uint32_t offset;
        };

        struct MakeConstant {
          uint32_t offset;
        };

        struct Pop {
          uint32_t count;
        };

        struct Topn {
          uint32_t offset;
        };

        struct Setn {
          uint32_t offset;
        };

        struct AdjustStack {
          uint32_t count;
        };

        struct Call {
          uint32_t argc;
        };

        struct CallMember {
          uint32_t argc;
        };

        struct Throw {
          ThrowType type;
        };

        struct Branch {
          uint32_t offset;
        };

        struct BranchIf {
          uint32_t offset;
        };

        struct BranchUnless {
          uint32_t offset;
        };
      } as;
    };

  }
}
