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
#include "charly/value.h"

using namespace charly;
using namespace charly::core::runtime;

CATCH_TEST_CASE("Immediate encoded values") {
  CATCH_SECTION("pointers") {
    CATCH_CHECK(RawObject::make_from_ptr(0).address() == 0);
    CATCH_CHECK(RawObject::make_from_ptr(0x8).address() == 8);
    CATCH_CHECK(RawObject::make_from_ptr(0x1000).address() == 0x1000);
    CATCH_CHECK(RawObject::make_from_ptr(0xfffffffffffffff8).address() == 0xfffffffffffffff8);
  }

  CATCH_SECTION("integers") {
    CATCH_CHECK(RawInt::make(0).value() == 0);
    CATCH_CHECK(RawInt::make(1).value() == 1);
    CATCH_CHECK(RawInt::make(1000).value() == 1000);
    CATCH_CHECK(RawInt::make(-1000).value() == -1000);
    CATCH_CHECK(RawInt::make(0xaaffffffffff).value() == 0xaaffffffffff);
    CATCH_CHECK(RawInt::make(RawInt::kMinValue).value() == RawInt::kMinValue);
    CATCH_CHECK(RawInt::make(RawInt::kMaxValue).value() == RawInt::kMaxValue);
  }

  CATCH_SECTION("floats") {
    CATCH_CHECK(RawFloat::make(0.0).close_to(0.0));
    CATCH_CHECK(RawFloat::make(1.0).close_to(1.0));
    CATCH_CHECK(RawFloat::make(2.0).close_to(2.0));
    CATCH_CHECK(RawFloat::make(3.0).close_to(3.0));
    CATCH_CHECK(RawFloat::make(-1.0).close_to(-1.0));
    CATCH_CHECK(RawFloat::make(-2.0).close_to(-2.0));
    CATCH_CHECK(RawFloat::make(-3.0).close_to(-3.0));
    CATCH_CHECK(RawFloat::make(0.5).close_to(0.5));
    CATCH_CHECK(RawFloat::make(0.25).close_to(0.25));
    CATCH_CHECK(RawFloat::make(0.125).close_to(0.125));
    CATCH_CHECK(RawFloat::make(0.0625).close_to(0.0625));
    CATCH_CHECK(RawFloat::make(25.1234).close_to(25.1234, 0.000000001));
    CATCH_CHECK(RawFloat::make(-25.1234).close_to(-25.1234, 0.000000001));
  }

  CATCH_SECTION("small strings") {
    RawSmallString s1 = RawSmallString::make_from_cstr("");
    RawSmallString s2 = RawSmallString::make_from_cstr("a");
    RawSmallString s3 = RawSmallString::make_from_cstr("abcdefg");
    RawSmallString s4 = RawSmallString::make_from_cstr("       ");
    RawSmallString s5 = RawSmallString::make_from_cstr("\n\n\n\n\n\n\n");

    RawSmallString s6 = RawSmallString::make_from_cp(U'1');
    RawSmallString s7 = RawSmallString::make_from_cp(U' ');
    RawSmallString s8 = RawSmallString::make_from_cp(U'a');
    RawSmallString s9 = RawSmallString::make_from_cp(U'@');
    RawSmallString s10 = RawSmallString::make_from_cp(U'ä');
    RawSmallString s11 = RawSmallString::make_from_cp(U'©');
    RawSmallString s12 = RawSmallString::make_from_cp(U'ç');
    RawSmallString s13 = RawSmallString::make_from_cp(U'€');
    RawSmallString s14 = RawSmallString::make_from_cp(U'𐍈');

    CATCH_CHECK(RawString::compare(RawString::cast(s1), "") == 0);
    CATCH_CHECK(RawString::compare(RawString::cast(s2), "a") == 0);
    CATCH_CHECK(RawString::compare(RawString::cast(s3), "abcdefg") == 0);
    CATCH_CHECK(RawString::compare(RawString::cast(s4), "       ") == 0);
    CATCH_CHECK(RawString::compare(RawString::cast(s5), "\n\n\n\n\n\n\n") == 0);
    CATCH_CHECK(RawString::compare(RawString::cast(s6), "1") == 0);
    CATCH_CHECK(RawString::compare(RawString::cast(s7), " ") == 0);
    CATCH_CHECK(RawString::compare(RawString::cast(s8), "a") == 0);
    CATCH_CHECK(RawString::compare(RawString::cast(s9), "@") == 0);
    CATCH_CHECK(RawString::compare(RawString::cast(s10), "ä") == 0);
    CATCH_CHECK(RawString::compare(RawString::cast(s11), "©") == 0);
    CATCH_CHECK(RawString::compare(RawString::cast(s12), "ç") == 0);
    CATCH_CHECK(RawString::compare(RawString::cast(s13), "€") == 0);
    CATCH_CHECK(RawString::compare(RawString::cast(s14), "𐍈") == 0);

    CATCH_CHECK(s1.length() == 0);
    CATCH_CHECK(s2.length() == 1);
    CATCH_CHECK(s3.length() == 7);
    CATCH_CHECK(s4.length() == 7);
    CATCH_CHECK(s5.length() == 7);

    CATCH_CHECK(s6.length() == 1);
    CATCH_CHECK(s7.length() == 1);
    CATCH_CHECK(s8.length() == 1);
    CATCH_CHECK(s9.length() == 1);
    CATCH_CHECK(s10.length() == 2);
    CATCH_CHECK(s11.length() == 2);
    CATCH_CHECK(s12.length() == 2);
    CATCH_CHECK(s13.length() == 3);
    CATCH_CHECK(s14.length() == 4);
  }

  CATCH_SECTION("symbols") {
    CATCH_CHECK(RawSymbol::make(SYM("hello")).value() == SYM("hello"));
    CATCH_CHECK(RawSymbol::make(SYM("")).value() == SYM(""));
    CATCH_CHECK(RawSymbol::make(SYM("123")).value() == SYM("123"));
    CATCH_CHECK(RawSymbol::make(SYM("a")).value() == SYM("a"));
  }

  CATCH_SECTION("bools") {
    CATCH_CHECK(RawBool::make(true).value() == true);
    CATCH_CHECK(RawBool::make(false).value() == false);
  }

  CATCH_SECTION("null") {
    CATCH_CHECK(RawNull::make().error_code() == ErrorId::kErrorNone);
    CATCH_CHECK(RawNull::make_error(ErrorId::kErrorOk).error_code() == ErrorId::kErrorOk);
    CATCH_CHECK(RawNull::make_error(ErrorId::kErrorNotFound).error_code() == ErrorId::kErrorNotFound);
    CATCH_CHECK(RawNull::make_error(ErrorId::kErrorOutOfBounds).error_code() == ErrorId::kErrorOutOfBounds);
    CATCH_CHECK(RawNull::make_error(ErrorId::kErrorException).error_code() == ErrorId::kErrorException);
  }

  CATCH_SECTION("shape group checks") {
    CATCH_CHECK(is_immediate_shape(ShapeId::kInt) == true);
    CATCH_CHECK(is_immediate_shape(ShapeId::kFloat) == true);
    CATCH_CHECK(is_immediate_shape(ShapeId::kBool) == true);
    CATCH_CHECK(is_immediate_shape(ShapeId::kSymbol) == true);
    CATCH_CHECK(is_immediate_shape(ShapeId::kNull) == true);
    CATCH_CHECK(is_immediate_shape(ShapeId::kSmallString) == true);
    CATCH_CHECK(is_immediate_shape(ShapeId::kSmallBytes) == true);
    CATCH_CHECK(is_immediate_shape(ShapeId::kLargeBytes) == false);
    CATCH_CHECK(is_immediate_shape(ShapeId::kFunction) == false);
    CATCH_CHECK(is_immediate_shape(ShapeId::kException) == false);

    CATCH_CHECK(is_object_shape(ShapeId::kInt) == false);
    CATCH_CHECK(is_object_shape(ShapeId::kFloat) == false);
    CATCH_CHECK(is_object_shape(ShapeId::kBool) == false);
    CATCH_CHECK(is_object_shape(ShapeId::kSymbol) == false);
    CATCH_CHECK(is_object_shape(ShapeId::kNull) == false);
    CATCH_CHECK(is_object_shape(ShapeId::kSmallString) == false);
    CATCH_CHECK(is_object_shape(ShapeId::kSmallBytes) == false);
    CATCH_CHECK(is_object_shape(ShapeId::kLargeBytes) == true);
    CATCH_CHECK(is_object_shape(ShapeId::kFunction) == true);
    CATCH_CHECK(is_object_shape(ShapeId::kException) == true);

    CATCH_CHECK(is_data_shape(ShapeId::kInt) == false);
    CATCH_CHECK(is_data_shape(ShapeId::kFloat) == false);
    CATCH_CHECK(is_data_shape(ShapeId::kBool) == false);
    CATCH_CHECK(is_data_shape(ShapeId::kSymbol) == false);
    CATCH_CHECK(is_data_shape(ShapeId::kNull) == false);
    CATCH_CHECK(is_data_shape(ShapeId::kSmallString) == false);
    CATCH_CHECK(is_data_shape(ShapeId::kSmallBytes) == false);
    CATCH_CHECK(is_data_shape(ShapeId::kLargeBytes) == true);
    CATCH_CHECK(is_data_shape(ShapeId::kFunction) == false);
    CATCH_CHECK(is_data_shape(ShapeId::kException) == false);

    CATCH_CHECK(is_instance_shape(ShapeId::kInt) == false);
    CATCH_CHECK(is_instance_shape(ShapeId::kFloat) == false);
    CATCH_CHECK(is_instance_shape(ShapeId::kBool) == false);
    CATCH_CHECK(is_instance_shape(ShapeId::kSymbol) == false);
    CATCH_CHECK(is_instance_shape(ShapeId::kNull) == false);
    CATCH_CHECK(is_instance_shape(ShapeId::kSmallString) == false);
    CATCH_CHECK(is_instance_shape(ShapeId::kSmallBytes) == false);
    CATCH_CHECK(is_instance_shape(ShapeId::kLargeBytes) == false);
    CATCH_CHECK(is_instance_shape(ShapeId::kFunction) == true);
    CATCH_CHECK(is_instance_shape(ShapeId::kException) == true);

    CATCH_CHECK(is_exception_shape(ShapeId::kInt) == false);
    CATCH_CHECK(is_exception_shape(ShapeId::kFloat) == false);
    CATCH_CHECK(is_exception_shape(ShapeId::kBool) == false);
    CATCH_CHECK(is_exception_shape(ShapeId::kSymbol) == false);
    CATCH_CHECK(is_exception_shape(ShapeId::kNull) == false);
    CATCH_CHECK(is_exception_shape(ShapeId::kSmallString) == false);
    CATCH_CHECK(is_exception_shape(ShapeId::kSmallBytes) == false);
    CATCH_CHECK(is_exception_shape(ShapeId::kLargeBytes) == false);
    CATCH_CHECK(is_exception_shape(ShapeId::kFunction) == false);
    CATCH_CHECK(is_exception_shape(ShapeId::kException) == true);

    CATCH_CHECK(is_builtin_shape(ShapeId::kInt) == true);
    CATCH_CHECK(is_builtin_shape(ShapeId::kFloat) == true);
    CATCH_CHECK(is_builtin_shape(ShapeId::kBool) == true);
    CATCH_CHECK(is_builtin_shape(ShapeId::kSymbol) == true);
    CATCH_CHECK(is_builtin_shape(ShapeId::kNull) == true);
    CATCH_CHECK(is_builtin_shape(ShapeId::kSmallString) == true);
    CATCH_CHECK(is_builtin_shape(ShapeId::kSmallBytes) == true);
    CATCH_CHECK(is_builtin_shape(ShapeId::kLargeBytes) == true);
    CATCH_CHECK(is_builtin_shape(ShapeId::kFunction) == true);
    CATCH_CHECK(is_builtin_shape(ShapeId::kException) == true);

    CATCH_CHECK(is_user_shape(ShapeId::kInt) == false);
    CATCH_CHECK(is_user_shape(ShapeId::kFloat) == false);
    CATCH_CHECK(is_user_shape(ShapeId::kBool) == false);
    CATCH_CHECK(is_user_shape(ShapeId::kSymbol) == false);
    CATCH_CHECK(is_user_shape(ShapeId::kNull) == false);
    CATCH_CHECK(is_user_shape(ShapeId::kSmallString) == false);
    CATCH_CHECK(is_user_shape(ShapeId::kSmallBytes) == false);
    CATCH_CHECK(is_user_shape(ShapeId::kLargeBytes) == false);
    CATCH_CHECK(is_user_shape(ShapeId::kFunction) == false);
    CATCH_CHECK(is_user_shape(ShapeId::kException) == false);
  }

  CATCH_SECTION("value truthyness") {
    CATCH_CHECK(RawInt::make(0).truthyness() == false);
    CATCH_CHECK(RawInt::make(1).truthyness() == true);
    CATCH_CHECK(RawInt::make(200).truthyness() == true);
    CATCH_CHECK(RawInt::make(-200).truthyness() == true);

    CATCH_CHECK(RawObject::make_from_ptr(0).truthyness() == false);
    CATCH_CHECK(RawObject::make_from_ptr(0x10000).truthyness() == true);

    CATCH_CHECK(RawFloat::make(0.0).truthyness() == false);
    CATCH_CHECK(RawFloat::make(0.1).truthyness() == true);
    CATCH_CHECK(RawFloat::make(0.5).truthyness() == true);
    CATCH_CHECK(RawFloat::make(1).truthyness() == true);
    CATCH_CHECK(RawFloat::make(10).truthyness() == true);
    CATCH_CHECK(RawFloat::make(-0.1).truthyness() == true);
    CATCH_CHECK(RawFloat::make(-0.5).truthyness() == true);
    CATCH_CHECK(RawFloat::make(-1).truthyness() == true);
    CATCH_CHECK(RawFloat::make(-10).truthyness() == true);
    CATCH_CHECK(RawFloat::make(NAN).truthyness() == false);
    CATCH_CHECK(RawFloat::make(INFINITY).truthyness() == true);
    CATCH_CHECK(RawFloat::make(-INFINITY).truthyness() == true);

    CATCH_CHECK(RawBool::make(true).truthyness() == true);
    CATCH_CHECK(RawBool::make(false).truthyness() == false);

    CATCH_CHECK(RawSymbol::make(SYM("hello")).truthyness() == true);

    CATCH_CHECK(RawNull::make().truthyness() == false);
    CATCH_CHECK(kErrorNone.truthyness() == false);
    CATCH_CHECK(kErrorOk.truthyness() == false);
    CATCH_CHECK(kErrorException.truthyness() == false);
    CATCH_CHECK(kErrorNotFound.truthyness() == false);
    CATCH_CHECK(kErrorOutOfBounds.truthyness() == false);
  }

  CATCH_SECTION("value shapes") {
    CATCH_CHECK(RawInt::make(1).shape_id_not_object_int() == ShapeId::kInt);
    CATCH_CHECK(RawFloat::make(3.1415).shape_id_not_object_int() == ShapeId::kFloat);
    CATCH_CHECK(RawBool::make(false).shape_id_not_object_int() == ShapeId::kBool);
    CATCH_CHECK(RawSymbol::make(SYM("hello")).shape_id_not_object_int() == ShapeId::kSymbol);
    CATCH_CHECK(RawNull::make().shape_id_not_object_int() == ShapeId::kNull);
    CATCH_CHECK(RawSmallString::make_from_cstr("test123").shape_id_not_object_int() == ShapeId::kSmallString);
    const char* foo = "test123";
    const auto* ptr = bitcast<const uint8_t*>(foo);
    CATCH_CHECK(RawSmallBytes::make_from_memory(ptr, 7).shape_id_not_object_int() == ShapeId::kSmallBytes);
  }
}
