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
  GCExecuting,
  Native
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
    int rand_num = get_random_number() % 1000;

    std::ostringstream stream;
    stream << "rand_num = " << rand_num;
    safeprint(worker_thread->id, stream.str());

    // check if we want to pause the system for some time
    if (rand_num == 0) {
      safeprint(worker_thread->id, "        requesting GC pause");

      // check if we are the first thread to request a pause
      SystemState old = SystemState::Working;
      state.compare_exchange_strong(old, SystemState::Waiting);

      if (old == SystemState::Working) {
        worker_thread->state.store(WorkerState::GCRequested, std::memory_order_release);
        safeprint(worker_thread->id, "        wait phase started");

        // wait until all other threads have paused.
        // we only need to make sure all working threads are paused.
        for (WorkerThread* handle : worker_threads) {

          // check if we have to wait for this thread
          if (handle->id != worker_thread->id) {
            if (handle->state.load(std::memory_order_acquire) == WorkerState::Working) {
              std::ostringstream stream;
              stream << "        init wait for thread #" << handle->id;
              safeprint(worker_thread->id, stream.str());

              // wait until the thread has reached the waiting state
              std::mutex foo;
              std::unique_lock<std::mutex> lk(foo);
              while (handle->state.load(std::memory_order_acquire) != WorkerState::Waiting) {

                // Don't wait for native running threads
                if (handle->state.load(std::memory_order_acquire) == WorkerState::Native) {
                  break;
                }

                std::ostringstream stream;
                stream << "        wait iteration for thread #" << handle->id;
                safeprint(worker_thread->id, stream.str());
                handle->cv.wait_for(lk, std::chrono::microseconds(100), [&] {
                  WorkerState state = handle->state.load(std::memory_order_acquire);
                  return state == WorkerState::Waiting || state == WorkerState::Native;
                });
              }
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
        //std::terminate();
        worker_thread->state = WorkerState::Working;
        state = SystemState::Working;
        cv.notify_all();
        safeprint(worker_thread->id, "        notified waiting threads");
      }
    }

    if (state.load(std::memory_order_acquire) == SystemState::Waiting) {
      safeprint(worker_thread->id, "reached gc wait point");
      worker_thread->state.store(WorkerState::Waiting, std::memory_order_release);
      worker_thread->cv.notify_all();

      safeprint(worker_thread->id, "waiting for gc finish signal");
      std::unique_lock<std::mutex> lk(wait_mutex);
      cv.wait(lk, [&] { return state == SystemState::Working; });
      worker_thread->state = WorkerState::Working;
    }

    // random chance to enter into native mode for 10 seconds
    if (get_random_number() % 1000 == 0) {
      worker_thread->state.store(WorkerState::Native, std::memory_order_release);
      std::this_thread::sleep_for(std::chrono::seconds(5));

      // wait for the GC to finish
      if (state.load(std::memory_order_acquire) == SystemState::Waiting) {
        safeprint(worker_thread->id, "reached gc wait point");
        worker_thread->state.store(WorkerState::Waiting, std::memory_order_release);

        safeprint(worker_thread->id, "waiting for gc finish signal");
        std::unique_lock<std::mutex> lk(wait_mutex);
        cv.wait(lk, [&] { return state == SystemState::Working; });
      }

      worker_thread->state = WorkerState::Working;
    } else {
      // Get a random amount of milliseconds to wait for
      uint64_t wait_milliseconds = get_random_number() % 500;
      if (wait_milliseconds < 450) wait_milliseconds = 0;
      auto wait_delay = std::chrono::milliseconds(wait_milliseconds);

      // safeprint(worker_thread->id, "beginning work");
      std::this_thread::sleep_for(std::chrono::milliseconds(wait_delay));
      safeprint(worker_thread->id, "finished work");
    }
  }
}

int main(int argc, char** argv) {
  std::srand(std::time(nullptr));

  if (argc != 2) {
    std::cout << "Expected one argument for amount of threads to spawn" << std::endl;
    return 1;
  }

  // Setup workers array
  for (uint32_t i = 0; i < std::atoi(argv[1]); i++) {
    WorkerThread* handle = new WorkerThread { i, nullptr, WorkerState::Idle };
    worker_threads.push_back(handle);
  }

  for (WorkerThread* handle : worker_threads) {
    handle->thread_handle = new std::thread(worker_thread, handle);
  }

  // print current status of threads
  for (;;) {
    //break;

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
        case WorkerState::Native: {
          std::cout << "NATI";
          break;
        }
      }
      std::cout << " ";
    }
    std::cout << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  for (WorkerThread* thread : worker_threads) {
    thread->thread_handle->join();
  }

  return 0;
}

// Modifications to add to this prototype program
//
// - Rewrite to be more organized, Coordinator class, Allocate stub etc
