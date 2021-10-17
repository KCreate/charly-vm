#!/bin/sh

mkdir -p cmake-build-release
cd cmake-build-release || exit

# initial cmake run
if ! test -f Makefile;
then
  cmake .. -DCMAKE_BUILD_TYPE=Release
fi

if cmake --build . --target charly -j8;
then
  cd ..
  cmake-build-release/charly "$@"
fi
