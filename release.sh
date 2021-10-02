#!/bin/sh

mkdir buildrelease -p
cd buildrelease

# initial cmake run
test -f Makefile
if [ $? -gt 0 ]
then
  cmake .. -DCMAKE_BUILD_TYPE=Release
fi

cmake --build . --target charly -j12
if [ $? -eq 0 ]
then
  cd ..
  buildrelease/charly $@
fi
