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

#include <catch2/catch.hpp>

#include "charly/utils/buffer.h"

using Catch::Matchers::Equals;

using namespace charly;

TEST_CASE("Buffer") {
  utils::Buffer buf(128);

  REQUIRE(buf.data() != nullptr);
  REQUIRE(buf.capacity() == 128);
  REQUIRE(buf.writeoffset() == 0);
  REQUIRE(buf.readoffset() == 0);
  REQUIRE_THAT(buf.window_string(), Equals(""));

  SECTION("Initialize buffer with string value") {
    utils::Buffer buf("hello world!!");

    for (int i = 0; i < 13; i++) {
      buf.read_utf8();
    }

    CHECK_THAT(buf.window_string(), Equals("hello world!!"));
  }

  SECTION("Append data to buffer") {
    buf.append_string("hello world\n");

    CHECK(buf.read_utf8() == 0x68);
    CHECK(buf.read_utf8() == 0x65);
    CHECK(buf.read_utf8() == 0x6c);
    CHECK(buf.read_utf8() == 0x6c);
    CHECK(buf.read_utf8() == 0x6f);

    CHECK(buf.read_utf8() == 0x20);
    CHECK(buf.read_utf8() == 0x77);
    CHECK(buf.read_utf8() == 0x6f);
    CHECK(buf.read_utf8() == 0x72);
    CHECK(buf.read_utf8() == 0x6c);

    CHECK(buf.read_utf8() == 0x64);
    CHECK(buf.read_utf8() == 0x0a);

    const char* data = "teststring";
    buf.append_block(data, 10);
    buf.append_string(data);
    buf.append_string("hallo welt");
    buf.append_buffer(buf);

    CHECK(buf.capacity() == 128);
    CHECK(buf.writeoffset() == 84);
    CHECK(buf.readoffset() == 12);
  }

  SECTION("reads / peeks utf8 codepoints") {
    buf.append_utf8(L'ä');
    buf.append_utf8(L'Ʒ');
    buf.append_utf8(L'π');

    CHECK(buf.peek_utf8() == 0xE4);
    CHECK(buf.peek_utf8() == 0xE4);
    CHECK(buf.peek_utf8() == 0xE4);

    CHECK(buf.read_utf8() == 0xE4);
    CHECK(buf.read_utf8() == 0x01B7);
    CHECK(buf.read_utf8() == 0x03C0);

    CHECK(buf.writeoffset() == 6);
    CHECK(buf.readoffset() == 6);
  }

  SECTION("reads ascii chars") {
    buf.append_string("abc123");

    CHECK(buf.read_utf8() == 0x61);
    CHECK(buf.read_utf8() == 0x62);
    CHECK(buf.read_utf8() == 0x63);

    CHECK(buf.readoffset() == 3);

    CHECK(buf.read_utf8() == 0x31);
    CHECK(buf.read_utf8() == 0x32);
    CHECK(buf.read_utf8() == 0x33);

    CHECK(buf.writeoffset() == 6);
    CHECK(buf.readoffset() == 6);
  }

  SECTION("throw exception on buffer resize failure") {
    CHECK_THROWS(buf.append_block(nullptr, 0xFFFFFFFFFF));
  }

  SECTION("copies window contents into string") {
    CHECK_THAT(buf.window_string(), Equals(""));
    buf.append_string("hello world!!");

    for (int i = 0; i < 13; i++) {
      buf.read_utf8();
    }

    std::string window = buf.window_string();
    CHECK_THAT(window, Equals("hello world!!"));
  }

  SECTION("resets window") {
    buf.append_string("test");

    for (int i = 0; i < 4; i++) {
      buf.read_utf8();
    }

    {
      std::string window = buf.window_string();
      CHECK_THAT(buf.window_string(), Equals("test"));
    }

    buf.reset_window();

    {
      std::string window = buf.window_string();
      CHECK_THAT(buf.window_string(), Equals(""));
    }
  }

  SECTION("creates a string view of the whole buffer and window sections") {
    buf.append_string("hello world my name is leonard!");

    for (int i = 0; i < 12; i++) {
      buf.read_utf8();
    }

    CHECK_THAT(buf.window_string(), Equals("hello world "));
    CHECK_THAT(buf.buffer_string(), Equals("hello world my name is leonard!"));
  }
}
