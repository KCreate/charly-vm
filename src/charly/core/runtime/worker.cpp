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

using namespace boost::context::detail;
using namespace std::chrono_literals;

void Worker::main() {

  // initialize worker and wait for the start signal from the runtime
  g_worker = this;
  g_worker->set_gc_region(GarbageCollector::instance->allocate_heap_region());
  pause();
  assert(g_worker->state() == Worker::State::Running);

  safeprint("worker %: started", m_id);

  for (;;) {
    HeapFiber* fiber = Scheduler::instance->get_ready_fiber();
    if (fiber == nullptr) {
      safeprint("worker %: no ready fiber", m_id);
      break;
    }

    assert(fiber->m_fiber->m_status == Fiber::Status::Ready);
    assert(fiber->m_fiber->m_stack.bottom());
    assert(fiber->m_fiber->m_stack.size());

    m_context_switch_counter++;

    m_current_fiber.assert_cas(nullptr, fiber);
    // safeprint("worker %: scheduling fiber %", m_id, fiber->m_fiber->m_id);
    fiber->m_fiber->m_status.assert_cas(Fiber::Status::Ready, Fiber::Status::Running);
    fiber->m_fiber->m_scheduled_at = Scheduler::current_timestamp();
    fiber->m_fiber->jump_context(nullptr);
    fiber->m_fiber->m_scheduled_at = 0;
    // safeprint("worker %: fiber % yielded", m_id, fiber->m_fiber->m_id);
    m_current_fiber.assert_cas(fiber, nullptr);

    if (fiber->m_fiber->m_status == Fiber::Status::Running) {
      fiber->m_fiber->m_status.assert_cas(Fiber::Status::Running, Fiber::Status::Ready);
      Scheduler::instance->schedule_fiber(fiber);
    }
  }

  g_worker->exit();
}

void Worker::join() {
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void Worker::idle() {
  safeprint("worker % -> idle mode", m_id);
  m_state.assert_cas(Worker::State::Running, Worker::State::Idle);
  m_resume_from_idle.store(false);

  Scheduler::instance->register_idle(this);

  std::unique_lock<std::mutex> locker(m_mutex);
  m_idle_cv.wait(locker, [&] {
    return m_resume_from_idle.load() == true;
  });

  m_state.assert_cas(Worker::State::Idle, Worker::State::Running);
  safeprint("worker % -> running mode", m_id);
}

void Worker::resume_from_idle() {
  m_resume_from_idle.assert_cas(false, true);
  m_idle_cv.notify_all();
}

void Worker::exit() {
  m_state.assert_cas(Worker::State::Running, Worker::State::Exited);
  m_safe_cv.notify_one();
}

void Worker::set_gc_region(HeapRegion* region) {
  m_active_region.assert_cas(nullptr, region);
}

void Worker::clear_gc_region() {
  m_active_region.assert_cas(m_active_region.load(), nullptr);
}

void Worker::checkpoint() {
  while (m_pause_request.load()) {
    assert(m_state.load() == State::Running);
    safeprint("parking worker %", m_id);
    pause();
    safeprint("unparking worker %", m_id);
  }
}

void Worker::wait_for_checkpoint() {
  while (m_state.load() == Worker::State::Running) {

    // spin and wait for the state to change
    for (uint32_t spin = 0; spin < 10000; spin++) {
      if (m_state.load() != Worker::State::Running) {
        return;
      }
    }

    // wait for the worker to leave running mode
    {
      std::unique_lock<std::mutex> locker(m_mutex);
      m_safe_cv.wait(locker, [&] {
        return m_state.load() != Worker::State::Running;
      });
    }
  }
}

void Worker::resume() {
  m_state.assert_cas(Worker::State::Paused, Worker::State::Running);
  m_running_cv.notify_all();
}

void Worker::pause() {
  m_state.assert_cas(Worker::State::Running, Worker::State::Paused);
  m_safe_cv.notify_one();

  while (m_state.load() == Worker::State::Paused) {
    std::unique_lock<std::mutex> locker(m_mutex);
    m_running_cv.wait(locker, [&] {
      return m_state.load() == Worker::State::Running;
    });
  }
}

void Worker::enter_native() {
  // safeprint("worker % -> native mode", m_id);
  m_state.assert_cas(Worker::State::Running, Worker::State::Native);
  m_safe_cv.notify_one();
}

void Worker::leave_native() {
  // safeprint("worker % -> running mode", m_id);
  m_state.assert_cas(Worker::State::Native, Worker::State::Running);
  Scheduler::instance->checkpoint();
}

void Worker::fiber_pause() {
  Fiber* fiber = m_current_fiber.load()->m_fiber;
  assert(fiber->m_status.load() == Fiber::Status::Running);
  fiber->m_status.assert_cas(Fiber::Status::Running, Fiber::Status::Paused);
  jump_context(nullptr);
}

void Worker::fiber_reschedule() {
  Fiber* fiber = m_current_fiber.load()->m_fiber;
  assert(fiber->m_status.load() == Fiber::Status::Running);
  jump_context(nullptr);
  assert(fiber->m_status.load() == Fiber::Status::Running);
}

void Worker::fiber_exit() {
  Fiber* fiber = m_current_fiber.load()->m_fiber;
  fiber->m_status.assert_cas(Fiber::Status::Running, Fiber::Status::Exited);
  jump_context(nullptr);
  __builtin_unreachable();
}

void* Worker::jump_context(void* argument) {
  transfer_t transfer = jump_fcontext(m_context, argument);
  m_context = transfer.fctx;
  return transfer.data;
}

}  // namespace charly::core::runtime
