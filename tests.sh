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
  cd ..

  # --batch exits lldb on success and prompts for further input on failure
  cmake-build-release/tests "$@"
fi
