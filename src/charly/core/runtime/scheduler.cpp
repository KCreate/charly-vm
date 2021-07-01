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
#include "charly/core/runtime/compiled_module.h"

namespace charly::core::runtime {

using namespace std::chrono_literals;

Processor::Processor() {
  static charly::atomic<uint64_t> processor_id_counter = 0;
  this->id = processor_id_counter++;
}

Worker::Worker() : os_handle(std::thread(&Scheduler::worker_main, Scheduler::instance, this)) {
  static charly::atomic<uint64_t> worker_id_counter = 0;
  this->id = worker_id_counter++;
  this->context_switch_counter = 0;
  this->random_source = this->id;
  this->idle_sleep_duration = 10;

  this->stw_request.store(false);

  this->state.store(State::Created);
  this->fiber.store(nullptr);
  this->processor.store(nullptr);
}

void Worker::join() {
  if (this->os_handle.joinable()) {
    this->os_handle.join();
  }
}

void Worker::wake_idle() {
  if (this->state == State::Idle) {
    std::lock_guard<std::mutex> locker(this->mutex);
    this->state.cas(State::Idle, State::Acquiring);
  }
  this->wake_cv.notify_one();
}

void Worker::wake_stw() {
  {
    std::lock_guard<std::mutex> locker(this->mutex);
    this->stw_request.store(false);
  }
  this->stw_cv.notify_one();
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
  std::unique_lock<std::mutex> locker(this->mutex);

  if (this->state == oldstate) {
    this->state_cv.wait(locker, [&] {
      return this->state != oldstate;
    });
  }

  return this->state;
}

uint32_t Worker::rand() {
  uint32_t seed = this->random_source;
  uint32_t t = (214013 * seed + 2531011);
  return this->random_source = (t >> 16) & 0x7FFF;
}

void Worker::increase_sleep_duration() {
  uint32_t next_duration = this->idle_sleep_duration * 2;
  if (next_duration > kWorkerMaximumIdleSleepDuration) {
    next_duration = kWorkerMaximumIdleSleepDuration;
  }
  this->idle_sleep_duration = next_duration;
}

void Worker::reset_sleep_duration() {
  this->idle_sleep_duration = 10;
}

Fiber::Fiber() {

  // allocate stack
  void* stack_lo = std::malloc(kFiberStackSize);
  this->stack.lo = stack_lo;
  this->stack.hi = (void*)((uintptr_t)stack_lo + kFiberStackSize);
  this->stack.size = kFiberStackSize;

  // init fiber state
  static charly::atomic<uint64_t> fiber_id_counter = 0;
  this->id = fiber_id_counter++;
  this->state.store(Fiber::State::Created);
  this->worker.store(nullptr);
  this->last_scheduled_at.store(0);
  this->context = make_fcontext(this->stack.hi, kFiberStackSize, &Scheduler::fiber_main);
}

Fiber::~Fiber() {
  this->clean();
}

void Fiber::clean() {
  if (this->stack.lo) {
    std::free(this->stack.lo);
    this->stack.lo = nullptr;
    this->stack.hi = nullptr;
    this->stack.size = 0;
  }
}

void Scheduler::init_scheduler() {
  MemoryAllocator::initialize();
  GarbageCollector::initialize();

  m_start_timestamp = get_steady_timestamp();

  // initialize processors and workers
  {
    std::lock_guard<std::mutex> locker(m_init_mutex);
    for (size_t i = 0; i < kHardwareConcurrency; i++) {
      Processor* processor = new Processor();
      m_processors.push_back(processor);
      m_idle_processors.push(processor);

      Worker* worker = new Worker();
      m_workers.push_back(worker);
    }
  }
}

void Scheduler::worker_main(Worker* worker) {

  // wait for the scheduler to finish initializing
  {
    std::lock_guard<std::mutex> locker(m_init_mutex);
  }

  g_worker = worker;

  worker->assert_change_state(Worker::State::Created, Worker::State::Acquiring);

  while (!m_wants_exit) {
    Scheduler::instance->scheduler_checkpoint();

    // attempt to acquire idle processor
    if (!acquire_processor_for_worker(worker)) {
      worker->increase_sleep_duration();
      idle_worker();
      continue;
    }

    assert(worker->processor);
    worker->assert_change_state(Worker::State::Acquiring, Worker::State::Scheduling);

    // execute tasks using on the acquired processor until there are no
    // more tasks left to execute or if the worker should exit
    while (!m_wants_exit) {
      Scheduler::instance->scheduler_checkpoint();
      if (m_wants_exit) {
        break;
      }

      Fiber* fiber = get_ready_fiber();

      if (fiber == nullptr) {
        worker->increase_sleep_duration();
        break;
      }

      worker->reset_sleep_duration();

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

      safeprint("worker % left fiber %", worker->id, fiber->id);

      switch (fiber->state) {
        case Fiber::State::Paused: {
          break;
        }
        case Fiber::State::Ready: {
          schedule_fiber(fiber);
          break;
        }
        case Fiber::State::Exited: {
          fiber->clean();
          break;
        }
        default: {
          assert(false && "unexpected fiber state");
          break;
        }
      }
    }

    // release processor
    release_processor_from_worker(worker);
    worker->assert_change_state(Worker::State::Scheduling, Worker::State::Acquiring);
    idle_worker();
  }

  worker->assert_change_state(Worker::State::Acquiring, Worker::State::Exited);
}

void Scheduler::fiber_main(transfer_t transfer) {
  Fiber* fiber = Scheduler::instance->fiber();
  Worker* worker = Scheduler::instance->worker();
  Processor* proc = Scheduler::instance->processor();
  worker->context = transfer.fctx;

  for (int i = 0; i < 100; i++) {
    worker = Scheduler::instance->worker();
    proc = Scheduler::instance->processor();
    fiber = Scheduler::instance->fiber();

    safeprint("fiber % on worker % on proc % counter = %", fiber->id, worker->id, proc->id, i);
    Scheduler::instance->worker_checkpoint();
    std::this_thread::sleep_for(500us);
  }

  for (int i = 0; i < 2; i++) {

    // allocate new fiber
    Fiber* fiber = MemoryAllocator::allocate<Fiber>();
    if (!fiber) {
      worker = Scheduler::instance->worker();
      safeprint("failed allocation in worker %", worker->id);
      Scheduler::instance->abort(1);
    }

    // schedule fiber
    fiber->state.acas(Fiber::State::Created, Fiber::State::Ready);
    Scheduler::instance->schedule_fiber(fiber);
  }

  Scheduler::instance->exit_fiber();
}

int32_t Scheduler::join() {

  // wait for the runtime to request the exit
  {
    std::unique_lock<std::mutex> locker(m_exit_mutex);
    m_exit_cv.wait(locker, [&] {
      return m_wants_exit == true;
    });
  }

  GarbageCollector::instance->shutdown();

  // wait for all worker threads to exit
  {
    std::unique_lock<std::mutex> locker(m_workers_mutex);

    for (Worker* worker : m_workers) {
      worker->wake_idle();
    }

    for (Worker* worker : m_workers) {
      if (worker->state != Worker::State::Exited) {
        locker.unlock();

        safeprint("waiting for worker % to exit", worker->id);
        worker->join();

        locker.lock();
      }
    }
  }

  GarbageCollector::instance->join();

  return m_exit_code;
}

void Scheduler::abort(int32_t exit_code) {
  if (m_exit_code.cas(0, 1)) {
    {
      std::lock_guard<std::mutex> locker(m_exit_mutex);
      m_exit_code.cas(1, exit_code);
      m_wants_exit.store(true);
    }
    m_exit_cv.notify_all();
  }

  if (this->worker()) {
    exit_fiber();
    assert(false && "unreachable");
  }
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
      return;
    }
  }

  // schedule in global queue
  {
    std::unique_lock<std::mutex> locker(m_run_queue_mutex);
    m_run_queue.push_back(fiber);
  }

  wake_idle_worker();
}

