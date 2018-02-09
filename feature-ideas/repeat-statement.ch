// Repeat statement with known expression
//
// Desugared to:
//
// print("hello world")
// print("hello world")
// print("hello world")
// print("hello world")
// print("hello world")
repeat 5 {
  print("hello world")
}

// Repeat statement with unknown expression
//
// Desugared to:
//
// %i = 0
// %limit = count
// while %i < %limit { <body> }
repeat count {
  print("hello world")
}
