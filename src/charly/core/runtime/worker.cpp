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
#include "charly/core/runtime/heapvalue.h"

namespace charly::core::runtime {

using namespace std::chrono_literals;

void Worker::main() {

  // initialize thread local g_worker variable
  // and wait for the scheduler to give us the start signal
  g_worker = this;

  // allocate initial region
  g_worker->set_gc_region(GarbageCollector::instance->allocate_heap_region());

  // wait for the go signal from the runtime
  pause();
  assert(g_worker->state() == Worker::State::Running);

  for (uint64_t i = 0; true; i++) {
    Scheduler::instance->checkpoint();

    if (i % 3 == 0) {
      HeapTestType* value_ptr = GarbageCollector::instance->alloc<HeapTestType>(i, m_head_cell);
      if (value_ptr == nullptr) {
        safeprint("allocation failed in worker %", this->id);
        break;
      }
      m_head_cell.store(VALUE::Pointer(value_ptr));
    } else {
      HeapTestType* value_ptr = GarbageCollector::instance->alloc<HeapTestType>(i, kNull);
      if (value_ptr == nullptr) {
        safeprint("allocation failed in worker %", this->id);
        break;
      }
    }

    g_worker->enter_native();
    // std::this_thread::sleep_for(10ms);
    std::this_thread::yield();
    g_worker->leave_native();
  }

  g_worker->exit();
}

void Worker::join() {
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void Worker::pause() {
  m_state.assert_cas(Worker::State::Running, Worker::State::Paused);
  m_cv.notify_all();

  while (m_state.load() == Worker::State::Paused) {
    wait_for_state_change(Worker::State::Paused);
  }
}

void Worker::enter_native() {
  // safeprint("worker % -> native mode", this->id);
  m_state.assert_cas(Worker::State::Running, Worker::State::Native);
  m_cv.notify_all();
}

void Worker::leave_native() {
  // safeprint("worker % -> running mode", this->id);
  m_state.assert_cas(Worker::State::Native, Worker::State::Running);
  m_cv.notify_all();
  Scheduler::instance->checkpoint();
}

void Worker::exit() {
  safeprint("worker % -> exit mode", this->id);
  m_state.assert_cas(Worker::State::Running, Worker::State::Exited);
  m_cv.notify_all();
}

void Worker::wait_for_safepoint() {
  while (m_state.load() == Worker::State::Running) {
    wait_for_state_change(Worker::State::Running);
  }
}

void Worker::wake() {
  m_state.assert_cas(Worker::State::Paused, Worker::State::Running);
  m_cv.notify_all();
}

void Worker::set_gc_region(HeapRegion* region) {
  m_active_region.assert_cas(nullptr, region);
}

void Worker::clear_gc_region() {
  m_active_region.assert_cas(m_active_region.load(), nullptr);
}

void Worker::wait_for_state_change(Worker::State old_state) {
  while (m_state.load(std::memory_order_acquire) == old_state) {

    // spin and wait for the state to change
    for (uint32_t spin = 0; spin < 10000; spin++) {
      if (m_state.load(std::memory_order_acquire) != old_state) {
        break;
      }
    }

    {
      std::unique_lock<std::mutex> locker(m_mutex);
      m_cv.wait(locker, [&] {
        return m_state.load(std::memory_order_acquire) != old_state;
      });
    }
  }
}

}  // namespace charly::core::runtime
