#!/bin/bash
clang   -g \
        -Wall \
        -fPIC \
        -shared \
        -o libraries/sdl2.lib \
        libraries/sdl2.cpp \
        -Iinclude \
        -Ilibs \
        -I/usr/local/opt/llvm/include/c++/v1 \
        -lstdc++ \
        -std=c++17 \
        -lm \
        -framework sfml-window \
        -framework sfml-graphics  \
        -framework sfml-system \
        -undefined dynamic_lookup
