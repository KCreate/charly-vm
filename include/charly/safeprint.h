


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
#include <iomanip>
#include <cstdint>
#include <memory>
#include <mutex>
#include <chrono>
#include <unistd.h>

#pragma once

namespace charly {
/*
 * thread-safe printing meant for debugging
 * */
extern std::mutex safeprint_mutex;
extern std::chrono::steady_clock::time_point program_startup_timestamp;

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

template <typename... Targs>
inline void safeprint_concurrent(const char* format, Targs... Fargs) {
  {
    std::unique_lock<std::mutex> locker(safeprint_mutex);

    // get elapsed duration since program start
    auto now = std::chrono::steady_clock::now();
    auto dur = now - program_startup_timestamp;
    uint32_t ticks = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    double elapsed_seconds = (float)ticks / 1000;

    // format as seconds with trailing milliseconds
    std::cout << "[";
    std::cout << std::fixed;
    std::cout << std::setprecision(3);
    std::cout << std::setw(12);
    std::cout << elapsed_seconds;
    std::cout << std::setw(1);
    std::cout << "]: ";

    safeprint_impl(format, Fargs...);
    std::cout << std::endl;
  }
}

#ifdef NDEBUG

template <typename... Targs>
inline void safeprint(const char*, Targs...) {}

#else

template <typename... Targs>
inline void safeprint(const char* format, Targs... args) {
  safeprint_concurrent(format, std::forward<Targs>(args)...);
}

#endif

template <typename... Targs>
inline void safeprint_release(const char* format, Targs... args) {
  safeprint_concurrent(format, std::forward<Targs>(args)...);
}

}  // namespace charly
