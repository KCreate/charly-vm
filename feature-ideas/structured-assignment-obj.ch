// Object destructuring initialisation
//
// Desugared to:
//
// let num
// let foo
// %tmp = obj
// num = %tmp.num
// foo = %tmp.foo
let {num, foo} = obj

// Object destructuring assignment
//
// Desugared to:
//
// %tmp = obj
// num = %tmp.num
// foo = %tmp.foo
{num, foo} = obj

// Object destructuring self initialisation
//
// Desugared to:
//
// let num
// let foo
// %tmp = obj
// num = self.num = %tmp.num
// foo = self.foo = %tmp.foo
let {@num, @foo} = obj

// Object destructuring self assignment
//
// Desugared to:
//
// %tmp = obj
// self.num = %tmp.num
// self.foo = %tmp.foo
{@num, @foo} = obj
