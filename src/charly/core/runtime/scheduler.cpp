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

#include "charly/core/runtime/scheduler.h"
#include "charly/core/runtime/allocator.h"
#include "charly/core/runtime/gc.h"

namespace charly::core::runtime {

using namespace std::chrono_literals;

Processor::Processor() {
  static charly::atomic<uint64_t> processor_id_counter = 0;
  this->id = processor_id_counter++;
}

Worker::Worker() : os_handle(std::thread(&Scheduler::worker_main_fn, Scheduler::instance, this)) {
  static charly::atomic<uint64_t> worker_id_counter = 0;
  this->id = worker_id_counter++;
  this->random_source.store(this->id);
}

void Worker::join() {
  if (this->os_handle.joinable()) {
    this->os_handle.join();
  }
}

bool Worker::change_state(State oldstate, State newstate) {
  {
    std::unique_lock<std::mutex> locker(this->mutex);

    if (this->state != oldstate) {
      return false;
    }

    this->state.store(newstate);
  }

  this->state_cv.notify_all();

  return true;
}

Worker::State Worker::wait_for_state_change(State oldstate) {
  Worker::State now_state = this->state;

  // wait for state to change
  if (now_state == oldstate) {
    std::unique_lock<std::mutex> locker(this->mutex);
    this->state_cv.wait(locker, [&] {
      now_state = this->state;
      return now_state != oldstate;
    });
  }

  return now_state;
}

uint32_t Worker::rand() {
  // source: https://stackoverflow.com/a/3747462
  for (;;) {
    uint32_t seed = this->random_source;
    uint32_t t = (214013 * seed + 2531011);
    t = (t >> 16) & 0x7FFF;

    if (this->random_source.cas(seed, t)) {
      return t;
    }
  }
}

Fiber* Fiber::allocate() {
  Fiber* fiber = (Fiber*)MemoryAllocator::instance->allocate(HeapType::Fiber, sizeof(Fiber));

  // allocate and initialize fiber stack
  void* stack_lo = std::malloc(kFiberStackSize);
  fiber->stack.lo = stack_lo;
  fiber->stack.hi = (void*)((uintptr_t)stack_lo + kFiberStackSize);
  fiber->stack.size = kFiberStackSize;

  static charly::atomic<uint64_t> fiber_id_counter = 0;
  fiber->id = fiber_id_counter++;
  fiber->state.store(Fiber::State::Created);
  fiber->worker.store(nullptr);
  fiber->last_scheduled_at.store(0);
  fiber->context = make_fcontext(fiber->stack.hi, kFiberStackSize, &Scheduler::fiber_main_fn);

  return fiber;
}

void Fiber::clean(Fiber* fiber) {
  if (fiber->stack.lo) {
    std::free(fiber->stack.lo);
    fiber->stack.lo = nullptr;
    fiber->stack.hi = nullptr;
    fiber->stack.size = 0;
  }
}

void Fiber::deallocate(Fiber* fiber) {
  Fiber::clean(fiber);
}

void Scheduler::init_scheduler() {
  MemoryAllocator::initialize();
  GarbageCollector::initialize();

  m_start_timestamp = get_steady_timestamp();

  // initialize processors and workers
  {
    std::lock_guard<std::mutex> workers_lock(m_workers_mutex);
    std::lock_guard<std::mutex> proc_lock(m_processors_mutex);
    for (size_t i = 0; i < kHardwareConcurrency; i++) {
      Processor* processor = new Processor();
      m_processors.push_back(processor);
      m_idle_processors.push(processor);

      Worker* worker = new Worker();
      m_workers.push_back(worker);
    }
  }

  safeprint("finished initializing the scheduler");
}

void Scheduler::worker_main_fn(Worker* worker) {
  safeprint("initializing worker %", worker->id);
  g_worker = worker;

  worker->assert_change_state(Worker::State::Created, Worker::State::Acquiring);

  while (!worker->should_exit) {

    // attempt to acquire idle processor
    if (!acquire_processor_for_worker(worker)) {
      idle_worker();
      continue;
    }

    assert(worker->processor);

    worker->assert_change_state(Worker::State::Acquiring, Worker::State::Scheduling);

    // execute tasks using on the acquired processor until there are no
    // more tasks left to execute
    for (;;) {
      Fiber* fiber = get_ready_fiber();

      if (fiber == nullptr) {
        break;
      }

      safeprint("worker % acquired fiber %", worker->id, fiber->id);

      // change into running mode
      worker->change_state(Worker::State::Scheduling, Worker::State::Running);
      fiber->state.acas(Fiber::State::Ready, Fiber::State::Running);

      // context-switch
      worker->context_switch_counter++;
      fiber->last_scheduled_at.store(get_steady_timestamp());
      worker->fiber.acas(nullptr, fiber);
      fiber->worker.acas(nullptr, worker);
      enter_fiber(fiber);
      worker->fiber.acas(fiber, nullptr);
      fiber->worker.acas(worker, nullptr);

      // change into scheduling mode
      worker->change_state(Worker::State::Running, Worker::State::Scheduling);

      switch (fiber->state) {
        case Fiber::State::Paused: {
          break;
        }
        case Fiber::State::Ready: {
          schedule_fiber(fiber);
          break;
        }
        case Fiber::State::Exited: {
          Fiber::clean(fiber);
          break;
        }
        default: {
          assert(false && "unexpected fiber state");
          break;
        }
      }
    }

    // release processor and idle
    release_processor_from_worker(worker);
    worker->assert_change_state(Worker::State::Scheduling, Worker::State::Acquiring);
    idle_worker();
  }

  safeprint("exiting worker %", worker->id);
  worker->assert_change_state(Worker::State::Acquiring, Worker::State::Exited);
}

void fiber_task_fn() {
  safeprint("inside fiber % task fn", Scheduler::instance->fiber()->id);

  // for (int i = 0; i < 1e4; i++) {
  //   Worker* worker = Scheduler::instance->worker();
  //   Processor* proc = Scheduler::instance->processor();
  //   Fiber* fiber = Scheduler::instance->fiber();
  //   safeprint("fiber % on worker % on proc % counter = %", fiber->id, worker->id, proc->id, i);
  //   Scheduler::instance->checkpoint();
  // }

  Scheduler::instance->checkpoint();

  std::this_thread::sleep_for(10ms);

  // spawn new fibers
  Fiber* fiber1 = Fiber::allocate();
  Fiber* fiber2 = Fiber::allocate();
  fiber1->state.acas(Fiber::State::Created, Fiber::State::Ready);
  fiber2->state.acas(Fiber::State::Created, Fiber::State::Ready);
  Scheduler::instance->schedule_fiber(fiber1);
  Scheduler::instance->schedule_fiber(fiber2);

  safeprint("leaving fiber % task fn", Scheduler::instance->fiber()->id);
}

void Scheduler::fiber_main_fn(transfer_t transfer) {
  Fiber* fiber = Scheduler::instance->fiber();
  Worker* worker = Scheduler::instance->worker();
  worker->context = transfer.fctx;

  safeprint("fiber % started on worker %", fiber->id, worker->id);

  // start executing fiber function
  fiber_task_fn();

  // exit fiber
  fiber->state.acas(Fiber::State::Running, Fiber::State::Exited);
  Scheduler::instance->enter_scheduler();
}

void Scheduler::join() {
  m_wants_join = true;

  // wait for all worker threads to exit
  bool all_workers_exited = false;
  while (!all_workers_exited) {
    std::unique_lock<std::mutex> locker(m_workers_mutex);
    all_workers_exited = true;
    for (Worker* worker : m_workers) {
      if (worker->state != Worker::State::Exited) {
        locker.unlock();
        safeprint("waiting for worker % to exit", worker->id);
        worker->join();
        safeprint("finished waiting for worker % to exit", worker->id);
        all_workers_exited = false;
        break; // repeat while loop
      }
      safeprint("worker % already exited", worker->id);
    }
  }

  // wait for the GC worker to exit
  GarbageCollector::instance->shutdown();
}

Worker* Scheduler::worker() {
  return g_worker;
}

Processor* Scheduler::processor() {
  if (Worker* worker = this->worker()) {
    return worker->processor;
  }
  return nullptr;
}

Fiber* Scheduler::fiber() {
  if (Worker* worker = this->worker()) {
    return worker->fiber;
  }
  return nullptr;
}

void Scheduler::schedule_fiber(Fiber* fiber) {
  assert(fiber->state == Fiber::State::Ready);

  // schedule fiber in current workers processor
  if (Processor* proc = this->processor()) {
    std::unique_lock<std::mutex> locker(proc->run_queue_mutex);
    if (proc->run_queue.size() < kLocalReadyQueueMaxSize) {
      proc->run_queue.push_back(fiber);
      safeprint("scheduled fiber % on processor %", fiber->id, proc->id);

      // TODO: how to detect overstressed workers and start other workers accordingly?

      return;
    }
  }

  // schedule in global queue
  {
    std::unique_lock<std::mutex> locker(m_run_queue_mutex);
    m_run_queue.push_back(fiber);
    safeprint("scheduled fiber % in global run queue", fiber->id);
  }

  wake_idle_worker();
}

void Scheduler::checkpoint() {
  Worker* worker = this->worker();

  // stop the world check
  if (worker->should_stop) {
    worker->change_state(Worker::State::Running, Worker::State::WorldStopped);
    {
      std::unique_lock<std::mutex> locker(worker->mutex);
      worker->wake_cv.wait(locker, [&] {
        return worker->should_stop == false;
      });
    }
    worker->change_state(Worker::State::WorldStopped, Worker::State::Running);
  }

  // check if we're inside a fiber or the scheduler
  if (Fiber* fiber = worker->fiber) {

    // check if the executing fiber exceeded its timeslice
    uint64_t timestamp_now = get_steady_timestamp();
    if (timestamp_now - fiber->last_scheduled_at >= kSchedulerFiberTimeslice) {
      safeprint("fiber % exceeded its timeslice", fiber->id);
      reschedule_fiber();
    }
  }
}

void Scheduler::stop_the_world() {
  safeprint("scheduler is stopping the world");

  // wait for the worker threads to pause
  std::unique_lock<std::mutex> workers_lock(m_workers_mutex);

  // set all the should_stop flags
  for (Worker* worker : m_workers) {
    worker->should_stop.acas(false, true);
  }

  // wait for workers to enter into a heap-safe mode
  for (Worker* worker : m_workers) {

    // wait for the worker to enter into a heap safe mode
    for (;;) {
      Worker::State now_state = worker->state;
      switch (now_state) {
        case Worker::State::WorldStopped:
        case Worker::State::Native:
        case Worker::State::Idle: {
          break;
        }
        default: {
          safeprint("waiting for worker %", worker->id);
          workers_lock.unlock();
          now_state = worker->wait_for_state_change(now_state);
          workers_lock.lock();
          continue;
        }
      }
      break;
    }

    safeprint("worker % stopped the world", worker->id);
  }

  safeprint("scheduler has stopped the world");
}

void Scheduler::start_the_world() {
  safeprint("scheduler is starting the world");

  std::unique_lock<std::mutex> workers_lock(m_workers_mutex);

  for (Worker* worker : m_workers) {
    worker->should_stop.acas(true, false);
    wake_worker(worker);
    safeprint("worker % started the world", worker->id);
  }

  safeprint("scheduler has started the world");
}

void Scheduler::enter_native_mode() {
  // TODO: implement
  safeprint("implement scheduler enter native mode");
}

bool Scheduler::exit_native_mode() {
  // TODO: implement
  safeprint("implement scheduler exit native mode");
  return true;
}

void Scheduler::wake_idle_worker() {
  std::unique_lock<std::mutex> processors_lock(m_processors_mutex);
  if (m_idle_processors.size()) {

    std::lock_guard<std::mutex> workers_lock(m_workers_mutex);
    if (m_idle_workers.size()) {
      Worker* worker = m_idle_workers.top();
      m_idle_workers.pop();
      wake_worker(worker);
    } else {
      m_workers.push_back(new Worker());
    }
  }
}

void Scheduler::reschedule_fiber() {
  Fiber* fiber = this->fiber();
  fiber->state.acas(Fiber::State::Running, Fiber::State::Ready);
  enter_scheduler();
}

bool Scheduler::acquire_processor_for_worker(Worker* worker) {
  assert(worker->processor == nullptr);

  Processor* idle_proc = nullptr;
  {
    std::lock_guard<std::mutex> locker(m_processors_mutex);
    if (m_idle_processors.size()) {
      idle_proc = m_idle_processors.top();
      m_idle_processors.pop();
    }
  }

  if (idle_proc == nullptr) {
    return false;
  }

  safeprint("worker % acquired proc %", worker->id, idle_proc->id);

  idle_proc->state.acas(Processor::State::Idle, Processor::State::Owned);
  idle_proc->owner.acas(nullptr, worker);
  worker->processor.acas(nullptr, idle_proc);

  return true;
}

void Scheduler::release_processor_from_worker(Worker* worker) {
  Processor* proc = worker->processor;
  assert(proc);

  proc->state.acas(Processor::State::Owned, Processor::State::Idle);
  proc->owner.acas(worker, nullptr);
  worker->processor.acas(proc, nullptr);

  {
    std::lock_guard<std::mutex> locker(m_processors_mutex);
    m_idle_processors.push(proc);

    safeprint("worker % released proc %", worker->id, proc->id);
  }
}

void Scheduler::idle_worker() {
  Worker* worker = this->worker();
  safeprint("idling worker %", worker->id);

  // append to idle workers list
  {
    std::lock_guard<std::mutex> idle_workers_lock(m_workers_mutex);
    m_idle_workers.push(worker);
    worker->assert_change_state(Worker::State::Acquiring, Worker::State::Idle);
  }
  m_idle_workers_cv.notify_all();

  std::unique_lock<std::mutex> worker_lock(worker->mutex);
  worker->wake_cv.wait(worker_lock, [&] {
    return worker->state == Worker::State::Acquiring;
  });
  assert(worker->state == Worker::State::Acquiring);
  safeprint("waking worker %", worker->id);

  checkpoint();
}

void Scheduler::wake_worker(Worker* worker) {
  {
    std::lock_guard<std::mutex> locker(worker->mutex);
    Worker::State state = worker->state;
    switch (state) {
      case Worker::State::Idle: {
        worker->state.acas(state, Worker::State::Acquiring);
        break;
      }
      case Worker::State::WorldStopped: {
        worker->state.acas(state, Worker::State::Running);
        break;
      }
      default: {
        assert(false && "unexpected worker state");
        break;
      }
    }
  }
  worker->wake_cv.notify_one();
}

uint64_t Scheduler::get_steady_timestamp() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

Fiber* Scheduler::get_ready_fiber() {
  Worker* worker = this->worker();
  Processor* proc = worker->processor;

  assert(worker);
  assert(proc);

  // prefer global queue or steal from some other processor
  if (worker->rand() % kGlobalReadyQueuePriorityChance == 0) {

    // acquire from local queue if global queue is empty
    if (Fiber* fiber = get_ready_fiber_from_global_queue()) {
      safeprint("worker % acquired fiber % from global queue by chance", worker->id, fiber->id);
      return fiber;
    }

    // by stealing from another processors queue, we equalize their workloads
    // over time this will equalize the workload among all active workers
    if (Fiber* fiber = steal_ready_fiber()) {
      return fiber;
    }
  }

  // check current processors local queue
  {
    std::lock_guard<std::mutex> locker(proc->run_queue_mutex);
    if (proc->run_queue.size()) {
      Fiber* fiber = proc->run_queue.front();
      proc->run_queue.pop_front();
      safeprint("worker % acquired fiber % from local queue", worker->id, fiber->id);
      return fiber;
    }
  }

  // check global queue
  if (Fiber* fiber = get_ready_fiber_from_global_queue()) {
    safeprint("worker % acquired fiber % from global queue", worker->id, fiber->id);
    return fiber;
  }

  // attempt to steal a ready fiber from some other processor
  return steal_ready_fiber();
}

Fiber* Scheduler::get_ready_fiber_from_global_queue() {
  std::lock_guard<std::mutex> locker(m_run_queue_mutex);
  if (m_run_queue.size()) {
    Fiber* fiber = m_run_queue.front();
    m_run_queue.pop_front();
    return fiber;
  }

  return nullptr;
}

Fiber* Scheduler::steal_ready_fiber() {
  Worker* worker = this->worker();
  Processor* proc = worker->processor;

  std::lock_guard<std::mutex> processors_lock(m_processors_mutex);
  std::lock_guard<std::mutex> run_queue_lock(proc->run_queue_mutex);

  // find the processor with the highest run queue size, that is still bigger
  // than the current processor
  size_t most_stressed_queue_size = proc->run_queue.size();
  Processor* most_stressed_processor = nullptr;
  for (Processor* i_proc : m_processors) {

    // skip own processor
    if (proc == i_proc) {
      continue;
    }

    std::lock_guard<std::mutex> proc_lock(i_proc->run_queue_mutex);
    if (i_proc->run_queue.size() > most_stressed_queue_size) {
      most_stressed_processor = i_proc;
    }
  }

  // all other processors are empty
  if (most_stressed_processor == nullptr) {
    return nullptr;
  }

  Processor* ms_proc = most_stressed_processor;

  std::lock_guard<std::mutex> other_proc_lock(ms_proc->run_queue_mutex);

  // steal enough fibers to equalize the run queue sizes on both processors
  size_t amount_to_steal = (ms_proc->run_queue.size() - proc->run_queue.size()) / 2;
  Fiber* first_fiber = nullptr;
  for (size_t i = 0; i < amount_to_steal; i++) {
    Fiber* stolen_fiber = ms_proc->run_queue.back();
    ms_proc->run_queue.pop_back();

    if (first_fiber == nullptr) {
      first_fiber = stolen_fiber;
      continue;
    }

    proc->run_queue.push_back(stolen_fiber);
  }

  safeprint("worker % stole % fibers from proc %", worker->id, amount_to_steal, ms_proc->id);

  return first_fiber;
}

void Scheduler::enter_scheduler() {
  Worker* worker = Scheduler::instance->worker();
  transfer_t transfer = jump_fcontext(worker->context, nullptr);
  worker = (Worker*)transfer.data;
  assert(worker == (Worker*)transfer.data);
  worker->context = transfer.fctx;
}

void Scheduler::enter_fiber(Fiber* fiber) {
  Worker* worker = this->worker();
  transfer_t transfer = jump_fcontext(fiber->context, worker);
  fiber->context = transfer.fctx;
}

}  // namespace charly::core::runtime
