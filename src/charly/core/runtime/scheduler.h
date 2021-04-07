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

#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>
#include <queue>

#include <boost/context/detail/fcontext.hpp>

#include "charly/charly.h"

#pragma once

namespace charly::core::runtime {

static constexpr uint64_t kFiberStackSize = 16 * 1024;

struct Task;
class Scheduler;
class FiberWorker {
  friend class Scheduler;
public:
  FiberWorker() : m_thread(&FiberWorker::main, this) {
    static uint64_t _id = 1;
    this->id = _id++;
  }
  FiberWorker(FiberWorker&)        = delete; // delete copy constructor
  FiberWorker(FiberWorker&& other) = delete; // delete move constructor

  ~FiberWorker() {
    if (m_thread.joinable()) {
      m_thread.join();
    }
  }

  // notify the fiber worker that it can now start
  void start();

  // wait for the thread to finish
  void wait();

  // fiber worker main function
  void main();

// private:

  uint64_t id = 0;

  // the currently executing task
  Task* m_current_task = nullptr;

  // schedule a task from the scheduler in this fiber worker
  void schedule();

  std::thread m_thread;

  std::jmp_buf m_context;

  // mutex + cv used to delay startup until all fiber workers are ready
  std::mutex m_startup_mutex;
  std::condition_variable m_startup_cv;
};
thread_local extern FiberWorker* g_fiber;

enum class SchedulerCall {
  Initialize = 0,
  Schedule,
  ExitTask
};

struct Task {
  enum class Status {
    Created,
    Running,
    Waiting
  } status;

  uint64_t id;

  std::jmp_buf context;

  std::function<void(void*)> handler;
  void* argument;

  size_t stack_size;
  void* stack_bottom;
  void* stack_top;
};

class Scheduler;
extern Scheduler* const g_scheduler;
class Scheduler {
  friend class FiberWorker;
public:
  ~Scheduler() {
    for (FiberWorker* fiber : m_fibers) {
      if (fiber) {
        delete fiber;
        fiber = nullptr;
      }
    }
  }

  // create a new task and pass an argument
  static Task* create_task(std::function<void(void*)> handler, void* argument = nullptr);

  // mark a task as runnable and append it to the ready queue
  static void run_task(Task* task);

  // get the next executable task to execute
  static Task* get_next_task();

  // exit from the current task
  static void exit_current_task();

  // free the current tasks memory
  static void free_current_task();

  // yield control to the scheduler
  static void yield();

  // pause the current task
  static void pause();

  // mark a task as ready to be executed
  static void resume(uint64_t task_id);

  // initialize global scheduler data
  static void init();

  // schedule ready fibers until there are no more tasks left
  // then return
  static void run();

private:

  // determine the amount of fiber worker threads to spawn
  // based on actual logical cpu count
  static uint32_t determine_fiber_workers_count();

  // check if all fibers are ready
  bool fibers_ready() const {
    return m_fibers_ready;
  }

  // list of running fiber workers
  std::vector<FiberWorker*> m_fibers;
  bool m_fibers_ready = false;

  // the set of tasks in the scheduler
  std::unordered_map<uint64_t, Task*> m_tasks;

  // queue of tasks that are ready to be executed
  std::mutex m_ready_queue_mutex;
  std::condition_variable m_ready_queue_cv;
  std::queue<Task*> m_ready_queue;
};

}  // namespace charly::core::runtime
