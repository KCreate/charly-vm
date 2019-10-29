![Charly Programming Language](docs/charly-vm.png)

###### Status: Pre-Alpha

This repository contains the Charly Virtual Machine.

`charly-vm` is a bytecode-interpreter written in C++, it compiles and executes
the Charly Programming Language. Charly is a dynamic, weakly-typed multi-paradigm
programming language.

## Interesting Features

### Generators

```javascript
const create_counter = ->{
  let i = 0
  loop {
    yield i
    i += 1
  }
}

const c = create_counter()
print(c()) // 0
print(c()) // 1
print(c()) // 2
```

### `TODO: Write a nice description of the project`

# Installation

You're going to need a Compiler supporting C++17.
I'm developing on `clang 5.0.0`.

1. `git clone http://github.com/KCreate/charly-vm`
2. `make`
3. `bin/vm yourfile.ch`

# Credits

- [Leonard Schütz @KCreate](http://github.com/KCreate) Lead Developer
