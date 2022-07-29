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

#include <thread>

#include "charly/core/runtime/scheduler.h"

#include "charly/utils/argumentparser.h"
#include "charly/utils/cast.h"

namespace charly::core::runtime {

Scheduler::Scheduler(Runtime* runtime) : m_runtime(runtime) {
  // determine the amount of virtual processors to spawn
  uint32_t proc_count = Scheduler::hardware_concurrency();
  if (utils::ArgumentParser::is_flag_set("maxprocs")) {
    const auto& values = utils::ArgumentParser::get_arguments_for_flag("maxprocs");
    CHECK(values.size(), "expected at least one value for maxprocs");
    const std::string& value = values.front();
    int64_t num = utils::string_to_int(value);

    if (num > 0) {
      proc_count = num;
    }
  }

  // setup processors and worker threads
  {
    std::lock_guard<std::mutex> locker(m_idle_procs_mutex);
    for (uint32_t i = 0; i < proc_count; i++) {
      auto* processor = new Processor(m_runtime);
      m_processors.push_back(processor);
      m_idle_processors.push(processor);

      auto* worker = new Worker(m_runtime);
      m_workers.push_back(worker);
    }
  }

  // initialize the main thread
  auto main_thread = get_free_thread();
  main_thread->init_main_thread();
  main_thread->ready();
  schedule_thread(main_thread);
}

Scheduler::~Scheduler() {
  for (Worker* worker : m_workers) {
    delete worker;
  }
  m_workers.clear();

  for (Processor* proc : m_processors) {
    delete proc;
  }
  m_processors.clear();

  for (Thread* thread : m_threads) {
    delete thread;
  }
  m_threads.clear();
}

uint32_t Scheduler::hardware_concurrency() {
  return std::thread::hardware_concurrency();
}

Runtime* Scheduler::runtime() const {
  return m_runtime;
}

void Scheduler::join() {
  for (Worker* worker : m_workers) {
    worker->wake();
  }

  for (Worker* worker : m_workers) {
    worker->join();
    CHECK(worker->state() == Worker::State::Exited);
  }
}

Thread* Scheduler::get_free_thread() {
  Thread* thread;

  {
    std::lock_guard<std::mutex> locker(m_threads_mutex);
    if (!m_free_threads.empty()) {
      thread = m_free_threads.top();
      m_free_threads.pop();
    } else {
      thread = new Thread(m_runtime);
      m_threads.insert(thread);
    }
  }

  return thread;
}

Stack* Scheduler::get_free_stack() {
  Stack* stack;

  {
    std::lock_guard<std::mutex> locker(m_stacks_mutex);
    if (!m_free_stacks.empty()) {
      stack = m_free_stacks.top();
      m_free_stacks.pop();
    } else {
      stack = new Stack();
      m_stacks.insert(stack);
    }
  }

  return stack;
}

void Scheduler::recycle_thread(Thread* thread) {
  thread->clean();
  std::lock_guard<std::mutex> locker(m_threads_mutex);
  m_free_threads.push(thread);
}

void Scheduler::recycle_stack(Stack* stack) {
  stack->clear();
  std::lock_guard<std::mutex> locker(m_stacks_mutex);
  m_free_stacks.push(stack);
}

void Scheduler::schedule_thread(Thread* thread, Processor* current_proc) {
  DCHECK(thread->state() == Thread::State::Ready);

  // attempt to schedule in current processor
  if (current_proc) {
    if (current_proc->schedule_thread(thread)) {
      return;
    }
  }

  // schedule in global queue
  {
    std::unique_lock<std::mutex> locker(m_run_queue_mutex);
    m_run_queue.push_back(thread);
  }

  wake_idle_worker();
}

void Scheduler::stop_the_world() {
  for (Worker* worker : m_workers) {
    worker->stop_the_world();
  }
}

void Scheduler::start_the_world() {
  for (Worker* worker : m_workers) {
    worker->start_the_world();
  }
}

bool Scheduler::acquire_processor_for_worker(Worker* worker) {
  DCHECK(worker->processor() == nullptr);

  Processor* proc = nullptr;
  {
    std::lock_guard<std::mutex> locker(m_idle_procs_mutex);
    if (!m_idle_processors.empty()) {
      proc = m_idle_processors.top();
      m_idle_processors.pop();
    }
  }

  if (proc == nullptr) {
    return false;
  }

  DCHECK(!proc->is_live());
  DCHECK(proc->worker() == nullptr);

  proc->set_live(true);
  proc->set_worker(worker);
  worker->set_processor(proc);

  return true;
}

void Scheduler::release_processor_from_worker(Worker* worker) {
  Processor* proc = worker->processor();
  DCHECK(proc);

  proc->set_live(false);
  proc->set_worker(nullptr);
  worker->set_processor(nullptr);

  std::lock_guard<std::mutex> locker(m_idle_procs_mutex);
  m_idle_processors.push(proc);
}

Thread* Scheduler::get_ready_thread_from_global_run_queue() {
  std::lock_guard<std::mutex> locker(m_run_queue_mutex);
  if (!m_run_queue.empty()) {
    Thread* thread = m_run_queue.front();
    m_run_queue.pop_front();
    return thread;
  }

  return nullptr;
}

bool Scheduler::steal_ready_threads(Processor* target_proc) {
  for (Processor* proc : m_processors) {
    if (proc != target_proc) {
      if (proc->steal_ready_threads(target_proc)) {
        return true;
      }
    }
  }

  return false;
}

void Scheduler::wake_idle_worker() {
  for (Worker* worker : m_workers) {
    if (worker->state() == Worker::State::Idle) {
      if (worker->wake()) {
        return;
      }
    }
  }
}

}  // namespace charly::core::runtime
