class Math {
  func constructor {
    throw "Cannot initialize an instance of the Math class"
  }

  static property PI    = 3.14159265358979323846
  static property E     = 2.7182818284590451
  static property LOG2  = 0.69314718055994529
  static property LOG10 = 2.3025850929940459

  static property cos   = Charly.internals.get_method("Math::cos")
  static property cosh  = Charly.internals.get_method("Math::cosh")
  static property acos  = Charly.internals.get_method("Math::acos")
  static property acosh = Charly.internals.get_method("Math::acosh")

  static property sin   = Charly.internals.get_method("Math::sin")
  static property sinh  = Charly.internals.get_method("Math::sinh")
  static property asin  = Charly.internals.get_method("Math::asin")
  static property asinh = Charly.internals.get_method("Math::asinh")

  static property tan   = Charly.internals.get_method("Math::tan")
  static property tanh  = Charly.internals.get_method("Math::tanh")
  static property atan  = Charly.internals.get_method("Math::atan")
  static property atanh = Charly.internals.get_method("Math::atanh")

  static property cbrt  = Charly.internals.get_method("Math::cbrt")
  static property sqrt  = Charly.internals.get_method("Math::sqrt")
  static property hypot = ->(x, y) Math.sqrt(x ** 2, y ** 2)

  static property ceil  = Charly.internals.get_method("Math::ceil")
  static property floor = Charly.internals.get_method("Math::floor")

  static property log   = Charly.internals.get_method("Math::log")

  static property rand  = Charly.internals.get_method("Math::rand")
}

export = Math
