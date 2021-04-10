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

using namespace std::chrono_literals;

void Worker::main() {

  // initialize thread local g_worker variable
  // and wait for the scheduler to give us the start signal
  g_worker = this;
  assert(g_worker->state() == Worker::State::Created);
  wait_for_state_change(Worker::State::Created);
  assert(g_worker->state() == Worker::State::Running);

  for (uint64_t i = 0;; i++) {
    Scheduler::instance->checkpoint();
  }

  g_worker->change_state(Worker::State::Exited);
}

void Worker::start() {
  std::unique_lock<std::mutex> locker(m_mutex);
  if (!m_state.cas(Worker::State::Created, Worker::State::Running)) {
    assert(false && "race in Worker::start");
  }
  m_cv.notify_one();
}

void Worker::change_state(Worker::State state) {

  // state changes to paused or untracked do not require a call
  // to the scheduler checkpoint
  if (state == Worker::State::Running || state == Worker::State::Exited) {
    Scheduler::instance->checkpoint();
  }

  // update state and notify any threads waiting for a state change
  // safeprint("worker % changes state to %", this->id, (int)state);
  m_state.store(state);
  m_cv.notify_all();
}

void Worker::wait_for_state_change(Worker::State old_state) {
  while (m_state.load() == old_state) {
    std::unique_lock<std::mutex> locker(m_mutex);
    m_cv.wait(locker, [&] {
      return m_state.load() != old_state;
    });
  }
}

}  // namespace charly::core::runtime
