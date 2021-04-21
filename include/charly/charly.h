


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

#include <iostream>
#include <cstdint>
#include <memory>
#include <mutex>
#include <chrono>

#pragma once

namespace charly {

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

/*
 * thread-safe printing meant for debugging
 * */
inline void safeprint_impl(const char* format) {
  std::cout << format;
}

template <typename T, typename... Targs>
inline void safeprint_impl(const char* format, T value, Targs... Fargs) {
  while (*format != '\0') {
    if (*format == '%') {
      std::cout << value;
      safeprint_impl(format + 1, Fargs...);
      return;
    }
    std::cout << *format;

    format++;
  }
}

extern std::mutex safeprint_mutex;

#ifdef NDEBUG
template <typename... Targs>
inline void safeprint(const char*, Targs...) {}
#else
template <typename... Targs>
inline void safeprint(const char* format, Targs... Fargs) {
  {
    std::unique_lock<std::mutex> locker(safeprint_mutex);
    auto now = std::chrono::steady_clock::now();
    auto ticks = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::cout << ticks << ": ";
    safeprint_impl(format, Fargs...);
    std::cout << std::endl;
  }
}
#endif

}  // namespace charly
