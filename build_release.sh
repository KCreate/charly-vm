#!/bin/sh

mkdir -p cmake-build-release

# initial cmake run
if ! test -f cmake-build-release/Makefile;
then
  cmake -DCMAKE_BUILD_TYPE=Release -S . -B cmake-build-release
fi

cmake --build cmake-build-release --target charly
