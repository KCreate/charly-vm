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

    /*
     * Calls *self* using ctx as the target object
     * and the values inside args as the parameters
     * */
    call(ctx, args = []) {
      __internal_function_call(self, ctx, args)
    }

    /*
     * Call this function asynchronously
     *
     * if *self* is a charly function this will invoke it via defer
     * if *self* is a c function this will start a worker thread
     *
     * TODO: Replace this code with a Promise implementation once implemented
     * */
    call_async(ctx, args = [], callback = null) {
      if @is_charly_func() {
        defer(->{
          callback(self.call(ctx, args))
        })
      } else {
        const error_template = new Error(self.name + ": ")
        __internal_function_call_async(self, args, ->(a1, error, a3, a4) {
          if typeof error == "string" {
            error_template.message += error
            error = error_template
          }

          if callback callback(a1, error, a3, a4)
        })
      }

      return null
    }

    /*
     * Bind a self value for this function
     * */
    bind_self(value) {
      __internal_function_bind_self(self, value)
      self
    }

    /*
     * Unbind the self value
     * */
    unbind_self {
      __internal_function_unbind_self(self)
      self
    }

    /*
     * Checks wether this function is a C function
     * */
    is_c_func      = __internal_function_is_cfunc(self)
    is_charly_func = !@is_c_func()
  }
}
