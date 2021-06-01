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

#include <shared_mutex>
#include <condition_variable>
#include <thread>
#include <list>
#include <vector>
#include <stack>

#include "charly/charly.h"
#include "charly/atomic.h"

#include "charly/utils/shared_queue.h"

#include <boost/context/detail/fcontext.hpp>

#pragma once

namespace charly::core::runtime {

// boost context bindings
//
// TODO: replace with custom implementation, as the boost version is only a couple lines
//       of assembly.
using fcontext_t = boost::context::detail::fcontext_t;
using transfer_t = boost::context::detail::transfer_t;
using boost::context::detail::make_fcontext;
using boost::context::detail::jump_fcontext;

// the amount of concurrent threads the underlying machine can run
// static const uint64_t kHardwareConcurrency = std::thread::hardware_concurrency();
static const uint64_t kHardwareConcurrency = 1;

// chance that a processor will acquire from the global queue instead of the local queue
// if the global queue is empty during such a random check, the scheduler will instead
// steal work from some other processor, in order to equalize fiber pressure across all
// processors
static const uint32_t kGlobalReadyQueuePriorityChance = 16;

// preempt a running fiber at a scheduler checkpoint
// if its running time exceeds this amount of milliseconds
static const uint64_t kSchedulerFiberTimeslice = 10;

// the maximum amount of fibers to be queued in each workers local queue
static const uint64_t kLocalReadyQueueMaxSize = 256;

struct Processor;
struct Worker;
struct Fiber;
struct HeapRegion;

// a processor keeps track of the per-processor data the charly runtime manages.
//
// if a worker thread wishes to execute charly code, it first has to acquire
// a processor in the system and use it as the source of its work.
//
// by assigning each worker thread a different processor, each worker can
// safely work on its own set of data while not touching any other worker's processor
struct Processor {
  Processor();

  enum class State : uint8_t {
    Idle,             // processor is idle and not owned by any worker
    Owned             // processor is owned by a worker
  };

  uint64_t id;
  charly::atomic<State> state = State::Idle;
  charly::atomic<Worker*> owner = nullptr;

  // allocated GC heap region for this processor
  HeapRegion* active_region = nullptr;

  // queue of fibers that are ready to be executed
  std::mutex run_queue_mutex;
  std::list<Fiber*> run_queue;
};

// represents an actual hardware thread and its runtime data
// a worker owns a processor and uses it to execute code in the charly runtime
//
// workers can enter into native sections, during which they are not allowed to
// interact with the charly heap in any way. once the worker exits native mode
// it has to sync with the GC and pause itself if the GC is currently running.
//
// workers inside native sections that have exceeded their current timeslice
// will get preempted by the system monitor thread. the system monitor thread
// tries to detach the workers acquired processor and then attempts to give
// it to some other worker thread, or if none are idle, creates a new one.
//
// the goal is to always have a constant amount of worker threads executing charly
// code concurrently.
struct Worker {
  Worker();

  enum class State : uint8_t {
    Created = 0,      // initial state
    Idle,             // worker is currently idling and can be woken
    Acquiring,        // worker is trying to acquire a processor
    Scheduling,       // worker owns a processor and currently executes scheduler code
    Running,          // worker owns a processor and currently executes fiber code
    Native,           // worker is inside a native section
    NativePreempted,  // worker got preempted during native section, does not own a processor anymore
    WorldStopped,     // worker stopped due to scheduler stop the world request
    Exited            // worker has exited
  };

  uint64_t id;
  uint64_t context_switch_counter = 1;
  charly::atomic<uint32_t> random_source = 0;
  fcontext_t context;
  std::thread os_handle;

  charly::atomic<bool> should_exit; // set by the scheduler if it wants this worker to exit

  charly::atomic<State> state = State::Created;
  charly::atomic<Fiber*> fiber = nullptr;
  charly::atomic<Processor*> processor = nullptr;

  std::mutex mutex;
  std::condition_variable wake_cv;   // signalled by the scheduler to wake the worker from idle mode
  std::condition_variable state_cv;  // signalled by the worker when it changes its state

  // join the native os thread
  void join();

  // attempt to change the worker state from an old to a new state
  // returns true if the state could be successfully changed, false otherwise
  bool change_state(State oldstate, State newstate);

  // attempts to change the worker state from and old to a new state
  // contains an assert that aborts the program if the state change fails
  void assert_change_state(State oldstate, State newstate) {
    bool changed = change_state(oldstate, newstate);
    (void)changed;
    assert(changed);
  }

  // generate a random number
  uint32_t rand();
};

static const size_t kFiberStackSize = 1024 * 4; // x kilobytes

// Fibers hold the state information of a single strand of execution inside a charly
// program. each fiber has its own hardware stack and stores register data when paused
struct Fiber {
  static Fiber* allocate();
  static void clean(Fiber* fiber);
  static void deallocate(Fiber* fiber);

