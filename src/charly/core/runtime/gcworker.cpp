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
#include "charly/core/runtime/gcworker.h"

using namespace std::chrono_literals;

namespace charly::core::runtime {

void GCConcurrentWorker::start_thread() {
  m_thread = std::thread(&GCConcurrentWorker::main, this);
}

void GCConcurrentWorker::stop_thread() {
  m_wants_exit = true;
  request_gc(); // wake the GC worker thread if it isn't running already
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void GCConcurrentWorker::request_gc() {
  if (m_phase == Phase::Idle) {
    m_cv.notify_one();
  }
}

void GCConcurrentWorker::main() {
  for (;;) {
    wait_for_gc_request();

    if (m_wants_exit)
      break;

    // mark phase
    Scheduler::instance->pause();
    m_phase.assert_cas(Phase::Idle, Phase::Mark);
    safeprint("GC mark phase");
    std::this_thread::sleep_for(1s);
    safeprint("GC mark phase end");
    Scheduler::instance->resume();
    std::this_thread::sleep_for(2s);

    if (m_wants_exit)
      break;

    // evacuate phase
    Scheduler::instance->pause();
    m_phase.assert_cas(Phase::Mark, Phase::Evacuate);
    safeprint("GC evacuate phase");
    std::this_thread::sleep_for(1s);
    safeprint("GC evacuate phase end");
    Scheduler::instance->resume();
    std::this_thread::sleep_for(2s);

    if (m_wants_exit)
      break;

    // updateref phase
    Scheduler::instance->pause();
    m_phase.assert_cas(Phase::Evacuate, Phase::UpdateRef);
    safeprint("GC updateref phase");
    std::this_thread::sleep_for(1s);
    safeprint("GC updateref phase end");
    Scheduler::instance->resume();
    std::this_thread::sleep_for(2s);

    if (m_wants_exit)
      break;

    // idle phase
    Scheduler::instance->pause();
    m_phase.assert_cas(Phase::UpdateRef, Phase::Idle);
    safeprint("GC idle phase");
    std::this_thread::sleep_for(1s);
    safeprint("GC idle phase end");
    Scheduler::instance->resume();

    if (m_wants_exit)
      break;
  }
}

bool GCConcurrentWorker::wait_for_gc_request() {
  safeprint("GC worker waiting for GC request");

  bool waited = false;

  while (!m_gc->should_begin_collection()) {
    waited = true;
    safeprint("GC worker wait iteration");
    std::unique_lock<std::mutex> locker(m_mutex);
    m_cv.wait(locker, [&]() {
      return m_gc->should_begin_collection();
    });
  }

  safeprint("GC worker finished waiting");
  return waited;
}

}  // namespace charly::core::runtime
