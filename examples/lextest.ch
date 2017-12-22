func f(a, b) {
  const funcargs = arguments
  const arguments = null
  return funcargs;
}
