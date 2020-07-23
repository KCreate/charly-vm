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

#include <mutex>

#include "defines.h"
#include "value.h"
#include "vm.h"

#pragma once

namespace Charly {
class ManagedContext {
private:
  VM& vm;
  std::vector<VALUE> temporaries;

public:
  ManagedContext(VM& t_vm) : vm(t_vm) {
  }
  ManagedContext(VM* t_vm) : vm(*t_vm) {}
  ~ManagedContext() {
    for (auto& temp : this->temporaries) {
      this->vm.gc.unmark_persistent(temp);
    }
  }

  template <class T>
  inline T mark_in_gc(T&& value) {
    this->vm.gc.mark_persistent(reinterpret_cast<VALUE>(value));
    this->temporaries.push_back(reinterpret_cast<VALUE>(value));
    return value;
  }

  template <typename... Args>
  inline Frame* create_frame(Args&&... params) {
    return mark_in_gc(this->vm.create_frame(std::forward<Args>(params)...));
  }

  template <typename... Args>
  inline CatchTable* create_catchtable(Args&&... params) {
    return mark_in_gc(this->vm.create_catchtable(std::forward<Args>(params)...));
  }

  template <typename... Args>
  inline VALUE create_object(Args&&... params) {
    return mark_in_gc(this->vm.create_object(std::forward<Args>(params)...));
  }

  template <typename... Args>
  inline VALUE create_array(Args&&... params) {
    return mark_in_gc(this->vm.create_array(std::forward<Args>(params)...));
  }

  template <typename... Args>
  inline VALUE create_string(Args&&... params) {
    return mark_in_gc(this->vm.create_string(std::forward<Args>(params)...));
  }

  template <typename... Args>
  inline VALUE create_weak_string(Args&&... params) {
    return mark_in_gc(this->vm.create_weak_string(std::forward<Args>(params)...));
  }

  template <typename... Args>
  inline VALUE create_function(Args&&... params) {
    return mark_in_gc(this->vm.create_function(std::forward<Args>(params)...));
  }

  template <typename... Args>
  inline VALUE create_cfunction(Args&&... params) {
    return mark_in_gc(this->vm.create_cfunction(std::forward<Args>(params)...));
  }

  template <typename... Args>
  inline VALUE create_generator(Args&&... params) {
    return mark_in_gc(this->vm.create_generator(std::forward<Args>(params)...));
  }

  template <typename... Args>
  inline VALUE create_class(Args&&... params) {
    return mark_in_gc(this->vm.create_class(std::forward<Args>(params)...));
  }

  template <typename... Args>
  inline VALUE create_cpointer(Args&&... params) {
    return mark_in_gc(this->vm.create_cpointer(std::forward<Args>(params)...));
  }
};
}  // namespace Charly
