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

#include "charly/utils/cast.h"

using namespace charly::utils;

CATCH_TEST_CASE("string_to_int") {
  // decimals
  CATCH_CHECK(string_to_int("0") == 0);
  CATCH_CHECK(string_to_int("1") == 1);
  CATCH_CHECK(string_to_int("100") == 100);
  CATCH_CHECK(string_to_int("-250") == -250);

  // octal
  CATCH_CHECK(string_to_int("10", 8) == 8);
  CATCH_CHECK(string_to_int("20", 8) == 16);
  CATCH_CHECK(string_to_int("40", 8) == 32);
  CATCH_CHECK(string_to_int("-20", 8) == -16);
  CATCH_CHECK(string_to_int("777", 8) == 511);

  // binary
  CATCH_CHECK(string_to_int("0", 2) == 0);
  CATCH_CHECK(string_to_int("1", 2) == 1);
  CATCH_CHECK(string_to_int("10", 2) == 2);
  CATCH_CHECK(string_to_int("100", 2) == 4);
  CATCH_CHECK(string_to_int("1000", 2) == 8);
  CATCH_CHECK(string_to_int("11111111", 2) == 255);
  CATCH_CHECK(string_to_int("-10", 2) == -2);

  // hex
  CATCH_CHECK(string_to_int("0", 16) == 0);
  CATCH_CHECK(string_to_int("10", 16) == 16);
  CATCH_CHECK(string_to_int("20", 16) == 32);
  CATCH_CHECK(string_to_int("ff", 16) == 255);
  CATCH_CHECK(string_to_int("ffff", 16) == 65535);
  CATCH_CHECK(string_to_int("-ff", 16) == -255);
}

CATCH_TEST_CASE("string_to_double") {
  CATCH_CHECK(string_to_double("0.0") == 0.0);
  CATCH_CHECK(string_to_double("1.0") == 1.0);
  CATCH_CHECK(string_to_double("1.5") == 1.5);
  CATCH_CHECK(string_to_double("25.25") == 25.25);
  CATCH_CHECK(string_to_double("-25.25") == -25.25);
  CATCH_CHECK(string_to_double("0.33333") == 0.33333);
}
