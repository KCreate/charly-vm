/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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
#include <mutex>
#include <atomic>

#include "value.h"
#include "immortal.h"

#pragma once

namespace Charly {

// Item inside the MemoryCell
struct MemoryCell {
  union {
    struct {
      Header header;
      MemoryCell* next;
    } free;
    Header header;
    Container container;
    Object object;
    Array array;
    String string;
    Function function;
    CFunction cfunction;
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

static const size_t kInitialHeapCount = 4;
static const size_t kHeapCellCount    = 4096;

class GarbageCollector {
  friend class VM;
public:
  struct Config {
    std::ostream& out_stream = std::cerr;
    std::ostream& err_stream = std::cout;
  };

  GarbageCollector(Config cfg, VM* vm) : config(cfg), host_vm(vm) {
    this->freelist = nullptr;
    for (size_t i = 0; i < kInitialHeapCount; i++) {
      this->add_heap();
    }
  }

  ~GarbageCollector() {
    for (MemoryCell* heap : this->heaps) {
      this->free_heap(heap);
    }
  }

  GarbageCollector(GarbageCollector&) = delete;
  GarbageCollector(GarbageCollector&&) = delete;

  template <class T, typename... Args>
  T* allocate(Args&&... params) {
    MemoryCell* cell = this->allocate_cell();
    T* value = cell->as<T>();
    value->init(std::forward<Args>(params)...);
    return value;
  }

  // Strings can be represented using the immediate encoded form
  VALUE create_string(const char* data, uint32_t length) {
    if (length <= 6)
      return charly_create_istring(data, length);
    return this->allocate<String>(data, length)->as_value();
  }

  VALUE create_string(const std::string& source) { return this->create_string(source.c_str(), source.size()); }

protected:
  void add_heap();
  void free_heap(MemoryCell* heap);
  void grow_heap();
  void collect();
  void deallocate(MemoryCell* cell);
  void clean(MemoryCell* cell);

  MemoryCell* allocate_cell();

  void mark(VALUE cell);
  void mark(Header* cell) {
    if (cell) {
      this->mark(cell->as_value());
    }
  }

  Config config;
  VM* host_vm;
  MemoryCell* freelist;
  std::vector<MemoryCell*> heaps;
  std::recursive_mutex mutex;
};
}  // namespace Charly
