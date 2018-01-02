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
#include <iostream>

#include "value.h"

#pragma once

namespace Charly {

// Item inside the MemoryCell
struct MemoryCell {
  union {
    MemoryCell* next;
    VALUE value;
    VALUE flags;
    Basic basic;
    Object object;
    Array array;
    String string;
    Float flonum;
    Function function;
    CFunction cfunction;
    Frame frame;
    CatchTable catchtable;
  };

  template <typename T>
  inline T* as() {
    return reinterpret_cast<T*>(this);
  }
};

// Flagas for the memory manager
struct GarbageCollectorConfig {
  size_t initial_heap_count = 2;
  size_t heap_cell_count = 256;
  float heap_growth_factor = 2;

  bool trace = false;
  std::ostream& err_stream = std::cout;
  std::ostream& log_stream = std::cerr;
};

class GarbageCollector {
  GarbageCollectorConfig config;
  MemoryCell* free_cell;
  std::vector<MemoryCell*> heaps;
  std::unordered_set<VALUE> temporaries;

  void add_heap();
  void free_heap(MemoryCell* heap);
  void grow_heap();

public:
  GarbageCollector(GarbageCollectorConfig cfg) : config(cfg) {
    this->free_cell = nullptr;

    // Allocate the initial heaps
    size_t heapc = cfg.initial_heap_count;
    this->heaps.reserve(heapc);
    while (heapc--) {
      this->add_heap();
    }
  }
  ~GarbageCollector() {
    for (const auto heap : this->heaps) {
      free_heap(heap);
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
};
}  // namespace Charly
