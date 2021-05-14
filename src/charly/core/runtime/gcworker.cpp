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
  m_cv.notify_all();
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void GCConcurrentWorker::request_gc() {
  m_cv.notify_one();
}

GCPhase GCConcurrentWorker::phase() const {
  return m_phase.load();
}

void GCConcurrentWorker::main() {
  for (;;) {
    wait_for_gc_request();

    Scheduler::instance->stop_the_world();
    if (m_wants_exit)
      break;
    init_mark();
    Scheduler::instance->start_the_world();
    phase_mark();

    Scheduler::instance->stop_the_world();
    if (m_wants_exit)
      break;
    init_evacuate();
    Scheduler::instance->start_the_world();
    phase_evacuate();

    Scheduler::instance->stop_the_world();
    if (m_wants_exit)
      break;
    init_updateref();
    Scheduler::instance->start_the_world();
    phase_updateref();

    Scheduler::instance->stop_the_world();
    if (m_wants_exit)
      break;
    init_idle();
    Scheduler::instance->start_the_world();
  }
}

void GCConcurrentWorker::wait_for_gc_request() {
  safeprint("GC worker waiting for GC request");
  safeprint("GC utilization = %", m_gc->utilization());

  safeprint("GC worker wait iteration");
  std::unique_lock<std::mutex> locker(m_mutex);
  m_cv.wait(locker, [&]() {
    return m_gc->should_begin_collection() || m_wants_exit;
  });

  safeprint("GC worker finished waiting");
}

void GCConcurrentWorker::init_mark() {
  m_phase.assert_cas(Phase::Idle, Phase::Mark);
  safeprint("GC init mark phase");
}

void GCConcurrentWorker::phase_mark() {
  safeprint("GC mark phase");

  // traverse live object graph
  while (m_greylist.size()) {
    HeapHeader* cell = m_greylist.front();
    m_greylist.pop_front();

    switch (cell->type()) {
      case kTypeFiber: {
        // HeapFiber* value = static_cast<HeapFiber*>(cell);
        break;
      }
      default: {
        assert(false && "unexpected cell type");
        break;
      }
    }

    cell->set_color(kMarkColorBlack);
  }

  safeprint("GC end mark phase");
}

void GCConcurrentWorker::init_evacuate() {
  m_phase.assert_cas(Phase::Mark, Phase::Evacuate);
  safeprint("GC init evacuate phase");
  std::this_thread::sleep_for(1s);
}

void GCConcurrentWorker::phase_evacuate() {
  safeprint("GC evacuate phase");
  std::this_thread::sleep_for(1s);
  safeprint("GC end evacuate phase");
}

void GCConcurrentWorker::init_updateref() {
  m_phase.assert_cas(Phase::Evacuate, Phase::UpdateRef);
  safeprint("GC init updateref phase");
  std::this_thread::sleep_for(1s);
}

void GCConcurrentWorker::phase_updateref() {
  safeprint("GC updateref phase");
  std::this_thread::sleep_for(1s);
  safeprint("GC end updateref phase");
}

void GCConcurrentWorker::init_idle() {
  m_phase.assert_cas(Phase::UpdateRef, Phase::Idle);
  safeprint("GC init idle phase");
  std::this_thread::sleep_for(1s);
}

void GCConcurrentWorker::mark(VALUE value) {
  if (value.is_pointer()) {
    HeapHeader* cell_ptr = value.to_pointer<HeapHeader>();
    assert(cell_ptr);
    cell_ptr->set_color(kMarkColorGrey);
    m_greylist.push_back(cell_ptr);
  }
}

}  // namespace charly::core::runtime
