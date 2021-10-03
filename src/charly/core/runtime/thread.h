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

#include <boost/context/detail/fcontext.hpp>

#include "charly/utils/guardedbuffer.h"

#include "charly/value.h"
#include "charly/handle.h"

#pragma once

namespace charly::core::runtime {

class Worker;
class Runtime;
class Interpreter;
class Frame;

// boost context bindings
//
// TODO: replace with custom implementation, as the boost version is only a couple lines
//       of assembly.
using fcontext_t = boost::context::detail::fcontext_t;
using transfer_t = boost::context::detail::transfer_t;

static const size_t kThreadStackSize = 1024 * 1024 * 8;

class Stack {
public:
  Stack() : m_buffer(kThreadStackSize) {}

  void* lo() const {
    return m_buffer.buffer();
  }

  void* hi() const {
    return (void*)((uintptr_t)m_buffer.buffer() + m_buffer.size());
  }

  size_t size() const {
    return m_buffer.size();
  }

  void clear() {
    m_buffer.clear();
  }

private:
  utils::GuardedBuffer m_buffer;
};

class ThreadLocalHandles {
public:
  ThreadLocalHandles() : m_head(nullptr) {}

  Value* push(Value* handle) {
    Value* old_head = m_head;
    m_head = handle;
    return old_head;
  }

  void pop(Value* next) {
    m_head = next;
  }

  Value* head() const {
    return m_head;
  }

private:
  Value* m_head;
};

// threads keep track of the stack memory of fibers and their runtime state
// threads are pre-allocated by the scheduler and kept in a freelist
//
// the threads of fibers that have exited are reused by future fibers
class Thread {
  friend class Interpreter;
public:
  Thread(Runtime* runtime);

  static Thread* current();
  static void set_current(Thread* worker);

  enum class State {
    Free,           // thread sits on a freelist somewhere and isn't tied to a fiber yet
    Waiting,        // thread is paused
    Ready,          // thread is ready to be executed and is currently placed in some run queue
    Running,        // thread is currently running
    Native,         // thread is currently executing a native section
    Exited,         // thread has exited
    Aborted         // thread has aborted, runtime should terminate all other threads too
  };

  // getters / setters
  uint64_t id() const;
  State state() const;
  int32_t exit_code() const;
  RawValue fiber() const;
  Worker* worker() const;
  Runtime* runtime() const;
  uint64_t last_scheduled_at() const;
  void extend_timeslice(uint64_t ms);
  bool has_exceeded_timeslice() const;
  const Stack& stack() const;
  ThreadLocalHandles* handles();
  Frame* frame() const;
  bool has_pending_exception() const;
  RawValue pending_exception() const;

  // initialize this thread as the main thread
  void init_main_thread();

  // initialize this thread with a fiber
  void init_fiber_thread(RawFiber fiber);

  // unbind this thread from its fiber after it has exited
  // clears out the stack and prepares the thread for insertion
  // into the runtime freelist
  void clean();

  // scheduler checkpoint which gets routinely called by each thread
  // this gives the scheduler an opportunity to schedule another thread
  void checkpoint();

  // exit from this thread and instruct the scheduler to give the
  // exit signal to all other threads
  [[noreturn]] void abort(int32_t exit_code);

  // change thread state from waiting to ready
  // fails if competing with another thread
  void ready();

  // resume execution of a thread by performing a context switch
  // thread is expected to already be in the Ready state
  // meant to be called by the Scheduler
  void context_switch(Worker* worker);

  // enter / exit a native section
  // void enter_native();
  // void exit_native();

  // exception handling
  void throw_value(RawValue value);
  void reset_pending_exception();

  // frame handling
  void push_frame(Frame* frame);
  void pop_frame(Frame* frame);

private:
  void entry_main_thread();
  void entry_fiber_thread();

  // yield control back to the scheduler and update thread state
  void enter_scheduler(State state);

private:
  uint64_t m_id;
  bool m_is_main_thread;
  atomic<State> m_state;
  Stack m_stack;
  Runtime* m_runtime;

  int32_t m_exit_code;
  RawValue m_fiber;
  Worker* m_worker;
  uint64_t m_last_scheduled_at;
  fcontext_t m_context;

  ThreadLocalHandles m_handles;
  Frame* m_frame;
  RawValue m_pending_exception;
};

}  // namespace charly::core::runtime
