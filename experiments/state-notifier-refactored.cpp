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
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>

// data structures
#include <vector>

void safeprint(std::string output, int payload = -1);

int rand_seed = 0;
int get_random_number() {
  static std::mutex rand_mutex;
  std::unique_lock<std::mutex> lk(rand_mutex);
  std::srand(rand_seed++);
  return std::rand();
}

struct WorkerThread {
  enum class State : uint8_t {
    Idle,         // waiting for work
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
    WorkerThread* thandle = this->get_current_worker_thread();

    // check if there is a garbage collection currently active
    while (m_state.load(std::memory_order_acquire) == State::GarbageCollection) {
      safeprint("parking thread");

      // mark as waiting for gc and notify the controller thread
      thandle->gc_state.store(WorkerThread::GCState::WaitingForGC, std::memory_order_release);
      thandle->condition.notify_all();

      // wait for the garbage collection phase to be over
      //
      // TODO: should we spin on this?
      std::unique_lock<std::mutex> lk(m_wait_mutex);
      m_wait_condition.wait(lk, [&] {
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

      // thread is in idle or untracked mode
      WorkerThread::State state = thandle->state.load(std::memory_order_acquire);
      if (state == WorkerThread::State::Idle || state == WorkerThread::State::Untracked) {
        safeprint("thread is idle / untracked", thandle->id);
        continue;
      }

      // thread is still running, wait for it to change its state
      // or set the wait for gc flag
      while (thandle->gc_state != WorkerThread::GCState::WaitingForGC) {
        safeprint("wait iteration", thandle->id);

        // wait for status changes on the worker thread
        std::unique_lock<std::mutex> lk(thandle->condition_mutex);
        thandle->condition.wait_for(lk, std::chrono::milliseconds(1000), [&] {
          WorkerThread::GCState gc_state = thandle->gc_state.load(std::memory_order_acquire);
          return gc_state == WorkerThread::GCState::WaitingForGC;
        });
      }
    }

    current_thread->gc_state.store(WorkerThread::GCState::ExecutingGC, std::memory_order_release);
    return true;
  }

  // resume parked threads
  void finish_gc() {
    safeprint("gc finished");

    // set coordinator status to working
    assert(m_state.load(std::memory_order_acquire) == State::GarbageCollection);
    m_state.store(State::Working, std::memory_order_release);

    // set worker thread state to working
    WorkerThread* thandle = this->get_current_worker_thread();
    thandle->gc_state.store(WorkerThread::GCState::None, std::memory_order_release);
    m_wait_condition.notify_all();
  }

  // request a state change of the calling thread
  void request_state_change(WorkerThread::State state) {
    WorkerThread* thandle = this->get_current_worker_thread();
    safeprint("changing state to", (int)state);

    // changing into idle and untracked state does not require a sync
    // we still need to notify a potential controller thread so it can
    // understand that it no longer has to wait for us
    if (state != WorkerThread::State::Working) {
      thandle->state.store(state, std::memory_order_release);
      thandle->condition.notify_all();
      return;
    }

    this->sync();
    thandle->state.store(WorkerThread::State::Working, std::memory_order_release);
  }

  std::mutex                 m_worker_threads_mutex;
  std::vector<WorkerThread*> m_worker_threads;

  WorkerThread* get_current_worker_thread() {
    for (WorkerThread* thandle : m_worker_threads) {
      if (thandle->thread_handle->get_id() == std::this_thread::get_id()) {
        return thandle;
      }
    }

    return nullptr;
  }

private:
  enum class State : uint8_t {
    Working,            // currently running in normal mode
    GarbageCollection   // currently inside a garbage collection phase
  };

  std::atomic<State>      m_state = State::Working;
  std::condition_variable m_wait_condition;
  std::mutex              m_wait_mutex;
};

static Coordinator global_coordinator;

void worker_thread() {
  // acquire the worker threads mutex at least once to make sure
  // we don't start our thread before all other threads are registered
  {
    std::unique_lock<std::mutex> lk(global_coordinator.m_worker_threads_mutex);
  }

  global_coordinator.request_state_change(WorkerThread::State::Working);

  // runtime loop
  int task_id = 0;
  for (;; task_id++) {
    global_coordinator.sync();

    // random chance of causing a garbage collection
    int rand_num = get_random_number() % 100;
    if (rand_num == 0) {
      safeprint("request gc");
      if (global_coordinator.request_gc()) {
        safeprint("starting gc");

        for (int i = 0; i < 10; i++) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          safeprint("gc progress", i);
        }

        global_coordinator.finish_gc();
      }

      continue;
    }

    // random chance of switching into untracked mode
    if (get_random_number() % 100 == 0) {
      global_coordinator.request_state_change(WorkerThread::State::Untracked);
      std::this_thread::sleep_for(std::chrono::seconds(5));
      global_coordinator.request_state_change(WorkerThread::State::Working);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(get_random_number() % 400));
    safeprint("executed task", task_id);
  }
}

std::mutex print_mutex;

bool should_log = true;
int main(int argc, char** argv) {
  std::srand(std::time(nullptr));
  if (argc < 2) {
    std::cout << "Missing argument" << std::endl;
    std::cout << "./program thread count [status]" << std::endl;
    std::cout << "    log             : print log output of worker threads" << std::endl;
    std::cout << "    status          : print overview of thread status" << std::endl;
    return 1;
  }

  {
    std::unique_lock<std::mutex> lk(global_coordinator.m_worker_threads_mutex);
    for (uint32_t i = 0; i < std::atoi(argv[1]); i++) {
      WorkerThread* thandle = new WorkerThread {
        .id = i,
        .thread_handle = new std::thread(worker_thread)
      };
      global_coordinator.m_worker_threads.push_back(thandle);
    }
  }

  if (argc >= 3 && std::strcmp(argv[2], "status") == 0) {
    should_log = false;
    for (;;) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      std::unique_lock<std::mutex> lk(print_mutex);
      for (WorkerThread* handle : global_coordinator.m_worker_threads) {
        switch (handle->gc_state) {
          case WorkerThread::GCState::RequestGC: {
            std::cout << " GCRE ";
            break;
          }
          case WorkerThread::GCState::WaitingForGC: {
            std::cout << " WAIT ";
            break;
          }
          case WorkerThread::GCState::ExecutingGC: {
            std::cout << " GCEX ";
            break;
          }
          default: {
            switch (handle->state) {
              case WorkerThread::State::Idle: {
                std::cout << "      ";
                break;
              }
              case WorkerThread::State::Working: {
                std::cout << " ~~~~ ";
                break;
              }
              case WorkerThread::State::Untracked: {
                std::cout << " FREE ";
                break;
              }
            }
          }
        }
      }
      std::cout << std::endl;
    }
  }

  for (WorkerThread* thandle : global_coordinator.m_worker_threads) {
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
  std::cout << std::setw(2);
  std::cout << std::right;
  std::cout << handle->id;
  std::cout << std::setw(1);
  std::cout << ": ";
  std::cout << output;
  if (payload >= 0) {
    std::cout << " : ";
    std::cout << payload;
  }
  std::cout << std::endl;
}

// TODO:
//
// - Is thread mode idle really needed?
// - Implement mock task queue for threads
// - Implement mock networking thread
