ignoreconst {
  require = Charly.internals.get_method("require")
  print = Charly.internals.get_method("print")
  write = Charly.internals.get_method("write")

  Charly.internals.set_primitive_object = Charly.internals.get_method("set_primitive_object")
  Charly.internals.set_primitive_class = Charly.internals.get_method("set_primitive_class")
  Charly.internals.set_primitive_array = Charly.internals.get_method("set_primitive_array")
  Charly.internals.set_primitive_string = Charly.internals.get_method("set_primitive_string")
  Charly.internals.set_primitive_number = Charly.internals.get_method("set_primitive_number")
  Charly.internals.set_primitive_function = Charly.internals.get_method("set_primitive_function")
  Charly.internals.set_primitive_boolean = Charly.internals.get_method("set_primitive_boolean")
  Charly.internals.set_primitive_null = Charly.internals.get_method("set_primitive_null")

  Object = class Object {
    func tap(cb) {
      cb(self)
    }
  }
  Charly.internals.set_primitive_object(Object);

  Array = class Object {
    func each(cb) {
      let i = 0

      while i < @length {
        cb(self[i], i, self)
        i += 1
      }

      self
    }
  }
  Charly.internals.set_primitive_array(Array);

  Number = class Number {
    func times(cb) {
      let i = 0

      while i < self {
        cb(i)
        i += 1
      }

      null
    }
  }
  Charly.internals.set_primitive_number(Number);
}
