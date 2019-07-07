export = {
  PI: 3.14159265358979323846,
  E: 2.7182818284590451,
  LOG2: 0.69314718055994529,
  LOG10: 2.3025850929940459,

  cos: Charly.internals.get_method("Math::cos"),
  cosh: Charly.internals.get_method("Math::cosh"),
  acos: Charly.internals.get_method("Math::acos"),
  acosh: Charly.internals.get_method("Math::acosh"),

  sin: Charly.internals.get_method("Math::sin"),
  sinh: Charly.internals.get_method("Math::sinh"),
  asin: Charly.internals.get_method("Math::asin"),
  asinh: Charly.internals.get_method("Math::asinh"),

  tan: Charly.internals.get_method("Math::tan"),
  tanh: Charly.internals.get_method("Math::tanh"),
  atan: Charly.internals.get_method("Math::atan"),
  atanh: Charly.internals.get_method("Math::atanh"),

  cbrt: Charly.internals.get_method("Math::cbrt"),
  sqrt: Charly.internals.get_method("Math::sqrt"),

  ceil: Charly.internals.get_method("Math::ceil"),
  floor: Charly.internals.get_method("Math::floor"),

  log: Charly.internals.get_method("Math::log"),

  rand: Charly.internals.get_method("Math::rand")
}
