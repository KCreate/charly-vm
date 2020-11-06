/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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

// thread synchronisation
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

// stdlib
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <functional>

// data structures
#include <vector>
#include <list>

void safeprint(std::string output, int payload = -1);

class TaskQueue {
public:
  void push(int value) {
    std::unique_lock<std::mutex> lk(m_queue_mutex);
    safeprint("pushing task", (int)value);
    m_queue.push_back(value);
    m_queue_cv.notify_one();
  }

  int pop() {
    std::unique_lock<std::mutex> lk(m_queue_mutex);

    // wait for items to arrive
    if (m_queue.size() == 0) {
      safeprint("waiting for task");
      m_queue_cv.wait(lk, [&] { return m_queue.size() > 0; });
    }

    // pop item off queue
    int front = m_queue.front();
    m_queue.pop_front();
    safeprint("popping task", (int)front);
    return front;
  }

private:
  std::list<int>            m_queue;
  std::mutex              m_queue_mutex;
  std::condition_variable m_queue_cv;
};

int rand_seed = 0;
int get_random_number() {
  static std::mutex rand_mutex;
  std::unique_lock<std::mutex> lk(rand_mutex);
  std::srand(rand_seed++);
  return std::rand();
}

struct WorkerThread {
  enum class State : uint8_t {
    Idle,         // doing nothing, waiting for work
    Working,      // regular working mode
    Untracked     // thread does not need to be paused
  };

  enum class GCState : uint8_t {
    None,         // no interaction with gc
    RequestGC,    // requesting garbage collection
    WaitingForGC, // waiting for gc to finish
    ExecutingGC   // executing garbage collection
  };

  uint32_t                id;                       // id of this thread
  std::thread*            thread_handle = nullptr;  // native thread handle
  std::atomic<State>      state = State::Idle;      // state of this worker thread
  std::atomic<GCState>    gc_state = GCState::None; // current gc interaction state
  std::mutex              condition_mutex;          // mutex used to protect the condition variable
  std::condition_variable condition;                // used to wait for status changes
};

class Coordinator {
public:

  // park this thread and wait for gc to complete, if active
  void sync() {
    while (m_state.load(std::memory_order_acquire) == State::GarbageCollection) {
      WorkerThread* thandle = this->get_current_worker_thread();
      safeprint("parking thread");

      // mark as waiting for gc and notify the controller thread
      thandle->gc_state.store(WorkerThread::GCState::WaitingForGC, std::memory_order_release);
      thandle->condition.notify_one();

      // wait for the garbage collection phase to be over
      //
      // TODO: should we spin on this?
      std::unique_lock<std::mutex> lk(m_state_mutex);
      m_state_cv.wait(lk, [&] {
        return m_state.load(std::memory_order_acquire) == State::Working;
      });

      // unmark as waiting for gc
      thandle->gc_state.store(WorkerThread::GCState::None, std::memory_order_release);
      safeprint("unparking thread");
    }
  }

  // request a gc pause, parks active threads
  bool request_gc() {
    State old = State::Working;
    m_state.compare_exchange_strong(old, State::GarbageCollection);

    if (old != State::Working) {
      this->sync();
      return false;
    }

    // wait for all registered threads to park
    WorkerThread* current_thread = this->get_current_worker_thread();
    current_thread->gc_state.store(WorkerThread::GCState::RequestGC, std::memory_order_release);
    for (WorkerThread* thandle : m_worker_threads) {
      safeprint("waiting on thread", thandle->id);

      // do not wait for current thread
      if (thandle->gc_state.load(std::memory_order_acquire) == WorkerThread::GCState::RequestGC) {
        safeprint("thread is current thread", thandle->id);
        continue;
      }

      // thread is already waiting
      if (thandle->gc_state.load(std::memory_order_acquire) == WorkerThread::GCState::WaitingForGC) {
        safeprint("thread is already waiting", thandle->id);
        continue;
      }

      // proceed in the following cases
      // state = Working && gc_state = WaitingForGC
      // state = Idle | Untracked

      // wait for the condition to either switch to Idle / Untracked or
      // for the gc state to change to WaitingForGC
      while (thandle->state.load() == WorkerThread::State::Working &&
             thandle->gc_state.load() != WorkerThread::GCState::WaitingForGC) {
        safeprint("wait iteration", thandle->id);
        std::unique_lock<std::mutex> lk(thandle->condition_mutex);
        thandle->condition.wait_for(lk, std::chrono::milliseconds(1000));
      }
    }

    current_thread->gc_state.store(WorkerThread::GCState::ExecutingGC, std::memory_order_release);
    return true;
  }

