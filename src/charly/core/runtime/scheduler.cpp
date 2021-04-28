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

#include <cassert>
#include <chrono>

#include "charly/utils/argumentparser.h"
#include "charly/core/runtime/scheduler.h"
#include "charly/core/runtime/gc.h"

namespace charly::core::runtime {

void Scheduler::initialize() {
  static Scheduler scheduler;
  Scheduler::instance = &scheduler;
  scheduler.init_workers();
}

void Scheduler::start() {
  safeprint("starting GC worker");
  GarbageCollector::instance->start_background_worker();

  // worker threads pause themselves once they start until the scheduler
  // finishes setup and resumes them
  safeprint("starting fiber workers");
  for (Worker* worker : m_fiber_workers) {
    worker->wake();
  }
}

void Scheduler::join() {
  for (Worker* worker : m_fiber_workers) {
    worker->join();
  }
  safeprint("scheduler workers joined");

  GarbageCollector::instance->stop_background_worker();
  safeprint("GC worker joined");
}

void Scheduler::init_workers() {
  for (uint32_t i = 0; i < kHardwareConcurrency; i++) {
    m_fiber_workers.emplace_back(new Worker());
  }
}

void Scheduler::checkpoint() {
  Worker* worker = g_worker;

  while (m_state.load() == Scheduler::State::Paused) {
    safeprint("parking worker %", worker->id);
    assert(worker->state() == Worker::State::Running);
    worker->pause();
    safeprint("unparking worker %", worker->id);
  }
}

bool Scheduler::pause() {
  assert(g_worker == nullptr);

  // attempt to pause the scheduler
  safeprint("scheduler request pause");
  m_state.assert_cas(Scheduler::State::Running, Scheduler::State::Paused);

  // wait for all workers to pause or enter native mode
  auto begin_time = std::chrono::steady_clock::now();
  for (Worker* worker : m_fiber_workers) {
    worker->wait_for_safepoint();
  }
  auto end_time = std::chrono::steady_clock::now();
  safeprint("finished waiting in % microseconds",
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time).count());

  safeprint("scheduler begin pause");
  return true;
}

void Scheduler::resume() {

  // resume cannot be called from within a fiber worker
  assert(g_worker == nullptr);

  // change scheduler state
  safeprint("scheduler exit pause");
  m_state.assert_cas(Scheduler::State::Paused, Scheduler::State::Running);
  for (Worker* worker : m_fiber_workers) {

    // do not resume thread with no active region unless the
    // GC is currently able to allocate more cells
    // if (worker->active_region() == nullptr && !GarbageCollector::instance->can_allocate()) {
    //   continue;
    // }

    if (worker->state() == Worker::State::Paused) {
      worker->wake();
    }
  }
}

}  // namespace charly::core::runtime
