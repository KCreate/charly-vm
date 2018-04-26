// Do-While loops
//
// Gets transpiled to:
// .START
//   <block>
//   %tmp = <condition>
//   if %tmp goto .START
do {
  <block>
} while (<condition>);

// You can also omit the curly braces
//
// Gets transpiled to:
// .START
//   print("hello world")
//   %tmp = foo
//   if %tmp goto .START
do print("hello world") while foo;

// Alternatively you can use the interverted syntax
//
// Gets transpiled to:
// .START
//   <block>
//   %tmp = condition
//   unless %tmp goto .START
do {
  <block>
} until (<condition>);
