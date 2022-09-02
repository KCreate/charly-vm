/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

#include "charly/core/runtime/worker.h"
#include "charly/core/runtime/runtime.h"

namespace charly::core::runtime {

using namespace std::chrono_literals;

static atomic<size_t> worker_id_counter = 0;

Worker::Worker(Runtime* runtime) :
  m_id(worker_id_counter++), m_os_thread_handle(&Worker::scheduler_loop, this, runtime), m_runtime(runtime) {}

bool Worker::is_heap_safe_mode(State state) {
  switch (state) {
    case State::Idle:
    case State::Native:
    case State::WorldStopped:
    case State::Exited: {
      return true;
    }
    default: {
      return false;
    }
  }
}

size_t Worker::id() const {
  return m_id;
}

Worker::State Worker::state() const {
  return m_state;
}

size_t Worker::context_switch_counter() const {
  return m_context_switch_counter;
}

size_t Worker::rand() {
  return m_random_device.get();
}

fcontext_t& Worker::context() {
  return m_context;
}

void Worker::set_context(fcontext_t& context) {
  m_context = context;
}

bool Worker::has_stop_flag() const {
  return m_stop_flag;
}

bool Worker::has_idle_flag() const {
  return m_idle_flag;
}

bool Worker::set_stop_flag() {
  return m_stop_flag.cas(false, true);
}

bool Worker::set_idle_flag() {
  return m_idle_flag.cas(false, true);
}

bool Worker::clear_stop_flag() {
  return m_stop_flag.cas(true, false);
}

bool Worker::clear_idle_flag() {
  return m_idle_flag.cas(true, false);
}

Thread* Worker::thread() const {
  return m_thread;
}

Processor* Worker::processor() const {
  return m_processor;
}

Runtime* Worker::runtime() const {
  return m_runtime;
}

void Worker::set_processor(Processor* processor) {
  m_processor = processor;
}

void Worker::join() {
  State now_state = state();
  if (now_state != State::Exited) {
    while (now_state != State::Exited) {
      now_state = wait_for_state_change(now_state);
    }
  }

  m_os_thread_handle.join();
}

bool Worker::wake() {
  bool first_to_wake;
  {
    std::unique_lock locker(m_mutex);
    first_to_wake = clear_idle_flag();
  }
  m_idle_cv.notify_one();

  return first_to_wake;
}

void Worker::idle() {
  set_idle_flag();
  assert_change_state(State::Acquiring, State::Idle);

  {
    std::unique_lock locker(m_mutex);
    auto idle_duration = 1ms * m_idle_sleep_duration;
    m_idle_cv.wait_for(locker, idle_duration, [&] {
      return !has_idle_flag();
    });

    clear_idle_flag();
  }

  assert_change_state(State::Idle, State::Acquiring);
}

void Worker::checkpoint() {
  if (has_stop_flag()) {
    State old_state = state();
    assert_change_state(old_state, State::WorldStopped);
    {
      std::unique_lock locker(m_mutex);
      m_stw_cv.wait(locker, [&] {
        return !has_stop_flag();
      });
    }
    assert_change_state(State::WorldStopped, old_state);
  }
}

void Worker::stop_the_world() {
  {
    std::unique_lock locker(m_mutex);
    set_stop_flag();
  }

  // wait for the worker to enter into a safe state
  State now_state = state();
  while (!Worker::is_heap_safe_mode(now_state)) {
    now_state = wait_for_state_change(now_state);
  }
}

void Worker::start_the_world() {
  {
    std::unique_lock locker(m_mutex);
    clear_stop_flag();
  }
  m_stw_cv.notify_one();
}

void Worker::enter_native() {
  assert_change_state(State::Running, State::Native);
}

void Worker::exit_native() {
  assert_change_state(State::Native, State::Running);
}

bool Worker::change_state(State expected_state, State new_state) {
  bool changed;
  {
    std::unique_lock locker(m_mutex);
    changed = m_state.cas(expected_state, new_state);
  }
  m_state_cv.notify_all();

  return changed;
}

void Worker::assert_change_state(State expected_state, State new_state) {
  bool result = change_state(expected_state, new_state);
  CHECK(result);
}

Worker::State Worker::wait_for_state_change(State old_state) {
  std::unique_lock locker(m_mutex);
  m_state_cv.wait(locker, [&] {
    return state() != old_state;
  });
  return state();
}

void Worker::scheduler_loop(Runtime* runtime) {
  runtime->wait_for_initialization();
  Scheduler* scheduler = runtime->scheduler();

  assert_change_state(State::Created, State::Acquiring);

  while (!runtime->wants_exit()) {
    checkpoint();

    // attempt to acquire idle processor from scheduler
    if (!scheduler->acquire_processor_for_worker(this)) {
      increase_sleep_duration();
      idle();
      continue;
    }

    Processor* proc = m_processor;

    assert_change_state(State::Acquiring, State::Scheduling);
    while (!runtime->wants_exit()) {
      checkpoint();

      // fetch next ready thread
      Thread* thread = proc->get_ready_thread();
      if (thread == nullptr) {
        increase_sleep_duration();
        break;
      }

      reset_sleep_duration();

      // context switch into the thread
      assert_change_state(State::Scheduling, State::Running);
      m_context_switch_counter++;
      m_thread = thread;
      thread->context_switch(this);
      checkpoint();
      m_thread = nullptr;
      assert_change_state(State::Running, State::Scheduling);

      switch (thread->state()) {
        case Thread::State::Waiting: {
          continue;
        }
        case Thread::State::Ready: {
          scheduler->schedule_thread(thread, proc);
          continue;
        }
        case Thread::State::Exited: {
          scheduler->recycle_thread(thread);
          continue;
        }
        case Thread::State::Aborted: {
          int32_t exit_code = thread->exit_code();
          scheduler->recycle_thread(thread);
          runtime->abort(exit_code);
          continue;
        }
        default: {
          FAIL("unexpected worker state");
        }
      }
    }
    assert_change_state(State::Scheduling, State::Acquiring);

    // release processor back to scheduler and go into idle mode
    scheduler->release_processor_from_worker(this);
    idle();
  }

  assert_change_state(State::Acquiring, State::Exited);
}

void Worker::increase_sleep_duration() {
  uint32_t next_duration = m_idle_sleep_duration * 2;
  if (next_duration > kWorkerMaximumIdleSleepDuration) {
    next_duration = kWorkerMaximumIdleSleepDuration;
  }
  m_idle_sleep_duration = next_duration;
}

void Worker::reset_sleep_duration() {
  m_idle_sleep_duration = 10;
}

}  // namespace charly::core::runtime
