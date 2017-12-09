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
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>

#pragma once

namespace Charly {
typedef uintptr_t VALUE;
typedef intptr_t SIGNED_VALUE;
typedef uint16_t STATUS;
typedef void* FPOINTER;

class VM;
struct Frame;
struct CatchTable;
class InstructionBlock;

typedef std::function<void(InstructionBlock& block)> IBlockGenFunc;

template <class T>
class BlockLabel;
typedef BlockLabel<bool> BoolLabel;
typedef BlockLabel<uint8_t> UInt8Label;
typedef BlockLabel<uint16_t> UInt16Label;
typedef BlockLabel<uint32_t> UInt32Label;
typedef BlockLabel<uint64_t> UInt64Label;
typedef BlockLabel<int8_t> Int8Label;
typedef BlockLabel<int16_t> Int16Label;
typedef BlockLabel<int32_t> Int32Label;
typedef BlockLabel<int32_t> OffsetLabel;
typedef BlockLabel<int64_t> Int64Label;
typedef BlockLabel<double> DoubleLabel;
typedef BlockLabel<void*> PointerLabel;
typedef BlockLabel<InstructionBlock*> IBlockPointerLabel;

class CLI;
struct RunFlags;
struct Context;
class SourceFile;
class Buffer;
class SymbolTable;

enum ThrowType : uint8_t;
enum Opcode : uint8_t;

struct MemoryCell;
class MemoryManager;
class ManagedContext;

struct Basic;
struct Object;
struct Array;
struct String;
struct Float;
struct Function;
struct CFunction;
}  // namespace Charly
