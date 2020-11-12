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

#include "sync.h"
#include "vm.h"

namespace Charly {
namespace Internals {
namespace Sync {

VALUE init_timer(VM& vm, VALUE cb, VALUE dur) {
  CHECK(function, cb);
  CHECK(number, dur);

  uint32_t ms = charly_number_to_uint32(dur);

  if (ms == 0) {
    vm.register_task(VMTask::init_callback(charly_as_function(cb)));
    return kNull;
  }

  Timestamp now = std::chrono::steady_clock::now();
  Timestamp exec_at = now + std::chrono::milliseconds(ms);

  return charly_create_integer(vm.register_timer(exec_at, VMTask::init_callback(charly_as_function(cb))));
}

VALUE clear_timer(VM& vm, VALUE uid) {
  CHECK(number, uid);
  vm.clear_timer(charly_number_to_uint64(uid));
  return kNull;
}

VALUE init_ticker(VM& vm, VALUE cb, VALUE period) {
  CHECK(function, cb);
  CHECK(number, period);

  uint32_t ms = charly_number_to_uint32(period);

  return charly_create_integer(vm.register_ticker(ms, VMTask::init_callback(charly_as_function(cb))));
}

VALUE clear_ticker(VM& vm, VALUE uid) {
  CHECK(number, uid);
  vm.clear_ticker(charly_number_to_uint64(uid));
  return kNull;
}

VALUE suspend_thread(VM& vm) {
  vm.suspend_thread();
  return kNull;
}

VALUE resume_thread(VM& vm, VALUE uid, VALUE argument) {
  CHECK(number, uid);
  vm.resume_thread(charly_number_to_uint64(uid), argument);
  return kNull;
}

VALUE get_thread_uid(VM& vm) {
  return charly_create_integer(vm.get_thread_uid());
}

}  // namespace Sync
}  // namespace Internals
}  // namespace Charly
