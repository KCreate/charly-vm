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
#include <cmath>

#include "charly/symbol.h"

#pragma once

namespace charly::taggedvalue {

using Value = uintptr_t;

// pointer tagging scheme
//
// high                                                                 low
// XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXX 00  integer
//
// XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXX 001  reserved 1
// XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXX 010  reserved 2
// XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXX 011  reserved 3
// XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXX 101  reserved 4
// XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXX 110  misc. heap type
//
// 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000 111  null
// XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX 00000000 00000000 00000000 00001 111  float
// XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX 00000000 00000000 00000000 00010 111  character
// XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX 00000000 00000000 00000000 00011 111  symbol
// 00000000 00000000 00000000 00000000 00000000 00000000 00000000 X0100 111  bool
//
// (diagram adapted from https://bernsteinbear.com/blog/compiling-a-lisp-2/)

// type bits masks
static const uintptr_t kMaskInteger       = 0b00000011;
static const uintptr_t kMaskPointerTags   = 0b00000111;
static const uintptr_t kMaskImmediateType = 0b00111000;
static const uintptr_t kMaskSignature     = 0b00111111;

// payload masks
static const uintptr_t kPayloadPointer   = 0xFFFFFFFFFFFFFFF8;
static const uintptr_t kPayloadFloat     = 0xFFFFFFFF00000000;
static const uintptr_t kPayloadCharacter = 0xFFFFFFFF00000000;
static const uintptr_t kPayloadSymbol    = 0xFFFFFFFF00000000;
static const uintptr_t kPayloadBool      = 0x0000000000000080;

// pointer tags of some well-known types and misc pointers
static const uintptr_t kTagReserved1   = 0b00000001;
static const uintptr_t kTagReserved2   = 0b00000010;
static const uintptr_t kTagReserved3   = 0b00000011;
static const uintptr_t kTagReserved4   = 0b00000101;
static const uintptr_t kTagMiscPointer = 0b00000110;
static const uintptr_t kTagImmediate   = 0b00000111;

// signatures of immediate types
static const uintptr_t kSignatureNull      = kTagImmediate | 0b00000000;
static const uintptr_t kSignatureFloat     = kTagImmediate | 0b00001000;
static const uintptr_t kSignatureCharacter = kTagImmediate | 0b00010000;
static const uintptr_t kSignatureSymbol    = kTagImmediate | 0b00011000;
static const uintptr_t kSignatureBool      = kTagImmediate | 0b00100000;

// shift amounts for payloads
static const int32_t kIntShift       = 2;
static const int32_t kFloatShift     = 32;
static const int32_t kCharacterShift = 32;
static const int32_t kSymbolShift    = 32;
static const int32_t kBoolShift      = 7;

// integer bound limits
static const int64_t kIntLowerLimit = -((int64_t)1 << 61);
static const int64_t kIntUpperLimit = ((int64_t)1 << 61) - 1;

// constant atoms
static const int64_t kNull = kSignatureNull;
static const int64_t kTrue = kSignatureBool | (1 << kBoolShift);
static const int64_t kFalse = kSignatureBool;
static const int64_t kNaN = 0x7fc000000000000f;
static const int64_t kInfinity = 0x7f8000000000000f;
static const int64_t kNegInfinity = 0xff8000000000000f;

// check boxed value for type
bool is_immediate(Value value);
bool is_reserved1(Value value);
bool is_reserved2(Value value);
bool is_reserved3(Value value);
bool is_reserved4(Value value);
bool is_pointer(Value value);
bool is_int(Value value);
bool is_float(Value value);
bool is_char(Value value);
bool is_symbol(Value value);
bool is_bool(Value value);
bool is_null(Value value);

// encode value into boxed representation
Value encode_reserved1(void* value);
Value encode_reserved2(void* value);
Value encode_reserved3(void* value);
Value encode_reserved4(void* value);
Value encode_pointer(void* value);
Value encode_int(int64_t value);
Value encode_float(float value);
Value encode_char(uint32_t value);
Value encode_symbol(SYMBOL value);
Value encode_bool(bool value);
Value encode_null();

// decode boxed value
void* decode_pointer(Value value);
int64_t decode_int(Value value);
float decode_float(Value value);
uint32_t decode_char(Value value);
SYMBOL decode_symbol(Value value);
bool decode_bool(Value value);

}  // namespace charly::taggedvalue
