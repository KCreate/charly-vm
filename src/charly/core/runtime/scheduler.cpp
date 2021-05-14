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

namespace charly::core::runtime {

void Scheduler::initialize() {
  static Scheduler scheduler;
  Scheduler::instance = &scheduler;

  GarbageCollector::initialize();

  // initialize application workers
  for (size_t i = 0; i < kHardwareConcurrency; i++) {
    scheduler.m_application_threads.push_back(new Worker());
  }
}

void Scheduler::start() {
  GarbageCollector::instance->start_background_worker();

  for (Worker* worker : m_application_threads) {
    worker->wait_for_checkpoint();
    worker->resume();
  }
}

void Scheduler::join() {
  m_wants_join.assert_cas(false, true);

  resume_all_idle_workers();

  for (Worker* worker : m_application_threads) {
    worker->join();
  }

  GarbageCollector::instance->stop_background_worker();
}

void Scheduler::request_shutdown() {
  if (m_wants_exit.cas(false, true)) {
    resume_all_idle_workers();
  }
}

void Scheduler::checkpoint() {
  assert(g_worker);
  g_worker->checkpoint();

  // reschedule fiber if it has exceeded its timeslice
  uint64_t scheduled_at = g_worker->current_fiber()->m_scheduled_at;
  uint64_t now = Scheduler::current_timestamp();
  if (now - scheduled_at >= kSchedulerFiberTimeslice) {
    safeprint("fiber % exceeded its timeslice", g_worker->current_fiber()->m_id);
    g_worker->fiber_reschedule();
  }
}

void Scheduler::stop_the_world() {
  assert(g_worker == nullptr);

  // wait for all workers to pause or enter native mode
  auto begin_time = std::chrono::steady_clock::now();
  for (Worker* worker : m_application_threads) {
    worker->m_pause_request.assert_cas(false, true);
    worker->wait_for_checkpoint();
  }
  auto end_time = std::chrono::steady_clock::now();
  safeprint("finished waiting in % microseconds",
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time).count());

  safeprint("scheduler begin pause");
}

void Scheduler::start_the_world() {
  assert(g_worker == nullptr);

  // resume paused workers
  for (Worker* worker : m_application_threads) {
    if (worker->state() == Worker::State::Paused) {
      worker->m_pause_request.assert_cas(true, false);
      worker->resume();
    }
  }
}

void Scheduler::schedule_fiber(HeapFiber* fiber) {
  assert(fiber->status() == Fiber::Status::Ready);

  // schedule in local queue
  if (g_worker) {
    std::unique_lock<std::mutex> locker(g_worker->m_ready_queue_m);
    if (g_worker->m_ready_queue.size() < kLocalReadyQueueMaxSize) {
      g_worker->m_ready_queue.push_back(fiber);
      safeprint("fiber % scheduled in local queue of worker %", fiber->m_fiber->m_id, g_worker->id());

      if (g_worker->m_ready_queue.size() > 1 && m_idle_threads_counter.load()) {
        resume_idle_worker();
      }

      return;
    }
  }

  // schedule in global queue
  {
    std::lock_guard<std::mutex> locker(m_global_ready_queue_m);
    m_global_ready_queue.push_back(fiber);
    safeprint("fiber % scheduled in global queue", fiber->m_fiber->m_id);
    resume_idle_worker();
  }
}

