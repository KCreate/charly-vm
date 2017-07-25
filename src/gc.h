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
#include <unordered_set>

#include "defines.h"
#include "value.h"
#include "frame.h"
#include "exception.h"
#include "block.h"

#pragma once

namespace Charly {
  namespace GC {
    const size_t InitialHeapCount = 2;
    const size_t HeapCellCount = 256;
    const float HeapCountGrowthFactor = 2;

    // A single cell managed by the GC
    struct Cell {
      union U {
        struct {
          VALUE flags;
          Cell* next;
        } free;
        Value::Basic basic;
        Value::Object object;
        Value::Array array;
        Value::Float flonum;
        Value::Function function;
        Value::CFunction cfunction;
        Machine::Frame frame;
        Machine::CatchTable catchtable;
        Machine::InstructionBlock instructionblock;

        U() { memset(this, 0, sizeof(U)); }
        U& operator=(const U& other) { memmove(this, &other, sizeof(U)); return *this; }
        ~U() {};
      } as;

      inline Cell() { memset(this, 0, sizeof(Cell)); }
      inline Cell(const Cell& cell) { *this = cell; }
      ~Cell() {};
    };

    struct Collector {
      private:
        Cell* free_cell;
        std::vector<std::vector<Cell>> heaps;
        std::unordered_set<VALUE> temporaries;

        void mark(VALUE cell);
        void add_heap();
        void grow_heap();

      public:
        Collector();
        Cell* allocate(Machine::VM* vm);
        void collect(Machine::VM* vm);
        void free(Cell* cell);
        void inline free(VALUE value) { this->free((Cell*) value); }
        void register_temporary(VALUE value);
        void unregister_temporary(VALUE value);
    };
  }
}
