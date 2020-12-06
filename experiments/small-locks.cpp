#include <cstdint>
#include <iostream>
#include <cassert>

#include <vector>
#include <queue>
#include <unordered_map>

#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>

#include <atomic>

/*
 * Basic idea and code inspiration taken from:
 * https://webkit.org/blog/6161/locking-in-webkit/
 * */

static std::recursive_mutex mutex;
void safeprint_impl(const char* format) {
  std::lock_guard<std::recursive_mutex> locker(mutex);
  std::cout << format;
}

template<typename T, typename... Targs>
void safeprint_impl(const char* format, T value, Targs... Fargs) {
  std::lock_guard<std::recursive_mutex> locker(mutex);

  while (*format != '\0') {
    if (*format == '%') {
        std::cout << value;
        safeprint_impl(format+1, Fargs...);
        return;
    }
    std::cout << *format;

    format++;
  }
}

template<typename... Targs>
void safeprint(const char* format, Targs... Fargs) {
  std::lock_guard<std::recursive_mutex> locker(mutex);
  std::cout << std::this_thread::get_id() << ": ";
  safeprint_impl(format, Fargs...);
  std::cout << "\n";
  std::cout << std::flush;
}

struct ThreadData {
  bool                    should_park;
  std::mutex              parking_lock;
  std::condition_variable parking_condition;

  static ThreadData& current_thread_data() {
    thread_local ThreadData data;
    return data;
  }
};

class ThreadQueue {
public:
  ThreadQueue() {
    m_head = nullptr;
    m_tail = nullptr;
  }
  ThreadQueue(ThreadQueue&) = delete;
  ThreadQueue(ThreadQueue&&) = delete;

  void push(ThreadData* thread_data, std::atomic<uint8_t>* address) {
    std::lock_guard<std::mutex> lock(m_queue_lock);

    QueueEntry* entry = new QueueEntry();
    entry->data = thread_data;
    entry->address = address;
    entry->next = nullptr;

    if (m_tail) {
      m_tail->next = entry;
      m_tail = entry;
    } else {
      m_head = entry;
      m_tail = entry;
    }
  }

  ThreadData* pop(std::atomic<uint8_t>* address) {
    std::lock_guard<std::mutex> lock(m_queue_lock);

    if (!m_head)
      return nullptr;

    // check queue head for match
    if (m_head->address == address) {
      QueueEntry* head = m_head;
      ThreadData* data = head->data;

      m_head = head->next;
      delete head;

      if (!m_head)
        m_tail = nullptr;

      return data;
    }

    // search for matching address
    QueueEntry* previous = m_head;
    for (QueueEntry* current = m_head->next; current != nullptr; current = current->next) {
      if (current->address == address) {
        previous->next = current->next;

        if (current == m_tail)
          m_tail = previous;

        ThreadData* data = current->data;
        delete current;

        return data;
      }
    }

    return nullptr;
  }

  bool empty() {
    if (!m_head && !m_tail) {
      return true;
    }
    return false;
  }

private:
  struct QueueEntry {
    ThreadData*           data;
    std::atomic<uint8_t>* address;
    QueueEntry*           next;
  };

  QueueEntry* m_head;
  QueueEntry* m_tail;
  std::mutex  m_queue_lock;
};

class ParkingLot {
public:
  struct UnparkResult {
    bool unparked_thread;
    bool queue_is_empty;
  };

  static bool park(std::atomic<uint8_t>* address, std::function<bool()> validation) {
    return ParkingLot::get_reference().park_impl(address, validation);
  }

  static void unpark_one(std::atomic<uint8_t>* address, std::function<void(UnparkResult)> callback) {
    ParkingLot::get_reference().unpark_one_impl(address, callback);
  }

  static UnparkResult unpark_one(std::atomic<uint8_t>* address) {
    UnparkResult rv;

    ParkingLot::unpark_one(address, [&](UnparkResult result) {
      rv = result;
    });

    return rv;
  }

private:
  bool park_impl(std::atomic<uint8_t>* address, std::function<bool()> validation) {
    ThreadData& me = ThreadData::current_thread_data();

    {
      std::unique_lock<std::mutex> lock(m_hashtable_mutex);

      if (!validation()) {
        return false;
      }

      me.should_park = true;

      ThreadQueue* queue = get_queue(address);
      queue->push(&me, address);
    }

    {
      std::unique_lock<std::mutex> locker(me.parking_lock);
      while (me.should_park) {
        me.parking_condition.wait(locker);
      }
    }

    assert(!me.should_park);

    return true;
  }

