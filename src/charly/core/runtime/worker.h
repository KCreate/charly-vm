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

#include <boost/context/detail/fcontext.hpp>

#include <condition_variable>
#include <mutex>
#include <thread>

#include "charly/utils/random_device.h"
#include "charly/value.h"

#pragma once

namespace charly::core::runtime {

class Thread;
class Processor;
class Runtime;

// boost context bindings
//
// TODO: replace with custom implementation, as the boost version is only a couple lines
//       of assembly.
using fcontext_t = boost::context::detail::fcontext_t;

// represents an actual OS thread and its runtime data
// a worker owns a processor and uses it to execute code in the charly runtime
//
// workers can enter into native sections, during which they are not allowed to
// interact with the charly heap in any way. once the worker exits native mode
// it has to sync with the GC and pause itself if the GC is currently running.
class Worker {
public:
  explicit Worker(Runtime* runtime);

  enum class State {
    // worker does not own a processor
    Created,        // initial state
    Idle,           // worker is currently idling and can be woken
    AcquiringProc,  // worker is trying to acquire a processor
    Exited,         // worker has exited

    // worker owns a processor
    Scheduling,   // worker is currently in the scheduler
    Running,      // worker is currently in a fiber thread
    Native,       // worker is executing a native section in a fiber thread (code that cannot interact with heap)
    WorldStopped  // worker stopped due to scheduler stop the world request
  };

  static bool is_heap_safe_mode(State state);

  // getter / setter
  size_t id() const;
  State state() const;
  size_t context_switch_counter() const;
  void increase_context_switch_counter();
  size_t rand();
  fcontext_t& context();
  void set_context(fcontext_t& context);

  bool has_stop_flag() const;
  bool has_idle_flag() const;

  // return true if this is the first thread to do this change (cas succeeded)
  bool set_stop_flag();
  bool set_idle_flag();
  bool clear_stop_flag();
  bool clear_idle_flag();

  Thread* thread() const;
  Thread* scheduler_thread() const;
  Processor* processor() const;
  Runtime* runtime() const;

  void set_thread(Thread* thread);
  void set_scheduler_thread(Thread* thread);
  void set_processor(Processor* processor);

  // wait for the worker to exit
  void join();

  // wake the worker from idle mode
  bool wake();

  // stop / start the world
  void stop_the_world();
  void start_the_world();

  // enter / exit native mode
  void enter_native();
  void exit_native();

  // attempt to change the worker state
  bool change_state(State expected_state, State new_state);

  // wait for the worker to change its state
  State wait_for_state_change(State old_state);

  void acas_state(State expected_state, State new_state);

  void checkpoint();

private:
  void scheduler_loop(Runtime* runtime);

  // enter idle mode
  void idle();

private:
  size_t m_id;
  atomic<State> m_state = State::Created;
  size_t m_context_switch_counter = 0;
  utils::RandomDevice m_random_device;
  fcontext_t m_context = nullptr;
  std::thread m_os_thread_handle;

  atomic<Thread*> m_thread = nullptr;
  atomic<Thread*> m_scheduler_thread = nullptr;
  atomic<Processor*> m_processor = nullptr;
  Runtime* m_runtime = nullptr;

  std::mutex m_mutex;
  atomic<bool> m_stop_flag = false;
  atomic<bool> m_idle_flag = false;
  std::condition_variable m_idle_cv;   // signalled by the scheduler to wake the worker from Idle mode
  std::condition_variable m_stw_cv;    // signalled by the scheduler to wake the worker from WorldStopped mode
  std::condition_variable m_state_cv;  // signalled by the worker when it changes its state
};

}  // namespace charly::core::runtime
