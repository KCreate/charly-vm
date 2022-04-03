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

#include "charly/utils/lock.h"

namespace charly::utils {

static ParkingLot g_parking_lot;

ParkingLotThreadData* ParkingLotThreadData::get_local_thread_data() {
  thread_local ParkingLotThreadData data;
  return &data;
}

void ParkingLotThreadQueue::push(ParkingLotThreadData* data, uintptr_t address) {
  this->queue.emplace_back(address, data);
}

ParkingLotThreadData* ParkingLotThreadQueue::pop(uintptr_t address) {
  auto begin = this->queue.begin();
  auto end = this->queue.end();
  auto result = std::find_if(begin, end, [&](const auto& entry) {
    return std::get<uintptr_t>(entry) == address;
  });

  if (result != end) {
    auto data = std::get<ParkingLotThreadData*>(*result);
    this->queue.erase(result);
    return data;
  }

  return nullptr;
}

bool ParkingLotThreadQueue::empty() const {
  return this->queue.empty();
}

bool ParkingLot::park(uintptr_t address, const std::function<bool()>& validation) {
  return g_parking_lot.park_impl(address, validation);
}

void ParkingLot::unpark_one(uintptr_t address, const std::function<void(UnparkResult)>& callback) {
  g_parking_lot.unpark_one_impl(address, callback);
}

ParkingLot::UnparkResult ParkingLot::unpark_one(uintptr_t address) {
  UnparkResult rv;

  ParkingLot::unpark_one(address, [&](UnparkResult result) {
    rv = result;
  });

  return rv;
}

ParkingLot::ParkingLot() : m_table(new ThreadQueueTable(kInitialBucketCount)) {}

bool ParkingLot::park_impl(uintptr_t address, const std::function<bool()>& validation) {
  ParkingLotThreadData* me = ParkingLotThreadData::get_local_thread_data();
  ParkingLotThreadQueue* queue;
  for (;;) {
    ThreadQueueTable* table = m_table.load();
    DCHECK(table);

    queue = get_queue_for_address(table, address);
    queue->mutex.lock();

    // repeat lock acquisition in case the global thread queue table was replaced
    if (table != m_table) {
      queue->mutex.unlock();
      continue;
    }

    break;
  }

  if (!validation()) {
    queue->mutex.unlock();
    return false;
  }

  me->should_park = true;
  queue->push(me, address);
  queue->mutex.unlock();

  {
    std::unique_lock<std::mutex> lock(me->parking_lock);
    while (me->should_park) {
      me->parking_condition.wait(lock);
    }
  }

  DCHECK(!me->should_park);

  return true;
}

void ParkingLot::unpark_one_impl(uintptr_t address, const std::function<void(UnparkResult)>& callback) {
  ParkingLotThreadQueue* queue;
  for (;;) {
    ThreadQueueTable* table = m_table.load();
    DCHECK(table);

    queue = get_queue_for_address(table, address);
    queue->mutex.lock();

    // repeat lock acquisition in case the global thread queue table was replaced
    if (table != m_table) {
      queue->mutex.unlock();
      continue;
    }

    break;
  }

  ParkingLotThreadData* thread = queue->pop(address);

  callback({ .unparked_thread = thread != nullptr, .queue_is_empty = queue->empty() });

  queue->mutex.unlock();

  if (thread) {
    {
      std::lock_guard<std::mutex> lock(thread->parking_lock);
      thread->should_park = false;
    }

    thread->parking_condition.notify_one();
  }
}

ParkingLotThreadQueue* ParkingLot::get_queue_for_address(ThreadQueueTable* table, uintptr_t address) {
  size_t address_hash = std::hash<uintptr_t>()(address);
  size_t bucket_index = address_hash % table->size();

  // allocate and atomically replace queue on first access
  atomic<ParkingLotThreadQueue*>& queue_ptr = table->at(bucket_index);
  ParkingLotThreadQueue* queue = queue_ptr.load();
  if (queue == nullptr) {
    auto new_queue = new ParkingLotThreadQueue();
    if (queue_ptr.cas(nullptr, new_queue)) {
      queue = new_queue;
    } else {
      queue = queue_ptr.load();
      delete new_queue;
    }
  }

  return queue;
}

TinyLock::TinyLock() : m_state(kFreeLock) {}

void TinyLock::lock() {
  for (;;) {
    uint8_t state = m_state;

    // fast path in case the lock is unlocked but there are still threads parked
    // this can be the case if another thread just unlocked the lock, called the
    // unpark_one method but that method hasn't run yet
    if (!(state & kIsLocked) && m_state.cas_weak(state, state | kIsLocked)) {
      return;
    }

    // set parked bit
    // no check is necessary since the park validation callback
    // checks this condition again
    uint8_t expected = kIsLocked;
    m_state.cas_weak(expected, kIsLocked | kHasParked);

    ParkingLot::park(m_state.address(), [this]() -> bool {
      return m_state.load() == (kIsLocked | kHasParked);
    });
  }
}

void TinyLock::unlock() {
  DCHECK(m_state & kIsLocked, "expected lock to be locked");

  // fast path in case lock is uncontended
  uint8_t expected = kIsLocked;
  if (m_state.compare_exchange_weak(expected, 0)) {
    return;
  }

  // pass control to contending threads
  ParkingLot::unpark_one(m_state.address(), [this](ParkingLot::UnparkResult result) {
    if (result.queue_is_empty) {
      m_state.store(0);
    } else {
      m_state.store(kHasParked);
    }
  });
}

}  // namespace charly::utils
