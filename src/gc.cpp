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
#include "vm.h"

namespace Charly {
  namespace GC {

    Collector::Collector() {
      this->free_cell = NULL;
      size_t heap_initial_count = GC::InitialHeapCount;
      this->heaps.reserve(heap_initial_count);
      while (heap_initial_count--) this->add_heap();
    }

    void Collector::add_heap() {
      this->heaps.emplace_back();
      std::vector<Cell>& heap = this->heaps.back();

      // Add the cells for the heaps
      size_t cell_count = GC::HeapCellCount;
      while (cell_count--) heap.emplace_back();

      // Initialize the cells and append to the free list
      Cell* last_cell = this->free_cell;
      for (auto& cell : heap) {
        memset(&cell, 0, sizeof(Cell));
        cell.as.free.next = last_cell;
        last_cell = &cell;
      }

      this->free_cell = last_cell;
    }

    void Collector::grow_heap() {
      size_t heap_count = this->heaps.size();
      size_t heaps_to_add = (heap_count * GC::HeapCountGrowthFactor) - heap_count;
      while (heaps_to_add--) this->add_heap();
    }

    void Collector::mark(VALUE value) {
      if (!Value::is_pointer(value)) return;
      if (Value::basics(value)->mark()) return;

      Value::basics(value)->set_mark(true);
      switch (Value::basics(value)->type()) {
        case Value::Type::Object: {
          auto obj = (Value::Object *)value;
          this->mark(obj->klass);
          for (auto& obj_entry : *obj->container) this->mark(obj_entry.second);
          break;
        }

        case Value::Type::Array: {
          auto arr = (Value::Array *)value;
          for (auto& arr_entry : *arr->data) this->mark(arr_entry);
          break;
        }

        case Value::Type::Function: {
          auto func = (Value::Function *)value;
          this->mark((VALUE)func->context);
          this->mark((VALUE)func->block);
          if (func->bound_self_set) this->mark(func->bound_self);
          break;
        }

        case Value::Type::CFunction: {
          auto cfunc = (Value::CFunction *)value;
          if (cfunc->bound_self_set) this->mark(cfunc->bound_self);
          break;
        }

        case Value::Type::Frame: {
          auto frame = (Machine::Frame *)value;
          this->mark((VALUE)frame->parent);
          this->mark((VALUE)frame->parent_environment_frame);
          this->mark((VALUE)frame->function);
          this->mark(frame->self);
          for (auto& lvar : *frame->environment) this->mark(lvar.value);
          break;
        }

        case Value::Type::CatchTable: {
          auto table = (Machine::CatchTable *)value;
          this->mark((VALUE)table->frame);
          this->mark((VALUE)table->parent);
          break;
        }

        case Value::Type::InstructionBlock: {
          auto block = (Machine::InstructionBlock *)value;
          for (auto& child_block : *block->child_blocks) {
            this->mark((VALUE)child_block);
          }
          break;
        }
      }
    }

    void Collector::collect(Machine::VM* vm) {
      std::cout << "#-- GC: Pause --#" << std::endl;

      // Mark Phase
      for (auto& stack_item : vm->stack) this->mark(stack_item);
      this->mark((VALUE)vm->frames);
      this->mark((VALUE)vm->catchstack);

      // Sweep Phase
      int freed_cells_count = 0;
      for (auto& heap : this->heaps) {
        for (auto& cell : heap) {
          if (Value::basics((VALUE)&cell)->mark()) {
            Value::basics((VALUE)&cell)->set_mark(false);
          } else {
            // This cell might already be on the free list
            // Make sure we don't double free cells
            if (Value::basics((VALUE)&cell)->type() != Value::Type::DeadCell) {
              freed_cells_count++;
              this->free(&cell);
            }
          }
        }
      }

      std::cout << "#-- GC: Freed a total of " << freed_cells_count << " cells --#" << std::endl;
      std::cout << "#-- GC: Finished --#" << std::endl;
    }

    Cell* Collector::allocate(Machine::VM* vm) {
      Cell* cell = this->free_cell;

      if (cell) {
        this->free_cell = this->free_cell->as.free.next;

        // If we've just allocated the last available cell,
        // we do a collect in order to make sure we never get a failing
        // allocation in the future
        if (!this->free_cell) {
          this->collect(vm);

          // If a collection didn't yield new available space,
          // allocate more heaps
          if (!this->free_cell) {
            this->grow_heap();

            if (!this->free_cell) {
              std::cout << "Failed to expand heap, the next allocation will cause a segfault." << std::endl;
            }
          }
        }
      }

      return cell;
    }

    void Collector::free(Cell* cell) {

      // We don't actually free the cells that were allocated, we just
      // deallocat the properties inside these cells and memset(0)'em
      switch (Value::basics((VALUE)cell)->type()) {
        case Value::Type::Object: {
          delete cell->as.object.container;
          break;
        }

        case Value::Type::Array: {
          delete cell->as.array.data;
          break;
        }

        case Value::Type::Frame: {
          delete cell->as.frame.environment;
          break;
        }

        case Value::Type::InstructionBlock: {
          delete cell->as.instructionblock.child_blocks;
          break;
        }

        default: {
          break;
        }
      }

      memset(cell, 0, sizeof(Cell));
      cell->as.free.next = this->free_cell;
      this->free_cell = cell;
    }
  }
}
