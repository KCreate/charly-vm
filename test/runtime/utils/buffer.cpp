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

#include <catch2/catch_all.hpp>

#include "charly/utils/buffer.h"

using Catch::Matchers::Equals;

using namespace charly;

TEST_CASE("Buffer") {
  utils::Buffer buf(128);

  REQUIRE(buf.data() != nullptr);
  REQUIRE(buf.capacity() == 128);
  REQUIRE(buf.size() == 0);
  REQUIRE(buf.read_offset() == 0);
  REQUIRE_THAT(buf.window_string(), Equals(""));

  SECTION("Initialize buffer with string value") {
    utils::Buffer buf("hello world!!");

    for (int i = 0; i < 13; i++) {
      buf.read_utf8_cp();
    }

    CHECK_THAT(buf.window_string(), Equals("hello world!!"));
  }

  SECTION("Append data to buffer") {
    buf.emit_string("hello world\n");

    CHECK(buf.read_utf8_cp() == 0x68);
    CHECK(buf.read_utf8_cp() == 0x65);
    CHECK(buf.read_utf8_cp() == 0x6c);
    CHECK(buf.read_utf8_cp() == 0x6c);
    CHECK(buf.read_utf8_cp() == 0x6f);

    CHECK(buf.read_utf8_cp() == 0x20);

    CHECK(buf.read_utf8_cp() == 0x77);
    CHECK(buf.read_utf8_cp() == 0x6f);
    CHECK(buf.read_utf8_cp() == 0x72);
    CHECK(buf.read_utf8_cp() == 0x6c);
    CHECK(buf.read_utf8_cp() == 0x64);

    CHECK(buf.read_utf8_cp() == 0x0a);

    const char* data = "teststring";
    buf.emit_block(data, std::strlen(data));
    buf.emit_string(data);
    buf.emit_string("hallo welt");
    buf.emit_buffer(buf);

    CHECK(buf.capacity() == 128);
    CHECK(buf.size() == 84);
    CHECK(buf.read_offset() == 12);
    CHECK_THAT(buf.buffer_string(), Equals("hello world\nteststringteststringhallo welthello world\nteststringteststringhallo welt"));
  }

  SECTION("appends a string_view into the buffer") {
    buf.emit_string("hello");
    buf.emit_buffer(buf);

    std::string data = "hello";
    std::string_view view(data);
    buf.emit_string_view(view);

    CHECK(buf.size() == 15);
    CHECK_THAT(buf.buffer_string(), Equals("hellohellohello"));
  }

  SECTION("emit primitive data types into buffer") {
    CHECK_THAT(buf.window_string(), Equals(""));
    CHECK(buf.size() == 0);
    CHECK(buf.window_size() == 0);
    CHECK(buf.read_offset() == 0);
    CHECK(buf.write_offset() == 0);
    CHECK(buf.window_offset() == 0);

    buf.emit_u8(0x48);
    buf.emit_u8(0x45);
    buf.emit_u8(0x4C);
    buf.emit_u8(0x4C);
    buf.emit_u8(0x4F);

    CHECK_THAT(buf.window_string(), Equals(""));
    CHECK(buf.size() == 5);
    CHECK(buf.window_size() == 0);
    CHECK(buf.read_offset() == 0);
    CHECK(buf.write_offset() == 5);
    CHECK(buf.window_offset() == 0);

    buf.emit_u8(0x20);
    buf.emit_u8(0x57);
    buf.emit_u8(0x4F);
    buf.emit_u8(0x52);
    buf.emit_u8(0x4C);
    buf.emit_u8(0x44);

    CHECK(buf.read_utf8_cp() == 'H');
    CHECK(buf.read_utf8_cp() == 'E');
    CHECK(buf.read_utf8_cp() == 'L');
    CHECK(buf.read_utf8_cp() == 'L');
    CHECK(buf.read_utf8_cp() == 'O');

    CHECK_THAT(buf.window_string(), Equals("HELLO"));
    CHECK(buf.size() == 11);
    CHECK(buf.window_size() == 5);
    CHECK(buf.read_offset() == 5);
    CHECK(buf.write_offset() == 11);
    CHECK(buf.window_offset() == 0);

    CHECK(buf.read_utf8_cp() == ' ');
    CHECK(buf.read_utf8_cp() == 'W');
    CHECK(buf.read_utf8_cp() == 'O');
    CHECK(buf.read_utf8_cp() == 'R');
    CHECK(buf.read_utf8_cp() == 'L');
    CHECK(buf.read_utf8_cp() == 'D');

    CHECK_THAT(buf.window_string(), Equals("HELLO WORLD"));
    CHECK(buf.size() == 11);
    CHECK(buf.window_size() == 11);
    CHECK(buf.read_offset() == 11);
    CHECK(buf.write_offset() == 11);
    CHECK(buf.window_offset() == 0);
  }

  SECTION("reads / peeks utf8 codepoints") {
    buf.emit_utf8_cp(L'ä');
    buf.emit_utf8_cp(L'Ʒ');
    buf.emit_utf8_cp(L'π');

    CHECK(buf.peek_utf8_cp() == 0xE4);
    CHECK(buf.peek_utf8_cp() == 0xE4);
    CHECK(buf.peek_utf8_cp() == 0xE4);

    CHECK(buf.read_utf8_cp() == 0xE4);
    CHECK(buf.read_utf8_cp() == 0x01B7);
    CHECK(buf.read_utf8_cp() == 0x03C0);

    CHECK(buf.size() == 6);
    CHECK(buf.read_offset() == 6);
  }

  SECTION("reads ascii chars") {
    buf.emit_string("abc123");

    CHECK(buf.read_utf8_cp() == 0x61);
    CHECK(buf.read_utf8_cp() == 0x62);
    CHECK(buf.read_utf8_cp() == 0x63);

    CHECK(buf.read_offset() == 3);

    CHECK(buf.read_utf8_cp() == 0x31);
    CHECK(buf.read_utf8_cp() == 0x32);
    CHECK(buf.read_utf8_cp() == 0x33);

    CHECK(buf.size() == 6);
    CHECK(buf.read_offset() == 6);
  }

  SECTION("copies window contents into string") {
    CHECK_THAT(buf.window_string(), Equals(""));
    buf.emit_string("hello world!!");

    for (int i = 0; i < 13; i++) {
      buf.read_utf8_cp();
    }

    std::string window = buf.window_string();
    CHECK_THAT(window, Equals("hello world!!"));
  }

  SECTION("writes zeroes into the buffer") {
    buf.emit_zeroes(1024);
    CHECK(buf.size() == 1024);
    CHECK(buf.capacity() == 1024);
    CHECK(buf.write_offset() == 1024);
    CHECK(buf.size() == 1024);
  }

  SECTION("seeks to some offset") {
    buf.emit_string("aaa");
    buf.emit_string("bbb");
    buf.emit_string("ccc");
    buf.emit_string("ddd");

    buf.seek(6);
    buf.emit_string("222");

    buf.seek(3);
    buf.emit_string("111");

    buf.seek(0);
    buf.emit_string("000");

    buf.seek(9);
    buf.emit_string("333");

    buf.seek(-1);

    CHECK_THAT(buf.buffer_string(), Equals("000111222333"));
    CHECK(buf.size() == 12);
    CHECK(buf.write_offset() == 12);
  }

  SECTION("resets window") {
    buf.emit_string("test");

    for (int i = 0; i < 4; i++) {
      buf.read_utf8_cp();
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

  SECTION("resets window after seek") {
    buf.emit_string("hello world");

    for (int i = 0; i < 11; i++) {
      buf.read_utf8_cp();
    }

    CHECK_THAT(buf.window_string(), Equals("hello world"));

    buf.seek(0); // resets window
    buf.emit_string("foooo");

    CHECK_THAT(buf.window_string(), Equals(""));

    for (int i = 0; i < 5; i++) {
      buf.read_utf8_cp();
    }

    CHECK_THAT(buf.window_string(), Equals("foooo"));
  }

  SECTION("creates strings / stringviews of buffer content") {
    buf.emit_string("hello world this is a test sentence");

    for (int i = 0; i < 12; i++) {
      buf.read_utf8_cp();
    }

    CHECK_THAT(buf.window_string(), Equals("hello world "));
    CHECK_THAT(buf.buffer_string(), Equals("hello world this is a test sentence"));
    CHECK_THAT(std::string(buf.window_view()), Equals("hello world "));
    CHECK_THAT(std::string(buf.buffer_view()), Equals("hello world this is a test sentence"));
  }
}
