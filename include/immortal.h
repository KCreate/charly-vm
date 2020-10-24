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

#include "value.h"

#pragma once

namespace Charly {

template <typename T = VALUE> class Immortal;

// RAII wrapper to mark a charly value as immortal
template <typename T>
class Immortal {
public:
  Immortal(T* value = nullptr) : m_value(value) {
    this->mark();
  }

  Immortal(VALUE value) : Immortal(charly_as_pointer_to<T>(value)) {}

  ~Immortal() {
    this->unmark();
  }

  Immortal<T>& store(T* value) {
    this->unmark();
    this->m_value = value;
    this->mark();
    return *this;
  }

  Immortal<T>& store(VALUE value) {
    return this->store(charly_as_pointer_to<T>(value));
  }

  Immortal<T>& operator=(T* value) {
    return this->store(value);
  }

  Immortal<T>& operator=(VALUE value) {
    return this->store(value);
  }

  T* get() {
    return this->m_value;
  }

  operator T*() {
    return this->get();
  }

  T* operator->() {
    return this->m_value;
  }

  // delete copy and move constructors
  Immortal(Immortal<T>& o) = delete;
  Immortal(Immortal<T>&& o) = delete;

protected:

  // Mark the value as immortal
  void mark() {
    if (Header* header = this->m_value) {
      header->immortal = true;
    }
  }

  // Mark the value as mortal
  void unmark() {
    if (Header* header = this->m_value) {
      header->immortal = false;
    }
  }

  T* m_value;
};

// RAII wrapper to mark a single heap cell as immortal
template <>
class Immortal<VALUE> {
public:
  Immortal(VALUE value = kNull) : m_value(value) {
    this->mark();
  }

  Immortal(Header* ptr) : Immortal(ptr->as_value()) {}

  ~Immortal() {
    this->unmark();
  }

  Immortal& store(VALUE value) {
    this->unmark();
    this->m_value = value;
    this->mark();
    return *this;
  }

  Immortal& operator=(VALUE value) {
    return this->store(value);
  }

  // Load value
  VALUE get() {
    return this->m_value;
  }

  operator VALUE() {
    return this->get();
  }

  // delete copy and move constructors
  Immortal(Immortal& o) = delete;
  Immortal(Immortal&& o) = delete;

protected:

  // Mark the value as immortal
  void mark() {
    if (charly_is_on_heap(this->m_value)) {
      Header* header = charly_as_header(this->m_value);
      header->immortal = true;
    }
  }

  // Mark the value as mortal
  void unmark() {
    if (charly_is_on_heap(this->m_value)) {
      Header* header = charly_as_header(this->m_value);
      header->immortal = false;
    }
  }

  VALUE m_value;
};

}  // namespace Charly
