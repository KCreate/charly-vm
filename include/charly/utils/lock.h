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

#include <condition_variable>
#include <cstdint>
#include <functional>  // std::hash
#include <list>
#include <thread>  // std::hardware_concurrency
#include <vector>

#include "charly/atomic.h"

#pragma once

namespace charly::utils {

struct ParkingLotThreadData {
  bool should_park;
  std::mutex parking_lock;
  std::condition_variable parking_condition;
  static ParkingLotThreadData* get_local_thread_data();
};

struct ParkingLotThreadQueue {
  using QueueEntry = std::tuple<uintptr_t, ParkingLotThreadData*>;
  std::mutex mutex;
  std::list<QueueEntry> queue;

  void push(ParkingLotThreadData* data, uintptr_t address);
  ParkingLotThreadData* pop(uintptr_t address);
  bool empty() const;
};

class ParkingLot {
public:
  ParkingLot();

  struct UnparkResult {
    bool unparked_thread = false;
    bool queue_is_empty = false;
  };

  static bool park(uintptr_t address, const std::function<bool()>& validation);
  static void unpark_one(uintptr_t address, const std::function<void(UnparkResult)>& callback);
  static UnparkResult unpark_one(uintptr_t address);

private:
  static constexpr size_t kInitialBucketCount = 1;
  using ThreadQueueTable = std::vector<atomic<ParkingLotThreadQueue*>>;

  bool park_impl(uintptr_t address, const std::function<bool()>& validation);
  void unpark_one_impl(uintptr_t address, const std::function<void(UnparkResult)>& callback);

  static ParkingLotThreadQueue* get_queue_for_address(ThreadQueueTable* table, uintptr_t address);

private:
  atomic<ThreadQueueTable*> m_table;
  std::mutex m_old_tables_mutex;
  std::vector<ThreadQueueTable*> m_old_tables;
};

enum LockState : uint8_t {
  kFreeLock = 0,
  kIsLocked = 1,
  kHasParked = 2
};

// allows threads to barge in and steal the lock away from a parked thread
class TinyLock {
public:
  TinyLock();

  void lock();
  void unlock();

  bool is_locked() const {
    return (m_state & kIsLocked);
  }

  TinyLock& operator=(uint8_t value) {
    m_state = value;
    return *this;
  }

private:
  atomic<uint8_t> m_state;
};
static_assert(sizeof(TinyLock) == 1);

}  // namespace charly::utils
