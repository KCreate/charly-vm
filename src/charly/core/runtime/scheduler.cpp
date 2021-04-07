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

#include <cassert>
#include <chrono>

#include "charly/utils/argumentparser.h"
#include "charly/core/runtime/scheduler.h"

namespace charly::core::runtime {

using namespace std::chrono_literals;

// thread local pointer to the current fiber worker
thread_local FiberWorker* g_fiber;

// global pointer to scheduler
Scheduler scheduler;
Scheduler* const g_scheduler = &scheduler;

void FiberWorker::start() {
  m_startup_cv.notify_all();
}

void FiberWorker::wait() {
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void FiberWorker::main() {
  g_fiber = this;

  // wait for the start signal from the main thread
  {
    std::unique_lock<std::mutex> lk(m_startup_mutex);
    m_startup_cv.wait(lk, [] {
      return g_scheduler->fibers_ready();
    });
  }

  switch ((SchedulerCall)setjmp(m_context)) {
    case SchedulerCall::ExitTask: {
      Scheduler::free_current_task();
      // fallthrough
    }
    case SchedulerCall::Initialize:
    case SchedulerCall::Schedule: {
      schedule();
      return;
    }
    default: {
      assert(false && "invalid scheduler code");
      return;
    }
  }
}

void FiberWorker::schedule() {
  Task* next = Scheduler::get_next_task();

  if (next == nullptr) {
    return;
  }

  m_current_task = next;
  if (next->status == Task::Status::Created) {

    // swap stack
    // TODO: replace this with safe code
    void* top = next->stack_top;
    asm volatile(
      "mov %[rs], %%rsp \n"
      : [ rs ] "+r" (top) ::
    );

    // execute task
    next->status = Task::Status::Running;
    next->handler(next->argument);

    Scheduler::exit_current_task();
  } else {
    next->status = Task::Status::Running;
    std::longjmp(next->context, 1);
  }
}

Task* Scheduler::create_task(std::function<void(void*)> handler, void* argument) {
  static uint64_t id = 1;
  Task* task = new Task();
  task->id = id++;
  task->status = Task::Status::Created;
  task->handler = handler;
  task->argument = argument;
  task->stack_size = kFiberStackSize;
  task->stack_bottom = std::malloc(task->stack_size);
  task->stack_top = (void*)((uintptr_t)task->stack_bottom + task->stack_size);
  g_scheduler->m_tasks.insert({task->id, task});

  Scheduler::run_task(task);

  return task;
}

void Scheduler::run_task(Task* task) {
  std::unique_lock<std::mutex> lk(g_scheduler->m_ready_queue_mutex);
  g_scheduler->m_ready_queue.push(task);
  g_scheduler->m_ready_queue_cv.notify_one();
}

Task* Scheduler::get_next_task() {
  std::unique_lock<std::mutex> lk(g_scheduler->m_ready_queue_mutex);

  // wait for the next ready task
  if (g_scheduler->m_ready_queue.size() == 0) {
    g_scheduler->m_ready_queue_cv.wait(lk, [] {
      return g_scheduler->m_ready_queue.size();
    });
  }

  // return top task
  Task* next_task = g_scheduler->m_ready_queue.front();
  g_scheduler->m_ready_queue.pop();
  return next_task;
}

void Scheduler::free_current_task() {
  Task* task = g_fiber->m_current_task;
  g_fiber->m_current_task = nullptr;
  std::free(task->stack_bottom);
  delete task;
}

void Scheduler::exit_current_task() {
  Task* task = g_fiber->m_current_task;
  g_scheduler->m_tasks.erase(task->id);
  std::longjmp(g_fiber->m_context, (int)SchedulerCall::ExitTask);
}

void Scheduler::yield() {
  if (setjmp(g_fiber->m_current_task->context)) {
    return;
  } else {
    g_fiber->m_current_task->status = Task::Status::Waiting;
    run_task(g_fiber->m_current_task);
    std::longjmp(g_fiber->m_context, (int)SchedulerCall::Schedule);
  }
}

void Scheduler::pause() {
  if (setjmp(g_fiber->m_current_task->context)) {
    return;
  } else {
    g_fiber->m_current_task->status = Task::Status::Waiting;
    std::longjmp(g_fiber->m_context, (int)SchedulerCall::Schedule);
  }
}

void Scheduler::resume(uint64_t task_id) {
  assert(g_scheduler->m_tasks.count(task_id));
  run_task(g_scheduler->m_tasks.at(task_id));
}

void Scheduler::init() {

  // initialize fiber worker threads
  uint32_t fiber_worker_count = Scheduler::determine_fiber_workers_count();
  safeprint("spawning % fibers", fiber_worker_count);
  safeprint("jump_fcontext = %", (uintptr_t)&boost::context::detail::jump_fcontext);
  safeprint("make_fcontext = %", (uintptr_t)&boost::context::detail::make_fcontext);
  for (uint32_t i = 0; i < fiber_worker_count; i++) {
    FiberWorker* fiber = new FiberWorker();
    scheduler.m_fibers.push_back(fiber);
  }
}

void Scheduler::run() {

  // give the workers the start signal
  scheduler.m_fibers_ready = true;
  for (FiberWorker* fiber : scheduler.m_fibers) {
    fiber->start();
  }

  // wait for worker threads to finish
  for (FiberWorker* fiber : g_scheduler->m_fibers) {
    fiber->wait();
  }
}

uint32_t Scheduler::determine_fiber_workers_count() {
  const auto core_count = std::thread::hardware_concurrency();

  if (core_count == 0) {
    return 1;
  }

  return core_count;
}

}  // namespace charly::core::runtime
