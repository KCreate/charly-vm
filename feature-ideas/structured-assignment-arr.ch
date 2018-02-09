// Array destructuring initialisation
//
// Desugared to:
//
// let num
// let foo
// %tmp = obj
// num = %tmp[0]
// foo = %tmp[1]
let [num, foo] = obj

// Array destructuring assignment
//
// Desugared to:
//
// %tmp = obj
// num = %tmp[0]
// foo = %tmp[1]
[num, foo] = obj

// Array destructuring self initialisation
//
// Desugared to:
//
// let num
// let foo
// %tmp = obj
// num = self.num = %tmp[0]
// foo = self.foo = %tmp[1]
let [@num, @foo] = obj

// Array destructuring self assignment
//
// Desugared to:
//
// %tmp = obj
// self.num = %tmp[0]
// self.foo = %tmp[1]
[@num, @foo] = obj
