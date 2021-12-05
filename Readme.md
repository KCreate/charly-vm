![Charly Programming Language](docs/charly-vm.png)
![Unit Test](https://github.com/KCreate/charly-vm/workflows/Unit%20Test/badge.svg?branch=rewrite)

# Play

<strike>This launches a REPL which (at the moment) doesn't do very much.</strike>

This launches a REPL which (at the moment) does some cool stuff, but still not a lot.

`./debug.sh [path/to/file.ch]`

# Milestones

- [x] Lexer
- [x] Parser
- [x] Compiler
- [x] Scheduler
- [x] REPL
- [ ] Inline caching
- [ ] Object System
- [ ] Concurrent Garbage Collector
- [ ] Bytecode VM
- [ ] Object Maps
- [ ] Concurrency primitives (Channels, select, Promises, await)
- [ ] Core Utilities (Standard Library)
- [ ] Network Library (Standard Library)
- [ ] Graphics Library (Standard Library)

# Dependencies

- `sudo apt-get install libboost-all-dev`

# Installation

Follow the steps below to install the `charly` executable on your system.

1. `git clone https://github.com/KCreate/charly-vm charly-vm`
2. `cd charly-vm`
3. `git checkout rewrite`
4. `git submodule init`
5. `git submodule update`
6. `./install.sh`

> The last step might request sudo permissions in order to access the relevant system directories.

# Running the unit tests

```
$ ./tests.sh
[ 84%] Built target Catch2
[ 91%] Built target libcharly
[100%] Built target tests
===============================================================================
All tests passed (543 assertions in 31 test cases)
```
