/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2021 Leonard Schütz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <catch2/catch_test_macros.hpp>

#include "charly/utils/buffer.h"

using namespace charly;

TEST_CASE("Creates a buffer", "[buffer]") {
  utils::Buffer buf;

  REQUIRE(buf.buf() != nullptr);
  REQUIRE(buf.size() == 0);
  REQUIRE(buf.capacity() == 64);

  SECTION("writes primitive types") {
    buf.write_i8(-25);
    buf.write_i16(-550);
    buf.write_i32(1000000);
    buf.write_i64(123123123123);
    buf.write_float(25.25);
    buf.write_double(3.00000005);
    buf.write_ptr(nullptr);

    CHECK(buf.buf() != nullptr);
    CHECK(buf.size() == 35);
    CHECK(buf.capacity() == 64);

    CHECK(buf.read_i8(0)     == -25);
    CHECK(buf.read_i16(1)    == -550);
    CHECK(buf.read_i32(3)    == 1000000);
    CHECK(buf.read_i64(7)    == 123123123123);
    CHECK(buf.read_float(15)  == 25.25);
    CHECK(buf.read_double(19) == 3.00000005);
    CHECK(buf.read_ptr(27)    == nullptr);
  }

  SECTION("writes strings") {
    buf.write_string("abcdef");

    CHECK(buf.buf() != nullptr);
    CHECK(buf.size() == 6);
    CHECK(buf.capacity() == 64);
  }

  SECTION("writes blocks of memory") {
    char data[] = "hello world this is a string";

    buf.write_block(data + 5, 15);

    CHECK(buf.buf() != nullptr);
    CHECK(buf.size() == 15);
    CHECK(buf.capacity() == 64);
  }

  SECTION("seeks to some position") {
    buf.seek(3);
    buf.write_u8(1);

    buf.seek(4);
    buf.seek(2);
    buf.write_u8(2);

    buf.seek(1);
    buf.write_u8(3);

    buf.seek(0);
    buf.write_u8(4);

    buf.seek(0);
    CHECK(buf.read_u32(0) == 0x01020304);

    buf.seek(200);

    CHECK(buf.buf() != nullptr);
    CHECK(buf.size() == 200);
    CHECK(buf.capacity() == 256);
  }
}
