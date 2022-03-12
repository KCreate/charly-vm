#!/bin/sh

mkdir -p cmake-build-release

# initial cmake run
if ! test -f cmake-build-release/Makefile;
then
  cmake -DCMAKE_BUILD_TYPE=Release -S . -B cmake-build-release
fi

if cmake --build cmake-build-release --target tests -j12;
then
  cmake-build-release/tests
fi