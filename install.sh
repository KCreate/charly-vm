#!/bin/sh

mkdir -p cmake-build-release
cd cmake-build-release || exit

# initial cmake run
if ! test -f Makefile;
then
  cmake .. -DCMAKE_BUILD_TYPE=Release
fi

if cmake --build . --target tests -j8;
then
  if ./tests;
  then
    sudo make install
  fi
fi
