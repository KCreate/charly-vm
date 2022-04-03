![Charly Programming Language](docs/charly-vm.png)
![Unit Test](https://github.com/KCreate/charly-vm/workflows/Unit%20Test/badge.svg?branch=rewrite)

<strike>This launches a REPL which (at the moment) doesn't do very much.</strike>

<strike>This launches a REPL which (at the moment) does some cool stuff, but still not a lot.</strike>

This launches a REPL which supports some cool stuff, but still not a lot

`./debug.sh [path/to/file.ch]`

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
[ 31%] Built target libcharly
[ 87%] Built target Catch2
[ 89%] Built target Catch2WithMain
[100%] Built target tests
===============================================================================
All tests passed (1422 assertions in 10 test cases)
```
