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
#include "charly/taggedvalue.h"
#include "charly/symbol.h"

using namespace charly;
using namespace charly::taggedvalue;

// - typecheck methods
// - decoding methods

TEST_CASE("pointers") {
  CHECK(encode_pointer((void*)0x0) == 0x0 + kTagMiscPointer);
  CHECK(encode_pointer((void*)0x200) == 0x200 + kTagMiscPointer);
  CHECK(encode_pointer((void*)0xfffffffffff8) == 0xfffffffffff8 + kTagMiscPointer);

  CHECK(decode_pointer(encode_pointer((void*)0x0)) == (void*)0x0);
  CHECK(decode_pointer(encode_pointer((void*)0x200)) == (void*)0x200);
  CHECK(decode_pointer(encode_pointer((void*)0xfffffffffff8)) == (void*)0xfffffffffff8);
}

TEST_CASE("integers") {
  CHECK(encode_int(0x0) == 0x0);
  CHECK(encode_int(0x1) == 0x4);
  CHECK(encode_int(0x2) == 0x8);
  CHECK(encode_int(0x4) == 0x10);
  CHECK(encode_int(0x500) == 0x1400);
  CHECK(encode_int(-500) == 0xfffffffffffff830);
  CHECK(encode_int(-800) == 0xfffffffffffff380);
  CHECK(encode_int(kIntLowerLimit) == 0x8000000000000000);
  CHECK(encode_int(kIntUpperLimit) == 0x7ffffffffffffffc);

  CHECK(decode_int(encode_int(0x0)) == 0x0);
  CHECK(decode_int(encode_int(0x1)) == 0x1);
  CHECK(decode_int(encode_int(0x2)) == 0x2);
  CHECK(decode_int(encode_int(0x4)) == 0x4);
  CHECK(decode_int(encode_int(0x500)) == 0x500);
  CHECK(decode_int(encode_int(-500)) == -500);
  CHECK(decode_int(encode_int(-800)) == -800);
  CHECK(decode_int(encode_int(kIntLowerLimit)) == kIntLowerLimit);
  CHECK(decode_int(encode_int(kIntUpperLimit)) == kIntUpperLimit);
}

TEST_CASE("floats") {
  CHECK(encode_float(0) == 0x0f);
  CHECK(encode_float(1) == 0x3f8000000000000f);
  CHECK(encode_float(-1) == 0xbf8000000000000f);
  CHECK(encode_float(10000) == 0x461c40000000000f);
  CHECK(encode_float(-10000) == 0xc61c40000000000f);
  CHECK(encode_float(0.125) == 0x3e0000000000000f);
  CHECK(encode_float(100.125) == 0x42c840000000000f);
  CHECK(encode_float(-0.125) == 0xbe0000000000000f);
  CHECK(encode_float(-100.125) == 0xc2c840000000000f);
  CHECK(encode_float(NAN) == kNaN);

  CHECK(decode_float(encode_float(0)) == 0.0);
  CHECK(decode_float(encode_float(1)) == 1.0);
  CHECK(decode_float(encode_float(-1)) == -1.0);
  CHECK(decode_float(encode_float(10000)) == 10000);
  CHECK(decode_float(encode_float(-10000)) == -10000);
  CHECK(decode_float(encode_float(0.125)) == 0.125);
  CHECK(decode_float(encode_float(100.125)) == 100.125);
  CHECK(decode_float(encode_float(-0.125)) == -0.125);
  CHECK(decode_float(encode_float(-100.125)) == -100.125);
  CHECK(std::isnan(decode_float(encode_float(NAN))));
}

TEST_CASE("characters") {
  CHECK(encode_char(u'\0') == 0x0000000000000017);
  CHECK(encode_char(u'\n') == 0x0000000a00000017);
  CHECK(encode_char(u'a')  == 0x0000006100000017);
  CHECK(encode_char(u'®')  == 0x000000ae00000017);
  CHECK(encode_char(u'©')  == 0x000000a900000017);
  CHECK(encode_char(u'π')  == 0x000003c000000017);

  utils::Buffer buf(4);
  buf.emit_string("🔥");
  CHECK(buf.size() == 4);
  uint32_t character = buf.read_utf8_cp();
  CHECK(encode_char(character) == 0x0001f52500000017);
}

TEST_CASE("symbols") {
  CHECK(encode_symbol(SYM("foo")) == 0x8c7365210000001f);
  CHECK(encode_symbol(SYM("bar")) == 0x76ff8caa0000001f);
  CHECK(encode_symbol(SYM("hello world")) == 0xd4a11850000001f);
  CHECK(encode_symbol(SYM("")) == 0x1f);
}

TEST_CASE("bools") {
  CHECK(encode_bool(true) == 0xa7);
  CHECK(encode_bool(false) == 0x27);
}

TEST_CASE("null") {
  CHECK(encode_null() == 0x07);
}
