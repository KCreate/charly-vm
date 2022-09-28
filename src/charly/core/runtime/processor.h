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

#include <list>
#include <mutex>
#include <unordered_map>

#include "charly/core/runtime/heap.h"
#include "charly/value.h"

#pragma once

namespace charly::core::runtime {

class Runtime;
class Worker;
class Thread;
class ThreadAllocationBuffer;
class GarbageCollector;

using TimerId = size_t;
struct TimerEvent {
  enum Type {
    kEventFiberCreate,
    kEventFutureResolve,
    kEventFutureReject
  };

  Type type;
  size_t timestamp;
  RawValue arg1;
  RawValue arg2;
  RawValue arg3;
  RawValue arg4;

  struct Compare {
    constexpr bool operator()(const TimerEvent& left, const TimerEvent& right) const {
      return left.timestamp < right.timestamp;
    }
  };
};

// represents a virtual processor
class Processor {
  friend class GarbageCollector;
  friend class Runtime;

public:
  explicit Processor(Runtime* runtime);

  // getter / setter
  Runtime* runtime() const;
  size_t id() const;
  bool is_live() const;
  void set_live(bool value);
  Worker* worker() const;
  void set_worker(Worker* worker);
  ThreadAllocationBuffer* tab() const;

  // attempt to schedule a thread on this processor
  // returns false if the run queue is already at peak capacity
  bool schedule_thread(Thread* thread);

  // schedule a new timer at some timestamp in the future
  void init_timer_fiber_create(size_t timestamp, RawFunction function, RawValue context, RawValue arguments);
  // acquire the next ready thread to execute
  Thread* get_ready_thread();

  // look up a symbol in the processor symbol table, or global if not found
  // returns kNull if no such symbol exists
  // copies the symbol into the local symbol table if it doesn't already exist
  RawValue lookup_symbol(SYMBOL symbol);

  // Attempt to steal some threads from this processor
  // and put them into target_procs run queue
  bool steal_ready_threads(Processor* target_proc);

  void fire_timer_events(Thread* thread);
  size_t timestamp_of_next_timer_event();

private:
  Runtime* m_runtime;
  size_t m_id;
  bool m_live = false;
  atomic<Worker*> m_worker = nullptr;
  std::unique_ptr<ThreadAllocationBuffer> m_tab;

  std::mutex m_run_queue_mutex;
  std::list<Thread*> m_run_queue;

  std::mutex m_timer_events_mutex;
  std::vector<TimerEvent> m_timer_events;

  std::unordered_map<SYMBOL, RawString> m_symbol_table;
};

}  // namespace charly::core::runtime
