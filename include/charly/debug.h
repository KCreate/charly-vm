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
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>

#include "utils/buffer.h"

#pragma once

namespace charly {
/*
 * thread-safe printing meant for debugging
 * */
extern std::mutex debugln_mutex;
extern std::chrono::steady_clock::time_point program_startup_timestamp;

inline void debugln_impl(std::ostream&) {
  // do nothing
}

inline void debugln_impl(std::ostream& stream, const char* format) {
  stream << format;
}

template <typename T, typename... Targs>
inline void debugln_impl(std::ostream& stream, const char* format, T value, Targs... Fargs) {
  while (*format != '\0') {
    if (*format == '%') {
      stream << value;
      debugln_impl(stream, format + 1, Fargs...);
      return;
    }
    stream << *format;

    format++;
  }
}

template <typename... Targs>
inline void debugln_impl_time(std::ostream& stream, const char* format, Targs... Fargs) {
  // get elapsed duration since program start
  auto now = std::chrono::steady_clock::now();
  auto dur = now - program_startup_timestamp;
  uint32_t ticks = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
  double elapsed_seconds = (float)ticks / 1000;

  // format as seconds with trailing milliseconds
  stream << "[";
  stream << std::right;
  stream << std::fixed;
  stream << std::setprecision(3);
  stream << std::setw(12);
  stream << elapsed_seconds;
  stream << std::setw(1);
  stream << "]: ";

  debugln_impl(stream, format, Fargs...);
}

template <typename... Targs>
inline void debugln_concurrent(const char* format, Targs... Fargs) {
  {
    std::unique_lock<std::mutex> locker(debugln_mutex);
    debugln_impl_time(std::cout, format, Fargs...);
    std::cout << std::endl;
  }
}

#ifdef NDEBUG

template <typename... Targs>
inline void debugln(const char*, Targs...) {}

#else

template <typename... Targs>
inline void debugln(const char* format, Targs... args) {
  debugln_concurrent(format, std::forward<Targs>(args)...);
}

#endif

template <typename... Targs>
inline void debuglnf(const char* format, Targs... args) {
  debugln_concurrent(format, std::forward<Targs>(args)...);
}

#define LIKELY(X) __builtin_expect(!!(X), 1)
#define UNLIKELY(X) __builtin_expect(!!(X), 0)

void print_runtime_debug_state(std::ostream& stream);

template <typename... Args>
[[noreturn]] void failed_check(
  const char* filename, int32_t line, const char* function, const char* expression, Args&&... args) {
  utils::Buffer buf;
  debugln_impl(buf, "Failed check!\n");
  debugln_impl_time(buf, "At %:% %:\n", filename, line, function);
  debugln_impl_time(buf, "Check '%' failed: ", expression);
  debugln_impl(buf, std::forward<Args>(args)...);
  buf << "\n";

  print_runtime_debug_state(buf);

  std::string str = buf.str();
  debugln_concurrent(str.c_str());

  std::abort();
}

#define CHECK(expr, ...)                                                        \
  do {                                                                          \
    if (UNLIKELY(!(expr))) {                                                    \
      charly::failed_check(__FILE__, __LINE__, __func__, #expr, ##__VA_ARGS__); \
    }                                                                           \
  } while (0)

#ifdef NDEBUG
  #define DCHECK(expr, ...)      \
    do {                         \
      if (UNLIKELY(!(expr))) {   \
        __builtin_unreachable(); \
      }                          \
    } while (0)
#else
  #define DCHECK(...) CHECK(__VA_ARGS__)
#endif

#define UNREACHABLE() DCHECK(false, "reached unreachable code")
#define UNIMPLEMENTED() CHECK(false, "not implemented")
#define FAIL(...)              \
  do {                         \
    CHECK(false, __VA_ARGS__); \
    __builtin_unreachable();   \
  } while (0)

}  // namespace charly
