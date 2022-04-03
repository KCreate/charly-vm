/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

CATCH_TEST_CASE("Buffer") {
  utils::Buffer buf(128);

  CATCH_REQUIRE(buf.data() != nullptr);
  CATCH_REQUIRE(buf.capacity() == 128);
  CATCH_REQUIRE(buf.size() == 0);
  CATCH_REQUIRE(buf.read_offset() == 0);
  CATCH_REQUIRE_THAT(buf.window_str(), Equals(""));

  CATCH_SECTION("Initialize buffer with string value") {
    utils::Buffer buf2("hello world!!");

    for (int i = 0; i < 13; i++) {
      CATCH_CHECK(buf2.read_utf8_cp() >= 0);
    }

    CATCH_CHECK_THAT(buf2.window_str(), Equals("hello world!!"));
  }

  CATCH_SECTION("Append data to buffer") {
    buf << "hello world\n";

    CATCH_CHECK(buf.read_utf8_cp() == 0x68);
    CATCH_CHECK(buf.read_utf8_cp() == 0x65);
    CATCH_CHECK(buf.read_utf8_cp() == 0x6c);
    CATCH_CHECK(buf.read_utf8_cp() == 0x6c);
    CATCH_CHECK(buf.read_utf8_cp() == 0x6f);

    CATCH_CHECK(buf.read_utf8_cp() == 0x20);

    CATCH_CHECK(buf.read_utf8_cp() == 0x77);
    CATCH_CHECK(buf.read_utf8_cp() == 0x6f);
    CATCH_CHECK(buf.read_utf8_cp() == 0x72);
    CATCH_CHECK(buf.read_utf8_cp() == 0x6c);
    CATCH_CHECK(buf.read_utf8_cp() == 0x64);

    CATCH_CHECK(buf.read_utf8_cp() == 0x0a);

    const char* data = "teststring";
    buf.write(data, std::strlen(data));
    buf << data;
    buf << "hallo welt";
    buf.write_buffer(buf);

    CATCH_CHECK(buf.capacity() == 128);
    CATCH_CHECK(buf.size() == 84);
    CATCH_CHECK(buf.read_offset() == 12);
    CATCH_CHECK_THAT(buf.str(),
                     Equals("hello world\nteststringteststringhallo welthello world\nteststringteststringhallo welt"));
  }

  CATCH_SECTION("appends a string_view into the buffer") {
    buf << "hello";
    buf.write_buffer(buf);

    std::string data = "hello";
    std::string_view view(data);
    buf << view;

    CATCH_CHECK(buf.size() == 15);
    CATCH_CHECK_THAT(buf.str(), Equals("hellohellohello"));
  }

  CATCH_SECTION("emit primitive data types into buffer") {
    CATCH_CHECK_THAT(buf.window_str(), Equals(""));
    CATCH_CHECK(buf.size() == 0);
    CATCH_CHECK(buf.window_size() == 0);
    CATCH_CHECK(buf.read_offset() == 0);
    CATCH_CHECK(buf.write_offset() == 0);
    CATCH_CHECK(buf.window_offset() == 0);

    buf.write_u8(0x48);
    buf.write_u8(0x45);
    buf.write_u8(0x4C);
    buf.write_u8(0x4C);
    buf.write_u8(0x4F);

    CATCH_CHECK_THAT(buf.window_str(), Equals(""));
    CATCH_CHECK(buf.size() == 5);
    CATCH_CHECK(buf.window_size() == 0);
    CATCH_CHECK(buf.read_offset() == 0);
    CATCH_CHECK(buf.write_offset() == 5);
    CATCH_CHECK(buf.window_offset() == 0);

    buf.write_u8(0x20);
    buf.write_u8(0x57);
    buf.write_u8(0x4F);
    buf.write_u8(0x52);
    buf.write_u8(0x4C);
    buf.write_u8(0x44);

    CATCH_CHECK(buf.read_utf8_cp() == 'H');
    CATCH_CHECK(buf.read_utf8_cp() == 'E');
    CATCH_CHECK(buf.read_utf8_cp() == 'L');
    CATCH_CHECK(buf.read_utf8_cp() == 'L');
    CATCH_CHECK(buf.read_utf8_cp() == 'O');

    CATCH_CHECK_THAT(buf.window_str(), Equals("HELLO"));
    CATCH_CHECK(buf.size() == 11);
    CATCH_CHECK(buf.window_size() == 5);
    CATCH_CHECK(buf.read_offset() == 5);
    CATCH_CHECK(buf.write_offset() == 11);
    CATCH_CHECK(buf.window_offset() == 0);

    CATCH_CHECK(buf.read_utf8_cp() == ' ');
    CATCH_CHECK(buf.read_utf8_cp() == 'W');
    CATCH_CHECK(buf.read_utf8_cp() == 'O');
    CATCH_CHECK(buf.read_utf8_cp() == 'R');
    CATCH_CHECK(buf.read_utf8_cp() == 'L');
    CATCH_CHECK(buf.read_utf8_cp() == 'D');

    CATCH_CHECK_THAT(buf.window_str(), Equals("HELLO WORLD"));
    CATCH_CHECK(buf.size() == 11);
    CATCH_CHECK(buf.window_size() == 11);
    CATCH_CHECK(buf.read_offset() == 11);
    CATCH_CHECK(buf.write_offset() == 11);
    CATCH_CHECK(buf.window_offset() == 0);
  }

  CATCH_SECTION("reads / peeks utf8 codepoints") {
    buf.write_utf8_cp(L'ä');
    buf.write_utf8_cp(L'Ʒ');
    buf.write_utf8_cp(L'π');

    CATCH_CHECK(buf.peek_utf8_cp() == 0xE4);
    CATCH_CHECK(buf.peek_utf8_cp() == 0xE4);
    CATCH_CHECK(buf.peek_utf8_cp() == 0xE4);

    CATCH_CHECK(buf.read_utf8_cp() == 0xE4);
    CATCH_CHECK(buf.read_utf8_cp() == 0x01B7);
    CATCH_CHECK(buf.read_utf8_cp() == 0x03C0);
    CATCH_CHECK(buf.read_utf8_cp() == -1);

    CATCH_CHECK(buf.size() == 6);
    CATCH_CHECK(buf.read_offset() == 6);
  }

  CATCH_SECTION("reads ascii chars") {
    buf << "abc123";

    CATCH_CHECK(buf.read_utf8_cp() == 0x61);
    CATCH_CHECK(buf.read_utf8_cp() == 0x62);
    CATCH_CHECK(buf.read_utf8_cp() == 0x63);

    CATCH_CHECK(buf.read_offset() == 3);

    CATCH_CHECK(buf.read_utf8_cp() == 0x31);
    CATCH_CHECK(buf.read_utf8_cp() == 0x32);
    CATCH_CHECK(buf.read_utf8_cp() == 0x33);
    CATCH_CHECK(buf.read_utf8_cp() == -1);

    CATCH_CHECK(buf.size() == 6);
    CATCH_CHECK(buf.read_offset() == 6);
  }

  CATCH_SECTION("copies window contents into string") {
    CATCH_CHECK_THAT(buf.window_str(), Equals(""));
    buf << "hello world!!";

    for (int i = 0; i < 13; i++) {
      CATCH_CHECK(buf.read_utf8_cp() >= 0);
    }

    std::string window = buf.window_str();
    CATCH_CHECK_THAT(window, Equals("hello world!!"));
  }

  CATCH_SECTION("seeks to some offset") {
    buf << "aaa";
    buf << "bbb";
    buf << "ccc";
    buf << "ddd";

    auto old = buf.tellp();

    buf.seekp(6);
    buf << "222";
    buf.seekp(3);
    buf << "111";
    buf.seekp(0);
    buf << "000";
    buf.seekp(9);
    buf << "333";
    buf.seekp(old);

    CATCH_CHECK_THAT(buf.str(), Equals("000111222333"));
    CATCH_CHECK(buf.size() == 12);
    CATCH_CHECK(buf.write_offset() == 12);
  }

  CATCH_SECTION("resets window") {
    buf << "test";

    for (int i = 0; i < 4; i++) {
      CATCH_CHECK(buf.read_utf8_cp() >= 0);
    }

    {
      std::string window = buf.window_str();
      CATCH_CHECK_THAT(buf.window_str(), Equals("test"));
    }

    buf.reset_window();

    {
      std::string window = buf.window_str();
      CATCH_CHECK_THAT(buf.window_str(), Equals(""));
    }
  }

  CATCH_SECTION("creates strings / stringviews of buffer content") {
    buf << "hello world this is a test sentence";

    for (int i = 0; i < 12; i++) {
      CATCH_CHECK(buf.read_utf8_cp() >= 0);
    }

    CATCH_CHECK_THAT(buf.window_str(), Equals("hello world "));
    CATCH_CHECK_THAT(buf.str(), Equals("hello world this is a test sentence"));
    CATCH_CHECK_THAT(std::string(buf.window_view()), Equals("hello world "));
    CATCH_CHECK_THAT(std::string(buf.view()), Equals("hello world this is a test sentence"));
  }

  CATCH_SECTION("enables / disables memory protection") {
    buf << "hello world";
    CATCH_CHECK_THAT(buf.str(), Equals("hello world"));
    buf.protect();
    CATCH_CHECK_THAT(buf.str(), Equals("hello world"));
    buf.unprotect();
    CATCH_CHECK_THAT(buf.str(), Equals("hello world"));
  }
}