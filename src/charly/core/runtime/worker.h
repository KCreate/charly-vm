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

#include "charly/charly.h"
#include "charly/atomic.h"

#pragma once

namespace charly::core::runtime {

class Worker;
inline thread_local Worker* g_worker = nullptr;
class Worker {
public:
  Worker() : m_thread(&Worker::main, this), m_state(State::Created) {
    static uint64_t id_counter = 1;
    this->id = id_counter++;
  }

  uint64_t id;

  // worker main method
  void main();

  // start the worker thread
  void start();

  // represents the state the worker is currently in
  enum class State : uint8_t {
    Created,      // worker has been created, but hasn't started yet
    Running,      // worker is running
    Native,       // worker is running native code
    Paused,       // worker is paused
    Exited        // worker has exited
  };

  // change the state of this worker
  // calls into to scheduler to request state change
  void change_state(Worker::State state);

  void wait_for_state_change(Worker::State old_state);

  Worker::State state() const {
    return m_state.load();
  }

private:
  std::thread m_thread;
  charly::atomic<State> m_state;
  std::mutex m_mutex;
  std::condition_variable m_cv;
};

}  // namespace charly::core::runtime
