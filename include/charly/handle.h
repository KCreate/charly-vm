/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

namespace charly::core::runtime {

class Thread;
class ThreadLocalHandles;

class HandleScope {
  CHARLY_NON_COPYABLE(HandleScope);

public:
  explicit HandleScope(Thread* thread) : m_thread(thread) {}

  Thread* thread() const {
    return m_thread;
  }

private:
  Thread* m_thread;
};

template <typename T>
class Handle : public T {
  friend class ThreadLocalHandles;

public:
  Handle(HandleScope& scope, RawValue value);
  ~Handle();

  static_assert(std::is_base_of<RawValue, T>::value, "Expected T to be child of RawValue");
  CHARLY_NON_HEAP_ALLOCATABLE();

  // disallow copies using another handle
  template <typename O>
  Handle(HandleScope*, const Handle<O>&) = delete;

  T& operator*() {
    return *static_cast<T*>(this);
  }

  const T& operator*() const {
    return *static_cast<const T*>(this);
  }

  // allow assigning RawValue values to the handle
  template <typename S>
  Handle<T>& operator=(S other) {
    static_assert(std::is_base_of<S, T>::value || std::is_base_of<T, S>::value);
    *static_cast<T*>(this) = other.template rawCast<T>();
    DCHECK(is_valid_type(), "expected valid type");
    return *this;
  }

  // Handle<T> can be casted to Handle<S> if S is a subclass of T
  template <typename S>
  explicit operator const Handle<S>&() const {
    static_assert(std::is_base_of<S, T>::value, "Only up-casts are permitted");
    return *reinterpret_cast<const Handle<S>*>(this);
  }

  Handle<RawValue>* next() const {
    return m_next;
  }

private:
  bool is_valid_type() const {
    return T::value_is_type(**this);
  }

  Handle<RawValue>* pointer() {
    return bitcast<Handle<RawValue>*>(this);
  }

  Thread* m_thread;
  Handle<RawValue>* m_next;
};

#define HANDLE_DEF(T) using T = Handle<Raw##T>;
TYPE_NAMES(HANDLE_DEF)
#undef HANDLE_DEF

}  // namespace charly::core::runtime
