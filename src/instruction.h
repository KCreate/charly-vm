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

#include <vector>

#include "defines.h"

#include "opcode.h"

#pragma once

namespace Charly {
  namespace Machine {

    // Forward declaration, original is inside block.h
    struct InstructionBlock;

    enum class ThrowType {
      Return,
      Throw,
      Break,
      Continue,
      Finish
    };

    struct Instruction {
      uint8_t opcode;

      union {
        struct ReadSymbol {
          ID symbol;
        };

        struct ReadMemberSymbol {
          ID symbol;
        };

        struct SetSymbol {
          ID symbol;
        };

        struct SetMemberSymbol {
          ID symbol;
        };

        struct PutValue {
          ID value;
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
          ID id;
          InstructionBlock* block;
          bool anonymous;
          uint32_t argc;
          std::vector<ID> argumentlist;
        };

        struct PutCFunction {
          ID id;
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
          ID id;
          uint32_t parent_class_count;
        };

        struct RegisterLocal {
          ID id;
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
  };
};
