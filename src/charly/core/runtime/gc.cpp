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
#include "charly/core/runtime/gc.h"

using namespace std::chrono_literals;

namespace charly::core::runtime {

void GarbageCollector::initialize() {
  static GarbageCollector collector;
  GarbageCollector::instance = &collector;
}

void GarbageCollector::shutdown() {
  {
    std::unique_lock<std::mutex> locker(m_mutex);
    m_wants_exit.store(true);
  }
  m_cv.notify_all();
}

void GarbageCollector::join() {
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void GarbageCollector::request_gc() {
  if (m_wants_collection == false) {
    std::unique_lock<std::mutex> locker(m_mutex);
    if (m_wants_collection.cas(false, true)) {
      m_cv.notify_all();
    }
  }
}

void GarbageCollector::main() {
  for (;;) {
    wait_for_gc_request();

    if (m_wants_exit)
      break;

    Scheduler::instance->stop_the_world();
    if (m_wants_exit) {
      Scheduler::instance->start_the_world();
      break;
    }
    init_mark();
    Scheduler::instance->start_the_world();

    if (m_wants_exit)
      break;
    phase_mark();
    if (m_wants_exit)
      break;

    Scheduler::instance->stop_the_world();
    if (m_wants_exit) {
      Scheduler::instance->start_the_world();
      break;
    }
    init_evacuate();
    Scheduler::instance->start_the_world();

    if (m_wants_exit)
      break;
    phase_evacuate();
    if (m_wants_exit)
      break;

    Scheduler::instance->stop_the_world();
    if (m_wants_exit) {
      Scheduler::instance->start_the_world();
      break;
    }
    init_updateref();
    Scheduler::instance->start_the_world();

    if (m_wants_exit)
      break;
    phase_updateref();
    if (m_wants_exit)
      break;

    Scheduler::instance->stop_the_world();
    if (m_wants_exit) {
      Scheduler::instance->start_the_world();
      break;
    }
    init_idle();
    Scheduler::instance->start_the_world();

    if (m_wants_exit)
      break;
  }
}

void GarbageCollector::wait_for_gc_request() {
  std::unique_lock<std::mutex> locker(m_mutex);
  m_cv.wait(locker, [&]() {
    return m_wants_collection || m_wants_exit;
  });
}

void GarbageCollector::init_mark() {
  m_state.acas(State::Idle, State::Mark);
  safeprint("GC init mark phase");
  std::this_thread::sleep_for(1s);
  safeprint("GC end init mark phase");
}

void GarbageCollector::phase_mark() {
  safeprint("GC mark phase");
  std::this_thread::sleep_for(1s);
  safeprint("GC end mark phase");
}

void GarbageCollector::init_evacuate() {
  m_state.acas(State::Mark, State::Evacuate);
  safeprint("GC init evacuate phase");
  std::this_thread::sleep_for(1s);
  safeprint("GC end init evacuate phase");
}

void GarbageCollector::phase_evacuate() {
  safeprint("GC evacuate phase");
  std::this_thread::sleep_for(1s);
  safeprint("GC end evacuate phase");
}

void GarbageCollector::init_updateref() {
  m_state.acas(State::Evacuate, State::UpdateRef);
  safeprint("GC init updateref phase");
  std::this_thread::sleep_for(1s);
  safeprint("GC end init updateref phase");
}

void GarbageCollector::phase_updateref() {
  safeprint("GC updateref phase");
  std::this_thread::sleep_for(1s);
  safeprint("GC end updateref phase");
}

void GarbageCollector::init_idle() {
  m_state.acas(State::UpdateRef, State::Idle);
  m_wants_collection.acas(true, false);
  safeprint("GC init idle phase");
  std::this_thread::sleep_for(1s);
  safeprint("GC end init idle phase");
}

void GarbageCollector::mark(VALUE value) {
  if (value.is_pointer()) {
    HeapHeader* header = MemoryAllocator::object_header(value.to_pointer<void>());

    // color object grey and append to greylist
    if (header->gcmark == MarkColor::White) {
      if (header->gcmark.cas(MarkColor::White, MarkColor::Grey)) {
        m_greylist.push_back(header);
      }
    }
  }
}

}  // namespace charly::core::runtime
