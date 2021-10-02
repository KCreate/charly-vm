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

#include <mutex>
#include <condition_variable>
#include <thread>

#include "charly/value.h"
#include "charly/utils/random_device.h"

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
  Worker(Runtime* runtime);

  enum class State {

    // worker does not own a processor
    Created,          // initial state
    Idle,             // worker is currently idling and can be woken
    Acquiring,        // worker is trying to acquire a processor
    Exited,           // worker has exited

    Scheduling,       // worker is currently in the scheduler
    Running,          // worker is currently in a fiber thread
    Native,           // worker is executing a native section in a fiber thread (code that cannot interact with heap)
    WorldStopped      // worker stopped due to scheduler stop the world request
  };

  static bool is_heap_safe_mode(State state);

  // getter / setter
  uint64_t id() const;
  State state() const;
  uint64_t context_switch_counter() const;
  uint64_t rand();
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
  Processor* processor() const;
  Runtime* runtime() const;

  // called by Scheduler::acquire_processor_for_worker
  void set_processor(Processor* processor);

  // wait for the worker to exit
  void join();

  // wake the worker from idle mode
  bool wake();

  // stop / start the world
  void stop_the_world();
  void start_the_world();

  // attempt to change the worker state
  bool change_state(State expected_state, State new_state);

  // wait for the worker to change its state
  State wait_for_state_change(State old_state);

private:
  void scheduler_loop(Runtime* runtime);
  void assert_change_state(State expected_state, State new_state);

  // enter idle mode
  void idle();

  // implements a scheduler checkpoint
  void checkpoint();

  // increase the amount of time the worker should sleep in its next idle state
  void increase_sleep_duration();
  void reset_sleep_duration();

private:
  uint64_t m_id;
  atomic<State> m_state;
  uint64_t m_context_switch_counter;
  uint32_t m_idle_sleep_duration;
  utils::RandomDevice m_random_device;
  fcontext_t m_context;
  std::thread m_os_thread_handle;

  atomic<Thread*> m_thread;
  atomic<Processor*> m_processor;
  Runtime* m_runtime;

  std::mutex m_mutex;
  atomic<bool> m_stop_flag;
  atomic<bool> m_idle_flag;
  std::condition_variable m_idle_cv;   // signalled by the scheduler to wake the worker from Idle mode
  std::condition_variable m_stw_cv;    // signalled by the scheduler to wake the worker from WorldStopped mode
  std::condition_variable m_state_cv;  // signalled by the worker when it changes its state
};

}  // namespace charly::core::runtime
