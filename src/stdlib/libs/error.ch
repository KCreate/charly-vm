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

const __internal_get_active_frame  = @"charly.vm.get_active_frame"
const __internal_get_parent_frame  = @"charly.vm.get_parent_frame"
const __internal_get_block_address = @"charly.vm.get_block_address"
const __internal_resolve_address   = @"charly.vm.resolve_address"

/*
 * Represents a stackframe inside the VM
 * */
class StackFrame {
  property id
  property caller
  property self_value
  property origin_address

  // Create a StackFrame obj from the obj returned by the builtin methods
  static from_internal(obj) = new StackFrame(obj.id, obj.caller, obj.self_value, obj.origin_address)

  // Returns the frame of the calling function
  static get_current = StackFrame.from_internal(__internal_get_active_frame()).parent()

  // Return the parent frame
  parent {
    const p = __internal_get_parent_frame(@id)
    if !p return null
    StackFrame.from_internal(p)
  }
}

/*
 * List of callbacks to be called before the vm terminates because
 * of an uncaught exception
 * */
const uncaught_exception_handlers = []

// Base error class
class Error {
  property message
  property stacktrace

  constructor(@message = "Unspecified error") {
    @stacktrace = Error.capture_stack_trace(null, 1)
  }

  /*
   * @klass.name : @message
   *   at Person.foo (/home/user/file.ch)
   *   at bar (/home/user/file.ch)
   *   at main (/home/user/file.ch)
   * */
  to_s {
    const buf = new String.Buffer(32)

    // Generate error description
    buf.write(@klass.name + " : " + @message)
    buf.write("\n")

    // Generate stack trace entries
    @stacktrace.each(->(frame, index) {
      const host_class = frame.caller.host_class
      const func_name  = frame.caller.name.length > 0 ? frame.caller.name : "??"
      const block_addr = __internal_get_block_address(frame.caller)
      const filename   = __internal_resolve_address(block_addr) || "??"

      buf.write("  at ")
      if host_class buf.write(host_class.name + ".")
      buf.write(func_name)
      buf.write(" (" + filename + ")")

      // Don't print newline on last entry
      if index < @stacktrace.length - 1 buf.write("\n")
    })

    buf.to_s()
  }

  /*
   * Rethrow this exception
   * */
  rethrow {
    throw self
  }

  /*
   * Retrace the stacktrace from the current position on
   * */
  retrace_stack {
    @stacktrace = Error.capture_stack_trace(null, 1)
  }

  /*
   * The amount of stack frames to include
   * in the stack trace
   *
   * If set to anything but a number, no frames
   * will be printed
   * */
  static property stack_trace_limit = 20

  /*
   * Captures a stacktrace
   *
   * skip_func:       do not append frames to list, until this function is hit
   * skip_n:          skip 'n' frames before appending to list, ignores skip_func
   * */
  static capture_stack_trace(skip_func = null, skip_n = 0) {
    const frames = []

    // Get frame of the caller
    let top = StackFrame.get_current().parent()

    // The skip_func argument allows us to skip all
    // frames until we reach a specific function
    let allow_insertion = true
    if typeof skip_func == "function" allow_insertion = false

    // Traverse frame hierarchy
    while top {
      if skip_n > 0 {
        skip_n -= 1
      } else {
        if !allow_insertion && top.caller == skip_func {
          allow_insertion = true
        } else {
          frames << top
        }
      }

      if frames.length >= Error.stack_trace_limit break

      // Load parent frame
      top = top.parent()
    }

    frames
  }

  /*
   * Register a new callback that will be called if there is an uncaught
   * exception. Callbacks are being called in insertion-order
   * */
  static register_uncaught_exception_handler(cb) {
    uncaught_exception_handlers << cb
    null
  }

  /*
   * Expect the callback to throw an exception
   * Will throw an exception itself if no exception gets thrown
   * */
  static expect(callback) {
    try {
      callback()
      throw new Error("Expected an exception to be thrown")
    } catch(e) {
      return e
    }
  }

  static property StackFrame = StackFrame
}

// Error class thrown by the VM
// Not meant to be used by the user directly
@"charly.vm.internal_error_class" = class InternalError extends Error {
  constructor(message) {
    super(message)
    @stacktrace = Error.capture_stack_trace(null, 1)
    throw self
  }
}

/*
 * Global uncaught exception handler
 * This method is responsible for calling all the termination
 * callbacks and the optional user provided exception handler
 * */
@"charly.vm.uncaught_exception_handler" = ->(exception) {
  try {

    // Invoke all uncaught exception handlers
    uncaught_exception_handlers.each(->(handler) {
      if typeof handler == "function" {
        handler(exception)
      }
    })

    print("Uncaught exception")
    print(exception)
  } catch(e) {
    print("Exception thrown inside uncaught exception handler, exiting now")
    print.dir(e)
  } finally {

    // Status code 1 means an error has occured
    exit(1)
  }
}

export = Error
