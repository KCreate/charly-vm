print("hello {name + "{age}"}")



let a = 0x
print(0xABCG)
print("hello \z")
if {}
if return {}

test.ch:1:9: error: Hex number literal expects at least one hex character
test.ch:2:7: error: Invalid character in hex number literal: 'G'
test.ch:3:7: error: Invalid escape sequence in string literal: '\z'
test.ch:4:4: error: Expected an expression, found '{'
test.ch:5:4: error: Expected an expression, found 'return'








/*
  Compilation steps:
  - Read the source file
  - Split into tokens
    - Return if errors
  - Parse into AST
    - Return if errors
  - Semantic analysis
    - Return if errors
  - Codegeneration
    - Return if errors
  - Result
    - Instructionblock
    - Sourcemap
*/


from fs import stdout, stdin

loop {
  const line = stdin.readline("> ")

  match line {
    ".exit"     => break loop
  }
}


try {
  foo()
} catch (RuntimeError exc) {

}








print("hello { name({foo}) } foo {bar} baz {test} ")


Identifier      "print"
LeftParen       "("

IStringBegin    "hello "
  Identifier      "name"
  LeftParen
    LeftCurly
      Identifier      "foo"
    RightCurly
  RightParen
IStringPart     " foo "
  Identifier      "bar"
IStringPart     " baz "
  Identifier      "test"
IStringEnd      " "

RightParen




TopLevel                  0
String                    1
InterpolatedExpression    2
String                    3
InterpolatedExpression    4








> "value: { {a, b, c} } test"

- InterpolatedString
  - Elements
    - String: "value: "
    - DictLiteral
      - Identifier: a
      - Identifier: b
      - Identifier: c
    - String: " test"

> "name: "

- String: "value: {"






> "name: {name} age: {age}"

StringPart          "name: "
Identifier          name
StringPart          " age: "
Identifier          age
StringEnd           ""

- InterpolatedString
  - Elements
    - String      "name: "
    - Identifier name
    - String      " age: "
    - Identifier age


> "name: {name}"

StringPart          "name: "
Identifier          name
StringEnd           ""

- InterpolatedString
  - Elements
    - String      "name: "
    - Identifier name


> "{name}"

StringPart          ""
Identifier          name
StringEnd           ""

- InterpolatedString
  - Elements
    - Identifier name









                            v
> "name: {name + "substring"}"