  void unpark_one_impl(std::atomic<uint8_t>* address, std::function<void(UnparkResult)> callback) {
    std::unique_lock<std::mutex> lock(m_hashtable_mutex);

    ThreadQueue* queue = get_queue(address);
    ThreadData* thread = queue->pop(address);

    callback({
      .unparked_thread = thread != nullptr,
      .queue_is_empty = queue->empty()
    });

    lock.unlock();

    if (thread) {
      {
        std::lock_guard<std::mutex> lock(thread->parking_lock);
        thread->should_park = false;
      }

      thread->parking_condition.notify_one();
    }
  }

  ThreadQueue* get_queue(std::atomic<uint8_t>* address) {
    if (m_hashtable.count(address)) {
      return m_hashtable.at(address);
    }

    m_hashtable.insert({ address, new ThreadQueue() });
    return m_hashtable.at(address);
  }

  static ParkingLot& get_reference() {
    static ParkingLot lot;
    return lot;
  }

  std::mutex m_hashtable_mutex;
  std::unordered_map<std::atomic<uint8_t>*, ThreadQueue*> m_hashtable;
};

class BargingLock {
public:
  BargingLock() {
    m_state.store(0);
  }

  void lock() {
    for (;;) {
      uint8_t current_state = m_state.load();

      // fast path in case the lock is unlocked but there are still threads parked
      // this can be the case if another thread just unlocked the lock, called the
      // unpark_one method but that method hasn't yet
      if (!(current_state & is_locked)
          && m_state.compare_exchange_weak(current_state, current_state | is_locked)) {
        return;
      }

      // set parked bit
      // no check is necessary since the park validation callback
      // checks this condition again
      uint8_t expected = is_locked;
      m_state.compare_exchange_weak(expected, is_locked | has_parked);

      ParkingLot::park(
        &m_state,
        [this]() -> bool {
          return m_state.load() == (is_locked | has_parked);
        }
      );
    }
  }

  void unlock() {

    // fast path in case lock is uncontended
    uint8_t expected = is_locked;
    if (m_state.compare_exchange_weak(expected, 0))
      return;

    // pass control to contending threads
    ParkingLot::unpark_one(
      &m_state,
      [this](ParkingLot::UnparkResult result) {
        if (result.queue_is_empty) {
          m_state.store(0);
        } else {
          m_state.store(has_parked);
        }
      }
    );
  }

private:
  static const uint8_t is_locked = 1;
  static const uint8_t has_parked = 2;

  std::atomic<uint8_t> m_state;
};

/*
 * Ensures fair FIFO access to the lock
 * */
class FairLock {
public:
  FairLock() {
    m_state.store(0);
  }

  void lock() {
    for (;;) {

      // fast path in case lock is uncontended
      uint8_t expected = 0;
      if (m_state.compare_exchange_weak(expected, is_locked))
        return;

      // set parked bit
      // no check is necessary since the park validation callback
      // checks this condition again
      expected = is_locked;
      m_state.compare_exchange_weak(expected, is_locked | has_parked);

      bool could_park = ParkingLot::park(
        &m_state,
        [this]() -> bool {
          return m_state.load() == (is_locked | has_parked);
        }
      );

      // if we successfully parked and another thread
      // just woke us up, we now own the lock
      if (could_park)
        return;
    }
  }

  void unlock() {

    // fast path if there are no parked threads
    uint8_t expected = is_locked;
    if (m_state.compare_exchange_weak(expected, 0))
      return;

    // hand off control to another thread
    // we do not have to check wether a thread has actually been unparked
    // since at this point there has to be a thread in the queue
    ParkingLot::UnparkResult result = ParkingLot::unpark_one(&m_state);
    if (result.unparked_thread) {
      if (result.queue_is_empty) {
        m_state.store(is_locked);
      } else {
        m_state.store(is_locked | has_parked);
      }
    } else {
      m_state.store(0);
    }
  }

private:
  static const uint8_t is_locked = 1;
  static const uint8_t has_parked = 2;

  std::atomic<uint8_t> m_state;
};

static BargingLock locker;
const int ITERATIONS = 1000;
const int THREADC = 16;

static int count = 0;

int main() {
  std::thread* threads[THREADC];

  for (int i = 0; i < THREADC; i++) {
    threads[i] = new std::thread([](int id) {
      safeprint("\% started processing", id);
      auto start = std::chrono::high_resolution_clock::now();

      {
        for (int i = 0; i < ITERATIONS; i++) {
          std::lock_guard<BargingLock> lock(locker);
          count++;
          std::this_thread::sleep_for(std::chrono::microseconds(1));
          //std::this_thread::yield();
        }
      }

      auto stop = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
      safeprint("\% finished processing in % milliseconds", id, duration.count());
    }, i);
  }

  for (int i = 0; i < THREADC; i++) {
    threads[i]->join();
    delete threads[i];
  }

  safeprint("counter = %", count);
  return 0;
}
