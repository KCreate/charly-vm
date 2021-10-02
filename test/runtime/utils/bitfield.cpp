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

#include "charly/utils/bitfield.h"

using Catch::Matchers::Equals;

using namespace charly;

TEST_CASE("BitField") {

  SECTION("Creates a bitfield") {
    utils::BitField<64> field1;
    CHECK(field1.size() == 64);

    utils::BitField<8> field2;
    CHECK(field2.size() == 8);

    utils::BitField<16> field3;
    CHECK(field3.size() == 16);
  }

  SECTION("Sets and unsets bits in the bitfield") {
    utils::BitField<64> field;

    field.set_bit(0);
    field.set_bit(1);
    field.set_bit(7);
    field.set_bit(32);
    field.set_bit(33);
    field.set_bit(63);

    field.unset_bit(7);
    field.unset_bit(33);

    CHECK(field.get_bit(0) == true);
    CHECK(field.get_bit(1) == true);
    CHECK(field.get_bit(7) == false);
    CHECK(field.get_bit(32) == true);
    CHECK(field.get_bit(33) == false);
    CHECK(field.get_bit(63) == true);
  }

  SECTION("searches for the next set bit") {
    utils::BitField<64> field;

    field.set_bit(0);
    field.set_bit(1);
    field.set_bit(7);
    field.set_bit(32);
    field.set_bit(33);
    field.set_bit(63);

    field.unset_bit(7);
    field.unset_bit(33);

    int32_t index = 0;
    index = field.find_next_set_bit(index);
    CHECK(index == 0);
    index = field.find_next_set_bit(index + 1);
    CHECK(index == 1);
    index = field.find_next_set_bit(index + 1);
    CHECK(index == 32);
    index = field.find_next_set_bit(index + 1);
    CHECK(index == 63);
    index = field.find_next_set_bit(index + 1);
    CHECK(index == -1);
  }

  SECTION("resets the bitfield") {
    utils::BitField<64> field;

    field.set_bit(0);
    field.set_bit(1);
    field.set_bit(7);
    field.set_bit(32);
    field.set_bit(33);
    field.set_bit(63);

    field.unset_bit(7);
    field.unset_bit(33);

    CHECK(field.get_bit(0) == true);
    CHECK(field.get_bit(1) == true);
    CHECK(field.get_bit(7) == false);
    CHECK(field.get_bit(32) == true);
    CHECK(field.get_bit(33) == false);
    CHECK(field.get_bit(63) == true);

    field.reset();

    CHECK(field.get_bit(0) == false);
    CHECK(field.get_bit(1) == false);
    CHECK(field.get_bit(7) == false);
    CHECK(field.get_bit(32) == false);
    CHECK(field.get_bit(33) == false);
    CHECK(field.get_bit(63) == false);
  }

}
