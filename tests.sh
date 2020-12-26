#!/bin/sh

mkdir build -p
cd build

make tests
if [ $? -gt 0 ]
then
  cmake ..
  make tests
fi

cd ..
build/tests
