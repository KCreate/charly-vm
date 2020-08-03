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

// Status field flags
const kFlagPending  = 0x01
const kFlagResolved = 0x02
const kFlagRejected = 0x04
const kFlagSettled  = 0x08

/*
 * Easy handling of asynchronous logic
 * */
class Promise {
  property then_handlers
  property catch_handlers
  property status
  property value

  constructor(callback) {
    @then_handlers    = []
    @catch_handlers   = []
    @status           = kFlagPending
    @value            = null

    // Invoke callback
    try {
      callback(->@resolve($0), ->@reject($0))
    } catch(e) {
      @reject(e)
    }
  }

  // Status checks
  is_pending  = @status & kFlagPending ? true : false
  is_resolved = @status & kFlagResolved ? true : false
  is_rejected = @status & kFlagRejected ? true : false
  is_settled  = @status & kFlagSettled ? true : false

  /*
   * Resolve this promise
   * */
  resolve(value = null) {
    unless @status == kFlagPending return null
    @status = kFlagResolved
    @value = value

    if @then_handlers.length {
      @status |= kFlagSettled
      @then_handlers.each(->(handler) spawn(->handler(value)))
      @then_handlers.clear()
    }

    null
  }

  /*
   * Reject this promise
   * */
  reject(value = null) {
    unless @status == kFlagPending return null
    @status = kFlagRejected
    @value = value

    if @catch_handlers.length {
      @status |= kFlagSettled
      @catch_handlers.each(->(handler) spawn(->handler(value)))
      @catch_handlers.clear()
    }

    null
  }

  /*
   * Invoked once the promise is resolved
   * */
  then(callback) {
    if @is_pending() @then_handlers << callback
    if @is_resolved() {
      @status |= kFlagSettled
      spawn(->callback(@value))
    }

    self
  }

  /*
   * Synchronously waits for this promise to finish
   * Throws an exception if the promise gets rejected
   * */
  wait {
    const n = new Sync.Notifier()

    @then(->n.close($0))
    @catch(->n.error($0))

    n.wait()
  }

  /*
   * Synchronously wait for this promise to settle
   * */
  wait_settle {
    const n = new Sync.Notifier()

    @then(->n.close(self))
    @catch(->n.close(self))

    n.wait()
  }

  /*
   * Invoked once the promise is rejected
   * */
  catch(callback) {
    if @is_pending() @catch_handlers << callback
    if @is_rejected() {
      @status |= kFlagSettled
      spawn(->callback(@value))
    }

    self
  }

  /*
   * Convert to string
   * */
  to_s {
    const buf = new StringBuffer()

    buf.write("<Promise:")
    if @is_pending() {
      buf.write("pending")
    } else {
      if @is_resolved() {
        buf.write("resolved ")
      } else {
        buf.write("rejected ")
      }
      buf.write(@value)
    }
    buf.write(">")

    buf.to_s()
  }

  /*
   * Synchronously waits for a list of promises to resolve
   * Throws an error if any promise gets rejected
   * */
  static wait(promises) {
    if typeof promises ! "array" promises = arguments
    promises.map(->(p) p.wait())
  }

  /*
   * Synchronously waits for a list of promises to settle
   * Result list will contain list of promises
   * */
  static wait_settled(promises) {
    if typeof promises ! "array" promises = arguments
    promises.map(->(p) p.wait_settle())
  }

  /*
   * Returns a promise which resolves once all input promises have resolved
   * The returned promise will reject if any of the input promises have been rejected
   * */
  static all(promises) {
    if typeof promises ! "array" promises = arguments
    return new Promise(->(resolve, reject) {
      const results = Array.create(promises.length)
      let amount = 0
      promises.each(->(p, i) {
        p.then(->(v) {
          results[i] = v
          amount += 1
          if amount == promises.length resolve(results)
        })

        p.catch(->(e) reject(e))
      })
    })
  }

  /*
   * Returns a promise which resolves once all input promises have settled
   * The returned promise can only resolve and the result will be the list of input
   * promises
   * */
  static all_settled(promises) {
    if typeof promises ! "array" promises = arguments
    new Promise(->(resolve, reject) {
      const results = Array.create(promises.length)
      let amount = 0
      promises.each(->(p, i) {
        const handle = ->(v) {
          results[i] = p
          amount += 1
          if amount == promises.length resolve(results)
        }

        p.then(handle)
        p.catch(handle)
      })
    })
  }

  /*
   * Returns a promise which resolves with the first input promise that resolves
   * The returned promise will reject if the first promise to settle is rejected
   * */
  static race(promises) {
    if typeof promises ! "array" promises = arguments

    new Promise(->(resolve, reject) {
      promises.each(->(p) {
        p.then(resolve)
        p.catch(reject)
      })
    })
  }

  /*
   * Returns a promise which resolves with the first promise to settle
   * */
  static race_settled(promises) {
    if typeof promises ! "array" promises = arguments

    new Promise(->(resolve, reject) {
      promises.each(->(p) {
        p.then(->resolve(p))
        p.catch(->resolve(p))
      })
    })
  }

  /*
   * Returns a resolved / rejected promise
   * */
  static resolve(value) = new Promise(->(resolve) resolve(value))
  static reject(value)  = new Promise(->(_, reject) reject(value))
}

export = Promise