  enum class State : uint8_t {
    Created,    // fiber has been created, but never executed
    Paused,     // fiber is paused and is waiting to be resumed
    Ready,      // fiber is ready for execution and is placed in some run queue
    Running,    // fiber is being executed
    Native,     // fiber is being executed, currently in native section
    Exited      // fiber has exited
  };

  // fiber hardware stack
  struct Stack {
    void* hi;
    void* lo;
    size_t size;
  };

  uint64_t id;
  Stack stack;
  charly::atomic<State> state;
  charly::atomic<Worker*> worker;
  charly::atomic<uint64_t> last_scheduled_at;
  fcontext_t context;
};

// thread local pointer to the current worker thread
inline thread_local Worker* g_worker = nullptr;

// charly scheduler
class Scheduler {
  friend struct Worker;
  friend struct Processor;
public:
  static void initialize() {
    static Scheduler scheduler;
    Scheduler::instance = &scheduler;
    scheduler.init_scheduler();
  }
  void init_scheduler();
  inline static Scheduler* instance = nullptr;

  // main function executed by application worker threads
  void worker_main_fn(Worker* worker);

  // main function executed by fibers
  static void fiber_main_fn(transfer_t transfer);

  // suspends calling thread until the scheduler has exited
  //
  // this method only returns if the scheduler exited gracefully, if the
  // machine crashed or if the user called the abort method, the program
  // will just terminate then and there, without returning from this method
  //
  // meant to be called from the main program thread setting up the runtime
  void join();

  // returns current worker / processor / fiber or nullptr
  Worker* worker();
  Processor* processor();
  Fiber* fiber();

  // scheduler checkpoint
  //
  // provides a place for the scheduler to cooperatively preempt the
  // currently running fiber and switch it out for some other fiber.
  //
  // pauses if the GC has requested the scheduler to stop the world
  //
  // meant to be called from within application worker threads
  void checkpoint();

  // schedule a ready fiber into the runtime
  //
  // the fiber already has to be in the ready state before calling this method
  // if called from a worker thread, the fiber will be scheduled into the workers
  // local run queue. if called from somewhere else, the fiber gets scheduled into the
  // schedulers global run queue
  void schedule_fiber(Fiber* fiber);

  // stop / start the world
  // when the world is stopped, the GC can safely traverse and scan the
  // heap without any interference from the application threads
  //
  // meant to be called from the GC worker thread
  void stop_the_world();
  void start_the_world();

  // transition the current worker and associated processor into native mode
  //
  // meant to be called from within application worker threads
  void enter_native_mode();

  // attempt to exit from native mode
  //
  // in the case where the system monitor thread preempted the calling
  // worker before it could exit, this method returns false
  //
  // calling worker should yield itself back to the scheduler if this call fails
  //
  // meant to be called from within application worker threads
  bool exit_native_mode();

  // wake an idle worker
  void wake_idle_worker();

  // reschedule the currently executing fiber
  //
  // meant to be called from within application worker threads
  void reschedule_fiber();

  // set by the scheduler if all worker threads should immediately exit
  void wants_to_exit();

private:

  // attempt to acquire and bind an idle processor to a worker
  // returns false if no idle processor was found
  bool acquire_processor_for_worker(Worker* worker);

  // releases the processor associated with a worker back to the scheduler
  void release_processor_from_worker(Worker* worker);

  // put the calling worker thread into idle mode
  void idle_worker(Worker* worker);

  // wake up a worker from idle mode
  // returns false if another thread woke the worker before us
  void wake_worker(Worker* worker);

  // returns the current unix timestamp in millisecond precision
  uint64_t get_steady_timestamp();

  // acquire the next ready fiber to execute
  //
  // returns a nullptr if no available task could be found
  Fiber* get_ready_fiber();

  // acquire a ready fiber from the global run queue
  //
  // returns a nullptr if no available task could be found
  Fiber* get_ready_fiber_from_global_queue();

  // attempt to steal a ready fiber from some other processor
  //
  // returns a nullptr if no available task could be found
  Fiber* steal_ready_fiber();

  // suspend currently executing fiber and jump back into the scheduler
  void enter_scheduler();

  // suspend the scheduler and jump into a fiber
  void enter_fiber(Fiber* fiber);

private:
  uint64_t m_start_timestamp = 0;

  bool m_wants_exit = false;
  bool m_wants_join = false;

  std::mutex m_workers_mutex;
  std::condition_variable m_idle_workers_cv;
  std::vector<Worker*> m_workers;
  std::stack<Worker*> m_idle_workers;

  std::mutex m_processors_mutex;
  std::vector<Processor*> m_processors;
  std::stack<Processor*> m_idle_processors;

  std::mutex m_run_queue_mutex;
  std::list<Fiber*> m_run_queue;
};

}  // namespace charly::core::runtime
