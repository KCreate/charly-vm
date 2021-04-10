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

namespace charly::core::runtime {

void Scheduler::initialize() {
  static Scheduler scheduler;
  scheduler.init_workers();
  Scheduler::instance = &scheduler;
}

void Scheduler::start() {
  this->m_state.store(Scheduler::State::Running);

  // notify worker threads
  for (Worker* worker : m_fiber_workers) {
    worker->start();
  }
}

void Scheduler::init_workers() {
  for (uint32_t i = 0; i < kHardwareConcurrency; i++) {
    m_fiber_workers.emplace_back(new Worker());
  }
}

void Scheduler::checkpoint() {
  Worker* worker = g_worker;
  Worker::State old_state = worker->state();
  while (m_state.load() == Scheduler::State::Paused) {
    safeprint("parking worker %", worker->id);

    // mark this worker as paused and notify the controller thread
    worker->change_state(Worker::State::Paused);

    // wait for the pause to be over
    {
      std::unique_lock<std::mutex> locker(m_state_m);
      m_state_cv.wait(locker, [&] {
        return m_state.load() == Scheduler::State::Running;
      });
    }

    // mark as running again
    worker->change_state(old_state);
    safeprint("unparking worker %", worker->id);
  }
}

void Scheduler::pause() {

  // pause cannot be called from within a fiber worker
  assert(g_worker == nullptr);

  // attempt to pause the scheduler
  if (!m_state.cas(Scheduler::State::Running, Scheduler::State::Paused)) {
    assert(false && "race inside Scheduler::resume");
    return;
  }

  // wait for all workers to pause
  auto begin_time = std::chrono::steady_clock::now();
  for (Worker* worker : m_fiber_workers) {

    // skip threads that are already paused
    if (worker->state() == Worker::State::Paused) {
      continue;
    }

    // wait for the worker to enter either into the paused or native state
    for (;;) {
      Worker::State state = worker->state();

      if (state == Worker::State::Native || state == Worker::State::Paused) {
        break;
      }

      worker->wait_for_state_change(state);
    }
  }
  auto end_time = std::chrono::steady_clock::now();
  safeprint("finished waiting in % microseconds",
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time).count());
}

void Scheduler::resume() {

  // resume cannot be called from within a fiber worker
  assert(g_worker == nullptr);

  // change scheduler state
  if (!m_state.cas(Scheduler::State::Paused, Scheduler::State::Running)) {
    assert(false && "race inside Scheduler::resume");
    return;
  }

  // notify all worker threads to continue
  m_state_cv.notify_all();
}

}  // namespace charly::core::runtime
