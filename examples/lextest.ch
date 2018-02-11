func foo(arg) {
  let arg // error

  arguments // [arg]

  let arguments // works!

  arguments // null
}
