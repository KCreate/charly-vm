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
  property elements

  constructor(v) {
    if v.is_a(Path) {
      @elements = v.elements.copy()
    } else if typeof v == "string" {
      @elements = v.split(DIRECTORY_SEPARATOR).map(->$0.trim())
    } else {
      @elements = ["."]
    }
  }

  // Append another path to this path
  append(other) {
    unless other.is_a(Path) other = new Path(other)

    const new_elements = []
    @elements.each(->(e) new_elements.push(e))
    other.elements.each(->(e) new_elements.push(e))

    @elements = new_elements

    @normalize()

    self
  }

  // Prepend another path to this path
  prepend(other) {
    unless other.is_a(Path) other = new Path(other)

    const new_elements = []
    other.elements.each(->(e) new_elements.push(e))
    @elements.each(->(e) new_elements.push(e))

    @elements = new_elements

    @normalize()

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
      const element = @elements[i]

      switch element {
        case "" {
          unless i == 0 break
          stack.push(element)
        }

        case PARENT_DIRECTORY {
          if stack.length {
            stack.pop()
          } else {
            stack.push("..")
          }
        }

        case CURRENT_DIRECTORY {
          unless i == @elements.length - 1 {
            break
          }

          stack.push(element)
        }

        default {
          stack.push(element)
        }
      }

      i += 1
    }

    @elements = stack

    self
  }

  // Resolve the path
  resolve(base = Path.getwd()) {
    if @is_absolute() return self

    // Resolve home directory
    if @is_home() {
      @elements.shift()
      @prepend(Path.get_home())
      return self
    }

    unless base.is_a(Path) base = new Path(base)
    self.prepend(base)
    self
  }

  // Returns this path, relative to another path
  relative_to(other) {}

  // Removes the last component of the path
  remove_filename {
    @elements.pop()
    self
  }

  // Removes an extension if present
  remove_extension {}

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
  is_absolute = @elements.first() == ""

  // Checks wether this is a home path
  is_home = @elements.first() == HOME_DIRECTORY

  // Checks wether this is a relative path
  is_relative = !@is_absolute() && !@is_home()

  // Check wether this path begins with another path
  begins_with(other) {}

  // Check wether this path ends with another path
  ends_with(other) {}

  // Check wether this path contains another path
  contains(other) {}

  // Returns this path as a string
  to_s {
    @elements.join(DIRECTORY_SEPARATOR)
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
    path.remove_filename()
    path
  }

  // Normalizes a path
  static normalize(path) = (new Path(path)).normalize()

  // Returns the current working directory
  // TODO: Implement with native method
  static getwd { ENVIRONMENT["PWD"] }

  // Changes the current working directory
  static chwd(path) {}

  // Returns the users home directory
  static get_home { ENVIRONMENT["HOME"] }
}

export = Path
