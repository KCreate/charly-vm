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

export = ->(describe, it, assert) {

  describe("timers", ->{

    it("waits for a timer to finish", ->{
      let v = false
      defer(->v = true).wait()
      assert(v)
    })

    it("calls a timer after some time", ->{
      const start_time = Time.now_steady()
      let v = false
      defer(->v = true, 10.milliseconds()).wait()
      assert(v, true)

      const current_time = Time.now_steady()
      assert(start_time.add(5.milliseconds()).less_than(current_time))
    })

    it("waits for multiple timers to finish", ->{
      const n = []

      const t1 = defer(->n << 1, 8.milliseconds())
      const t2 = defer(->n << 2, 2.milliseconds())
      const t3 = defer(->n << 3, 4.milliseconds())

      defer.wait(t1, t2, t3)

      assert(n, [2, 3, 1])
    })

    it("clears a timer", ->{
      let t1_called = false
      let t2_called = false

      const t1 = defer(->t1_called = true, 5.milliseconds())
      const t2 = defer(->{
        t2_called = true
        t1.clear()
      }, 2.milliseconds())

      defer.wait(t1, t2)

      assert(t1_called, false)
      assert(t2_called, true)
    })

    it("throws an exception when trying to wait for own thread", ->{
      let caught_exception = null

      const task = defer(->{
        try {
          task.wait()
        } catch(e) {
          caught_exception = e
        }
      })

      task.wait()

      assert(caught_exception.is_a(Error))
      assert(typeof caught_exception, "object")
      assert(caught_exception.message, "Task cannot wait for itself to complete")
    })

    it("multiple threads can wait for another thread to finish", ->{
      let t2_called = false
      let t3_called = false

      const t1 = defer(->{}, 10.milliseconds())

      const t2 = defer(->{
        t1.wait()
        t2_called = true
      }, 3.milliseconds())

      const t3 = defer(->{
        t1.wait()
        t3_called = true
      }, 5.milliseconds())

      defer.wait(t1, t2, t3)

      assert(t2_called)
      assert(t3_called)
    })

  })

  describe("intervals", ->{

    it("waits for an interval to finish", ->{
      let v = false
      const task = defer.interval(->(i) {
        if i == 10 task.clear()
      })

      task.wait()
      assert(task.iterations, 11)
    })

    it("intervals get called in between certain timesteps", ->{
      const start_time = Time.now_steady()
      let times_called = 0

      const task = defer.interval(->{
        if times_called == 10 task.clear()
        times_called += 1
      }, 5.milliseconds())

      task.wait()

      assert(times_called, 11)

      const current_time = Time.now_steady()
      assert(start_time.add(45.milliseconds()).less_than(current_time))
    })

    it("sets the iteration property", ->{
      const task = defer.interval(->(i, task) {
        if i == 10 task.clear()
      })

      task.wait()

      assert(task.iterations, 11)
    })

  })

}
