#!/bin/sh

for build_dir in cmake-build-*; do
  mkdir -p "$build_dir"
  cd "$build_dir" || exit
  if test -f Makefile;
  then
    cmake --build . --target clean -- -j 6
  fi

  cd ..
done