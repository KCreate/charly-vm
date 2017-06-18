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
#include <array>

#include "defines.h"
#include "value.h"

#pragma once

namespace Charly {
  namespace GC {

    /*
     * A single cell managed by the GC
     * */
    struct Cell {
      union U {
        struct {
          VALUE flags;
          Cell* next;
        } free;
        Primitive::Basic basic;
        Primitive::Object object;
        Primitive::Float flonum;

        U() { memset(this, 0, sizeof(U)); }
      } as;
    };

    class Collector {
      private:
        Cell* free_cell;
        std::vector<std::vector<Cell>> heaps;
        std::vector<Cell*> root_nodes;
        std::vector<Cell*> mark_todo_list;

      public:
        Collector(size_t heap_initial_count, size_t heap_cell_count);
        Cell* allocate();
        void free(Cell* cell);
        void inline free(VALUE value) { this->free((Cell*) value); }

        // TODO: Add garbage collection logic
    };
  }
}
