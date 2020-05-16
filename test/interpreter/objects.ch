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

  describe("writing", ->{

    it("adds properties to objects", ->{
      class Box {}
      const myBox = new Box()
      myBox.name = "charly"
      myBox.age = 16

      assert(myBox.name, "charly")
      assert(myBox.age, 16)
    })

    it("adds functions to objects", ->{
      class Box {}
      const myBox = new Box()
      myBox.name = "charly"
      myBox.age = 16
      myBox.to_s = func() {
        assert(self == myBox, true)
        myBox.do_stuff = func() {
          "it works!"
        }

        myBox.name + " - " + myBox.age
      }

      assert(myBox.name, "charly")
      assert(myBox.age, 16)
      assert(myBox.do_stuff, null)
      assert(myBox.to_s(), "charly - 16")
      assert(myBox.do_stuff(), "it works!")
    })

    it("assigns via index expressions", ->{
      const box = {
        name: "test"
      }

      box["name"] = "it works"

      assert(box["name"], "it works")
    })

    describe("pretty-printing", ->{

      it("renders arrays", ->{
        let arr = [1, 2, 3, 4]

        let render = arr.to_s()

        assert(render, "[1, 2, 3, 4]")
      })

      it("prints circular arrays", ->{
        let a = []
        let b = []

        a << b
        b << a

        let render = a.to_s()

        assert(render, "[[[...]]]")
      })

    })

    describe("stdlib methods", ->{

      describe("tap", ->{

        "leonard".tap(->(name) {
          assert(name, "leonard")
        })

      })

      describe("pipe", ->{

        let v = 0

        const a = ->v += $0
        const b = ->v *= $0
        const c = ->v -= $0

        10.pipe(a, b, c)

        assert(v, 90)
      })

      describe("transform", ->{

        const r = 10.transform(
          ->(v) {
            v + 10
          },
          ->(v) {
            v * 10
          },
          ->(v) {
            v / 2
          }
        )

        assert(r, 100)
      })

    })

  })

}
