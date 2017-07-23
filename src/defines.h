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

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#pragma once

namespace Charly {
  typedef uintptr_t VALUE;
  typedef intptr_t SIGNED_VALUE;
  typedef uint16_t STATUS;

  namespace Machine {
    struct VM;
    struct Frame;
    struct FrameEnvironmentEntry;
    struct CatchTable;

    enum ThrowType : uint8_t;
    enum Opcode : uint8_t;

    struct InstructionBlock;
  }

  namespace GC {
    struct Cell;
    struct Collector;
  }

  namespace Value {
    struct Basic;
    struct Object;
    struct Array;
    struct Float;
    struct Function;
    struct CFunction;
  }
}
