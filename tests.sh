#!/bin/sh

mkdir -p buildtest
cd buildtest

# initial cmake run
test -f Makefile
if [ $? -gt 0 ]
then
  cmake .. -DCMAKE_BUILD_TYPE=Release
fi

make tests -j12
if [ $? -eq 0 ]
then
  cd ..

  # --batch exits lldb on success and prompts for further input on failure
  buildtest/tests $@
fi
