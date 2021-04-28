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

#include <condition_variable>
#include <thread>

#include "charly/charly.h"
#include "charly/atomic.h"

#include "charly/core/runtime/gc.h"

#pragma once

namespace charly::core::runtime {

static std::string kWorkerStateNames[] = {
  "Running",
  "Native",
  "Paused",
  "Exited"
};

class Worker;
inline thread_local Worker* g_worker = nullptr;
class Worker {
  friend class GarbageCollector;
  friend class GCConcurrentWorker;
public:
  Worker() : m_thread(&Worker::main, this) {
    static uint64_t id_counter = 1;
    this->id = id_counter++;
  }

  uint64_t id;

  // worker main method
  void main();

  // start the worker thread
  void start();

  // wait for the worker thread to join
  void join();

  // represents the state the worker is currently in
  enum class State : uint8_t {
    Running,      // worker is running charly code
    Native,       // worker is running native code
    Paused,       // worker is paused
    Exited        // worker has exited
  };

  // go from running into paused mode
  // worker resumes once Worker::wake gets called
  //
  // if the worker paused because it ran out of memory,
  // the scheduler will only wake the worker if the GC has
  // free regions available
  void pause();

  // go from running into native mode
  //
  // native mode is meant for long operations that do not
  // interact with the charly heap. this allows the scheduler
  // to keep this worker running while it pauses all other
  // worker threads
  void enter_native();

  // go from native into running mode
  //
  // pauses if the scheduler is currently in a paused state
  void leave_native();

  // exit worker
  void exit();

  // wait for this worker to arrive at a safepoint
  void wait_for_safepoint();

  // wake up the worker
  void wake();

  // assign the worker a heap region
  void set_gc_region(HeapRegion* region);

  // remove the assigned heap region
  void clear_gc_region();

  // wait for the state to be different than some old state
  void wait_for_state_change(Worker::State old_state);

  Worker::State state() const {
    return m_state.load(std::memory_order_acquire);
  }

  HeapRegion* active_region() {
    return m_active_region.load(std::memory_order_relaxed);
  }

private:
  charly::atomic<HeapRegion*> m_active_region = nullptr;
  charly::atomic<VALUE> m_head_cell = kNull;
  std::thread m_thread;
  charly::atomic<State> m_state = State::Running;
  std::mutex m_mutex;
  std::condition_variable m_cv;
};

}  // namespace charly::core::runtime
