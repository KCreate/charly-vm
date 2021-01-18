#!/bin/sh

mkdir cmake-build-debug -p
cd cmake-build-debug

# initial cmake run
test -f Makefile
if [ $? -gt 0 ]
then
  cmake .. -DCMAKE_BUILD_TYPE=Debug
fi

make charly
if [ $? -eq 0 ]
then
  cd ..
  cmake-build-debug/charly $@
fi
