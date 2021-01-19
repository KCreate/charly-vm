#!/bin/sh

mkdir buildrelease -p
cd buildrelease

# initial cmake run
test -f Makefile
if [ $? -gt 0 ]
then
  cmake .. -DCMAKE_BUILD_TYPE=Release
fi

make charly tests -j12
if [ $? -eq 0 ]
then
  ./tests

  if [ $? -eq 0 ]
  then
    sudo make install
  fi
fi