void Scheduler::stw_checkpoint() {
  Worker* worker = this->worker();

  if (worker->stw_request) {
    Worker::State old_state = worker->state;
    worker->change_state(old_state, Worker::State::WorldStopped);
    {
      std::unique_lock<std::mutex> locker(worker->mutex);
      worker->stw_cv.wait(locker, [&] {
        return worker->stw_request == false;
      });
    }
    worker->change_state(Worker::State::WorldStopped, old_state);
  }
}

void Scheduler::worker_checkpoint() {
  stw_checkpoint();

  Fiber* fiber = this->fiber();

  // timeslice exceeded check
  uint64_t timestamp_now = get_steady_timestamp();
  if (timestamp_now - fiber->last_scheduled_at >= kSchedulerFiberTimeslice) {
    reschedule_fiber();
  }

  // runtime exit check
  if (m_wants_exit) {
    exit_fiber();
  }
}

void Scheduler::scheduler_checkpoint() {
  stw_checkpoint();
}

void Scheduler::stop_the_world() {
  safeprint("scheduler is stopping the world");

  // wait for the worker threads to pause
  std::unique_lock<std::mutex> workers_lock(m_workers_mutex);

  // request each worker to stop
  for (Worker* worker : m_workers) {
    std::lock_guard<std::mutex> locker(worker->mutex);
    worker->stw_request.acas(false, true);
  }

  // wait for workers to enter into a heap-safe mode
  for (Worker* worker : m_workers) {

    // wait for the worker to enter into a heap safe mode
    for (;;) {
      Worker::State now_state = worker->state;
      switch (now_state) {
        case Worker::State::WorldStopped:
        case Worker::State::Native:
        case Worker::State::Idle:
        case Worker::State::Exited: {
          break;
        }
        default: {
          Processor* proc = worker->processor;
          safeprint("waiting for worker % on proc % (currently in mode %)", worker->id, proc ? proc->id : 0,
                    (int)now_state);
          workers_lock.unlock();
          now_state = worker->wait_for_state_change(now_state);
          workers_lock.lock();
          continue;
        }
      }
      break;
    }
  }

  safeprint("scheduler has stopped the world");
}

