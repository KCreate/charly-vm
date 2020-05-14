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

  describe("test", ->{

    it("evaluates", ->{
      let ran = false
      func foo() {
        ran = true
      }

      switch foo() {}

      assert(ran, true)
    })

    it("runs the test only once", ->{
      let ran = 0
      func foo() {
        ran += 1
      }

      switch foo() {
        case false {}
        case false {}
        case false {}
        case false {}
        case false {}
      }

      assert(ran, 1)
    })

    it("runs a block if at least 1 value passes", ->{
      let passed
      let amount_run = 0

      func obj() {
        amount_run += 1
        return 25;
      }

      switch obj() {
        case 0, 5, 10, 15, 20, 25 {
          passed = true
        }
      }

      assert(amount_run, 1)
      assert(passed, true)
    })

  })

  describe("branching", ->{

    it("branches to a block", ->{
      let num = 25
      let got = false

      switch num {
        case 20 {
          got = 20
        }

        case 25 {
          got = 25
        }

        case 30 {
          got = 30
        }
      }

      assert(got, 25)
    })

    it("can have multiple values in one case", ->{
      let num = 25
      let got = false

      switch num {
        case 20, 25, 30 {
          got = true
        }
      }

      assert(got, true)
    })

    it("runs the default block", ->{
      let num = 25
      let got = false

      switch num {
        case 20 {
          got = true
        }

        default {
          got = "default was run"
        }
      }

      assert(got, "default was run")
    })

    it("runs duplicate cases", ->{
      let ran = 0

      func foo() {
        ran += 1
        return false
      }

      switch 20 {
        case foo() {}
        case foo() {}
        case foo() {}
        case foo() {}
        case foo() {}
      }

      assert(ran, 5)
    })

    it("returns after the first block that succeeded", ->{
      let ran = 0

      func foo() {
        ran += 1
        return true
      }

      switch true {
        case foo(), foo(), foo() {}
      }

      assert(ran, 1)
    })

    it("can return from the parent function", ->{
      func foo() {
        switch 25 {
          case 25 {
            return "hello world"
          }
        }
      }

      let result = foo()

      assert(result, "hello world")
    })

  })

  describe("control flow", ->{

    it("breaks out of a block", ->{
      let num = 25

      switch num {
        case 25 {
          break
        }

        default {
          num = 50
        }
      }

      assert(num, 25)
    })

    it("breaks out of the default block", ->{
      let num = 25

      switch num {
        default {
          break
          num = 50
        }
      }

      assert(num, 25)
    })

    it("captures the break event", ->{
      let i = 0;

      loop {

        // this should behave like a no-op
        // as the break is captured by the switch
        switch 25 {
          case 25 {
            break
          }
        }

        i += 1

        if i == 2 {
          break
        }
      }

      assert(i, 2)
    })

    it("nested loops break correctly", ->{
      let passed = false

      switch 25 {
        case 25 {
          loop {
            break
          }

          passed = true
        }
      }

      assert(passed, true)
    })

    it("lets continue bubble up", ->{
      let sum = 0

      let i = 0
      loop {
        i += 1

        switch i % 2 {
          case 1 {
            continue
          }
        }

        sum += i

        if i == 10 {
          break
        }
      }

      assert(i, 10)
      assert(sum, 30)
    })

  })

}
