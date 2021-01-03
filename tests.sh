#!/bin/sh

mkdir cmake-build-debug -p
cd cmake-build-debug

# initial cmake run
test -f Makefile
if [ $? -gt 0 ]
then
  cmake .. -DCMAKE_BUILD_TYPE=Debug
fi

make tests -j6
if [ $? -eq 0 ]
then
  cd ..

  # --batch exits lldb on success and prompts for further input on failure
  cmake-build-debug/tests $@
fi