  // resume parked threads
  void finish_gc() {
    assert(m_state.load(std::memory_order_acquire) == State::GarbageCollection);
    m_state.store(State::Working, std::memory_order_release);

    WorkerThread* thandle = this->get_current_worker_thread();
    thandle->gc_state.store(WorkerThread::GCState::None, std::memory_order_release);
    m_state_cv.notify_all();
  }

  // request a state change of the calling thread
  void request_state_change(WorkerThread::State state) {
    WorkerThread* thandle = this->get_current_worker_thread();

    // changing into idle / untracked state does not require a sync
    // we still need to notify a potential controller thread so it can
    // understand that it no longer has to wait for us
    if (state != WorkerThread::State::Working) {
      safeprint("changing state", (int)state);
      thandle->state.store(state, std::memory_order_release);
      thandle->condition.notify_one();
      return;
    }

    this->sync();
    safeprint("changing state", (int)state);
    thandle->state.store(WorkerThread::State::Working, std::memory_order_release);
  }

  // try to hold the lock, this effectively waits until
  // the main thread has registered all other threads with
  // the controller
  void wait_for_start() {
    std::unique_lock<std::mutex> lk(m_worker_threads_mutex);
  }

  // register new worker threads
  // holds the mutex to prevent started threads from starting
  // before all threads are registered
  void register_worker_threads(std::function<void(std::vector<WorkerThread*>&)> cb) {
    std::unique_lock<std::mutex> lk(m_worker_threads_mutex);
    cb(m_worker_threads);
  }

  std::vector<WorkerThread*>& get_workers() {
    return m_worker_threads;
  }

  WorkerThread* get_current_worker_thread() {
    for (WorkerThread* thandle : m_worker_threads) {
      if (thandle->thread_handle->get_id() == std::this_thread::get_id()) {
        return thandle;
      }
    }

    return nullptr;
  }

  void queue_task(int task) {
    assert_thread_state(WorkerThread::State::Working);
    safeprint("waiting for task insertion", task);
    m_task_queue.push(task);
  }

  int pop_task() {
    assert_thread_state(WorkerThread::State::Working);
    request_state_change(WorkerThread::State::Idle);
    int task = m_task_queue.pop();
    request_state_change(WorkerThread::State::Working);
    return task;
  }

  void assert_thread_state(WorkerThread::State expected) {
    WorkerThread* thandle = get_current_worker_thread();
    assert(thandle->state == expected);
  }

private:
  enum class State : uint8_t {
    Working,            // currently running in normal mode
    GarbageCollection   // currently inside a garbage collection phase
  };

  // thread synchronisation
  std::vector<WorkerThread*> m_worker_threads;
  std::mutex                 m_worker_threads_mutex;
  std::atomic<State>         m_state = State::Working;
  std::mutex                 m_state_mutex;
  std::condition_variable    m_state_cv;
  TaskQueue                  m_task_queue;
};

static Coordinator global_coordinator;

