/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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
#include <unordered_set>
#include <vector>

#include "value.h"

#pragma once

namespace Charly {

// Item inside the MemoryCell
struct MemoryCell {
  union {
    struct {
      Basic basic;
      MemoryCell* next;
    } free;
    Basic basic;
    Object object;
    Array array;
    String string;
    Function function;
    CFunction cfunction;
    Generator generator;
    Class klass;
    Frame frame;
    CatchTable catchtable;
    CPointer cpointer;
  };

  template <typename T>
  inline T* as() {
    return reinterpret_cast<T*>(this);
  }

  inline VALUE as_value() {
    return charly_create_pointer(this);
  }
};

// Flagas for the memory manager
struct GarbageCollectorConfig {
  size_t initial_heap_count = 4;
  size_t heap_cell_count = 1 << 16;
  float heap_growth_factor = 2;
  size_t min_free_cells = 32;

  bool trace = false;
  std::ostream& out_stream = std::cerr;
  std::ostream& err_stream = std::cout;
};

class GarbageCollector {
  GarbageCollectorConfig config;
  VM* host_vm;
  MemoryCell* free_cell;
  size_t remaining_free_cells = 0;
  std::vector<MemoryCell*> heaps;
  std::unordered_set<VALUE> temporaries;
  std::unordered_set<void**> temporary_ptrs;  // Pointers to pointers
  std::unordered_set<std::vector<VALUE>*> temporary_vector_ptrs;

  void add_heap();
  void grow_heap();

public:
  GarbageCollector(GarbageCollectorConfig cfg, VM* host_vm) : config(cfg), host_vm(host_vm), free_cell(nullptr) {
    this->free_cell = nullptr;

    // Allocate the initial heaps
    size_t heapc = cfg.initial_heap_count;
    while (heapc--) {
      this->add_heap();
    }
  }
  GarbageCollector(const GarbageCollector&) = delete;
  GarbageCollector(GarbageCollector&&) = delete;
  ~GarbageCollector() {
    for (MemoryCell* heap : this->heaps) {
      std::free(heap);
    }
  }
  MemoryCell* allocate();
  void deallocate(MemoryCell* value);

  template <typename T>
  inline void deallocate(T value) {
    this->deallocate(reinterpret_cast<MemoryCell*>(value));
  }
  void mark(VALUE cell);
  template <typename T>
  inline void mark(T cell) {
    this->mark(reinterpret_cast<VALUE>(cell));
  }
  void mark(const std::vector<VALUE>& list);
  void collect();
  void mark_persistent(VALUE value);
  void unmark_persistent(VALUE value);
  void mark_ptr_persistent(void** value);
  void unmark_ptr_persistent(void** value);
  void mark_vector_ptr_persistent(std::vector<VALUE>* vec);
  void unmark_vector_ptr_persistent(std::vector<VALUE>* vec);
};
}  // namespace Charly
