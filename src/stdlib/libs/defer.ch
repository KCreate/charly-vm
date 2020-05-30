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

// Stores the task id assigned to this timer / interval by the VM
class DeferHandle {
  property id
  property callbacks

  constructor {
    @callbacks = []
  }

  /*
   * Schedules a callback to be run when this timer / interval finishes
   *
   * Note: "Finishing" for an interval is defined as the interval being cleared
   * */
  then(cb, immediate = false) {
    @callbacks << {cb, immediate}
    self
  }

  /*
   * Invoke the scheduled callbacks
   * */
  finish {
    @callbacks.each(->(s) {
      unless s.immediate {
        return defer(s.cb)
      }
      const cb = s.cb
      cb()
    }).clear()

    self
  }
}

/*
 * Interval and Timer specific subclasses
 *
 * These classes are basically wrappers around the raw task scheduling functions
 * keeping track of the relevant ID and in the intervals case, the number
 * of iterations that have been run
 * */

class IntervalHandle extends DeferHandle {
  property iterations

  constructor(cb, period) {

    // Extract milliseconds from possible Duration objects
    if period.klass == Time.Duration {
      period = period.in_milliseconds()
    }

    @id = __internal_defer_interval(->{
      cb(@iterations)
      @iterations += 1
    }, period)
  }

  clear {
    __internal_clear_interval(@id)
    @id = null
    @finish()

    self
  }
}

class TimerHandle extends DeferHandle {
  constructor(cb, period) {

    // Extract milliseconds from possible Duration objects
    if period.klass == Time.Duration {
      period = period.in_milliseconds()
    }

    @id = __internal_defer(->{
      cb()
      @finish()
    }, period)
  }

  clear {
    __internal_clear_timer(@id)
    @id = null
    @finish()

    self
  }
}

export = {
  TimerHandle,
  IntervalHandle
}
