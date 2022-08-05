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

#include <condition_variable>
#include <mutex>
#include <thread>

#include "charly/charly.h"

#pragma once

namespace charly::core::runtime {

class Runtime;
class Thread;

class GarbageCollector {
  friend class Heap;
public:
  explicit GarbageCollector(Runtime* runtime) :
    m_runtime(runtime),
    m_wants_collection(false),
    m_wants_exit(false),
    m_gc_cycle(0),
    m_thread(std::thread(&GarbageCollector::main, this)) {}

  ~GarbageCollector() {
    DCHECK(!m_thread.joinable());
  }

  void shutdown();
  void join();
  void perform_gc(Thread* thread);

private:
  void main();

  Runtime* m_runtime;

  atomic<bool> m_wants_collection;
  atomic<bool> m_wants_exit;
  atomic<uint64_t> m_gc_cycle;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::condition_variable m_finished_cv;

  std::thread m_thread;
};

}  // namespace charly::core::runtime
