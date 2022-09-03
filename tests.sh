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
mkdir -p cmake-build-release

# setup build files
if ! test -f cmake-build-debug/Makefile; then
  cmake -DCMAKE_BUILD_TYPE=Debug -S . -B cmake-build-debug
fi
if ! test -f cmake-build-release/Makefile; then
  cmake -DCMAKE_BUILD_TYPE=Release -S . -B cmake-build-release
fi

# build tests and charly executable in debug and release mode
if ! cmake --build cmake-build-debug --target tests --target charly -j12; then
  echo_red "Test build failed!"
  exit 1
fi
if ! cmake --build cmake-build-debug --target charly -j12; then
  echo_red "Charly debug build failed!"
  exit 1
fi
if ! cmake --build cmake-build-release --target charly -j12; then
  echo_red "Charly release build failed!"
  exit 1
fi

# run c++ unit tests
if ! cmake-build-debug/tests; then
  echo_red "C++ unit tests failed"
  exit 1
fi

# run charly unit tests in both multi- and single-threaded mode and with/without optimizations
echo_green "Running charly debug unit tests"
if ! cmake-build-debug/charly test/charly/test.ch; then
  echo_red "Charly debug unit tests failed"
  exit 1
fi

echo_green "Running charly debug (no optimizations) unit tests"
if ! cmake-build-debug/charly test/charly/test.ch --no_ir_opt --no_ast_opt; then
  echo_red "Charly debug (no optimizations) unit tests failed"
  exit 1
fi

echo_green "Running charly debug (singlethreaded) unit tests"
if ! cmake-build-debug/charly test/charly/test.ch --maxprocs 1; then
  echo_red "Charly debug (singlethreaded) unit tests failed"
  exit 1
fi

echo_green "Running charly release unit tests"
if ! cmake-build-release/charly test/charly/test.ch; then
  echo_red "Charly release unit tests failed"
  exit 1
fi

echo_green "Running charly release (no optimizations) unit tests"
if ! cmake-build-release/charly test/charly/test.ch --no_ir_opt --no_ast_opt; then
  echo_red "Charly release (no optimizations) unit tests failed"
  exit 1
fi

echo_green "Running charly release (singlethreaded) unit tests"
if ! cmake-build-release/charly test/charly/test.ch --maxprocs 1; then
  echo_red "Charly release (singlethreaded) unit tests failed"
  exit 1
fi

