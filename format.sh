#!/bin/bash

SOURCES=`find src -type f -name '*.cpp'`
PRIVATE_HEADERS=`find src -type f -name '*.h'`
PUBLIC_HEADERS=`find include -type f -name '*.h'`

TEST_SOURCES=`find test -type f -name '*.cpp'`
TEST_HEADERS=`find test -type f -name '*.h'`

clang-format -i $SOURCES $PRIVATE_HEADERS $PUBLIC_HEADERS $TEST_SOURCES $TEST_HEADERS --style=file
