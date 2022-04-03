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

#include <cassert>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <queue>
#include <thread>
#include <list>
#include <set>

#include <boost/context/detail/fcontext.hpp>

using namespace boost::context::detail;
using namespace std::chrono_literals;

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

std::mutex safeprint_mutex;
auto program_start_time = std::chrono::steady_clock::now();

#ifdef NDEBUG
template <typename... Targs>
inline void safeprint(const char*, Targs...) {}
#else
template <typename... Targs>
inline void safeprint(const char* format, Targs... Fargs) {
  {
    std::unique_lock<std::mutex> locker(safeprint_mutex);
    auto time_elapsed = std::chrono::steady_clock::now() - program_start_time;
    auto ticks = std::chrono::duration_cast<std::chrono::milliseconds>(time_elapsed).count();
    std::cout << std::setw(12) << std::setfill('_') << ticks << std::setfill(' ') << std::setw(1) << ": ";
    safeprint_impl(format, Fargs...);
    std::cout << std::endl;
  }
}
#endif


// actual implementation of scheduler
class SchedulerImpl {

};

// runtime scheduler API into scheduler
class Scheduler {
public:

  struct Transfer {
    void* data;
  };

  // initialize the scheduler
  static void initialize();

  // start all the scheduler workers
  static void start();

  static void shutdown();

  static void yield();

private:
  inline static const SchedulerImpl* m_instance;
};


void task_fn(Fiber* fiber) {

  for (int i = 0; i < 100; i++) {
    safeprint("fiber %: counter = %", fiber->id(), i);
    Scheduler::yield();
  }

}


int main() {

  for (int i = 0; i < 100; i++) {
    safeprint("counter = %", i);
    std::this_thread::sleep_for(10ms);
  }

  return 0;
}
