#!/bin/sh

mkdir build -p
cd build

test -f Makefile
if [ $? -gt 0 ]
then
  cmake ..
fi

make charly
if [ $? -eq 0 ]
then
  cd ..
  echo ""
  build/charly $@
fi
