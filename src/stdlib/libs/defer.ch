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

const Time = import "time"

const __internal_defer            = Charly.internals.get_method("defer")
const __internal_defer_interval   = Charly.internals.get_method("defer_interval")
const __internal_clear_timer      = Charly.internals.get_method("clear_timer")
const __internal_clear_interval   = Charly.internals.get_method("clear_interval")
const __internal_get_thread_uid   = Charly.internals.get_method("get_thread_uid")
const __internal_resume_thread    = Charly.internals.get_method("resume_thread")
const __internal_suspend_thread   = Charly.internals.get_method("suspend_thread")

__internal_suspend_thread.halt_after_return = true;

class DeferHandle {
  property id
  property origin_thread_uid
  property live_thread_uid
  property pending_threads
  property alive

  constructor {
    @pending_threads = []
    @origin_thread_uid = __internal_get_thread_uid()
    @live_thread_uid = null
    @alive = true
  }

  // Wait for this task to finish
  // This will suspend the calling thread and
  // resume it once this task finishes
  wait {
    unless @alive return null
    const uid = __internal_get_thread_uid()
    if uid == @live_thread_uid throw "Cannot wait for own thread to complete"

    @pending_threads << uid
    __internal_suspend_thread()

    null
  }

  finish {
    @alive = false
    @pending_threads.each(->(th) __internal_resume_thread(th))
    @pending_threads.clear()
  }
}

class IntervalHandle extends DeferHandle {
  property iterations

  constructor(cb, period) {
    @id = __internal_defer_interval(->{
      @live_thread_uid = __internal_get_thread_uid()
      cb(@iterations, self)
      @iterations += 1
    }, period.to_n())
    @iterations = 0
  }

  clear {
    __internal_clear_interval(@id)
    @finish()
  }
}

class TimerHandle extends DeferHandle {
  constructor(cb, period) {
    @id = __internal_defer(->{
      @live_thread_uid = __internal_get_thread_uid()
      cb()
      @finish()
    }, period.to_n())
  }

  clear {
    __internal_clear_timer(@id)
    @finish()
  }
}

const defer    = ->(cb, period = 0) new TimerHandle(cb, period)
defer.interval = ->(cb, period = 0) new IntervalHandle(cb, period)
defer.wait     = ->(threads) {
  if typeof threads ! "array" threads = arguments
  threads.each(->$0.wait())

  null
}

export = defer