void worker_thread() {
  global_coordinator.wait_for_start();
  global_coordinator.request_state_change(WorkerThread::State::Working);

  // runtime loop
  for (;;) {
    global_coordinator.sync();
    int task = global_coordinator.pop_task();

    // random chance of causing a garbage collection
    int rand_num = get_random_number() % 25;
    safeprint("rand_num", rand_num);
    if (rand_num == 0) {
      safeprint("request gc");
      if (global_coordinator.request_gc()) {
        safeprint("starting gc");

        for (int i = 0; i < 10; i++) {
          std::this_thread::sleep_for(std::chrono::milliseconds(300));
          safeprint("gc progress", i);
        }

        safeprint("finished gc");
        global_coordinator.finish_gc();
      }
    }

    // random chance of switching into untracked mode
    if (get_random_number() % 100 == 0) {
      safeprint("entering untracked mode");
      global_coordinator.request_state_change(WorkerThread::State::Untracked);
      std::this_thread::sleep_for(std::chrono::seconds(2));
      global_coordinator.request_state_change(WorkerThread::State::Working);
      safeprint("leaving untracked mode");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    safeprint("executed task", task);
  }
}

std::mutex print_mutex;

bool should_log = true;
int main(int argc, char** argv) {
  std::srand(std::time(nullptr));
  if (argc < 2) {
    std::cout << "Missing argument" << std::endl;
    std::cout << "./program thread count [status]" << std::endl;
    std::cout << "    status  : thread overview instead of log" << std::endl;
    return 1;
  }

  global_coordinator.register_worker_threads([&](std::vector<WorkerThread*>& workers) {
    for (uint32_t i = 0; i < std::atoi(argv[1]); i++) {
      WorkerThread* thandle = new WorkerThread {
        .id = i,
        .thread_handle = new std::thread(worker_thread)
      };
      workers.push_back(thandle);
    }

    workers.push_back(new WorkerThread {
      .id = 9999,
      .thread_handle = new std::thread([&] {
        for (int i = 0;; i++) {
          global_coordinator.request_state_change(WorkerThread::State::Working);
          global_coordinator.queue_task(i);
          global_coordinator.request_state_change(WorkerThread::State::Idle);
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
      })
    });
  });

  if (argc >= 3 && std::strcmp(argv[2], "status") == 0) {
    should_log = false;
    for (;;) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      std::unique_lock<std::mutex> lk(print_mutex);
      for (WorkerThread* handle : global_coordinator.get_workers()) {
        switch (handle->gc_state) {
          case WorkerThread::GCState::RequestGC: {
            std::cout << "|GCRE";
            break;
          }
          case WorkerThread::GCState::WaitingForGC: {
            std::cout << "|WAIT";
            break;
          }
          case WorkerThread::GCState::ExecutingGC: {
            std::cout << "|GCEX";
            break;
          }
          default: {
            switch (handle->state) {
              case WorkerThread::State::Idle: {
                std::cout << "|    ";
                break;
              }
              case WorkerThread::State::Working: {
                std::cout << "|WORK";
                break;
              }
              case WorkerThread::State::Untracked: {
                std::cout << "|FREE";
                break;
              }
            }
          }
        }
      }
      std::cout << "|";
      std::cout << std::endl;
    }
  }

  for (WorkerThread* thandle : global_coordinator.get_workers()) {
    thandle->thread_handle->join();
  }

  return 0;
}

auto program_start = std::chrono::high_resolution_clock::now();
void safeprint(std::string output, int payload) {
  if (!should_log)
    return;

  std::unique_lock<std::mutex> lk(print_mutex);

  WorkerThread* handle = global_coordinator.get_current_worker_thread();
  auto now = std::chrono::high_resolution_clock::now();
  auto current_point = std::chrono::duration_cast<std::chrono::microseconds>(now - program_start);
  auto duration_since_start = current_point.count();
  std::cout << "[";
  std::cout << std::setw(16);
  std::cout << duration_since_start;
  std::cout << std::setw(1);
  std::cout << "]";
  std::cout << " ";
  std::cout << std::setw(4);
  std::cout << std::right;
  if (handle) {
    std::cout << handle->id;
  } else {
    std::cout << "main";
  }
  std::cout << std::setw(1);
  std::cout << ": ";
  std::cout << output;
  if (payload >= 0) {
    std::cout << " : ";
    std::cout << payload;
  }
  std::cout << std::endl;
}
