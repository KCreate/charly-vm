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

export = ->(Base) {
  return class Function extends Base {

    // Calls this function with a self value and arguments array
    call(args = [], async = false) {
      if async {
        return spawn.promise(->self.call(args))
      }

      if typeof args ! "array" {
        throw new Error("Expected first argument to be an array")
      }

      __syscall("calldynamic", self, args)
    }

    // Call *self* with self value and arguments array
    call_member(ctx, args = [], async = false) {
      if async {
        return spawn.promise(->self.call_member(ctx, args))
      }

      if typeof args ! "array" {
        throw new Error("Expected first argument to be an array")
      }

      __syscall("callmemberdynamic", ctx, self, args)
    }

    // Unbind the self value
    unbind_self {
      __syscall("clearboundself", self)
      self
    }
  }
}
