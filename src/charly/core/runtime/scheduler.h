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
#include <unordered_set>

#include "charly/charly.h"
#include "charly/atomic.h"
#include "charly/value.h"

#include "charly/utils/buffer.h"

#include "charly/core/runtime/compiled_module.h"

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
static const uint64_t kHardwareConcurrency = std::thread::hardware_concurrency();
// static const uint64_t kHardwareConcurrency = 2;

// chance that a processor will acquire from the global queue instead of the local queue
static const uint32_t kGlobalRunQueuePriorityChance = 32;

// chance that a processor will attempt to steal from some other worker instead of acquiring
// ready fibers from its local queue
static const uint32_t kWorkerStealPriorityChance = 64;

// timeslice (in milliseconds) given to each fiber to run
static const uint64_t kSchedulerFiberTimeslice = 10;

// the maximum amount of fibers to be queued in each workers local queue
static const uint64_t kLocalReadyQueueMaxSize = 256;

// the maximum amount of milliseconds a worker should spend in its idle phase before checking
// if there are any work available
static const uint32_t kWorkerMaximumIdleSleepDuration = 1000;

struct Processor;
struct Worker;
struct Fiber;
struct Function;
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
    Exited,           // worker has exited
    __Count           // amount of worker states defined
  };

  uint64_t id;
  uint64_t context_switch_counter;
  uint32_t random_source;
  uint32_t idle_sleep_duration;
  fcontext_t context;
  std::thread os_handle;

  charly::atomic<bool> stw_request; // set by the scheduler during a stop-the-world pause

  charly::atomic<State> state;
  charly::atomic<Fiber*> fiber;
  charly::atomic<Processor*> processor;

  std::mutex mutex;
  std::condition_variable wake_cv;   // signalled by the scheduler to wake the worker from idle mode
  std::condition_variable stw_cv;    // signalled by the scheduler to wake the worker from STW mode
  std::condition_variable state_cv;  // signalled by the worker when it changes its state

  // join the native os thread
  void join();

  // wake the worker from idle mode
  // does nothing if the worker is not in idle mode
  void wake_idle();

  // wake the worker from world stopped mode
  void wake_stw();

  // attempt to change the worker state from an old to a new state
  // returns true if the state could be successfully changed, false otherwise
  bool change_state(State oldstate, State newstate);

  // waits for the workers state to change away from some old state
  State wait_for_state_change(State oldstate);

  // attempts to change the worker state from and old to a new state
  // contains an assert that aborts the program if the state change fails
  void assert_change_state(State oldstate, State newstate) {
    bool changed = change_state(oldstate, newstate);
    (void)changed;
    assert(changed);
  }

  // generate a random number
  uint32_t rand();

  // increase the amount of time the worker should sleep in its next idle state
  // will not increase beyond kWorkerMaximumIdleSleepDuration
  void increase_sleep_duration();
  void reset_sleep_duration();
};

static const size_t kFiberStackSize = 1024 * 8; // 8 kilobytes

// Fibers hold the state information of a single strand of execution inside a charly
// program. each fiber has its own hardware stack and stores register data when paused
struct Fiber {
  Fiber(Function* main_function);
  ~Fiber();

  static HeapType heap_value_type() {
    return HeapType::Fiber;
  }

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
  bool exit_machine_on_exit; // set on the main fiber
  Function* main_function;
  fcontext_t context;

  // deallocate stack
  void clean();
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
  void worker_main(Worker* worker);

  // main function executed by fibers
  void fiber_main(transfer_t transfer);

  // suspends calling thread until the scheduler has exited
  //
  // this method only returns if the scheduler exited gracefully, if the
  // machine crashed or if the user called the abort method, the program
  // will just terminate then and there, without returning from this method
  //
  // meant to be called from the main program thread setting up the runtime
  //
  // returns the exit code returned by the program
  int32_t join();

  // initiate exit from the runtime
  //
  // the first worker thread to call this method has the ability to set the
  // exit code. calls after the first one do not change the exit code
  //
  // runtime will immediately abort all executing worker threads, without
  // giving them any notice or oppurtunity to finish what they are doing
  void abort(int32_t exit_code);

  // returns current worker / processor / fiber or nullptr
  Worker* worker();
  Processor* processor();
  Fiber* fiber();

  // schedule a ready fiber into the runtime
  //
  // the fiber already has to be in the ready state before calling this method
  // if called from a worker thread, the fiber will be scheduled into the workers
  // local run queue. if called from somewhere else, the fiber gets scheduled into the
  // schedulers global run queue
  void schedule_fiber(Fiber* fiber);

  // provides a place for the scheduler to switch to another fiber,
  // to stop the world in case the GC is requesting a pause or to
  // exit from the machine entirely
  //
  // meant to be called from within application worker threads
  void worker_checkpoint();

  // provides a place for the scheduler to switch to another fiber,
  // to stop the world in case the GC is requesting a pause or to
  // exit from the machine entirely
  //
  // meant to be called from workers in acquiring mode
  void scheduler_checkpoint();

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
  // void enter_native_mode();

  // attempt to exit from native mode
  //
  // in the case where the system monitor thread preempted the calling
  // worker before it could exit, this method returns false
  //
  // calling worker should yield itself back to the scheduler if this call fails
  //
  // meant to be called from within application worker threads
  // bool exit_native_mode();

  // reschedule the currently executing fiber
  //
  // meant to be called from within application worker threads
  void reschedule_fiber();

  // exit from the current fiber
  //
  // meant to be called from within application worker threads
  void exit_fiber();

  // register a compiled module with the runtime
  Function* register_module(ref<CompiledModule> module);

private:

  // stops the calling worker thread until the stop-the-world pause is over
  void stw_checkpoint();

  // attempt to acquire and bind an idle processor to a worker
  // returns false if no idle processor was found
  bool acquire_processor_for_worker(Worker* worker);

  // releases the processor associated with a worker back to the scheduler
  void release_processor_from_worker(Worker* worker);

  // put the calling worker thread into idle mode
  void idle_worker();

  // wake an idle worker
  void wake_idle_worker();

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

  std::mutex m_exit_mutex;
  std::condition_variable m_exit_cv;
  charly::atomic<int> m_exit_code = 0;
  charly::atomic<bool> m_wants_exit = false;

  std::mutex m_init_mutex; // held by scheduler during initialization to prevent workers from starting early

  std::mutex m_workers_mutex;
  std::vector<Worker*> m_workers;
  std::unordered_set<Worker*> m_idle_workers;

  std::mutex m_processors_mutex;
  std::vector<Processor*> m_processors;
  std::stack<Processor*> m_idle_processors;

  std::mutex m_run_queue_mutex;
  std::list<Fiber*> m_run_queue;

  std::mutex m_registered_modules_mutex;
  std::vector<ref<CompiledModule>> m_registered_modules;
};

}  // namespace charly::core::runtime
