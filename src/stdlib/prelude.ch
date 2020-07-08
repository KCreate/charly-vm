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

// Setup globals and Charly object
@"charly.vm.globals"                          = {}
@"charly.vm.globals".Charly                   = { globals: @"charly.vm.globals" }
@"charly.vm.globals".__charly_internal_import = null

// Primitive classes
@"charly.vm.globals".Value     = null
@"charly.vm.globals".Object    = null
@"charly.vm.globals".Class     = null
@"charly.vm.globals".Array     = null
@"charly.vm.globals".String    = null
@"charly.vm.globals".Number    = null
@"charly.vm.globals".Function  = null
@"charly.vm.globals".Generator = null
@"charly.vm.globals".Boolean   = null
@"charly.vm.globals".Null      = null

// Error classes
@"charly.vm.globals".Error = null

// Global methods
@"charly.vm.globals".print = null
@"charly.vm.globals".write = null
@"charly.vm.globals".exit  = null
@"charly.vm.globals".defer = null

// Cache some internal methods
const __internal_get_argv        = @"charly.vm.get_argv"
const __internal_get_environment = @"charly.vm.get_environment"
const __internal_write           = @"charly.vm.write"
const __internal_getn            = @"charly.vm.getn"
const __internal_import          = @"charly.vm.import"
const __internal_dirname         = @"charly.vm.dirname"
const __internal_exit            = @"charly.vm.exit"
__internal_exit.halt_after_return = true

let __internal_standard_libs_names
let __internal_standard_libs
let __internal_import_cache = {}

// Import method for loading other files and libraries
__charly_internal_import = ->(path, source, ignore_cache = false) {

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
  "error",
  "heap",
  "unittest"
]

// All libraries that come with charly
__internal_standard_libs = {
  math: "_charly_math",
  time: "_charly_time",
  error: "_charly_error",
  heap: "_charly_heap",
  unittest: "_charly_unittest"
}

// Setup primitive classes
Value     = @"charly.vm.primitive.value"     = (import "_charly_value")()
Object    = @"charly.vm.primitive.object"    = (import "_charly_object")(Value)
Array     = @"charly.vm.primitive.array"     = (import "_charly_array")(Value)
Boolean   = @"charly.vm.primitive.boolean"   = (import "_charly_boolean")(Value)
Class     = @"charly.vm.primitive.class"     = (import "_charly_class")(Value)
Function  = @"charly.vm.primitive.function"  = (import "_charly_function")(Value)
Generator = @"charly.vm.primitive.generator" = (import "_charly_generator")(Value)
Null      = @"charly.vm.primitive.null"      = (import "_charly_null")(Value)
Number    = @"charly.vm.primitive.number"    = (import "_charly_number")(Value)
String    = @"charly.vm.primitive.string"    = (import "_charly_string")(Value)

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
exit = func exit(status = 0) {
  __internal_exit(status)
}

// Defer lib
defer = import "_charly_defer"

// Global error class
Error = import "_charly_error"

// Setup the charly object
Charly.argv        = __internal_get_argv()
Charly.environment = __internal_get_environment()
Charly.io = {
  write,
  print,
  getn: ->(msg = null) {
    if msg write(msg)
    return __internal_getn()
  },
  dirname: __internal_dirname
}

Charly.globals.Math = import "math"
Charly.globals.Time = import "time"
