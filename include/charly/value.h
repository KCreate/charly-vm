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

#include <cstdint>

#include "charly.h"
#include "taggedvalue.h"

#pragma once

namespace charly {

// wrapper class around the raw immediate encoded bytes
class HeapValue;
struct VALUE {
  taggedvalue::Value raw;
  constexpr VALUE(taggedvalue::Value raw) : raw(raw) {}

  static VALUE Pointer(void* value) {
    return VALUE(taggedvalue::encode_pointer(value));
  }

  static VALUE Pointer(HeapValue& value) {
    return VALUE(taggedvalue::encode_pointer(&value));
  }

  static VALUE Int(int64_t value) {
    return VALUE(taggedvalue::encode_int(value));
  }

  static VALUE Float(double value) {
    return VALUE(taggedvalue::encode_float(value));
  }

  static VALUE Bool(bool value) {
    return VALUE(taggedvalue::encode_bool(value));
  }

  static VALUE Null() {
    return VALUE(taggedvalue::encode_null());
  }

  static VALUE Char(uint32_t value) {
    return VALUE(taggedvalue::encode_char(value));
  }

  static VALUE Symbol(SYMBOL value) {
    return VALUE(taggedvalue::encode_symbol(value));
  }

  bool is_pointer() const { return taggedvalue::is_pointer(raw); }
  bool is_int() const { return taggedvalue::is_int(raw); }
  bool is_float() const { return taggedvalue::is_float(raw); }
  bool is_char() const { return taggedvalue::is_char(raw); }
  bool is_symbol() const { return taggedvalue::is_symbol(raw); }
  bool is_bool() const { return taggedvalue::is_bool(raw); }
  bool is_null() const { return taggedvalue::is_null(raw); }

  template <typename T>
  T* to_pointer() const { return static_cast<T*>(taggedvalue::decode_pointer(raw)); }
  int64_t to_int() const { return taggedvalue::decode_int(raw); }
  float to_float() const { return taggedvalue::decode_float(raw); }
  uint32_t to_char() const { return taggedvalue::decode_char(raw); }
  uint32_t to_symbol() const { return taggedvalue::decode_symbol(raw); }
  bool to_bool() const { return taggedvalue::decode_bool(raw); }

  friend std::ostream& operator<<(std::ostream& out, const VALUE& value) {
    out << value.raw;
    return out;
  }
};
static_assert(sizeof(VALUE) == sizeof(uintptr_t));

static const VALUE kNull        = VALUE(taggedvalue::kNull);
static const VALUE kNaN         = VALUE(taggedvalue::kNaN);
static const VALUE kInfinity    = VALUE(taggedvalue::kInfinity);
static const VALUE kNegInfinity = VALUE(taggedvalue::kNegInfinity);
static const VALUE kTrue        = VALUE(taggedvalue::kTrue);
static const VALUE kFalse       = VALUE(taggedvalue::kFalse);

}
