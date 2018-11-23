// Match Statement
//
// Gets transpiled to:
//   %tmp = condition
//   if %tmp == false push("condition is false") goto .END
//   if %tmp == 1 goto .COND_1_BODY
//   if %tmp == 2 goto .COND_1_BODY
//   if %tmp == 3 goto .COND_1_BODY
//   goto .COND_1_NOMATCH
//   .COND_1_BODY
//     push("condition is 1 or 2")
//     goto .END
//   .COND_1_NOMATCH
//   push("matches every single statement")
// .END
// result = pop()
const result = match (condition) {
  false => "condition is false"
  1 || 2 || 3 => "condition is 1 or 2"
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
  n < 0 => "condition is negative"
  50 => "condition is 50"
  n >= 100 => "condition is 100 or more"
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