void Scheduler::start_the_world() {
  std::unique_lock<std::mutex> workers_lock(m_workers_mutex);

  for (Worker* worker : m_workers) {
    if (worker->state == Worker::State::WorldStopped) {
      worker->wake_stw();
    }
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

void Scheduler::reschedule_fiber() {
  Fiber* fiber = this->fiber();
  fiber->state.acas(Fiber::State::Running, Fiber::State::Ready);
  enter_scheduler();
}

void Scheduler::exit_fiber() {
  Fiber* fiber = this->fiber();
  fiber->state.acas(Fiber::State::Running, Fiber::State::Exited);
  enter_scheduler();
}

void Scheduler::register_module(ref<CompiledModule> module) {
  safeprint("registering module at path %", module->filename);
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
  }
}

void Scheduler::idle_worker() {
  Worker* worker = this->worker();

  // append to idle workers list
  {
    std::lock_guard<std::mutex> locker(m_workers_mutex);
    m_idle_workers.insert(worker);
    worker->assert_change_state(Worker::State::Acquiring, Worker::State::Idle);
  }

  {
    std::unique_lock<std::mutex> locker(worker->mutex);
    auto idle_duration = 1ms * worker->idle_sleep_duration;
    worker->wake_cv.wait_for(locker, idle_duration, [&] {
      return worker->state == Worker::State::Acquiring;
    });
  }

  // got woken by other worker
  if (worker->state == Worker::State::Acquiring) {
    return;
  }

  // got woken because of timer timeout
  std::lock_guard<std::mutex> locker(m_workers_mutex);
  m_idle_workers.erase(worker);
  worker->change_state(Worker::State::Idle, Worker::State::Acquiring);
}

void Scheduler::wake_idle_worker() {
  std::unique_lock<std::mutex> processors_lock(m_processors_mutex);
  if (m_idle_processors.size()) {

    std::lock_guard<std::mutex> workers_lock(m_workers_mutex);
    if (m_idle_workers.size()) {
      auto begin_it = m_idle_workers.begin();
      Worker* worker = *begin_it;
      m_idle_workers.erase(begin_it);
      worker->wake_idle();
    } else {
      if (!m_wants_exit) {
        m_workers.push_back(new Worker());
      }
    }
  }
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

  // pull ready fiber from global run queue
  if (worker->rand() % kGlobalRunQueuePriorityChance == 0) {
    if (Fiber* fiber = get_ready_fiber_from_global_queue()) {
      return fiber;
    }
  }

  // steal from other workers and equalize their run queues
  if (worker->rand() % kWorkerStealPriorityChance == 0) {
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
      return fiber;
    }
  }

  // check global queue
  if (Fiber* fiber = get_ready_fiber_from_global_queue()) {
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
