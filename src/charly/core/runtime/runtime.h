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
#include <list>
#include <mutex>
#include <stack>
#include <vector>

#include "charly/handle.h"

#include "charly/utils/wait_flag.h"

#include "charly/core/runtime/compiled_module.h"
#include "charly/core/runtime/gc.h"
#include "charly/core/runtime/heap.h"
#include "charly/core/runtime/scheduler.h"

#pragma once

namespace charly::core::runtime {

class Runtime {
public:
  Runtime();

  Heap* heap();
  Scheduler* scheduler();
  GarbageCollector* gc();

  bool wants_exit() const;

  // wait for the runtime to exit
  // returns the status code returned by the application
  int32_t join();

  // initiate runtime exit
  // only the first thread that calls this method will set the exit code
  void abort(int32_t exit_code);

  // wait for the runtime to finish initializing
  void wait_for_initialization();

  // register a CompiledModule object with the runtime
  void register_module(const ref<CompiledModule>& module);

public:
  RawData create_data(Thread* thread, ShapeId shape_id, size_t size);
  RawObject create_instance(Thread* thread, ShapeId shape_id, size_t field_count);

  RawValue create_string(Thread* thread, const char* data, size_t size, SYMBOL hash);
  // create a new string by acquiring ownership over an existing allocation
  RawValue acquire_string(Thread* thread, char* cstr, size_t size, SYMBOL hash);
  RawLargeString create_large_string(Thread* thread, const char* data, size_t size, SYMBOL hash);
  RawHugeString create_huge_string(Thread* thread, const char* data, size_t size, SYMBOL hash);
  RawHugeString create_huge_string_acquire(Thread* thread, char* data, size_t size, SYMBOL hash);

  RawObject create_tuple(Thread* thread, uint32_t count);
  RawObject create_function(Thread* thread, RawValue context, SharedFunctionInfo* shared_info);
  RawObject create_fiber(Thread* thread, RawFunction function);

private:
  uint64_t m_start_timestamp;

  std::mutex m_mutex;
  utils::WaitFlag m_init_flag;
  utils::WaitFlag m_exit_flag;

  int32_t m_exit_code;
  atomic<bool> m_wants_exit;

  Heap* m_heap;
  Scheduler* m_scheduler;
  GarbageCollector* m_gc;

  std::vector<ref<CompiledModule>> m_compiled_modules;
};

}  // namespace charly::core::runtime
