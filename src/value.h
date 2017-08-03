/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include <unordered_map>

#include "defines.h"
#include "block.h"

#pragma once

namespace Charly {

  // Different masks for the flags field in the Basic struct
  static const VALUE kFlagType = 0b00011111;
  static const VALUE kFlagMark = 0b00100000;

  // Type identifiers
  static const uint8_t kTypeDead            = 0x00;
  static const uint8_t kTypeInteger         = 0x01;
  static const uint8_t kTypeFloat           = 0x02;
  static const uint8_t kTypeNumeric         = 0x03;
  static const uint8_t kTypeBoolean         = 0x04;
  static const uint8_t kTypeNull            = 0x05;
  static const uint8_t kTypeObject          = 0x06;
  static const uint8_t kTypeArray           = 0x07;
  static const uint8_t kTypeFunction        = 0x08;
  static const uint8_t kTypeCFunction       = 0x09;
  static const uint8_t kTypeSymbol          = 0x0a;

  // Machine internal types
  static const uint8_t kTypeFrame             = 0x0c;
  static const uint8_t kTypeCatchTable        = 0x0d;
  static const uint8_t kTypeInstructionBlock  = 0x0e;


  // Basic fields every data type in Charly has
  // This is inspired by the way Ruby stores it's values
  struct Basic {
    VALUE flags;
    Basic() : flags(kTypeDead) {}

    // Getters for different flag fields
    inline uint8_t type() { return this->flags & kFlagType; }
    inline bool mark() { return (this->flags & kFlagMark) != 0; }

    // Setters for different flag fields
    inline void set_type(uint8_t val) {
      this->flags = ((this->flags & ~kFlagType) | (kFlagType & val));
    }

    inline void set_mark(bool val) {
      this->flags ^= (-val ^ this->flags) & kFlagMark;
    }
  };

  // Memory that is allocated via the GC will be aligned to 8 bytes
  // This means that if VALUE is a pointer, the last 3 bits will be set to 0.
  // We can use this to our advantage to store some additional information
  // in there.
  static const uint8_t kPointerMask  = 0b00111;
  static const uint8_t kPointerFlag  = 0b00000;
  static const uint8_t kIntegerMask  = 0b00001;
  static const uint8_t kIntegerFlag  = 0b00001;
  static const uint8_t kFloatMask    = 0b00011;
  static const uint8_t kFloatFlag    = 0b00010;
  static const uint8_t kSymbolMask   = 0b01111;
  static const uint8_t kSymbolFlag   = 0b01100;
  static const uint8_t kFalse        = 0b00000;
  static const uint8_t kTrue         = 0b10100;
  static const uint8_t kNull         = 0b01000;

  inline bool is_boolean(VALUE value) { return value == kFalse || value == kTrue; }
  inline bool is_integer(VALUE value) { return (value & kIntegerMask) == kIntegerFlag; }
  inline bool is_ifloat(VALUE value)  { return (value & kFloatMask) == kFloatFlag; }
  inline bool is_symbol(VALUE value)  { return (value & kSymbolMask) == kSymbolFlag; }
  inline bool is_false(VALUE value)   { return value == kFalse; }
  inline bool is_true(VALUE value)    { return value == kTrue; }
  inline bool is_null(VALUE value)    { return value == kNull; }
  inline bool is_pointer(VALUE value) {
    return (
      !is_null(value) &&
      !is_false(value) &&
      ((value & kPointerMask) == kPointerFlag));
  }
  inline bool is_special(VALUE value) { return !is_pointer(value); }

  // Returns this value as a pointer to a Basic structure
  inline Basic* basics(VALUE value) { return (Basic *)value; }

  // Describes an object type
  //
  // It contains an unordered map which holds the objects values
  // The klass field is a VALUE containing the class the object was constructed from
  struct Object {
    Basic basic;
    VALUE klass;
    std::unordered_map<VALUE, VALUE>* container;
  };

  // Array type
  struct Array {
    Basic basic;
    std::vector<VALUE>* data;
  };

  // Heap-allocated float type
  //
  // Used when a floating-point value won't fit into the immediate-encoded format
  struct Float {
    Basic basic;
    double float_value;
  };

  // Normal functions defined inside the virtual machine.
  struct Function {
    Basic basic;
    VALUE name;
    uint32_t argc;
    Machine::Frame* context;
    Machine::InstructionBlock* block;
    bool anonymous;
    bool bound_self_set;
    VALUE bound_self;

    // TODO: Argumentlist and bound argumentlist
  };

  // Function type used for including external functions from C-Land into the virtual machine
  // These are basically just a function pointer with some metadata associated to them
  struct CFunction {
    Basic basic;
    VALUE name;
    void* pointer;
    uint32_t argc;
    bool bound_self_set;
    VALUE bound_self;

    // TODO: Argumentlist and bound argumentlist
  };

  // Rotate a given value to the left n times
  constexpr VALUE BIT_ROTL(VALUE v, VALUE n) {
    return (((v) << (n)) | ((v) >> ((sizeof(v) * 8) - n)));
  }

  // Rotate a given value to the right n times
  constexpr VALUE BIT_ROTR(VALUE v, VALUE n) {
    return (((v) >> (n)) | ((v) << ((sizeof(v) * 8) - n)));
  }
}
