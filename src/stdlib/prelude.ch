ignoreconst {

  // Cache some internal methods
  const __internal_get_method = Charly.internals.get_method
  const __internal_write = __internal_get_method("write")
  const __internal_getn = __internal_get_method("getn")
  const __internal_import = __internal_get_method("import")

  let __internal_standard_libs_names
  let __internal_standard_libs

  // Import method for loading other files and libraries
  __charly_internal_import = ->(path, source) {

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
      return __internal_standard_libs[path]
    }

    __internal_import(path, source)
  }

  // The names of all standard libraries that come with charly
  __internal_standard_libs_names = [
    "math"
  ]

  // All libraries that come with charly
  __internal_standard_libs = {
    math: import "_charly_math"
  }

  // Write a value to stdout, without a trailing newline
  write = func write {
    arguments.each(__internal_write)
    null
  }

  // Write a value to stdout, with a trailing newline
  print = func print {
    arguments.each(->(v) {
      __internal_write(v)
      __internal_write("\n")
    })

    null
  }

  // Setup the charly object
  Charly.io = {
    write,
    print,
    getn: ->(msg) {
      write(msg)
      return __internal_getn()
    },
    dirname: __internal_get_method("dirname")
  }

  // Method to modify the primitive objects
  const set_primitive_object = Charly.internals.get_method("set_primitive_object")
  const set_primitive_class = Charly.internals.get_method("set_primitive_class")
  const set_primitive_array = Charly.internals.get_method("set_primitive_array")
  const set_primitive_string = Charly.internals.get_method("set_primitive_string")
  const set_primitive_number = Charly.internals.get_method("set_primitive_number")
  const set_primitive_function = Charly.internals.get_method("set_primitive_function")
  const set_primitive_generator = Charly.internals.get_method("set_primitive_generator")
  const set_primitive_boolean = Charly.internals.get_method("set_primitive_boolean")
  const set_primitive_null = Charly.internals.get_method("set_primitive_null")

  class Value {
    func tap(cb) {
      cb(self)
    }
  }

  Object = set_primitive_object(class Object extends Value {});
  Class = set_primitive_class(class Class extends Value {});
  String = set_primitive_string(class String extends Value {});
  Function = set_primitive_function(class Function extends Value {});
  Generator = set_primitive_generator(class Generator extends Value {});
  Boolean = set_primitive_boolean(class Boolean extends Value {});
  Null = set_primitive_null(class Null extends Value {});

  Array = class Array extends Value {
    func each(cb) {
      let i = 0

      while i < @length {
        cb(self[i], i, self)
        i += 1
      }

      self
    }

    func map(cb) {
      let i = 0
      const new_array = []

      while i < @length {
        new_array << cb(self[i], i, self)
        i += 1
      }

      new_array
    }

    func filter(cb) {
      let i = 0
      const new_array = []

      while i < @length {
        const value = self[i]
        const include = cb(value, i, self)
        if include new_array << value
        i += 1
      }

      new_array
    }

    func reverse() {
      let i = @length - 1
      const new_array = []

      while i >= 0 {
        new_array << self[i]
        i -= 1
      }

      new_array
    }

    func contains(search) {
      let i = 0

      while i < @length {
        const value = self[i]
        if (value == search) return true;
        i += 1;
      }

      false
    }
  }
  set_primitive_array(Array);

  Number = class Number extends Value {
    func times(cb) {
      let i = 0

      while i < self {
        cb(i)
        i += 1
      }

      null
    }
  }
  set_primitive_number(Number);
}
