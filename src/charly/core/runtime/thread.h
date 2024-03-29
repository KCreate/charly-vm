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

#include <vector>

#include <boost/context/detail/fcontext.hpp>

#include "charly/utils/guarded_buffer.h"

#include "charly/handle.h"
#include "charly/value.h"

#pragma once

namespace charly::core::runtime {

class Worker;
class Processor;
class Runtime;
class Interpreter;
struct Frame;
class GarbageCollector;

// boost context bindings
//
// TODO: replace with custom implementation, as the boost version is only a couple lines
//       of assembly.
using fcontext_t = boost::context::detail::fcontext_t;
using transfer_t = boost::context::detail::transfer_t;

constexpr size_t kThreadStackSize = 1024 * 512; // 512kb

class Stack {
public:
  Stack() : m_buffer(kThreadStackSize) {}

  void* lo() const {
    return m_buffer.data();
  }

  void* hi() const {
    return (void*)((uintptr_t)m_buffer.data() + m_buffer.size());
  }

  size_t size() const {
    return m_buffer.size();
  }

  void clear() {
    if constexpr (kIsDebugBuild) {
      std::memset(m_buffer.data(), 0, m_buffer.size());
    }
  }

  bool pointer_points_into_stack(const void* pointer) const {
    return pointer >= lo() && pointer < hi();
  }

private:
  utils::GuardedBuffer m_buffer;
};

class ThreadLocalHandles {
public:
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
  Value* m_head = nullptr;
};

// threads keep track of the stack memory of fibers and their runtime state
// threads are pre-allocated by the scheduler and kept in a freelist
//
// the threads of fibers that have exited are reused by future fibers
class Thread {
  friend class Runtime;
  friend class Worker;
  friend class Interpreter;
  friend class GarbageCollector;

public:
  using SchedulerPostCtxSwitchCallback = std::function<void(Thread*, Processor*)>;

  explicit Thread(Runtime* runtime);

  static Thread* current();
  static void set_current(Thread* worker);

  static constexpr size_t kExceptionChainDepthLimit = 20;

  static constexpr size_t kBacktraceDepthLimit = 32;

  enum class State {
    Free,              // thread sits on a freelist somewhere and isn't tied to a fiber yet
    Waiting,           // thread is paused
    Ready,             // thread is ready to be executed and is currently placed in some run queue
    Running,           // thread is currently running
    Native,            // thread is currently executing a native section
    Exited,            // thread has exited
    Aborted            // thread has aborted, runtime should terminate all other threads too
  };

  enum class Type {
    Main,
    Scheduler,
    Fiber
  };

  static constexpr size_t kNeverScheduledTimestamp = 0;
  static constexpr size_t kShouldYieldToSchedulerTimestamp = 1;
  static constexpr size_t kFirstValidScheduledAtTimestamp = 2;

  // getters / setters
  size_t id() const;
  Type type() const;
  bool is_main() const { return type() == Type::Main; }
  bool is_scheduler() const { return type() == Type::Scheduler; }
  bool is_fiber() const { return type() == Type::Fiber; }
  State state() const;
  int32_t exit_code() const;
  RawValue fiber() const;
  SchedulerPostCtxSwitchCallback* wait_callback() const;
  void set_wait_callback(SchedulerPostCtxSwitchCallback* wait_callback);
  Worker* worker() const;
  void set_worker(Worker* worker);
  Runtime* runtime() const;
  size_t last_scheduled_at() const;
  bool set_last_scheduled_at(size_t old_timestamp, size_t timestamp);
  void set_last_scheduled_at(size_t timestamp);
  fcontext_t& context();
  void set_context(fcontext_t& context);
  const Stack* stack() const;
  ThreadLocalHandles* handles();
  Frame* frame() const;
  RawValue pending_exception() const;
  void set_pending_exception(RawValue value);

  // context switch one context to another
  static void context_handler(transfer_t transfer);
  static void context_switch_worker_to_scheduler(Worker*);
  static void context_switch_scheduler_to_worker(Thread*);
  static void context_switch_scheduler_to_thread(Thread*, Thread*);
  static void context_switch_thread_to_scheduler(Thread*, State state, SchedulerPostCtxSwitchCallback* callback = nullptr);

  // initialize this thread as the main thread
  void init_main_thread();

  // initialize this thread with a fiber
  void init_fiber_thread(RawFiber fiber);

  // initialize this thread as a proc scheduler thread
  void init_proc_scheduler_thread();

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

  // pause current fiber and wait for the future to complete
  void await_future(RawFuture future);

  void wake_from_wait();

  // suspend thread and resume execution in the future
  void sleep_until(size_t timestamp);

  // perform callback code in thread native mode
  // if a thread is in native mode, it is not allowed to interact with the charly heap or runtime
  // data structures in any way, which allows the GC to omit waiting for it
  void enter_native();
  void exit_native();
  template <typename F>
  void native_section(F&& callback) {
    enter_native();
    callback();
    exit_native();
  }

  // exception handling
  template <typename... Args>
  RawValue throw_message(const char* format, const Args&... args) {
    throw_exception(RawException::create(this, RawString::format(this, format, std::forward<const Args>(args)...)));
    return kErrorException;
  }
  RawValue throw_exception(RawException exception);
  void rethrow_exception(RawException exception);

  // frame handling
  void push_frame(Frame* frame);
  void pop_frame(Frame* frame);

  // look up symbol via active processor cache
  RawValue lookup_symbol(SYMBOL symbol) const;

  // allocate memory on the managed charly heap
  uintptr_t allocate(size_t size, bool contains_external_heap_pointers = false);

  void dump_exception_trace(RawException exception) const;

  void acas_state(Thread::State old_state, Thread::State new_state);

  // creates a tuple containing a stack trace of the current thread
  RawTuple create_backtrace();

private:
  void entry_main_thread();
  void entry_fiber_thread();
  void entry_proc_scheduler_thread();

  // acquire a stack from the scheduler
  void acquire_stack();

private:
  size_t m_id;
  Type m_type;
  atomic<State> m_state = State::Free;
  Stack* m_stack = nullptr;
  Runtime* m_runtime;

  int32_t m_exit_code = 0;
  RawValue m_fiber;
  SchedulerPostCtxSwitchCallback* m_wait_callback = nullptr;
  Worker* m_worker = nullptr;
  atomic<size_t> m_last_scheduled_at = kNeverScheduledTimestamp;
  fcontext_t m_context = nullptr;

  ThreadLocalHandles m_handles;
  Frame* m_frame = nullptr;
  RawValue m_pending_exception;
};

}  // namespace charly::core::runtime
