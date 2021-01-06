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
  - NodeList




import mylib as somename
- Import
  - AsExpression somename
    - Identifier mylib
  - NodeList




import "{name}" as result
- Import
  - AsExpression result
    - FormatString
      - Identifier name
  - NodeList




from "./mylib.ch" as myotherlib import foo as myfoo, bar as mybar
- Import
  - AsExpression myotherlib
    - String "./mylib.ch"
  - NodeList
    - AsExpression myfoo
      - Identifier foo
    - AsExpression mybar
      - Identifier bar




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
