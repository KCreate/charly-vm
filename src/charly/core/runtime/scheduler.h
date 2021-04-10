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

#include <condition_variable>
#include <thread>
#include <vector>

#include <boost/context/detail/fcontext.hpp>

#include "charly/charly.h"
#include "charly/atomic.h"

#include "charly/core/runtime/worker.h"

#pragma once

namespace charly::core::runtime {

// the amount of concurrent threads the underlying machine can run
// this is more or less the amount of logical cpu cores available
static const uint64_t kHardwareConcurrency = std::thread::hardware_concurrency();

class Scheduler {
public:
  Scheduler() : m_state(State::Created) {}

  // initialize the global scheduler object
  static void initialize();

  // pointer to the global scheduler object
  inline static Scheduler* instance = nullptr;

  // start the scheduler
  void start();

  // scheduler checkpoint
  void checkpoint();

  // pause all worker threads at a checkpoint
  void pause();

  // resume worker threads
  void resume();

private:

  // initialize the fiber worker threads
  void init_workers();

private:
  enum class State : uint8_t {
    Created,
    Running,  // scheduler is running normally
    Paused    // scheduler is paused
  };

  // fiber workers
  std::vector<Worker*> m_fiber_workers;
  std::mutex m_fiber_workers_m;

  // scheduler state
  charly::atomic<State> m_state;

  // cv and accompanying mutex to track changes to scheduler state
  std::mutex m_state_m;
  std::condition_variable m_state_cv;
};

}  // namespace charly::core::runtime
