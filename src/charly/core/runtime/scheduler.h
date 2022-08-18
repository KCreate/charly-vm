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
#include <list>
#include <mutex>
#include <stack>
#include <unordered_set>
#include <vector>

#include "charly/core/runtime/processor.h"
#include "charly/core/runtime/thread.h"
#include "charly/core/runtime/worker.h"

#pragma once

namespace charly::core::runtime {

// chance that a processor will acquire from the global queue instead of the local queue
constexpr uint32_t kGlobalRunQueuePriorityChance = 32;

// the maximum amount of milliseconds a worker should spend in its idle phase before checking
// if there are any work available
constexpr uint32_t kWorkerMaximumIdleSleepDuration = 1000;

// the maximum amount of threads to be queued in each workers local run queue
constexpr uint64_t kLocalRunQueueMaxSize = 256;

// milliseconds each thread can run before having to yield back control
constexpr uint64_t kThreadTimeslice = 10;

class Runtime;
class GarbageCollector;

class Scheduler {
  friend class GarbageCollector;
  friend class Runtime;
public:
  explicit Scheduler(Runtime* runtime);
  ~Scheduler();

  // returns the amount of physical cpu cores
  static uint32_t hardware_concurrency();

  Runtime* runtime() const;

  // wait for all worker threads to join
  void join();

  // acquire a free thread from the freelist or allocate new one
  Thread* get_free_thread();

  // acquire a free stack from the freelist or allocate new one
  Stack* get_free_stack();

  // give a thread instance back to the scheduler for recycling
  void recycle_thread(Thread* thread);

  // give a stack back to the scheduler for recycling
  void recycle_stack(Stack* stack);

  // schedule a thread for execution
  //
  // if the current_proc argument is passed, the scheduler will attempt to
  // schedule the thread in the passed processor.
  void schedule_thread(Thread* thread, Processor* current_proc = nullptr);

  // stop / start the world
  void stop_the_world();
  void start_the_world();

  // attempt to acquire and bind an idle processor to a worker
  // returns false if no idle processor was found
  bool acquire_processor_for_worker(Worker* worker);

  // releases the processor associated with a worker back to the scheduler
  void release_processor_from_worker(Worker* worker);

  // acquire a ready thread from the global run queue
  Thread* get_ready_thread_from_global_run_queue();

  // attempt to steal a free thread from another processor
  bool steal_ready_threads(Processor* target_proc);

private:
  void wake_idle_worker();

private:
  Runtime* m_runtime;

  std::vector<Worker*> m_workers;
  std::vector<Processor*> m_processors;

  std::mutex m_threads_mutex;
  std::unordered_set<Thread*> m_threads;
  std::stack<Thread*> m_free_threads;

  std::mutex m_stacks_mutex;
  std::unordered_set<Stack*> m_stacks;
  std::stack<Stack*> m_free_stacks;

  std::mutex m_idle_procs_mutex;
  std::stack<Processor*> m_idle_processors;

  std::mutex m_run_queue_mutex;
  std::list<Thread*> m_run_queue;
};

}  // namespace charly::core::runtime
