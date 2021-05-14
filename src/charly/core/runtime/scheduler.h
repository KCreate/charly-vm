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
#include "charly/core/runtime/worker.h"

#pragma once

namespace charly::core::runtime {

// the amount of concurrent threads the underlying machine can run
// this is more or less the amount of logical cpu cores available
// static const uint64_t kHardwareConcurrency = std::thread::hardware_concurrency();
static const uint64_t kHardwareConcurrency = 2;

// Scheduling fibers will prefer the global queue over their local queue 1/X of the time
static const uint64_t kGlobalReadyQueuePriorityChance = 16;

// preempt a running fiber at a scheduler checkpoint
// if its running time exceeds this amount of milliseconds
static const uint64_t kSchedulerFiberTimeslice = 100;

// thread local pointer to the current worker thread
inline thread_local Worker* g_worker = nullptr;

class Scheduler {
  friend class Worker;
public:

  // initialize scheduler and vm global state
  static void initialize();
  inline static Scheduler* instance = nullptr;

  // start the runtime and begin executing charly code
  void start();

  // wait for the executing program to exit
  void join();

  // request the scheduler to quit
  void request_shutdown();

  // scheduler pause checkpoint
  // meant to be called from the application worker threads
  void checkpoint();

  // start / stop the world so that the GC can safely traverse
  // the internal data structures
  //
  // meant to be called from the concurrent GC worker thread
  void stop_the_world();
  void start_the_world();

  // schedule a fiber for execution in the global ready queue
  void schedule_fiber(HeapFiber* fiber);

  // pop a ready fiber from the global queue
  // waits for a task to be scheduled
  HeapFiber* get_ready_fiber();

  // check if the global ready queue has available tasks
  bool has_available_tasks();

  // checks wether the scheduler is currently in a state
  // where it could produce more tasks in the future
  bool could_produce_more_tasks();

  // register a worker thread as idle
  void register_idle(Worker* worker);

  // resume idle worker threads
  void resume_idle_worker();
  void resume_all_idle_workers();

  // get current steady timestamp in millisecond precision
  static uint64_t current_timestamp();

private:
  std::vector<Worker*> m_application_threads;

  charly::atomic<bool> m_wants_exit = false;
  charly::atomic<bool> m_wants_join = false;

  std::mutex m_idle_threads_m;
  std::list<Worker*> m_idle_threads;
  charly::atomic<uint32_t> m_idle_threads_counter = 0;

  std::mutex m_global_ready_queue_m;
  std::list<HeapFiber*> m_global_ready_queue;
};

}  // namespace charly::core::runtime
