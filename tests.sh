#!/bin/sh

mkdir -p build
cd build

# initial cmake run
test -f Makefile
if [ $? -gt 0 ]
then
  cmake .. -DCMAKE_BUILD_TYPE=Debug
fi

make tests
if [ $? -eq 0 ]
then
  cd ..

  # --batch exits lldb on success and prompts for further input on failure
  build/tests $@
fi
