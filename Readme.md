![Charly Programming Language](docs/charly-vm.png)

## What is Charly?

Charly is a programming language with the following goals:

- Syntax similar to JavaScript (Compatibility is not a goal)
- Easy way to write C extensions
- Single-threaded work-queue for charly code
- Async IO, multithreadable C code
- Small and simple standard library

Charly was not created with the intention of it being used as a production level language for anything important. It's main purpose is to teach other programmers how to write the basic components of a programming language. This includes the various stages of producing an Abstract Syntax Tree, how to perform manipulations on that tree and finally how to generate bytecodes that can then be executed in a custom virtual machine.

Charly's implementation contains many interesting things, such as NAN-Boxing, a C-Module system using `dlopen`, a computed-goto main-switch, a mark-and-sweep garbage collector and a single-threaded task-queue backed by an asynchronous, multithreaded worker-queue.

## Example code

### Closures
```javascript
// This function returns another function which, when invoked,
// will return the current value of the counter and add 1 to it
func create_counter {
  let i = 0

  return ->{
    const v = i
    i += 1
    return v
  }
}

const counter = create_counter()
counter() // => 0
counter() // => 1
counter() // => 2
```

### Exceptions
```javascript
try {
  throw "Hello World"
} catch(e) {
  e // => "Hello World"
}

try {
  // do some things here
} finally {
  // The `finally` handler gets invoked no matter if an exception has been caught or not

  print("Hello World")
}
```

### Generators
```javascript
const summer = ->{
  let sum = 0
  sum = yield ->sum

  loop {
    sum += yield
  }
}

const read_state = summer()

summer(100)
summer(100)
summer(100)
summer(100)

read_state() // => 400
```

### Taskqueue interactions
```javascript
const iv = defer.interval(->{
  print("Doing stuff...")
}, 1000)

defer(->{
  defer.clear_interval(iv)

  print("Done!!")
}, 5000)

/* Console output:

   Doing stuff...
   Doing stuff...
   Doing stuff...
   Doing stuff...
   Done!!
*/
```

### Extending primitive classes

[Mocking Spongebob Meme](https://knowyourmeme.com/memes/mocking-spongebob)

```javascript
String.prototype.mockify = func {
  const sentence = self.lowercase()

  // Split the sentence into words
  const words = sentence.split(" ")

  // Change the case of each character
  let uppercase = false
  const mocked_words = words.map(->(word) {
    word.map(->(character) {
      if !uppercase { uppercase = true;  return character }
      if uppercase  { uppercase = false; return character.uppercase() }
    })
  })

  mocked_words.join(" ")
}

print("You should not use memes in example code".mockify())
// => yOu ShOuLd NoT uSe MeMeS iN eXaMpLe CoDe
```

### Classes
```javascript
class Greeter {
  property name

  func greet {
    print("Hello " + @name)
  }
}

class RudeGreeter extends Greeter {
  func greet {
    print("F**k you " + @name)
  }
}

const greeter = Greeter("leonard")
const rude_greeter = RudeGreeter("leonard")

greeter.greet() // => Hello leonard
rude_greeter.greet() // => F**k you leonard
```

## Teaching Features

Via the executable arguments, you can make the charly compiler print out various data structures that are being produced during the compilation phase.

```javascript
func greet(n) {
  print("Hello " + n)
}

greet("leonard")
```

### Tokens
`charly myfile.ch -fskipexec -fdump_tokens`

```
Func : func /path/myfile.ch:1:1
Identifier : greet /path/myfile.ch:1:6
LeftParen :  /path/myfile.ch:1:11
Identifier : n /path/myfile.ch:1:12
RightParen :  /path/myfile.ch:1:13
LeftCurly :  /path/myfile.ch:1:15
Identifier : print /path/myfile.ch:2:3
LeftParen :  /path/myfile.ch:2:8
String : Hello  /path/myfile.ch:2:9
Plus :  /path/myfile.ch:2:18
Identifier : n /path/myfile.ch:2:20
RightParen :  /path/myfile.ch:2:21
RightCurly :  /path/myfile.ch:3:1
Identifier : greet /path/myfile.ch:5:1
LeftParen :  /path/myfile.ch:5:6
String : leonard /path/myfile.ch:5:7
RightParen :  /path/myfile.ch:5:16
```

### Abstract Syntax Tree
`charly myfile.ch -fskipexec -fdump_ast`

```
- Block:
  - Assignment: greet
    - Function: greet (n) lvarcount=1
      - Block:
        - Return:
          - Call:
            - Identifier: print
            - NodeList:
              - Binary: Plus
                - String: Hello
                - Identifier: n
  - Call:
    - Identifier: greet
    - NodeList:
      - String: leonard
```

### Bytecode
`charly myfile.ch -fskipexec -fdump_asm`

```
*-- 0x7fd994e01eaa: putfunction @"greet", 0x0x7fd994e01eec, false, false, 1, 1
|   0x7fd994e01ec1: setlocal 1, 0
|   0x7fd994e01eca: readlocal 1, 0
|   0x7fd994e01ed3: putstring "leonard"
|   0x7fd994e01edc: call 1
|   0x7fd994e01ee1: pop
|   0x7fd994e01ee2: readlocal 0, 0
|   0x7fd994e01eeb: return
*-> 0x7fd994e01eec: nop
    0x7fd994e01eed: readlocal 15, 2
    0x7fd994e01ef6: putvalue 0x7ffe206f6c6c6548
    0x7fd994e01eff: readlocal 0, 0
    0x7fd994e01f08: add
    0x7fd994e01f09: call 1
    0x7fd994e01f0e: return
```

## Contributing

All contributions are welcome, no matter how small or big.

1. Fork it (https://github.com/KCreate/charly-vm/fork)
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am "Add some feature"`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create a new Pull Request

## Installation

You're going to need a Compiler supporting C++17.
I'm developing on `macOS Cataline 10.15.2` with `clang 5.0.0`.

1. `git clone http://github.com/KCreate/charly-vm`
2. Update the `Makefile` to accomodate your system
3. `make`
4. `bin/vm yourfile.ch`

## Credits

- [Leonard Schütz @KCreate](http://github.com/KCreate) Lead Developer

## Mentions

I would like to mention the following projects and people:

- [Crystal Programming Language](https://github.com/crystal-lang/crystal)
  This project got me into language development in the first place. By studying its source code, I taught myself how to write a parser for complex languages and how to model an AST. The [first version](https://github.com/charly-lang/charly) of the Charly language was also written in Crystal, before I moved on to C++.

- [The Wren Programming Language](https://github.com/wren-lang/wren)
  I learned a lot from this project, most notably how to implement NaN-Boxing.

- [Eli Bendersky's Website](https://eli.thegreenplace.net/)
  Eli's articles on language-dev related topics were of great use to me and I learned a lot from them. Most articles in the [#compilation](https://eli.thegreenplace.net/tag/compilation) section of his website contain very useful and interesting information.

- [Kartik Agaram](http://akkartik.name/) and [Max Bernstein](https://bernsteinbear.com/)
  They both helped me a lot during the learning process and I have always been able to bounce my ideas off of them, getting valuable feedback.
