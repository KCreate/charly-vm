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

#include "charly/utils/shared_queue.h"

#include "charly/core/runtime/gc.h"
#include "charly/core/runtime/fiber.h"

#pragma once

namespace charly::core::runtime {

// the maximum amount of fibers to be queued in each workers local queue
static const uint64_t kLocalReadyQueueMaxSize = 256;

class Worker {
  friend class GarbageCollector;
  friend class GCConcurrentWorker;
  friend class Fiber;
  friend class Scheduler;
public:

  // represents the state the worker is currently in
  enum class State : uint8_t {
    Running,        // worker is running charly code
    Native,         // worker is running native code
    Paused,         // worker is paused
    Idle,           // worker is waiting for work
    Exited          // worker has exited
  };

  Worker() : m_thread(&Worker::main, this) {
    static uint64_t id_counter = 1;
    m_id = id_counter++;
  }

  // worker main method
  void main();

  // wait for the worker thread to join
  void join();

  // register thread as idle in the scheduler
  void idle();

  // resume an idling thread
  void resume_from_idle();

  // exit worker
  void exit();

  // assign the worker a heap region
  void set_gc_region(HeapRegion* region);

  // remove the assigned heap region
  void clear_gc_region();

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

  /*
   * Query worker status
   * */
public:
  uint64_t id() const {
    return m_id;
  }

  Worker::State state() const {
    return m_state.load();
  }

  HeapRegion* active_region() {
    return m_active_region.load();
  }

  Fiber* current_fiber() {
    return m_current_fiber.load()->m_fiber;
  }

  /*
   * GC Pause mechanism
   * */
public:

  // GC checkpoint
  // called by the worker itself
  void checkpoint();

  // wait for this worker to arrive at a checkpoint
  // called by the GC worker
  void wait_for_checkpoint();

  // resume a paused worker
  void resume();

  // pause the currently running worker
  // the worker will only get resumed if the GC has memory available
  void pause();

  /*
   * Fiber management methods
   * */
public:

  // pause the current fiber
  void fiber_pause();

  // reschedule the current fiber
  void fiber_reschedule();

  // exit from the currently executing fiber and jump back into the main scheduling thread
  void fiber_exit();

private:

  // get the next ready fiber
  HeapFiber* get_local_ready_queue();

  // jump into the worker scheduler fiber
  void* jump_context(void* argument = nullptr);

private:
  uint64_t m_id;
  std::thread m_thread;
  charly::atomic<State> m_state = State::Running;
  charly::atomic<HeapRegion*> m_active_region = nullptr;
  charly::atomic<HeapFiber*> m_current_fiber = nullptr;

  // set by GC if it wants the worker to pause
  charly::atomic<bool> m_pause_request = false;

  // set by the scheduler if it wants the worker to wake from its idle pause
  charly::atomic<bool> m_resume_from_idle = false;

  // counts the amount of context switches between fibers the worker has performed
  uint8_t m_context_switch_counter = 0;

  // context of the workers main scheduling fiber
  boost::context::detail::fcontext_t m_context;

  // worker local queue of ready fibers to execute
  std::mutex m_ready_queue_m;
  std::list<HeapFiber*> m_ready_queue;

  // state change events
  std::mutex m_mutex;
  std::condition_variable m_safe_cv;    // fired when state changes to pause, native or exit
  std::condition_variable m_running_cv; // fired when state changes to running
  std::condition_variable m_idle_cv;    // fired when the worker gets woken from the idle state
};

}  // namespace charly::core::runtime
