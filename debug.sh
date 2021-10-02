#!/bin/sh

mkdir build -p
cd build

# initial cmake run
test -f Makefile
if [ $? -gt 0 ]
then
  cmake .. -DCMAKE_BUILD_TYPE=Debug
fi

# make charly -j -Oline
cmake --build . --target charly
if [ $? -eq 0 ]
then
  cd ..
  build/charly $@
fi
