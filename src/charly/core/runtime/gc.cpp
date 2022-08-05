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

#include "charly/core/runtime/gc.h"
#include "charly/core/runtime/runtime.h"

using namespace std::chrono_literals;

namespace charly::core::runtime {

void GarbageCollector::shutdown() {
  {
    std::unique_lock<std::mutex> locker(m_mutex);
    m_wants_exit = true;
  }
  m_cv.notify_one();
}

void GarbageCollector::join() {
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void GarbageCollector::perform_gc(Thread* thread) {
  uint64_t old_gc_cycle = m_gc_cycle;

  thread->native_section([&]() {
    // wake GC thread
    std::unique_lock<std::mutex> locker(m_mutex);
    if (m_wants_collection.cas(false, true)) {
      m_cv.notify_one();
    }

    // wait for GC thread to finish
    m_cv.wait(locker, [&]() {
      return (m_gc_cycle != old_gc_cycle) || m_wants_exit;
    });
  });

  thread->checkpoint();
}

void GarbageCollector::main() {
  m_runtime->wait_for_initialization();

  while (!m_wants_exit) {
    {
      std::unique_lock<std::mutex> locker(m_mutex);
      m_cv.wait(locker, [&]() {
        return m_wants_collection || m_wants_exit;
      });
    }

    if (!m_wants_collection) {
      continue;
    }

    m_runtime->scheduler()->stop_the_world();

    debugln("GC running");
    std::this_thread::sleep_for(1s);
    debugln("GC finished");

    m_gc_cycle++;
    m_wants_collection = false;
    m_cv.notify_all();

    m_runtime->scheduler()->start_the_world();
  }
}

}  // namespace charly::core::runtime
