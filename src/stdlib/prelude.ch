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

// Setup the global scope
@"charly.vm.globals"        = {}
@"charly.vm.globals".Charly = { globals: @"charly.vm.globals" }

// Load internal methods
const __internal_write           = @"charly.vm.write"
const __internal_getn            = @"charly.vm.getn"
const __internal_import          = @"charly.vm.import"
const __internal_dirname         = @"charly.vm.dirname"
const __internal_exit            = @"charly.vm.exit"
__internal_exit.halt_after_return = true

// Setup some information about our runtime environment
Charly.globals.ARGV         = @"charly.vm.argv"
Charly.globals.ENVIRONMENT  = @"charly.vm.env"
const CHARLYVMDIR = ENVIRONMENT["CHARLYVMDIR"] || ->{throw "Missing CHARLYVMDIR environment variable"}()

// Setup import function
const BUILTIN_LIBS = {
  unittest: "src/stdlib/libs/unittest/lib.ch"
}

const IMPORT_CACHE = {}
Charly.globals.__charly_internal_import = ->(path, source, ignore_cache = false) {

  // Check if we're importing a builtin library
  const lib = BUILTIN_LIBS[path]
  if lib {
    path = CHARLYVMDIR + "/" + lib
  }

  unless ignore_cache {
    const cache_entry = IMPORT_CACHE[path]
    if cache_entry return cache_entry.v
  }

  const module_fn = __internal_import(path, source)
  unless module_fn throw new Error("Import didn't return a module function")
  const v = module_fn({})
  IMPORT_CACHE[path] = {v}
  v
}

// Setup primitive classes
Charly.globals.Value     = @"charly.vm.primitive.value"     = (import "./primitives/value.ch")()
Charly.globals.Object    = @"charly.vm.primitive.object"    = (import "./primitives/object.ch")(Value)
Charly.globals.Array     = @"charly.vm.primitive.array"     = (import "./primitives/array.ch")(Value)
Charly.globals.Boolean   = @"charly.vm.primitive.boolean"   = (import "./primitives/boolean.ch")(Value)
Charly.globals.Class     = @"charly.vm.primitive.class"     = (import "./primitives/class.ch")(Value)
Charly.globals.Function  = @"charly.vm.primitive.function"  = (import "./primitives/function.ch")(Value)
Charly.globals.Null      = @"charly.vm.primitive.null"      = (import "./primitives/null.ch")(Value)
Charly.globals.Number    = @"charly.vm.primitive.number"    = (import "./primitives/number.ch")(Value)
Charly.globals.String    = @"charly.vm.primitive.string"    = (import "./primitives/string.ch")(Value)
Charly.globals.Frame     = @"charly.vm.primitive.frame"     = (import "./primitives/frame.ch")(Value)

// Setup some additional data structures

// Write a value to stdout, without a trailing newline
Charly.globals.write = func write {
  arguments.each(->(a) __internal_write(a.to_s()))
  write
}
Charly.globals.write.dir = func write_dir {
  arguments.each(->(v) __internal_write(v))
  write.dir
}

// Write a value to stdout, with a trailing newline
Charly.globals.print = func print {
  arguments.each(->(v) __internal_write(v.to_s()))
  __internal_write("\n")
  write
}
Charly.globals.print.dir = func print_dir {
  arguments.each(->(v) __internal_write(v))
  __internal_write("\n")
  print.dir
}

// Exits the program with a given status code
Charly.globals.exit = func exit(status = 0) {
  __internal_exit(status)
}

// Setup the charly object
Charly.io = {
  write,
  print,
  getn: ->(msg = null) {
    if msg write(msg)
    return __internal_getn()
  },
  dirname: __internal_dirname
}


// Some libraries and data structures
Charly.globals.CircularBuffer = import "./libs/circular_buffer.ch"
Charly.globals.StringBuffer   = import "./libs/stringbuffer.ch"
Charly.globals.Error          = import "./libs/error.ch"
Charly.globals.Heap           = import "./libs/heap.ch"
Charly.globals.Math           = import "./libs/math.ch"
Charly.globals.Path           = import "./libs/path.ch"
Charly.globals.Queue          = import "./libs/queue.ch"
Charly.globals.Sync           = import "./libs/sync.ch"
Charly.globals.Time           = import "./libs/time.ch"

// Quick access to commonly used stuff
Charly.globals.Promise = Sync.Promise
Charly.globals.Channel = Sync.Channel
Charly.globals.spawn   = Sync.spawn
Charly.globals.sleep   = Sync.sleep
