/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2021 Leonard Schütz
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

#include <atomic>

#pragma once

namespace charly {

template <class T>
struct atomic : public std::atomic<T> {
  using std::atomic<T>::atomic;

  // change default load memory ordering to acquire
  T load(std::memory_order order = std::memory_order_acquire) const noexcept {
    return std::atomic<T>::load(order);
  }
  T load(std::memory_order order = std::memory_order_acquire) const volatile noexcept {
    return std::atomic<T>::load(order);
  }

  // change default store memory ordering to release
  void store(T desired, std::memory_order order = std::memory_order_release) noexcept {
    return std::atomic<T>::store(desired, order);
  }
  void store(T desired, std::memory_order order = std::memory_order_release) volatile noexcept {
    return std::atomic<T>::store(desired, order);
  }

  // sane CAS
  bool cas(T expected, T desired) {
    return std::atomic<T>::compare_exchange_strong(expected, desired, std::memory_order_release);
  }
  bool cas_weak(T expected, T desired) {
    return std::atomic<T>::compare_exchange_weak(expected, desired, std::memory_order_release);
  }
};

}
