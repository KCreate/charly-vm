switch (foo) {
  case (foo, bar, baz) {}
  case (foo, bar, baz) {}
  default {}
}

switch (foo) {
  case (foo, bar, baz) foo;
  case (foo, bar, baz) foo;
  default foo;
}
