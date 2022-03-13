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

#include "charly/core/runtime/gc.h"
#include "charly/core/runtime/runtime.h"

using namespace std::chrono_literals;

namespace charly::core::runtime {

void GarbageCollector::shutdown() {
  {
    std::unique_lock<std::mutex> locker(m_mutex);
    m_wants_exit = true;
  }
  m_cv.notify_all();
}

void GarbageCollector::join() {
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void GarbageCollector::request_gc() {
  if (!m_wants_collection) {
    std::unique_lock<std::mutex> locker(m_mutex);
    if (m_wants_collection.cas(false, true)) {
      m_cv.notify_all();
    }
  }
}

void GarbageCollector::main() {
  m_runtime->wait_for_initialization();

  for (;;) {
    if (m_wants_exit)
      break;
    phase_idle();

    if (!m_wants_collection) {
      continue;
    }

    m_runtime->scheduler()->stop_the_world();
    init_mark();
    m_runtime->scheduler()->start_the_world();

    if (m_wants_exit)
      break;
    phase_mark();

    m_runtime->scheduler()->stop_the_world();
    init_evacuate();
    m_runtime->scheduler()->start_the_world();

    if (m_wants_exit)
      break;
    phase_evacuate();

    m_runtime->scheduler()->stop_the_world();
    init_updateref();
    m_runtime->scheduler()->start_the_world();

    if (m_wants_exit)
      break;
    phase_updateref();

    m_runtime->scheduler()->stop_the_world();
    init_idle();
    m_runtime->scheduler()->start_the_world();
  }
}

void GarbageCollector::init_mark() {
  m_wants_collection.acas(true, false);
  m_state.acas(State::Idle, State::Mark);
  debugln("GC init mark phase");
  //  std::this_thread::sleep_for(1s);
  debugln("GC end init mark phase");
}

void GarbageCollector::phase_idle() {
  std::unique_lock<std::mutex> locker(m_mutex);
  m_cv.wait(locker, [&]() {
    return m_wants_collection || m_wants_exit;
  });
}

void GarbageCollector::phase_mark() {
  debugln("GC mark phase");
  //  std::this_thread::sleep_for(1s);
  debugln("GC end mark phase");
}

void GarbageCollector::init_evacuate() {
  m_state.acas(State::Mark, State::Evacuate);
  debugln("GC init evacuate phase");
  //  std::this_thread::sleep_for(1s);
  debugln("GC end init evacuate phase");
}

void GarbageCollector::phase_evacuate() {
  debugln("GC evacuate phase");
  //  std::this_thread::sleep_for(1s);
  debugln("GC end evacuate phase");
}

void GarbageCollector::init_updateref() {
  m_state.acas(State::Evacuate, State::UpdateRef);
  debugln("GC init updateref phase");
  //  std::this_thread::sleep_for(1s);
  debugln("GC end init updateref phase");
}

void GarbageCollector::phase_updateref() {
  debugln("GC updateref phase");
  //  std::this_thread::sleep_for(1s);
  debugln("GC end updateref phase");
}

void GarbageCollector::init_idle() {
  m_state.acas(State::UpdateRef, State::Idle);
  debugln("GC init idle phase");
  //  std::this_thread::sleep_for(1s);
  debugln("GC end init idle phase");
}

}  // namespace charly::core::runtime
