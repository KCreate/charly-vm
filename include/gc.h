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

#include <unordered_set>
#include <vector>

#include "context.h"
#include "defines.h"
#include "exception.h"
#include "frame.h"
#include "instructionblock.h"
#include "value.h"

#pragma once

namespace Charly {
static const size_t kGCInitialHeapCount = 2;
static const size_t kGCHeapCellCount = 256;
static const float kGCHeapCountGrowthFactor = 2;

// This is the internal layout of a cell as managed by the MemoryHeap.
struct MemoryCell {
  union {
    struct {
      VALUE flags;
      MemoryCell* next;
    } free;
    Basic basic;
    Object object;
    Array array;
    String string;
    Float flonum;
    Function function;
    CFunction cfunction;
    Frame frame;
    CatchTable catchtable;
    InstructionBlock instructionblock;
  } as;
};

class MemoryManager {
private:
  Context& context;
  MemoryCell* free_cell;
  std::vector<MemoryCell*> heaps;
  std::unordered_set<VALUE> temporaries;

  void mark(VALUE cell);
  void add_heap();
  void grow_heap();

public:
  MemoryManager(Context& context);
  ~MemoryManager();
  MemoryCell* allocate();
  void collect();
  void free(MemoryCell* cell);
  void inline free(VALUE value) {
    this->free(reinterpret_cast<MemoryCell*>(value));
  }
  void register_temporary(VALUE value);
  void unregister_temporary(VALUE value);
};
}  // namespace Charly
