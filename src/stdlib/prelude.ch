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

ignoreconst {

  // Cache some internal methods
  const __internal_get_method       = Charly.internals.get_method
  const __internal_write            = __internal_get_method("write")
  const __internal_getn             = __internal_get_method("getn")
  const __internal_import           = __internal_get_method("import")
  const __internal_exit             = __internal_get_method("exit")

  let __internal_standard_libs_names
  let __internal_standard_libs
  let __internal_import_cache = {}

  // Import method for loading other files and libraries
  __charly_internal_import = ->(path, source) {
    const ignore_cache = arguments[2] ? true : false

    // Search for the path in the list of standard libraries
    let is_stdlib = false
    let i = 0
    while (i < __internal_standard_libs_names.length) {
      const value = __internal_standard_libs_names[i]
      if (value == path) {
        is_stdlib = true
        break
      }

      i += 1
    }

    if (is_stdlib) {
      path = __internal_standard_libs[path]
    }

    unless ignore_cache {
      const cache_entry = __internal_import_cache[path]
      if cache_entry return cache_entry.v
    }

    const v = __internal_import(path, source)
    __internal_import_cache[path] = {v}
    v
  }

  // The names of all standard libraries that come with charly
  __internal_standard_libs_names = [
    "math",
    "time",
    "heap",
    "unittest"
  ]

  // All libraries that come with charly
  __internal_standard_libs = {
    math: "_charly_math",
    time: "_charly_time",
    heap: "_charly_heap",
    unittest: "_charly_unittest"
  }

  // The runtime constructor is a special method the VM keeps track of.
  // All object allocations inside charly run through this method
  //
  // It walks the class hierarchy and executed all the necessary constructors
  const set_runtime_constructor = Charly.internals.get_method("set_runtime_constructor")
  set_runtime_constructor(->(klass, args, obj) {

    // Build queue of constructors to run
    const constructor_queue = []
    let tmp_klass = klass
    while tmp_klass ! Value {
      constructor_queue << tmp_klass.constructor
      tmp_klass = tmp_klass.parent_class
    }

    // Run constructor, from top to bottom
    while constructor_queue.length {
      const constructor = constructor_queue.pop()
      if constructor constructor.call(obj, args)
    }

    return obj
  })

  // Method to modify the primitive objects
  const set_primitive_value = Charly.internals.get_method("set_primitive_object")
  const set_primitive_object = Charly.internals.get_method("set_primitive_object")
  const set_primitive_class = Charly.internals.get_method("set_primitive_class")
  const set_primitive_array = Charly.internals.get_method("set_primitive_array")
  const set_primitive_string = Charly.internals.get_method("set_primitive_string")
  const set_primitive_number = Charly.internals.get_method("set_primitive_number")
  const set_primitive_function = Charly.internals.get_method("set_primitive_function")
  const set_primitive_generator = Charly.internals.get_method("set_primitive_generator")
  const set_primitive_boolean = Charly.internals.get_method("set_primitive_boolean")
  const set_primitive_null = Charly.internals.get_method("set_primitive_null")

  // Value class
  //
  // All classes automatically depend on this class
  Value = set_primitive_value((import "_charly_value")())

  // The rest of the primitive classes
  Object = set_primitive_object((import "_charly_object")(Value))
  Number = set_primitive_number((import "_charly_number")(Value))
  Array = set_primitive_array((import "_charly_array")(Value))
  Class = set_primitive_class((import "_charly_class")(Value))
  String = set_primitive_string((import "_charly_string")(Value))
  Function = set_primitive_function((import "_charly_function")(Value))
  Generator = set_primitive_generator((import "_charly_generator")(Value))
  Boolean = set_primitive_boolean((import "_charly_boolean")(Value))
  Null = set_primitive_null((import "_charly_null")(Value))

  // Write a value to stdout, without a trailing newline
  write = func write {
    arguments.each(->(a) __internal_write(a.to_s()))
    write
  }
  write.dir = func write_dir {
    arguments.each(->(v) __internal_write(v))
    write.dir
  }

  // Write a value to stdout, with a trailing newline
  print = func print {
    arguments.each(->(v) __internal_write(v.to_s()))
    __internal_write("\n")
    write
  }
  print.dir = func print_dir {
    arguments.each(->(v) __internal_write(v))
    __internal_write("\n")
    print.dir
  }

  // Exits the program with a given status code
  exit = func exit {
    __internal_exit(arguments.length ? $0 : 0)
  }

  const defer_internals = import "_charly_defer"
  defer          = ->(cb, period = 0) new defer_internals.TimerHandle(cb, period)
  defer.interval = ->(cb, period = 0) new defer_internals.IntervalHandle(cb, period)

  // Setup the charly object
  Charly.io = {
    write,
    print,
    getn: ->{
      if arguments.length > 0 write($0)
      return __internal_getn()
    },
    dirname: __internal_get_method("dirname")
  }
  Charly.math = import "math"
  Charly.time = import "time"
  Charly.heap = import "heap"
}
