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

#include "charly/core/runtime/builtins/timer.h"
#include "charly/core/runtime/runtime.h"

namespace charly::core::runtime::builtin::timer {

void initialize(Thread* thread) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  DEF_BUILTIN_TIMER(REGISTER_BUILTIN_FUNCTION)
}

RawValue fibercreate(Thread* thread, BuiltinFrame* frame) {
  CHECK(frame->arguments[0].isNumber());
  CHECK(frame->arguments[1].isFunction());

  int64_t delay = frame->arguments[0].int_value();

  HandleScope scope(thread);
  Function function(scope, frame->arguments[1]);
  Value context(scope, frame->arguments[2]);
  Value arguments(scope, frame->arguments[3]);

  if (delay <= 0) {
    RawFiber::create(thread, function, context, arguments);
    return kNull;
  }

  size_t now = get_steady_timestamp();
  size_t timestamp = now + delay;

  Processor* proc = thread->worker()->processor();
  TimerId id = proc->init_timer_fiber_create(timestamp, function, context, arguments);
  return RawInt::create(id);
}

RawValue sleep(Thread* thread, BuiltinFrame* frame) {
  CHECK(frame->arguments[0].isNumber());
  int64_t delay = frame->arguments[0].int_value();

  if (delay <= 0) {
    return kNull;
  }

  size_t now = get_steady_timestamp();
  size_t timestamp = now + delay;
  thread->sleep_until(timestamp);
  return kNull;
}

RawValue cancel(Thread* thread, BuiltinFrame* frame) {
  CHECK(frame->arguments[0].isInt());
  TimerId id = RawInt::cast(frame->arguments[0]).value();
  Processor* proc = thread->worker()->processor();

  if (!proc->cancel_timer(id)) {
    return thread->throw_message("Timer with id % either already expired or doesn't exist", id);
  }

  return kTrue;
}

}  // namespace charly::core::runtime::builtin::list
