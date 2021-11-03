#!/bin/sh

rm -r cmake-build-debug
rm -r cmake-build-release

mkdir -p cmake-build-debug
mkdir -p cmake-build-release

exit

for build_dir in cmake-build-*; do
  rm -r "$build_dir"
  mkdir -p "$build_dir"
  cmake -S . -DCMAKE_BUILD_TYPE=Debug -B "$build_dir"
done