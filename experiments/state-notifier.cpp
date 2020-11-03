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

#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <sstream>

#include <chrono>
#include <iostream>
#include <iomanip>

enum class SystemState : uint8_t {
  Working = 0x00,
  Waiting = 0x01
};

std::atomic<SystemState> state;
std::mutex wait_mutex;
std::condition_variable cv;

enum class WorkerState : uint8_t {
  Idle,
  Working,
  Waiting,
  GCRequested,
  GCExecuting
};

struct WorkerThread {
  uint32_t id;
  std::thread* thread_handle;
  std::atomic<WorkerState> state;
  std::condition_variable cv;
};

std::vector<WorkerThread*> worker_threads;

auto program_start = std::chrono::high_resolution_clock::now();
void safeprint(uint32_t id, std::string output) {
  return;
  static std::mutex print_mutex;
  std::unique_lock<std::mutex> lk(print_mutex);

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
  std::cout << id;
  std::cout << std::setw(1);
  std::cout << ": " << output << std::endl;
}

int rand_seed = 0;
int get_random_number() {
  static std::mutex rand_mutex;
  std::unique_lock<std::mutex> lk(rand_mutex);
  std::srand(rand_seed++);
  return std::rand();
}

void worker_thread(WorkerThread* worker_thread) {
  worker_thread->state = WorkerState::Working;
  for (;;) {

    int rand_num = get_random_number() % 100;

    std::ostringstream stream;
    stream << "rand_num = " << rand_num;
    safeprint(worker_thread->id, stream.str());

    // check if we want to pause the system for some time
    if (rand_num == 0) {

      // check if we are the first thread to request a pause
      SystemState old = SystemState::Working;
      state.compare_exchange_strong(old, SystemState::Waiting);

      if (old == SystemState::Working) {
        safeprint(worker_thread->id, "        requesting GC pause");
        worker_thread->state = WorkerState::GCRequested;

        // wait until all other threads have paused.
        // we only need to make sure all working threads are paused.
        for (WorkerThread* handle : worker_threads) {

          // check if we have to wait for this thread
          if (handle->id != worker_thread->id) {
            if (handle->state == WorkerState::Working) {
              std::ostringstream stream;
              stream << "        waiting for thread #" << handle->id;
              safeprint(worker_thread->id, stream.str());

              std::mutex foo;
              std::unique_lock<std::mutex> lk(foo);
              handle->cv.wait(lk, [&] { return handle->state == WorkerState::Waiting; });
            }
          }

          std::ostringstream stream;
          stream << "        finished waiting for thread #" << handle->id;
          safeprint(worker_thread->id, stream.str());
        }

        worker_thread->state = WorkerState::GCExecuting;

        // simulate GC work being done
        for (int i = 0; i < 10; i++) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          std::ostringstream stream;
          stream << "        gc progress " << (i + 1) * 10 << "%";
          safeprint(worker_thread->id, stream.str());
        }

        safeprint(worker_thread->id, "        finished GC");
        worker_thread->state = WorkerState::Working;
        state = SystemState::Working;
        cv.notify_all();
      } else {
        safeprint(worker_thread->id, "chance wait for gc to finish");
        worker_thread->state.store(WorkerState::Waiting, std::memory_order_seq_cst);
        worker_thread->cv.notify_one();
        std::unique_lock<std::mutex> lk(wait_mutex);
        cv.wait(lk, [&] { return state == SystemState::Working; });
        worker_thread->state = WorkerState::Working;
      }
    }

    if (state == SystemState::Waiting) {
      safeprint(worker_thread->id, "waiting for gc to finish");
      worker_thread->state.store(WorkerState::Waiting, std::memory_order_seq_cst);
      worker_thread->cv.notify_one();
      std::unique_lock<std::mutex> lk(wait_mutex);
      cv.wait(lk, [&] { return state == SystemState::Working; });
      worker_thread->state = WorkerState::Working;
    }

    // Get a random amount of milliseconds to wait for
    uint64_t wait_milliseconds = 100 + (get_random_number() % 100) + (worker_thread->id * 200);
    auto wait_delay = std::chrono::milliseconds(wait_milliseconds);

    safeprint(worker_thread->id, "beginning work");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //safeprint(worker_thread->id, "finished work");
  }
}

int main() {
  std::srand(std::time(nullptr));

  // Setup workers array
  for (uint32_t i = 0; i < 10; i++) {
    WorkerThread* handle = new WorkerThread { i, nullptr, WorkerState::Idle };
    worker_threads.push_back(handle);
  }

  for (WorkerThread* handle : worker_threads) {
    handle->thread_handle = new std::thread(worker_thread, handle);
  }

  // print current status of threads
  for (;;) {

    // print thread IDs
    //for (WorkerThread* handle : worker_threads) {
      //std::cout << " ";
      //std::cout << std::setw(4);
      //std::cout << handle->id;
      //std::cout << std::setw(1);
      //std::cout << " ";
    //}
    //std::cout << std::endl;

    // print thread statuses
    for (WorkerThread* handle : worker_threads) {
      std::cout << " ";
      switch (handle->state) {
        case WorkerState::Idle: {
          std::cout << "    ";
          break;
        }
        case WorkerState::Working: {
          std::cout << " ~~ ";
          break;
        }
        case WorkerState::Waiting: {
          std::cout << "WAIT";
          break;
        }
        case WorkerState::GCRequested: {
          std::cout << "GCRQ";
          break;
        }
        case WorkerState::GCExecuting: {
          std::cout << "GCEX";
          break;
        }
      }
      std::cout << " ";
    }
    std::cout << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  for (WorkerThread* thread : worker_threads) {
    thread->thread_handle->join();
  }

  return 0;
}
