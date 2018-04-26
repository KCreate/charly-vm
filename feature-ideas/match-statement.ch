// Match Statement
// Every match statement requires an exhaustive match condition.
// Inspired by languages like Rust and the recent TC39 proposal for a match statement in ECMAScript.
//
// Gets transpiled to:
//   %tmp = condition
//   if %tmp == false push("condition is false") goto .END
//   if %tmp == 1 goto .COND_1_BODY
//   unless %tmp == 2 goto .COND_1_NOMATCH
//   .COND_1_BODY
//     push("condition is 1 or 2")
//     goto .END
//   .COND_1_NOMATCH
//   if %tmp.foo == "hello world" push("condition has a foo property containing hello world") goto .END
//   if %tmp.bar.baz == 25 push("condition has a bar property containing an object with a baz property containing 25") goto .END
//   if %tmp == [1, 2] push("condition is an array of length 2, containing the numbers 1 and 2") goto .END
//   if %tmp.foo == n push("condition has a foo property containing n") goto .END
//   push("matches every single statement")
// .END
// result = pop()
const result = match (condition) {
  false => "condition is false",
  1 || 2 => "condition is 1 or 2",
  { foo: "hello world" } => "condition has a foo property containing hello world",
  { bar: { baz: 25 } } => "condition has a bar property containing an object with a baz property containing 25",
  [1, 2] => "condition is an array of length 2, containing the numbers 1 and 2",
  { foo: n } => "condition has a foo property containing n",
  _ => "matches every single statement"
}

// Match statement where the condition can be used in the match conditions.
// 
// Gets transpiled to:
//   %tmp = condition
//   {
//     const n = %tmp
//     if n < 0 push("condition is negative") goto .END
//     if %tmp == 50 push("condition is 50") goto .END
//     if n >= 100 push("condition is 100 or more") goto .END
//     push("condition is the range 0..100")
//   .END
//   }
const result = match (condition) -> n {
  n < 0 => "condition is negative",
  50 => "condition is 50",
  n >= 100 => "condition is 100 or more",
  _ => "condition is the range 0..100"
}

// Match statement can also contain blocks
// If a match statement contains at least one block, it can't yield a value and thus becomes
// a statement and can't be used as an expression anymore
//
// Gets transpiled to:
//   %tmp = condition
//   {
//     const s = %tmp
//     if %tmp == 200 <block1> goto .END
//     if s >= 400 <block2> goto .END
//     <block3>
//   .END
//   }
match (status) -> s {
  200 => { <block1> }
  s >= 400 { <block2> }
  _ => { <block3> }
}
