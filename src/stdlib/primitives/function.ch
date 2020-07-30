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

const __internal_function_bind_self   = @"charly.primitive.function.bind_self"
const __internal_function_unbind_self = @"charly.primitive.function.unbind_self"
const __internal_function_call        = @"charly.primitive.function.call"
const __internal_function_call_async  = @"charly.primitive.function.call_async"
const __internal_function_is_cfunc    = @"charly.primitive.function.is_cfunc"
__internal_function_call.push_return_value = false

export = ->(Base) {
  return class Function extends Base {

    // Calls this function with a self value and arguments array
    call(ctx, args = [], async = false) {
      unless async return __internal_function_call(self, ctx, args)

      if @is_charly_func() {
        return spawn.promise(->self.call(ctx, args))
      }

      const error_template = new Error(self.name + ": ")
      __internal_function_call_async(self, args, ->(a1, error, a2, a3) {

        // Exceptions thrown by the VM are delivered as a string
        if typeof error == "string" {
          error_template.message += error
          error = error_template
          return reject(error)
        }

        resolve(a1, a2, a3)
      })
    }

    // Bind a self value for this function
    bind_self(value) {
      __internal_function_bind_self(self, value)
      self
    }

    // Unbind the self value
    unbind_self {
      __internal_function_unbind_self(self)
      self
    }

    // Checks wether this function is a C function or not
    is_c_func      = __internal_function_is_cfunc(self)
    is_charly_func = !@is_c_func()
  }
}