HeapFiber* Scheduler::get_ready_fiber() {
  assert(g_worker);

  for (;;) {

    // no next fiber if scheduler wants to exit
    if (m_wants_exit.load()) {
      return nullptr;
    }

    // prefer the global queue over the local queue from time to time
    if (g_worker->m_context_switch_counter % kGlobalReadyQueuePriorityChance) {
      std::unique_lock<std::mutex> locker(m_global_ready_queue_m);
      if (m_global_ready_queue.size()) {
        HeapFiber* fiber = m_global_ready_queue.front();
        m_global_ready_queue.pop_front();
        safeprint("worker %: got task from global queue by chance [%]", g_worker->id(), m_global_ready_queue.size());
        return fiber;
      }
    }

    // check the current workers local queue
    {
      // safeprint("worker %: checking local queue", g_worker->id());
      std::unique_lock<std::mutex> locker(g_worker->m_ready_queue_m);
      if (g_worker->m_ready_queue.size()) {
        HeapFiber* fiber = g_worker->m_ready_queue.front();
        g_worker->m_ready_queue.pop_front();
        // safeprint("worker %: got task from local queue [%]", g_worker->id(), g_worker->m_ready_queue.size());
        return fiber;
      }
    }

    // check global ready queue
    {
      // safeprint("worker %: checking global queue", g_worker->id());
      std::unique_lock<std::mutex> locker(m_global_ready_queue_m);
      if (m_global_ready_queue.size()) {
        HeapFiber* fiber = m_global_ready_queue.front();
        m_global_ready_queue.pop_front();
        safeprint("worker %: got task from global queue [%]", g_worker->id(), m_global_ready_queue.size());
        return fiber;
      }
    }

    // attempt to steal from some random other worker
    uint32_t base_offset = g_worker->m_context_switch_counter;
    for (uint32_t i = 0; i < m_application_threads.size(); i++) {
      uint32_t wrapped_index = (base_offset + i) % m_application_threads.size();
      Worker* worker = m_application_threads[wrapped_index];

      // skip current worker
      if (worker == g_worker) {
        continue;
      }

      // safeprint("worker %: checking local queue of worker %", g_worker->id(), worker->id());

      // attempt to steal half of the workers tasks
      {
        std::lock_guard<std::mutex> schedule_locker(m_global_ready_queue_m);
        std::lock_guard<std::mutex> worker_locker(g_worker->m_ready_queue_m);
        if (worker->m_ready_queue.size() > 1) {

          // calculate amount of tasks to steal
          size_t amount_to_steal = worker->m_ready_queue.size() / 2;
          size_t counter = 0;
          HeapFiber* entry = nullptr;
          while (counter++ < amount_to_steal) {
            HeapFiber* task = worker->m_ready_queue.back();
            worker->m_ready_queue.pop_back();

            if (entry == nullptr) {
              entry = task;
            } else {
              g_worker->m_ready_queue.push_back(task);
            }
          }

          assert(entry);

          safeprint("worker %: stole % ready fibers from worker % [%]", g_worker->id(), amount_to_steal, worker->id(),
                    worker->m_ready_queue.size());
          return entry;
        }
      }
    }

    // enter idle mode and wait for more tasks to be ready
    //
    // TODO: refactor this to be more efficient, this sucks right now...
    g_worker->idle();
    g_worker->m_context_switch_counter++;
  }
}

bool Scheduler::has_available_tasks() {
  std::unique_lock<std::mutex> locker(m_global_ready_queue_m);
  return m_global_ready_queue.size();
}

void Scheduler::register_idle(Worker* worker) {
  std::unique_lock<std::mutex> locker(m_idle_threads_m);
  m_idle_threads.push_back(worker);
  m_idle_threads_counter++;

  if (m_idle_threads.size() == m_application_threads.size() && m_wants_join.load()) {
    locker.unlock();
    request_shutdown();
  }
}

void Scheduler::resume_idle_worker() {
  std::unique_lock<std::mutex> locker(m_idle_threads_m);
  if (m_idle_threads.size()) {
    Worker* worker = m_idle_threads.front();
    m_idle_threads.pop_front();
    m_idle_threads_counter--;
    worker->resume_from_idle();
  }
}

void Scheduler::resume_all_idle_workers() {
  std::unique_lock<std::mutex> locker(m_idle_threads_m);
  while (m_idle_threads.size()) {
    Worker* worker = m_idle_threads.front();
    m_idle_threads.pop_front();
    m_idle_threads_counter--;
    worker->resume_from_idle();
  }
}

uint64_t Scheduler::current_timestamp() {
  auto ticks = std::chrono::system_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(ticks).count();
}

}  // namespace charly::core::runtime
