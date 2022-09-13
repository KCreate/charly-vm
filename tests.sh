#!/bin/bash

red=$(tput setaf 1)
green=$(tput setaf 2)
reset=$(tput sgr0)

echo_green () {
   echo "${green}$*${reset}"
}

echo_red () {
   echo "${red}$*${reset}"
}

mkdir -p cmake-build-debug

# setup build files
if ! test -f cmake-build-debug/Makefile; then
  cmake -DCMAKE_BUILD_TYPE=Debug -S . -B cmake-build-debug
fi

# build tests and charly executable in debug and release mode
if ! cmake --build cmake-build-debug --target tests --target charly -j12; then
  echo_red "Test build failed!"
  exit 1
fi
if ! cmake --build cmake-build-debug --target charly -j12; then
  echo_red "Charly build failed!"
  exit 1
fi

# run c++ unit tests
if ! cmake-build-debug/tests; then
  echo_red "C++ unit tests failed"
  exit 1
fi

chmod -r test/charly/import-test/not-readable.ch

# run charly unit tests in both multi- and single-threaded mode and with/without optimizations
echo_green "Running charly unit tests"
if ! cmake-build-debug/charly test/charly/test.ch; then
  echo_red "Charly unit tests failed"
  exit 1
fi

echo_green "Running charly (no optimizations) unit tests"
if ! cmake-build-debug/charly test/charly/test.ch --no_ir_opt --no_ast_opt; then
  echo_red "Charly (no optimizations) unit tests failed"
  exit 1
fi

echo_green "Running charly (singlethreaded) unit tests"
if ! cmake-build-debug/charly test/charly/test.ch --maxprocs 1; then
  echo_red "Charly (singlethreaded) unit tests failed"
  exit 1
fi

chmod +r test/charly/import-test/not-readable.ch
