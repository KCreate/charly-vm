![Charly Programming Language](docs/charly-vm.png)

# Installation

1. `mkdir build`
2. `cmake ..`
4. `make install`

# Running the unit tests

1. `mkdir build`
2. `cmake ..`
3. `make tests`
4. `./tests`

# Required third-party libraries
- Catch2 (development only) ([https://github.com/catchorg/Catch2](https://github.com/catchorg/Catch2))
  - Ubuntu: `sudo apt-get install catch`
    - You may have to create an additional symlink `cd /usr/include; sudo ln -s catch catch2`
