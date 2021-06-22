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
#include <cstdint>
#include <thread>
#include <mutex>

#include "charly/atomic.h"

#pragma once

namespace charly::utils {

// forward declarations
class GarbageCollector;

template <typename T>
class SharedQueue {
  friend class GarbageCollector;
public:
  SharedQueue() {
    QueueEntry* dummy = new QueueEntry();
    m_head = dummy;
    m_tail = dummy;
  }

  // pop a value from the queue
  // immediately returns false if the queue is empty
  bool pop(T* result) {
    std::unique_lock<std::mutex> locker(m_read_mutex);

    if (m_closed) {
      return false;
    }

    QueueEntry* head = m_head;
    assert(head && head->is_dummy);

    // wait for value to be pushed or queue to be closed
    if (head->next == nullptr) {
      return false;
    }

    *result = std::move(head->next->data);
    head->next = head->next->next;
    m_element_count--;
    return true;
  }

  // pop a value from the queue
  // if the queue is empty, this method will wait until a value gets pushed into
  // the queue or the queue is closed, in which case it returns false
  bool pop_wait(T* result) {
    std::unique_lock<std::mutex> locker(m_read_mutex);

    if (m_closed) {
      return false;
    }

    QueueEntry* head = m_head;
    assert(head && head->is_dummy);

    // wait for value to be pushed or queue to be closed
    if (head->next == nullptr) {
      m_write_cv.wait(locker, [&] {
        return head->next != nullptr || m_closed;
      });

      if (m_closed) {
        return false;
      }
    }

    *result = std::move(head->next->data);
    head->next = head->next->next;
    m_element_count--;
    return true;
  }

  // push a value into the queue
  void push(T value) {
    {
      std::unique_lock<std::mutex> locker(m_write_mutex);

      QueueEntry* entry = new QueueEntry(value);

      assert(m_tail);
      m_tail->next = entry;
      m_tail = entry;
      m_element_count++;
    }
    m_write_cv.notify_one();
  }

  // closes the queue
  //
  // outstanding pops return and fail
  void close() {
    {
      std::unique_lock<std::mutex> read_locker(m_read_mutex);
      std::unique_lock<std::mutex> write_locker(m_write_mutex);
      m_closed = true;
    }
    m_write_cv.notify_all();
  }

  // returns the current amount of items in the queue
  size_t size() {
    return m_element_count;
  }

private:
  struct QueueEntry {
    QueueEntry() : next(nullptr), is_dummy(true) {}
    QueueEntry(T data) : data(data), next(nullptr), is_dummy(false) {}
    QueueEntry(T&& data) : data(std::forward<T>(data)), next(nullptr), is_dummy(false) {}

    T data;
    QueueEntry* next;
    bool is_dummy;
  };

  QueueEntry* m_head = nullptr;
  QueueEntry* m_tail = nullptr;

  std::mutex m_read_mutex;
  std::mutex m_write_mutex;
  std::condition_variable m_write_cv;

  charly::atomic<size_t> m_element_count = 0;

  bool m_closed = false;
};

}  // namespace charly::utils
