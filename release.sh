#!/bin/sh

mkdir cmake-build-release -p
cd cmake-build-release

# initial cmake run
test -f Makefile
if [ $? -gt 0 ]
then
  cmake .. -DCMAKE_BUILD_TYPE=Release
fi

make charly
if [ $? -eq 0 ]
then
  cd ..
  cmake-build-release/charly $@
fi
