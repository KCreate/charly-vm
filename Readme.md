![Charly Programming Language](docs/charly-vm.png)

# Setup

- Install the submodules `git submodule init`

# Play

This launches a REPL which (at the moment) doesn't do very much.

`./debug.sh`

# Installation

Installation via this method may require you to enter your root password
in order for make to copy the charly executable to the relevant system directory.

`./install.sh`

# Running the unit tests

```
$ ./tests.sh
[ 84%] Built target Catch2
[ 91%] Built target libcharly
[100%] Built target tests
===============================================================================
All tests passed (543 assertions in 31 test cases)
```
