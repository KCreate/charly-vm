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

const Promise = import "./promise.ch"

const __internal_init_timer     = @"charly.vm.init_timer"
const __internal_clear_timer    = @"charly.vm.clear_timer"
const __internal_init_ticker    = @"charly.vm.init_ticker"
const __internal_clear_ticker   = @"charly.vm.clear_ticker"
const __internal_get_thread_uid = @"charly.vm.get_thread_uid"
const __internal_resume_thread  = @"charly.vm.resume_thread"
const __internal_suspend_thread = @"charly.vm.suspend_thread".tap(->$0.halt_after_return = true)

/*
 * The Notifier class suspends threads and resumes them at a later time
 * */
class Notifier {
  property open
  property result
  property error_val
  property pending

  constructor {
    @open = true
    @result = null
    @error_val = null
    @pending = new Queue()
  }

  // Suspend calling thread until notify is called
  wait {
    unless @open {
      if @error_val throw @error_val
      return @result
    }

    const thread_id = __internal_get_thread_uid()
    @pending.write(thread_id)
    const return_value = __internal_suspend_thread()
    if @error_val {
      throw @error_val
    }

    // VM stale thread exception mechanism
    if typeof return_value == "object" && return_value.__charly_internal_stale_thread_exception {
      @pending.clear()
      throw new Error("VM exited while this thread was still paused")
    }

    return return_value
  }

  // Notify a single pending thread
  notify_one(result = null) {
    unless @open throw new Error("Cannot notify closed notifier")

    if @pending.length {
      const th = @pending.read()
      __internal_resume_thread(th, result)
    }

    null
  }

  // Closes the notifier, throwing error inside all pending threads
  // Future waits will throw with the same error
  error(@error_val) {
    unless @open throw new Error("Cannot throw error on closed notifier")
    @close()
  }

  // Resume all waiting threads with result,
  // future waits will return this result too
  close(@result = null) {
    unless @open throw new Error("Notifier already closed")

    while @pending.length {
      @notify_one(result)
    }
    @open = false

    null
  }

  // Check if anyone is waiting for this notifier currently
  has_waiters = @pending.length > 0
}

/*
 * The Timer schedules a callback to be invoked at a given date
 * in the future. Timer is a subclass of Promise and thus has all the
 * waiting and sync features of a regular Promise
 * */
class Timer extends Promise {
  property id

  constructor(callback, period = 0, arg = null) {
    super(->(resolve, reject) {
      const this = self
      @id = __internal_init_timer(func init_timer {
        this.id = null
        try {
          resolve(callback(arg))
        } catch(e) {
          reject(e)
        }
      }, period.to_n())
    })
  }

  clear(arg = null) {
    if @id == null return null

    __internal_clear_timer(@id)
    @id = null
    @reject(arg)

    null
  }

  cancel(arg = null) = @clear(arg)
}

/*
 * The Ticker tries to invoke a callback at a consistent rate
 * */
class Ticker extends Promise {
  property id
  property iterations

  constructor(callback, period = 0, arg = null) {
    super(->(resolve, reject) {
      const this = self
      @iterations = 0
      @id = __internal_init_ticker(func init_ticker {
        try {
          callback(this.iterations, this)
          this.iterations += 1
        } catch(e) {
          __internal_clear_ticker(this.id)
          this.id = null
          reject(e)
        }
      }, period.to_n())
    })
  }

  clear(arg = null) {
    if @id == null return null

    __internal_clear_ticker(@id)
    @resolve(arg)
    @id = null

    null
  }

  cancel(arg = null) = @clear(arg)
  stop(arg = null)   = @clear(arg)
}

/*
 * Schedules a task to be executed at a later date
 * The return value of the callback is the result of the promise
 * */
func defer(callback, period = 0, arg = null) = new Timer(callback, period, arg)

/*
 * Sleeps for a given amount of time
 * */
func sleep(duration) {
  const t = new Timer(->null, duration)
  t.wait()
}

export = {
  Notifier,
  Promise,
  Timer,
  Ticker,

  defer,
  sleep,

  wait:         Promise.wait,
  wait_settled: Promise.wait_settled,
  all:          Promise.all,
  all_settled:  Promise.all_settled,
  race:         Promise.race,
  race_settled: Promise.race_settled
}
