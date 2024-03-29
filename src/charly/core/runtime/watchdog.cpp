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

#include <chrono>

#include "charly/charly.h"
#include "charly/core/runtime/runtime.h"
#include "charly/core/runtime/watchdog.h"

namespace charly::core::runtime {

using namespace std::chrono_literals;

WatchDog::WatchDog(Runtime* runtime) :
  m_runtime(runtime), m_thread(std::thread(&WatchDog::main, this)) {
}

WatchDog::~WatchDog() {
  DCHECK(!m_thread.joinable());
}

void WatchDog::join() {
  m_thread.join();
}

void WatchDog::main() {
  m_runtime->wait_for_initialization();
  auto* scheduler = m_runtime->scheduler();

  while (!m_runtime->wants_exit()) {
    m_clock = get_steady_timestamp();

    for (Worker* worker : scheduler->m_workers) {
      if (worker->state() == Worker::State::Running) {
        if (auto* thread = worker->thread()) {

          // scheduler threads cannot be interrupted since there is no thread for them to yield to
          if (thread->is_scheduler()) {
            continue;
          }

          size_t last_scheduled_at = thread->last_scheduled_at();
          if (last_scheduled_at >= Thread::kFirstValidScheduledAtTimestamp && last_scheduled_at < m_clock) {
            size_t execution_time = m_clock - last_scheduled_at;
            bool has_exceeded_timeslice = execution_time >= kThreadTimeslice;
            if (has_exceeded_timeslice) {
              thread->set_last_scheduled_at(last_scheduled_at, Thread::kShouldYieldToSchedulerTimestamp);
            }
          }
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(kWatchDogWakeupInterval));
  }
}

}  // namespace charly::core::runtime
