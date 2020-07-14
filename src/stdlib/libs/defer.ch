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

const __internal_defer            = @"charly.vm.defer"
const __internal_defer_interval   = @"charly.vm.defer_interval"
const __internal_clear_timer      = @"charly.vm.clear_timer"
const __internal_clear_interval   = @"charly.vm.clear_interval"
const __internal_get_thread_uid   = @"charly.vm.get_thread_uid"
const __internal_resume_thread    = @"charly.vm.resume_thread"
const __internal_suspend_thread   = @"charly.vm.suspend_thread".tap(->$0.halt_after_return = true)

/*
 * Keeps track of paused threads
 * */
class Notifier {
  property alive
  property origin_thread_uid
  property pending_threads

  constructor {
    @alive = true
    @origin_thread_uid = __internal_get_thread_uid()
    @pending_threads = []
  }

  // Suspend calling thread until notify is called
  wait {
    unless @alive return null

    @pending_threads << __internal_get_thread_uid()
    __internal_suspend_thread()

    null
  }

  // Notify paused threads that they may resume execution
  notify {
    unless @alive return null

    @pending_threads.each(->__internal_resume_thread($0))
    @pending_threads.clear()

    @alive = false

    null
  }
}

class DeferHandle {
  property id
  property alive
  property notifier
  property live_thread_uid

  constructor(@id) {
    @alive = true
    @notifier = new Notifier()
    @live_thread_uid = null
  }

  // Wait for this handle to complete
  wait {
    if !@alive return null

    const thread_uid = __internal_get_thread_uid()
    if thread_uid == @live_thread_uid throw new Error("Task cannot wait for itself to complete")
    @notifier.wait()

    null
  }

  // Updates the currently active thread uid
  update_live_thread_uid {
    @live_thread_uid = __internal_get_thread_uid()

    null
  }

  // Clears this handle, marks it as dead and resumes all waiting threads
  clear {
    @id = null
    @alive = false
    @notifier.notify()
    @live_thread_uid = null

    null
  }
}

class TimerHandle extends DeferHandle {
  constructor(cb, period) {

    // Convert period to number
    if typeof period ! "number" period = period.to_n()

    // Schedule timer in VM
    const task_id = __internal_defer(->{
      @update_live_thread_uid()
      cb()
      @clear()
    }, period)

    super(task_id)
  }

  // Prevent this timer from running
  clear {
    if !@alive return null

    __internal_clear_timer(@id)
    super()

    null
  }
}

class IntervalHandle extends DeferHandle {
  property iterations

  constructor(cb, period) {
    @iterations = 0

    // Convert period to number
    if typeof period ! "number" period = period.to_n()

    // Schedule interval in VM
    const task_id = __internal_defer_interval(->{
      @update_live_thread_uid()
      cb(@iterations, self)
      @iterations += 1
    }, period)

    super(task_id)
  }

  // Prevent this interval from running again
  clear {
    if !@alive return null

    __internal_clear_interval(@id)
    super()

    null
  }
}

const defer    = ->(cb, period = 0) new TimerHandle(cb, period)
defer.interval = ->(cb, period = 0) new IntervalHandle(cb, period)
defer.wait     = ->(threads) {
  if typeof threads ! "array" threads = arguments
  threads.each(->$0.wait())

  null
}

defer.sleep = ->(duration) {
  const task = defer(->null, duration)
  task.wait()
  null
}

defer.Notifier = Notifier

export = defer
