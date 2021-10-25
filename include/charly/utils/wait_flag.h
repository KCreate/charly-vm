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
#include <cstdint>
#include <mutex>
#include <thread>

#include "charly/atomic.h"

#pragma once

namespace charly::utils {

class WaitFlag {
public:
  WaitFlag(std::mutex& mutex) : m_mutex(mutex), m_state(false) {}

  bool state() const {
    return m_state;
  }

  void wait() {
    std::unique_lock<std::mutex> locker(m_mutex);
    m_cv.wait(locker, [&]() -> bool {
      return m_state;
    });
  }

  bool signal() {
    bool first = false;
    {
      std::unique_lock<std::mutex> locker(m_mutex);
      first = m_state.cas(false, true);
    }

    m_cv.notify_all();

    return first;
  }

  bool reset() {
    bool first = false;
    {
      std::unique_lock<std::mutex> locker(m_mutex);
      first = m_state.cas(true, false);
    }

    m_cv.notify_all();

    return first;
  }

private:
  std::mutex& m_mutex;
  std::condition_variable m_cv;
  atomic<bool> m_state;
};

}  // namespace charly::utils
