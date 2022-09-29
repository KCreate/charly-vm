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

#include "charly/core/runtime/processor.h"
#include "charly/core/runtime/runtime.h"

namespace charly::core::runtime {

static atomic<size_t> processor_id_counter = 0;

Processor::Processor(Runtime* runtime) :
  m_runtime(runtime), m_id(processor_id_counter++), m_tab(std::make_unique<ThreadAllocationBuffer>(runtime->heap())) {}

Runtime* Processor::runtime() const {
  return m_runtime;
}

size_t Processor::id() const {
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
  return m_tab.get();
}

bool Processor::schedule_thread(Thread* thread) {
  std::lock_guard<std::mutex> locker(m_run_queue_mutex);
  DCHECK(thread->state() == Thread::State::Ready);
  if (m_run_queue.size() >= kLocalRunQueueMaxSize) {
    return false;
  }

  m_run_queue.push_back(thread);

  return true;
}

TimerId Processor::init_timer_fiber_create(size_t ts, RawFunction function, RawValue context, RawValue arguments) {
  std::lock_guard locker(m_timer_events_mutex);
  TimerId id = Processor::get_next_timer_id();
  TimerEvent event{ id, ts, TimerEvent::FiberCreate{ function, context, arguments } };
  m_timer_events.push_back(std::move(event));
  std::push_heap(m_timer_events.begin(), m_timer_events.end(), TimerEvent::Compare{});
  return id;
}

void Processor::suspend_thread_until(size_t ts, Thread* thread) {
  std::unique_lock locker(m_timer_events_mutex);
  auto cb = Thread::SchedulerPostCtxSwitchCallback([ts, &locker](Thread* thread, Processor* proc) {
    TimerId id = Processor::get_next_timer_id();
    TimerEvent event{ id, ts, TimerEvent::ThreadWake{ thread } };

    auto& events = proc->m_timer_events;
    events.push_back(std::move(event));
    std::push_heap(events.begin(), events.end(), TimerEvent::Compare{});
    locker.unlock();
  });
  Thread::context_switch_thread_to_scheduler(thread, Thread::State::Waiting, &cb);
}

bool Processor::cancel_timer(TimerId id) {
  // check current proc
  {
    std::lock_guard locker(m_timer_events_mutex);
    auto it = m_timer_events.begin();
    auto it_end = m_timer_events.end();
    while (it != it_end) {
      TimerEvent& event = *it;

      if (event.id == id) {
        m_timer_events.erase(it);
        return true;
      }

      it++;
    }
  }

  // check other procs
  for (Processor* other_proc : m_runtime->scheduler()->processors()) {
    if (other_proc != this) {
      std::lock_guard locker(other_proc->m_timer_events_mutex);
      auto it = other_proc->m_timer_events.begin();
      auto it_end = other_proc->m_timer_events.end();
      while (it != it_end) {
        TimerEvent& event = *it;

        if (event.id == id) {
          other_proc->m_timer_events.erase(it);
          return true;
        }

        it++;
      }
    }
  }

  return false;
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
    std::lock_guard<std::mutex> locker(m_run_queue_mutex);
    if (!m_run_queue.empty()) {
      Thread* thread = m_run_queue.front();
      m_run_queue.pop_front();
      return thread;
    }
  }

  // check global queue
  {
    if (Thread* thread = scheduler->get_ready_thread_from_global_run_queue()) {
      return thread;
    }
  }

  // attempt to steal from another processor
  {
    if (scheduler->steal_ready_threads(this)) {
      // FIXME: could this recursion cause an infinite loop in some weird scenarios?
      return get_ready_thread();
    }
  }

  return nullptr;
}

RawValue Processor::lookup_symbol(SYMBOL symbol) {
  if (m_symbol_table.count(symbol)) {
    return m_symbol_table.at(symbol);
  }

  RawValue result = m_runtime->lookup_symbol(symbol);

  if (result.isString()) {
    m_symbol_table[symbol] = RawString::cast(result);
  }

  return result;
}

bool Processor::steal_ready_threads(Processor* target_proc) {
  std::scoped_lock locker(m_run_queue_mutex, target_proc->m_run_queue_mutex);

  bool stole_some = false;
  while (!m_run_queue.empty() && target_proc->m_run_queue.size() < m_run_queue.size()) {
    Thread* thread = m_run_queue.front();
    m_run_queue.pop_front();
    target_proc->m_run_queue.push_back(thread);
    stole_some = true;
  }

  return stole_some;
}

void Processor::fire_timer_events(Thread* thread) {
  size_t now = get_steady_timestamp();
  std::lock_guard locker(m_timer_events_mutex);
  while (!m_timer_events.empty()) {
    TimerEvent& event = m_timer_events.front();
    if (event.timestamp <= now) {
      if (auto* arg = std::get_if<TimerEvent::FiberCreate>(&event.action)) {
        HandleScope scope(thread);
        Function function(scope, arg->function);
        Value context(scope, arg->context);
        Value arguments(scope, arg->arguments);
        RawFiber::create(thread, function, context, arguments);
      } else if (auto* arg = std::get_if<TimerEvent::ThreadWake>(&event.action)) {
        Thread* thread_to_wake = arg->thread;
        thread_to_wake->wake_from_wait();
        m_runtime->scheduler()->schedule_thread(thread_to_wake, this);
      } else {
        FAIL("Illegal timer event type");
      }

      std::pop_heap(m_timer_events.begin(), m_timer_events.end(), TimerEvent::Compare{});
      m_timer_events.pop_back();
      continue;
    }

    break;
  }
}

size_t Processor::timestamp_of_next_timer_event() {
  std::lock_guard locker(m_timer_events_mutex);
  if (m_timer_events.empty()) {
    return 0;
  }

  return m_timer_events.front().timestamp;
}

TimerId Processor::get_next_timer_id() {
  static atomic<TimerId> counter = 0;
  return counter++;
}

}  // namespace charly::core::runtime