mode           = interpolated
at_inter_start = false
i_bracketstack = [1]
bracketstack   = [{]









> "name: {name}"

StringPart        "name: "
Identifier        name
String            ""











yield 1, 2
x = yield 1, 2
x = (yield 1, 2)
foo(yield 1, 2)




func getTime() {
  return (18, 09, 58)
}

let (hour, minute, second) = getTime()

print("The time is: {hour}:{minute}:{second}.")












(x, y, z) = coords


loadname    coords
dup
unpack      3
setname     x
setname     y
setname     z







(head, neck, body..., foot) = human

loadname        human
dup
unpackspread    2, 1
setname         head
setname         neck
setname         body
setname         foot






const p = promise {
  return
}


const (u1, u2) = await user(1), user(2)

const list = [await user(1), ]



// src/a.ch
const c = 100
- Declaration const
  - Identifier c
  - Int 100




const x, y, z = coords
- Declaration const
  - Tuple
    - Identifier x
    - Identifier y
    - Identifier z
  - Identifiers coords




const { firstname, lastname, email, foo.bar as mydat } = user
- Declaration const
  - Set
    - Identifier firstname
    - Identifier lastname
    - Identifier email
    - AsExpression mydat
      - AttributeLookup bar
        - Identifier foo
  - Identifier user




import mylib
- Import
  - Identifier mylib

make<Import>(make<Id>("mylib"))



import mylib as somename
- Import
  - AsExpression somename
    - Identifier mylib

make<Import>(make<As>(make<Id>("mylib"), make<Id>("somename")))


import "{name}" as result
- Import
  - AsExpression result
    - FormatString
      - Identifier name

make<Import>(make<As>(make<FormatString>(make<Id>("name")), make<Id>("result")))

from "./mylib.ch" as myotherlib import foo as myfoo, bar as mybar
- Import
  - AsExpression myotherlib
    - String "./mylib.ch"
  - AsExpression myfoo
    - Identifier foo
  - AsExpression mybar
    - Identifier bar

make<Import>(
  make<As>(make<String>("./mylib.ch"), make<Id>("myotherlib")),
  make<As>(make<Id>("foo"), make<Id>("myfoo")),
  make<As>(make<Id>("bar"), make<Id>("mybar"))
)



func add(l, r = 2) = l + r + c
- Function add
  - DeclarationList
    - Declaration l
    - Declaration r
      - Int 2
  - Block
    - Return
      - BinOp +
        - Identifier l
        - Identifier r
      - Identifier c



const name = "Leonard"

if name.begins_with("L")
  print("your name begins with an L")

if name.beginsWith("L")
  print("your name begins with an L")










// original source
// ------------------------------------------
func foo(a, b = 1, c = 2, d...) {
  let x = "message: "
  let y = "hello world"
  let z = x + y
  return "{(a, b, c, d, x, y, z)}"
}
// ------------------------------------------

// parsed ast
// ------------------------------------------
- Function
  - Identifier foo
  - FunctionArguments
    - Identifier a
    - Assignment
      - Identifier b
      - Int 1
    - Assignment
      - Identifier c
      - Int 2
    - Spread
      - Identifier d
  - Block
    - Declaration
      - Identifier x
      - String "message: "
    - Declaration
      - Identifier y
      - String "hello world"
    - Declaration
      - Identifier z
      - BinOp +
        - Identifier x
        - Identifier y
    - Return
      - FormatString
        - Tuple
          - Identifier a
          - Identifier b
          - Identifier c
          - Identifier d
          - Identifier x
          - Identifier y
          - Identifier z
// ------------------------------------------

// generated ir
// ------------------------------------------
function foo() {
.locals 7
  jmp .default_b
  jmp .default_c
  jmp .entry

.label default_b
  %0 = 1
  store %0 1

.label default_c
  %1 = 2
  store %1 2

.label entry
  %2 = loadconst 0
  store %2 4

  %3 = loadconst 1
  store %3 5

  %4 = load 4
  %5 = load 5
  %6 = add %4 %5
  store %6 6

  %7 = load 0
  %8 = load 1
  %9 = load 2
  %10 = load 3
  %11 = load 4
  %12 = load 5
  %13 = load 6
  %14 = maketuple %7 %8 %9 %10 %11 %12 %13
  ret %15

.const {
  "message: "
  "hello world"
}
}
// ------------------------------------------

// optimized ir (constant folding)
// ------------------------------------------
function foo() {
.locals 7
  jmp .default_b
  jmp .default_c
  jmp .entry

.label default_b
  %0 = 1
  store %0 1

.label default_c
  %1 = 2
  store %1 2

.label entry
  %2 = loadconst 0
  store %2 4

  %3 = loadconst 1
  store %3 5

  %4 = loadconst 0
  %5 = loadconst 1
  %6 = loadconst 2
  store %6 6

  %7 = load 0
  %8 = load 1
  %9 = load 2
  %10 = load 3
  %11 = loadconst 0
  %12 = loadconst 1
  %13 = loadconst 2
  %14 = maketuple %7 %8 %9 %10 %11 %12 %13
  %15 = makestring %14
  ret %15

.const {
  "message: "
  "hello world"
  "message: hello world"
}
}
// ------------------------------------------

// optimized ir (dead code removal)
// ------------------------------------------
function foo(7) {
.label begin
  jmp .default_b
  jmp .default_c
  jmp .entry

.label default_b
  %0 = 1
  store %0 1

.label default_c
  %1 = 2
  store %1 2

.label entry
  %7 = load 0
  %8 = load 1
  %9 = load 2
  %10 = load 3
  %11 = loadconst 0
  %12 = loadconst 1
  %13 = loadconst 2
  %14 = maketuple %7 %8 %9 %10 %11 %12 %13
  %15 = makestring %14
  ret %15

.const {
  "message: "
  "hello world"
  "message: hello world"
}
}
// ------------------------------------------









// original source
func sum(n = 0) {
  let result = 0
  let i = 0

  while true {
    if i == n break
    i += 1
    result += i
  }

  return result
}

// parse tree
- Function sum
  - FunctionArguments
    - Assignment
      - Identifier n
      - Int 0
  - Block
    - Declaration
      - Identifier result
      - Int 0
    - Declaration
      - Identifier i
      - Int 0
    - While
      - Bool true
      - Block
        - If
          - BinOp '=='
            - Identifier i
            - Identifier n
          - Break
        - Assignment '+'
          - Identifier i
          - Int 1
        - Assignment '+'
          - Identifier result
          - Identifier i
    - Return
      - Identifier result

// local variable allocation
- Function name = "sum"
  - FunctionArguments
    - Assignment
      - Identifier n
      - Int 0
  - Block
    - Assignment
      - Local 1
      - Int 0
    - Assignment
      - Local 2
      - Int 0
    - Loop
      - Block
        - If
          - BinOp '=='
            - Local 2
            - Local 0
          - Break
        - Assignment '+'
          - Local 2
          - Int 1
        - Assignment '+'
          - Local 1
          - Local 2
    - Return
      - Local 1

// lowering
{
  goto .A0
  goto .BEGIN

.A0
  %0 = 0

.BEGIN
  %1 = 0
  %2 = 0

.LOOP
  goto .IF_END if %2 != %0
  goto .LOOP_END

.IF_END
  %2 = %2 + 1
  %1 = %1 + %2
  goto .LOOP

.LOOP_END
  ret %1
}

// codegen
{
  goto .A0
  goto .BEGIN

.A0
  load 0
  storelocal 0

.BEGIN
  load 0
  storelocal 1
  load 0
  storelocal 2

.LOOP
  loadlocal 2
  loadlocal 0
  eq
  jmpfalse .IF_END

.IF_THEN
  jmp .LOOP_END

.IF_END
  loadlocal 2
  load 1
  add
  storelocal 2

  loadlocal 1
  loadlocal 2
  add
  storelocal 1

  jmp .LOOP

.LOOP_END
  loadlocal 1
  ret
}





// original source
class Point {
  property x
  property y

  distance(other) {
    const dx = other.x - @x
    const dy = other.y - @y
    return sqrt(dx ** 2 + dy ** 2)
  }
}

const p1 = Point(2, 3)
const p2 = Point(14, 21)

print(p1.distance(p2))




// general bytecode
  loadsymbol      x
  loadsymbol      y
  makefunction    distance, .DISTANCE, 1
  makeclass       Point, 1, 2
  storelocal      0

  loadlocal       0
  load            2
  load            3
  call            2
  storelocal      1

  loadlocal       0
  load            14
  load            21
  call            2
  storelocal      2

  loadglobal      print
  loadlocal       1
  loadlocal       2
  callattr        distance, 1
  call            1
  pop

  load            null
  ret

.DISTANCE
  loadlocal       0
  readattr        x
  loadself
  readattr        x
  sub
  storelocal      1

  loadlocal       0
  readattr        y
  loadself
  readattr        y
  sub
  storelocal      2

  loadglobal      sqrt
  loadlocal       1
  load            2
  pow
  loadlocal       2
  load            2
  pow
  add
  call            1
  ret



1. real frames
2. charly vm frames

need to be intertwineable with each other
need to have access to each other





// file.ch
func bar(       // frame heap 0
  cb            // frame local 0
) = cb(42)      // load frame local 0

func foo(       // local 1
  x,            // frame heap 0
  y             // frame local 0
) {
  let a = 1     // frame heap 1
  let b = 2     // frame local 1
  let c = 3     // frame local 2

  const result = bar(->(p) {    // frame local 3, loadfar[1, 0]
    return p + x + a            // load local 0, loadfar[1, 0], loadfar[1, 1]
  })

  let i = 2             // frame local 4
  while i-- > 0 {       // loadlocal 4
    result *= sin(x)    // loadlocal 3, loadscope
  }

  return (a * b * c + x * y) * result // loadheap 1, loadlocal 1, loadlocal 2, loadheap 0, loadlocal 0, loadlocal 3
}

foo(3, 7) // loadlocal 1




for const item in [1, 2, 3] {
  spawn print(item.a.b.c) // always prints 3

  spawn(->(data) {
    print(data) // prints 1, 2, 3
  }, item.a.b.c)
}



spawn foo(1, 2)
-> spawn(null, foo, 1, 2)

spawn foo.bar(1, 2)
-> spawn(foo, foo.bar, 1, 2)

spawn foo().bar(1, 2)
-> %tmp = foo()
-> spawn(%tmp, %tmp.bar, 1, 2)





// frame local variables:
// a, c
//
// frame heap variables
// b, d
func foo(a, b, c, d) {
  return ->[b, d]
}





// import statement
import <identifier>
import <identifier> as <identifier>
import <string> as <identifier>

// import expression
import <expression>
