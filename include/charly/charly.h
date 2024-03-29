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

#include <unistd.h>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>

#include "atomic.h"
#include "debug.h"
#include "symbol.h"
#include "utils/allocator.h"
#include "utils/lock.h"

#pragma once

namespace charly {

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
template <typename T>
using weak_ref = std::weak_ptr<T>;

template <typename T, typename... Args>
inline ref<T> make(Args&&... params) {
  return std::make_shared<T>(std::forward<Args>(params)...);
}

template <typename T, typename O>
inline ref<T> cast(ref<O> node) {
  return std::dynamic_pointer_cast<T>(node);
}

inline size_t get_steady_timestamp() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

inline size_t get_steady_timestamp_micro() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

// helper method to cast one type to another
template <typename B, typename A>
B bitcast(A&& value) {
  return *reinterpret_cast<B*>(&value);
}

constexpr int64_t kInt32Min = -2147483647 - 1;
constexpr int64_t kInt32Max = 2147483647;
constexpr int64_t kInt24Min = -8388607 - 1;
constexpr int64_t kInt24Max = 8388607;
constexpr int64_t kInt16Min = -32767 - 1;
constexpr int64_t kInt16Max = 32767;
constexpr int64_t kInt8Min = -127 - 1;
constexpr int64_t kInt8Max = 127;

constexpr uint64_t kUInt32Min = 0;
constexpr uint64_t kUInt32Max = 0xffffffff;
constexpr uint64_t kUInt24Min = 0;
constexpr uint64_t kUInt24Max = 0x00ffffff;
constexpr uint64_t kUInt16Min = 0;
constexpr uint64_t kUInt16Max = 0x0000ffff;
constexpr uint64_t kUInt8Min = 0;
constexpr uint64_t kUInt8Max = 0x000000ff;

#ifdef NDEBUG
constexpr bool kIsDebugBuild = false;
#else
constexpr bool kIsDebugBuild = true;
#endif

inline constexpr bool double_fuzzy_equal(double left, double right) {
  if (!std::isfinite(left) || !std::isfinite(right)) {
    if (std::isnan(left) && std::isnan(right)) {
      return true;
    }

    return left == right;
  }

  constexpr double epsilon = 0.000001;
  return std::fabs(left - right) <= epsilon;
}

}  // namespace charly
