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

#include <cassert>

#include "charly/taggedvalue.h"

namespace charly::taggedvalue {

bool is_immediate(Value value) {
  return (value & kMaskPointerTags) == kTagImmediate;
}

bool is_reserved1(Value value) {
  return (value & kMaskPointerTags) == kTagReserved1;
}

bool is_reserved2(Value value) {
  return (value & kMaskPointerTags) == kTagReserved2;
}

bool is_reserved3(Value value) {
  return (value & kMaskPointerTags) == kTagReserved3;
}

bool is_reserved4(Value value) {
  return (value & kMaskPointerTags) == kTagReserved4;
}

bool is_pointer(Value value) {
  if (is_int(value)) {
    return false;
  }

  if (is_immediate(value)) {
    return false;
  }

  return true;
}

bool is_int(Value value) {
  return (value & kMaskInteger) == 0;
}

bool is_float(Value value) {
  return (value & kMaskSignature) == kSignatureFloat;
}

bool is_char(Value value) {
  return (value & kMaskSignature) == kSignatureCharacter;
}

bool is_symbol(Value value) {
  return (value & kMaskSignature) == kSignatureSymbol;
}

bool is_bool(Value value) {
  return (value & kMaskSignature) == kSignatureBool;
}

bool is_null(Value value) {
  return (value & kMaskSignature) == kSignatureNull;
}

Value encode_reserved1(void* value) {
  assert(((uintptr_t)value & kMaskPointerTags) == 0 && "invalid pointer alignment");
  return ((uintptr_t)value | kTagReserved1);
}

Value encode_reserved2(void* value) {
  assert(((uintptr_t)value & kMaskPointerTags) == 0 && "invalid pointer alignment");
  return ((uintptr_t)value | kTagReserved2);
}

Value encode_reserved3(void* value) {
  assert(((uintptr_t)value & kMaskPointerTags) == 0 && "invalid pointer alignment");
  return ((uintptr_t)value | kTagReserved3);
}

Value encode_reserved4(void* value) {
  assert(((uintptr_t)value & kMaskPointerTags) == 0 && "invalid pointer alignment");
  return ((uintptr_t)value | kTagReserved4);
}

Value encode_pointer(void* value) {
  assert(((uintptr_t)value & kMaskPointerTags) == 0 && "invalid pointer alignment");
  return ((uintptr_t)value | kTagMiscPointer);
}

Value encode_int(int64_t value) {

  // bounds check
  if (!(kIntLowerLimit <= value && value <= kIntUpperLimit)) {
    return encode_float(value);
  }

  return value << kIntShift;
}

Value encode_float(float value) {
  union {
    float f;
    uint32_t i;
  } cast;

  cast.f = value;

  return ((uintptr_t)cast.i << kFloatShift) | kSignatureFloat;
}

Value encode_char(uint32_t value) {
  return ((uintptr_t)value << kCharacterShift) | kSignatureCharacter;
}

Value encode_symbol(SYMBOL value) {
  return ((uintptr_t)value << kSymbolShift) | kSignatureSymbol;
}

Value encode_bool(bool value) {
  return value ? kTrue : kFalse;
}

Value encode_null() {
  return kNull;
}

void* decode_pointer(Value value) {
  return (void*)(value & kPayloadPointer);
}

int64_t decode_int(Value value) {
  return (int64_t)value >> kIntShift;
}

float decode_float(Value value) {
  union {
    float f;
    uint32_t i;
  } cast;

  cast.i = value >> kFloatShift;
  return cast.f;
}

uint32_t decode_char(Value value) {
  return value >> kCharacterShift;
}

SYMBOL decode_symbol(Value value) {
  return value >> kSymbolShift;
}

bool decode_bool(Value value) {
  return value & kPayloadBool;
}

}  // namespace charly::taggedvalue
