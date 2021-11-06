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

#include "charly/core/runtime/processor.h"
#include "charly/core/runtime/runtime.h"

namespace charly::core::runtime {

static atomic<uint64_t> processor_id_counter = 0;

Processor::Processor(Runtime* runtime) {
  m_runtime = runtime;
  m_id = processor_id_counter++;
  m_live = false;
  m_worker = nullptr;
  m_tab = new ThreadAllocationBuffer(runtime->heap());
}

Processor::~Processor() {
  delete m_tab;
}

Runtime* Processor::runtime() const {
  return m_runtime;
}

uint64_t Processor::id() const {
  return m_id;
}

bool Processor::is_live() const {
  return m_live;
}

void Processor::set_live(bool value) {
  m_live = value;
}

Worker* Processor::worker() const {
  return m_worker;
}

void Processor::set_worker(Worker* worker) {
  m_worker = worker;
}

ThreadAllocationBuffer* Processor::tab() const {
  return m_tab;
}

bool Processor::schedule_thread(Thread* thread) {
  DCHECK(thread->state() == Thread::State::Ready);

  std::lock_guard<std::mutex> locker(m_mutex);
  if (m_run_queue.size() >= kLocalRunQueueMaxSize) {
    return false;
  }

  m_run_queue.push_back(thread);

  return true;
}

Thread* Processor::get_ready_thread() {
  Scheduler* scheduler = this->runtime()->scheduler();
  Worker* worker = this->worker();

  DCHECK(scheduler);
  DCHECK(worker);

  // pull ready thread from global run queue at random intervals
  // this prevents long-running fibers from hogging a processor, thus
  // preventing the global run queue to be drained
  if (worker->rand() % kGlobalRunQueuePriorityChance == 0) {
    if (Thread* thread = scheduler->get_ready_thread_from_global_run_queue()) {
      return thread;
    }
  }

  // check current processors local run queue
  {
    std::lock_guard<std::mutex> locker(m_mutex);
    if (m_run_queue.size()) {
      Thread* thread = m_run_queue.front();
      m_run_queue.pop_front();
      return thread;
    }
  }

  // check global queue
  return scheduler->get_ready_thread_from_global_run_queue();
}

RawValue Processor::lookup_symbol(SYMBOL symbol) {
  if (m_symbol_table.count(symbol)) {
    return m_symbol_table.at(symbol);
  }

  return m_runtime->lookup_symbol(symbol);
}

}  // namespace charly::core::runtime
