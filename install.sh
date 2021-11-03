#!/bin/sh

mkdir -p cmake-build-release

# initial cmake run
if ! test -f cmake-build-release/Makefile;
then
  cmake -DCMAKE_BUILD_TYPE=Release -S . -B cmake-build-release
fi

if cmake --build cmake-build-release -j12 --target tests charly;
then
  if ./cmake-build-release/tests;
  then
    sudo cmake --install cmake-build-release
  fi
fi
