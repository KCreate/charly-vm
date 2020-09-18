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

// Relevant constants
const DIRECTORY_SEPARATOR = "/"
const CURRENT_DIRECTORY   = "."
const PARENT_DIRECTORY    = ".."
const HOME_DIRECTORY      = "~"

// Represents a file system path
class Path {
  property __is_absolute
  property elements

  constructor(v) {
    if v.is_a(Path) {
      @__is_absolute = v.is_absolute()
      @elements    = v.elements.copy()
    } else if typeof v == "string" && v.length {
      @__is_absolute = v[0] == "/"
      @elements    = v.split(DIRECTORY_SEPARATOR)
                      .map(->(part) part.trim())
                      .filter(->(part) part.length)
    } else {
      @__is_absolute = false
      @elements    = []
    }
  }

  // Append another path to this path
  append(other) {
    unless other.is_a(Path) other = new Path(other)

    if other.is_absolute() {
      throw new Error("Can't append an absolute path")
    }

    const new_elements = []
    @elements.each(->(e) new_elements.push(e))
    other.elements.each(->(e) new_elements.push(e))

    @elements = new_elements

    self
  }

  // Prepend another path to this path
  prepend(other) {
    unless other.is_a(Path) other = new Path(other)

    if @is_absolute() {
      throw new Error("Can't prepend to an absolute path")
    }

    const new_elements = []
    other.elements.each(->(e) new_elements.push(e))
    @elements.each(->(e) new_elements.push(e))

    @elements = new_elements

    if other.is_absolute() {
      @__is_absolute = true
    }

    self
  }

  // Change path
  cd(path) {
    @append(path)
    @normalize()
    self
  }

  // Removes unneccessary elements of the path
  normalize {
    let i = 0
    const stack = []

    while i < @elements.length {
      const current = @elements[i]

      switch current {
        case CURRENT_DIRECTORY {

          // Relative paths whose only element is a "." can keep it
          // In any other case we remove those
          if !@__is_absolute && @elements.length == 1 {
            stack.push(current)
          }
        }

        // ".." is preserved in the following cases
        //
        // - at the beginning of relative paths
        // - if the previous element pushed onto the stack is also a ".."
        case PARENT_DIRECTORY {
          if stack.length {
            if stack.last() == PARENT_DIRECTORY {
              stack.push(PARENT_DIRECTORY)
            } else {
              stack.pop()
            }
          } else {
            if !@__is_absolute {
              stack.push(PARENT_DIRECTORY)
            }
          }
        }

        default {
          stack.push(current)
        }
      }

      i += 1
    }

    @elements = stack
    self
  }

  // Expand the path
  expand(base = Path.getwd()) {
    if @is_absolute() return self

    // Expand home directory
    if @elements.length && @elements[0] == HOME_DIRECTORY {
      @elements.shift()
      @prepend(Path.get_home())
      @__is_absolute = true
      return self
    }

    unless base.is_a(Path) base = new Path(base)
    self.prepend(base)

    @normalize()

    self
  }

  // Removes the last component of the path
  remove_last {
    if @elements.length @elements.pop()
    self
  }

  // Removes an extension if present
  remove_extension {
    unless @elements.length return self
    const last = @elements.last()

    // Try to split on the "." character
    const parts = last.split(".")

    if parts.length > 1 {
      parts.pop()
      @elements.pop()
      @elements.push(parts.join("."))
    }

    self
  }

  // Replace the extension of the last component of this path
  replace_extension(ext) {}

  // Replace the last component of this path
  replace_filename(name) {}

  // Returns the last component of the path
  filename {}

  // Returns the extension of the last component of the path
  extension {}

  // Returns the last component of the path, without the extension
  stem {}

  // Returns the parent directory
  parent_directory {}

  // Checks wether this is an absolute path
  is_absolute = @__is_absolute

  // Checks wether this is a home path
  is_home = @elements.first() == HOME_DIRECTORY

  // Checks wether this is a relative path
  is_relative = !@__is_absolute

  // Check wether this path begins with another path
  begins_with(other) {}

  // Check wether this path ends with another path
  ends_with(other) {}

  // Check wether this path contains another path
  contains(other) {}

  // Returns this path as a string
  to_s {
    if @elements.length == 0 {
      return @__is_absolute ? "/" : "."
    }

    const elements_string = @elements.join(DIRECTORY_SEPARATOR)

    // Add initial slash for absolute strings
    if @__is_absolute {
      return "/" + elements_string
    }

    elements_string
  }

  // Returns the absolute path of the file where the call originated
  static current {
    const frame = Error.StackFrame.get_current()
    const parent_frame = frame.parent()
    const filename = parent_frame.get_filename()

    new Path(filename)
  }

  // Returns the absolute path of the directory where the call originated
  static current_dir {
    const frame = Error.StackFrame.get_current()
    const parent_frame = frame.parent()
    const filename = parent_frame.get_filename()

    const path = new Path(filename)
    path.remove_last()
    path
  }

  // Quick access methods
  static normalize(path) = (new Path(path)).normalize()
  static expand(path, base = Path.getwd()) = (new Path(path)).expand(base)

  // Returns the current working directory
  static getwd = new Path(ENVIRONMENT["PWD"])

  // Changes the current working directory
  static chwd(path) {}

  // Returns the users home directory
  static get_home = new Path(ENVIRONMENT["HOME"])
}

export = Path
