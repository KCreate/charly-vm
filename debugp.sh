#!/bin/sh

mkdir -p cmake-build-debug
cd cmake-build-debug || exit

# initial cmake run
if ! test -f Makefile;
then
  cmake .. -DCMAKE_BUILD_TYPE=Debug
fi

if cmake --build . --target charly -j8;
then
  cd ..
  cmake-build-debug/charly "$@"
fi
