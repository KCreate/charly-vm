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

#include "gc.h"

namespace Charly {
  namespace GC {

    Collector::Collector(size_t heap_initial_count, size_t heap_cell_count) {

      // Initialize all the heaps
      this->heaps.reserve(heap_initial_count);
      while (heap_initial_count--) this->heaps.emplace_back();

      // Fill in the cells for the heaps
      for (auto &heap : this->heaps) {
        size_t i = heap_cell_count;
        while (i--) heap.emplace_back();
      }

      // Initialize all the cells
      Cell* last_cell = NULL;
      for (auto &heap : this->heaps) {
        for (auto &cell : heap) {
          cell.as.free.next = last_cell;
          last_cell = &cell;
        }
      }

      this->free_cell = last_cell;
    }

    // TODO: Garbage collect when there is no more memory left
    Cell* Collector::allocate() {
      Cell* cell = this->free_cell;
      if (cell) this->free_cell = this->free_cell->as.free.next;
      return cell;
    }

    void Collector::free(Cell* cell) {
      memset(cell, 0, sizeof(Cell));
      cell->as.free.next = this->free_cell;
      this->free_cell = cell;
    }
  }
}
