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

#include <unistd.h>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>

#include "atomic.h"
#include "debug.h"
#include "symbol.h"

#include "utils/allocator.h"

#pragma once

namespace charly {

// Source: https://stackoverflow.com/a/11763277/2611393
#define CHARLY_FE_1(WHAT, X) WHAT(X)
#define CHARLY_FE_2(WHAT, X, ...) WHAT(X) CHARLY_FE_1(WHAT, __VA_ARGS__)
#define CHARLY_FE_3(WHAT, X, ...) WHAT(X) CHARLY_FE_2(WHAT, __VA_ARGS__)
#define CHARLY_FE_4(WHAT, X, ...) WHAT(X) CHARLY_FE_3(WHAT, __VA_ARGS__)
#define CHARLY_FE_5(WHAT, X, ...) WHAT(X) CHARLY_FE_4(WHAT, __VA_ARGS__)

#define CHARLY_GET_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME
#define CHARLY_VA_FOR_EACH(action, ...) \
  CHARLY_GET_MACRO(__VA_ARGS__, CHARLY_FE_5, CHARLY_FE_4, CHARLY_FE_3, CHARLY_FE_2, CHARLY_FE_1)(action, __VA_ARGS__)

#define CHARLY_NON_HEAP_ALLOCATABLE(C)      \
  void* operator new(size_t size) = delete; \
  void* operator new[](size_t size) = delete

#define CHARLY_NON_COPYABLE(C)     \
  C(const C&) = delete;            \
  C(C&) = delete;                  \
  C(const C&&) = delete;           \
  C(C&&) = delete;                 \
  C& operator=(C&) = delete;       \
  C& operator=(C&&) = delete;      \
  C& operator=(const C&) = delete; \
  C& operator=(const C&&) = delete

/*
 * shorthand method for often used shared_ptr stuff
 * */
template <typename T>
using ref = std::shared_ptr<T>;

template <typename T, typename... Args>
inline ref<T> make(Args&&... params) {
  return std::make_shared<T>(std::forward<Args>(params)...);
}

template <typename T, typename O>
inline ref<T> cast(ref<O> node) {
  return std::dynamic_pointer_cast<T>(node);
}

static const size_t kPointerSize = sizeof(uintptr_t);

inline uint64_t get_steady_timestamp() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

// helper method to cast one type to another
template <typename B, typename A>
B bitcast(A&& value) {
  return *reinterpret_cast<B*>(&value);
}

static const int32_t kInt32Min = -2147483647 - 1;
static const int32_t kInt32Max = 2147483647;
static const int32_t kInt24Min = -8388607 - 1;
static const int32_t kInt24Max = 8388607;
static const int16_t kInt16Min = -32767 - 1;
static const int16_t kInt16Max = 32767;
static const int16_t kInt8Min = -127 - 1;
static const int16_t kInt8Max = 127;

}  // namespace charly
