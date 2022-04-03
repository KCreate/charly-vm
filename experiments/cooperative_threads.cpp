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

#ifdef NDEBUG
template <typename... Targs>
inline void safeprint(const char*, Targs...) {}
#else
template <typename... Targs>
inline void safeprint(const char* format, Targs... Fargs) {
  {
    std::unique_lock<std::mutex> locker(safeprint_mutex);
    auto now = std::chrono::steady_clock::now();
    auto ticks = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::cout << ticks << ": ";
    safeprint_impl(format, Fargs...);
    std::cout << std::endl;
  }
}
#endif














struct FiberTask;
using FiberTaskUserFn = void (*)(FiberTask* task);

static const size_t kFiberStackSize = 8192;

struct FiberWorker;
struct FiberTask {
  friend struct FiberWorker;
  enum Status {
    StatusWaiting,
    StatusReady,
    StatusRunning,
    StatusExited
  };

  FiberTask(FiberTaskUserFn task_fn) {
    void* stack_bottom = std::malloc(kFiberStackSize);
    void* stack_top = (void*)((uintptr_t)stack_bottom + (uintptr_t)kFiberStackSize);

    static size_t id_counter = 1;
    this->id = id_counter++;
    this->status = FiberTask::StatusWaiting;
    this->stack_size = kFiberStackSize;
    this->stack_bottom = stack_bottom;
    this->stack_top = stack_top;
    this->context = make_fcontext(stack_top, kFiberStackSize, &FiberTask::ctx_handler);
    this->task_fn = task_fn;
  }

  ~FiberTask() {
    if (this->stack_bottom) {
      std::free(this->stack_bottom);
    }
    this->stack_bottom = nullptr;
    this->stack_top = nullptr;
  }

  void reschedule();

  void exit();

  static void ctx_handler(transfer_t transfer);

  size_t id;

  Status status;
  fcontext_t context;

  FiberTaskUserFn task_fn;

  void* stack_top;
  void* stack_bottom;
  size_t stack_size;
};

struct FiberWorker {
  friend struct FiberTask;
  FiberWorker(size_t id) :
    m_id(id), m_thread(&FiberWorker::main, this), m_current_task(nullptr) {}

  void join() {
    if (m_thread.joinable()) {
      m_thread.join();
    }
  }

  void main();

  size_t m_id;
  std::thread m_thread;
  fcontext_t m_main_ctx;
  FiberTask* m_current_task;
};

inline thread_local FiberWorker* active_worker = nullptr;

static const size_t kSchedulerWorkerCount = std::thread::hardware_concurrency();

class Scheduler {
  friend class FiberWorker;
  friend class FiberTask;
public:
  inline static Scheduler* instance = nullptr;
  static void initialize() {
    static Scheduler scheduler;
    Scheduler::instance = &scheduler;
    scheduler.init_workers();
  }

  Scheduler() : m_wants_exit(false) {}

  void shutdown() {
    m_wants_exit = true;

    m_cv.notify_all();

    for (FiberWorker* worker : m_workers) {
      worker->join();
    }

    for (FiberWorker*& worker : m_workers) {
      delete worker;
      worker = nullptr;
    }
  }

  void init_workers() {
    for (size_t i = 0; i < kSchedulerWorkerCount; i++) {
      m_workers.push_back(new FiberWorker(i));
    }
  }

  FiberTask* create_task(FiberTaskUserFn task_fn) {
    std::unique_lock<std::mutex> locker(m_mutex);
    FiberTask* task = new FiberTask(task_fn);
    m_tasks.insert(task);
    return task;
  }

  void delete_task(FiberTask* task) {
    std::unique_lock<std::mutex> locker(m_mutex);
    m_tasks.erase(task);
  }

  // append a task to the ready queue
  void schedule_task(FiberTask* task) {
    std::unique_lock<std::mutex> locker(m_mutex);
    // safeprint("scheduled task %", task->id);
    assert(task->status == FiberTask::StatusWaiting);
    task->status = FiberTask::StatusReady;
    m_ready_queue.push_back(task);
    m_cv.notify_all();
  }

  // pop ready task or wait
  FiberTask* get_ready_task() {
    std::unique_lock<std::mutex> locker(m_mutex);

    // wait for task
    if (m_ready_queue.size() == 0) {
      m_cv.wait(locker, [&] () -> bool {
        return m_ready_queue.size() || m_wants_exit;
      });
    }

    if (m_wants_exit) {
      return nullptr;
    }

    FiberTask* task = m_ready_queue.front();
    m_ready_queue.pop_front();
    return task;
  }

private:
  std::vector<FiberWorker*> m_workers;

  bool m_wants_exit;

  std::mutex m_mutex;
  std::condition_variable m_cv;

  std::set<FiberTask*> m_tasks;
  std::list<FiberTask*> m_ready_queue;
};

void FiberWorker::main() {
  safeprint("worker %: entered main method", m_id);

  while (!Scheduler::instance->m_wants_exit) {

    // get next ready task from scheduler
    FiberTask* task = Scheduler::instance->get_ready_task();
    if (task == nullptr) {
      break;
    }

    active_worker = this;
    m_current_task = task;

    assert(task->status == FiberTask::StatusReady);
    task->status = FiberTask::StatusRunning;
    safeprint("worker %: jumping to task %", m_id, task->id);
    transfer_t transfer = jump_fcontext(task->context, (void*)this);
    task->context = transfer.fctx;

    active_worker = nullptr;
    m_current_task = nullptr;

    switch (task->status) {

      // reschedule back into ready queue
      case FiberTask::StatusWaiting: {
        Scheduler::instance->schedule_task(task);
        break;
      }

      // delete task if it exited
      case FiberTask::StatusExited: {
        delete task;
        break;
      }

      default: {
        assert(false && "unexpected task status");
        break;
      }
    }
  }

  safeprint("worker %: leaving main method", m_id);
}

void FiberTask::reschedule() {
  this->status = StatusWaiting;
  safeprint("worker %: task % returning to scheduler", active_worker->m_id, this->id);
  transfer_t transfer = jump_fcontext(active_worker->m_main_ctx, nullptr);
  active_worker->m_main_ctx = transfer.fctx;
}

void FiberTask::exit() {
  this->status = StatusExited;
  Scheduler::instance->delete_task(this);
  safeprint("worker %: task % exiting", active_worker->m_id, this->id);
  jump_fcontext(active_worker->m_main_ctx, nullptr);
  assert(false && "never reached");
}

void FiberTask::ctx_handler(transfer_t transfer) {
  FiberWorker* worker = (FiberWorker*)transfer.data;
  worker->m_main_ctx = transfer.fctx;

  FiberTask* task = worker->m_current_task;
  task->task_fn(task);
  task->exit();
}

void task_fn(FiberTask* task) {
  for (int i = 0; i < 100; i++) {
    safeprint("task % on worker %: i = %", task->id, active_worker->m_id, i);
    std::this_thread::sleep_for(100ms);
    task->reschedule();
  }
}

int main() {
  safeprint("initialized scheduler");
  Scheduler::initialize();

  safeprint("creating tasks");
  for (int i = 0; i < 10; i++) {
    FiberTask* task = Scheduler::instance->create_task(&task_fn);

    Scheduler::instance->schedule_task(task);
  }

  std::this_thread::sleep_for(30s);
  Scheduler::instance->shutdown();

  return 0;
}
