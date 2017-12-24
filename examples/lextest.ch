func foo(a, b) {
  return arguments + 1 + b + __CHARLY_FUNCTION_ARGUMENTS
}

foo(1, 2)
