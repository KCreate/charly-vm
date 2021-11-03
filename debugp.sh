#!/bin/sh

mkdir -p cmake-build-debug

# initial cmake run
if ! test -f cmake-build-debug/Makefile;
then
  cmake -DCMAKE_BUILD_TYPE=Debug -S . -B cmake-build-debug
fi

if cmake --build cmake-build-debug --target charly -j12;
then
  cmake-build-debug/charly "$@"
fi
