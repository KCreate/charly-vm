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

#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>

#include "charly/charly.h"
#include "charly/atomic.h"

#include "charly/core/runtime/heapvalue.h"

#pragma once

namespace charly::core::runtime {

// forward declarations
class GarbageCollector;

class GCConcurrentWorker {
  friend class GarbageCollector;
public:
  GCConcurrentWorker(GarbageCollector* gc) : m_gc(gc) {}

  ~GCConcurrentWorker() {
    if (m_thread.joinable()) {
      m_thread.join();
    }
  }

  enum class Phase : uint8_t {
    Idle,
    Mark,
    Evacuate,
    UpdateRef
  };

  // start / stop the background GC worker thread
  void start_thread();
  void stop_thread();

  // starts a gc cycle if the GC worker is currently paused
  void request_gc();

  // get current worker state
  Phase phase() const;

private:

  // concurrent worker main method
  void main();

  // waits for the runtime to request a GC cycle
  void wait_for_gc_request();

  // GC STW pauses
  void init_mark();
  void init_evacuate();
  void init_updateref();
  void init_idle();

  // GC phases that run concurrently with the application threads
  void phase_mark();
  void phase_evacuate();
  void phase_updateref();

  // append a value to the greylist
  void mark(VALUE value);

private:
  std::thread m_thread;

  bool m_wants_exit = false;

  GarbageCollector* m_gc;
  charly::atomic<Phase> m_phase = Phase::Idle;

  std::list<HeapHeader*> m_greylist;

  std::mutex m_mutex;
  std::condition_variable m_cv;
};

using GCPhase = GCConcurrentWorker::Phase;

}  // namespace charly::core::runtime
