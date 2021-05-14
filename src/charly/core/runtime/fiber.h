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

#include "charly/charly.h"
#include "charly/atomic.h"
#include "charly/value.h"

#include <boost/context/detail/fcontext.hpp>

#pragma once

namespace charly::core::runtime {

static const size_t kFiberStackSize = 4096;

class FiberStack {
public:
  FiberStack();
  ~FiberStack();

  void* bottom() const;
  void* top() const;
  size_t size() const;

private:
  void* m_bottom;
  void* m_top;
  size_t m_size;
};

class Worker;

class Fiber {
  friend class Worker;
  friend class Scheduler;
public:
  using FiberTaskFn = void(*)();

  // status of a fiber
  enum class Status : uint8_t {
    Ready = 0,    // fiber is ready to be executed
    Running,      // fiber is currently executing
    Paused,       // fiber is paused
    Exited        // fiber has exited and can be deallocated
  };

  // allocate new fiber, don't start executing it
  template <typename F>
  Fiber(F fn) {
    static size_t fiber_id_counter = 1;

    safeprint("initializing fiber %", this);
    m_id = fiber_id_counter++;
    m_status.store(Status::Paused);
    m_task_function = fn;
    m_context = make_fcontext(m_stack.top(), m_stack.size(), &Fiber::fiber_handler_function);
  }

  // run the task function passed in the constructor
  void run_task_fn();

private:

  // fcontext_t handler function
  static void fiber_handler_function(boost::context::detail::transfer_t transfer);

  // jump into this fibers context
  void* jump_context(void* argument = nullptr);

public:
  uint64_t m_id = 0;
  charly::atomic<Status> m_status;
  void* m_argument;

  // millisecond timestamp when this fiber was started
  // used by the scheduler to preempt fibers that have been running
  // for more than 10ms
  uint64_t m_scheduled_at = 0;

private:
  FiberTaskFn m_task_function;
  boost::context::detail::fcontext_t m_context;
  FiberStack m_stack;
};

}  // namespace charly::core::runtime
