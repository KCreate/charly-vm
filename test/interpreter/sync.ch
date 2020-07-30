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

export = ->(describe, it, assert) {

  describe("timers", ->{

    it("waits for a timer to finish", ->{
      let v = false
      spawn.promise(->v = true).wait()
      assert(v)
    })

    it("calls a timer after some time", ->{
      const start_time = Time.now_steady()
      let v = false
      spawn.promise(->v = true, 10.milliseconds()).wait()
      assert(v, true)

      const current_time = Time.now_steady()
      assert(start_time.add(5.milliseconds()).less_than(current_time))
    })

    it("waits for multiple timers to finish", ->{
      const n = []

      const t1 = spawn.promise(->n << 1, 8.milliseconds())
      const t2 = spawn.promise(->n << 2, 2.milliseconds())
      const t3 = spawn.promise(->n << 3, 4.milliseconds())

      Sync.wait(t1, t2, t3)

      assert(n, [2, 3, 1])
    })

    it("clears a timer", ->{
      let t1_called = false
      let t2_called = false

      const t1 = spawn.promise(->t1_called = true, 5.milliseconds())
      const t2 = spawn.promise(->{
        t2_called = true
        t1.clear()
      }, 2.milliseconds())

      Sync.wait_settled(t1, t2)

      assert(t1_called, false)
      assert(t2_called, true)
    })

    it("multiple threads can wait for another thread to finish", ->{
      let t2_called = false
      let t3_called = false

      const t1 = spawn.promise(->{}, 10.milliseconds())

      const t2 = spawn.promise(->{
        t1.wait()
        t2_called = true
      }, 3.milliseconds())

      const t3 = spawn.promise(->{
        t1.wait()
        t3_called = true
      }, 5.milliseconds())

      Sync.wait(t1, t2, t3)

      assert(t2_called)
      assert(t3_called)
    })

  })

  describe("tickers", ->{

    it("waits for a ticker to finish", ->{
      let v = false
      const task = new Sync.Ticker(->(i) {
        if i == 10 task.clear()
      })

      task.wait()
      assert(task.iterations, 11)
    })

    it("ticker get called in between certain timesteps", ->{
      const start_time = Time.now_steady()
      let times_called = 0

      const task = new Sync.Ticker(->{
        if times_called == 10 task.clear()
        times_called += 1
      }, 1.milliseconds())

      task.wait()

      assert(times_called, 11)

      const current_time = Time.now_steady()
      assert(start_time.add(5.milliseconds()).less_than(current_time))
    })

    it("sets the iteration property", ->{
      const task = new Sync.Ticker(->(i, task) {
        if i == 10 task.clear()
      })

      task.wait()

      assert(task.iterations, 11)
    })

    it("exceptions inside ticker throw at wait call", ->{
      const task = new Sync.Ticker(->(i, task) {
        if i == 10 throw new Error("some error message")
      })

      let caught_exc = null
      try {
        task.wait()
      } catch(e) {
        caught_exc = e
      }

      assert(caught_exc)
      assert(caught_exc.message, "some error message")
      assert(task.iterations, 10)
    })

  })

  describe("Notifiers", ->{
    it("resumes flow of a thread at a later time", ->{
      const n = new Sync.Notifier()

      spawn(->n.notify_one("this is the result"), 10.milliseconds())

      const result = n.wait()
      assert(result, "this is the result")
    })

    it("closes a notifier, resuming all pending threads", ->{
      const n = new Sync.Notifier()

      const tasks = Array.create(4, ->(i) spawn.promise(->{
        const msg = n.wait()
        return [i, msg]
      }))

      spawn(->n.close("hello world"), 10.milliseconds())

      const results = Sync.wait(tasks)

      assert(typeof results, "array")
      assert(results.length, 4)
      assert(results, [[0, "hello world"], [1, "hello world"], [2, "hello world"], [3, "hello world"]])
    })

    it("throws exceptions in the waiting thread", ->{
      const n = new Sync.Notifier()

      spawn(->n.error(new Error("something went wrong")), 10.milliseconds())

      try {
        n.wait()
      } catch(e) {
        assert(typeof e, "object")
        assert(e.klass, Error)
        assert(e.message, "something went wrong")
      }
    })

    it("throws exceptions in multiple waiting threads", ->{
      const n = new Sync.Notifier()

      const tasks = Array.create(4, ->(i) spawn.promise(->{
        try {
          n.wait()
        } catch(e) {
          return [i, e]
        }
      }))

      spawn(->n.error("hello world"), 10.milliseconds())

      const results = Sync.wait(tasks)

      assert(typeof results, "array")
      assert(results.length, 4)
      assert(results, [[0, "hello world"], [1, "hello world"], [2, "hello world"], [3, "hello world"]])
    })

    it("throws an exception on outstanding waits", ->{
      const n = new Sync.Notifier()

      let caught_exception = null

      try {
        n.wait()
      } catch(e) {
        caught_exception = e
      }

      assert(caught_exception)
      assert(caught_exception.is_a(Error))
      assert(typeof caught_exception, "object")
      assert(caught_exception.message, "VM exited while this thread was still paused")
    })
  })

  describe("Promises", ->{
    it("creates an already resolved promise", ->(done) {
      const p = Promise.resolve(250)
      assert(p.is_pending(), false)
      assert(p.is_resolved(), true)
      assert(p.is_rejected(), false)
      assert(p.is_settled(), false)

      p.then(->(result) {
        assert(result, 250)
        assert(p.is_pending(), false)
        assert(p.is_resolved(), true)
        assert(p.is_rejected(), false)
        assert(p.is_settled(), true)

        done()
      })
    })

    it("creates an already rejected promise", ->(done) {
      const p = Promise.reject(250)
      assert(p.is_pending(), false)
      assert(p.is_resolved(), false)
      assert(p.is_rejected(), true)
      assert(p.is_settled(), false)

      p.catch(->(err) {
        assert(err, 250)
        assert(p.is_pending(), false)
        assert(p.is_resolved(), false)
        assert(p.is_rejected(), true)
        assert(p.is_settled(), true)

        done()
      })
    })

    it("allows multiple then handlers", ->(done) {
      const task = spawn.promise(->"test", 10.milliseconds())

      assert(task.is_pending(), true)
      assert(task.is_resolved(), false)
      assert(task.is_rejected(), false)
      assert(task.is_settled(), false)

      const results = []

      task.then(->(result) results << result)
      task.then(->(result) results << result)
      task.then(->(result) results << result)

      task.then(->{
        assert(results, ["test", "test", "test"])
        assert(task.is_pending(), false)
        assert(task.is_resolved(), true)
        assert(task.is_rejected(), false)
        assert(task.is_settled(), true)

        done()
      })
    })

    it("allows multiple catch handlers", ->(done) {
      const task = spawn.promise(->throw new Error("my error"), 10.milliseconds())

      assert(task.is_pending(), true)
      assert(task.is_resolved(), false)
      assert(task.is_rejected(), false)
      assert(task.is_settled(), false)

      const errors = []

      task.catch(->(err) errors << err)
      task.catch(->(err) errors << err)
      task.catch(->(err) errors << err)

      task.catch(->(err) {
        assert(errors, [err, err, err])
        assert(task.is_pending(), false)
        assert(task.is_resolved(), false)
        assert(task.is_rejected(), true)
        assert(task.is_settled(), true)

        done()
      })
    })

    it("waits synchronously for a promise to finish", ->{
      const task = spawn.promise(->"response message", 10.milliseconds())
      const result = task.wait()
      assert(result, "response message")
      assert(task.is_pending(), false)
      assert(task.is_resolved(), true)
      assert(task.is_rejected(), false)
      assert(task.is_settled(), true)
    })

    it("waits synchronously for a promise to settle", ->{
      const task = spawn.promise(->throw "my error", 10.milliseconds())
      const result = task.wait_settle()

      assert(result, task)
      assert(task.is_pending(), false)
      assert(task.is_resolved(), false)
      assert(task.is_rejected(), true)
      assert(task.is_settled(), true)
      assert(task.value, "my error")
    })

    it("Sync.wait", ->{
      const tasks = Array.create(5, ->(i) spawn.promise(->i * ".", i * 5.milliseconds()))
      const results = Sync.wait(tasks)
      assert(results, ["", ".", "..", "...", "...."])

      it("throws exceptions", ->{
        const tasks = Array.create(5, ->(i) spawn.promise(->throw new Error(i * "."), i * 5.milliseconds()))
        const exc = Error.expect(->Sync.wait(tasks))
        assert(typeof exc, "object")
        assert(exc.message, "")
      })
    })

    it("Sync.wait_settled", ->{
      const t1 = spawn.promise(->"result 1",      10.millisecond())
      const t2 = spawn.promise(->"result 2",      5.millisecond())
      const t3 = spawn.promise(->throw "error 3", 3.millisecond())

      const results = Sync.wait_settled(t1, t2, t3)
      assert(results,                         [t1, t2, t3])
      assert(results.map(->$0.is_resolved()), [true, true, false])
      assert(results.map(->$0.value),         ["result 1", "result 2", "error 3"])
    })

    it("Sync.all", ->{
      const t1 = spawn.promise(->"result 1", 10.millisecond())
      const t2 = spawn.promise(->"result 2", 5.millisecond())
      const t3 = spawn.promise(->"result 3", 3.millisecond())

      const p = Sync.all(t1, t2, t3)
      assert(p.is_pending(), true)
      assert(p.is_resolved(), false)
      assert(p.is_rejected(), false)
      assert(p.is_settled(), false)

      const results = p.wait()
      assert(p.is_pending(), false)
      assert(p.is_resolved(), true)
      assert(p.is_rejected(), false)
      assert(p.is_settled(), true)
      assert(results, ["result 1", "result 2", "result 3"])

      it("throws exceptions", ->{
        const t1 = spawn.promise(->"result 1",                 10.millisecond())
        const t2 = spawn.promise(->"result 2",                 5.millisecond())
        const t3 = spawn.promise(->throw new Error("error 3"), 3.millisecond())

        const p = Sync.all(t1, t2, t3)
        assert(p.is_pending(), true)
        assert(p.is_resolved(), false)
        assert(p.is_rejected(), false)
        assert(p.is_settled(), false)

        const exc = Error.expect(->{ p.wait() })
        assert(p.is_pending(), false)
        assert(p.is_resolved(), false)
        assert(p.is_rejected(), true)
        assert(p.is_settled(), true)

        assert(typeof exc, "object")
        assert(exc.message, "error 3")
      })
    })

    it("Sync.all_settled", ->{
      const t1 = spawn.promise(->"result 1",               10.millisecond())
      const t2 = spawn.promise(->"result 2",               5.millisecond())
      const t3 = spawn.promise(->throw new Error("error"), 3.millisecond())

      const p = Sync.all_settled(t1, t2, t3)
      assert(p.is_pending(), true)
      assert(p.is_resolved(), false)
      assert(p.is_rejected(), false)
      assert(p.is_settled(), false)

      const results = p.wait()
      assert(p.is_pending(), false)
      assert(p.is_resolved(), true)
      assert(p.is_rejected(), false)
      assert(p.is_settled(), true)
      assert(results.map(->typeof $0),        ["object", "object", "object"])
      assert(results.map(->$0.is_resolved()), [true, true, false])
      assert(results.map(->$0.is_settled()),  [true, true, true])
      assert(results.map(->typeof $0.value),  ["string", "string", "object"])
    })

    it("Sync.race", ->{
      const t1 = spawn.promise(->"result 1", 10.millisecond())
      const t2 = spawn.promise(->"result 2", 5.millisecond())
      const t3 = spawn.promise(->"result 3", 3.millisecond())

      const p = Sync.race(t1, t2, t3)
      const result = p.wait()

      assert(result, "result 3")
    })

    it("Sync.race_settled", ->{
      const t1 = spawn.promise(->"result 1", 10.millisecond())
      const t2 = spawn.promise(->"result 2", 5.millisecond())
      const t3 = spawn.promise(->throw "error 3", 3.millisecond())

      const p = Sync.race(t1, t2, t3)
      const exc = Error.expect(->p.wait())

      assert(exc, "error 3")
    })
  })

}
