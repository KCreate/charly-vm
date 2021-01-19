![Charly Programming Language](docs/charly-vm.png)

# Play

This launches a REPL which (at the moment) doesn't do very much.

`./debug.sh <arguments>`

# Installation

Installation via this method may require you to enter your root password
in order for make to copy the charly executable to the relevant system directory.

`./install.sh`

# Running the unit tests

`./tests.sh`

# Required third-party libraries
- Catch2 (development only) ([https://github.com/catchorg/Catch2](https://github.com/catchorg/Catch2))
  - Ubuntu: `sudo apt-get install catch`
    - You may have to create an additional symlink `cd /usr/include; sudo ln -s catch catch2`
