/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2018 Leonard Schütz
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
#include <vector>

#include "defines.h"

#pragma once

namespace Charly {

// Identifies which type a VALUE points to
// Types which are not allocated on the heap are also included here
// as these values are also used in type comparison functions
//
// Some machine internal types such as the call frame structure are
// also allocated on the heap, which means they also get a type id
enum {

  // Types which are stored explicitly on the heap
  kTypeDead,
  kTypeClass,
  kTypeObject,
  kTypeArray,
  kTypeString,
  kTypeShortString,
  kTypeFunction,
  kTypeCFunction,
  kTypeFrame,
  kTypeCatchTable,

  // Types which are represented implicitly (e.g. through NaN boxing)
  kTypeInteger,
  kTypeFloat,
  kTypeBoolean,
  kTypeNull,
  kTypeSymbol,
  kTypeImmediateString,

  // Types which represent a set of other types
  // These exist to make type comparisons less tedious to write
  kTypeNumeric,
};

// String representations of the value types
// These are used in dumps and debugging output
const std::string kValueTypeString[] = {
  "dead", "string", "object", "array", "function", "cfunction", "class", "frame",
  "catchtable", "integer", "float", "boolean", "null", "symbol", "numeric"
};

// Every heap allocated structure in Charly contains this structure at
// the beginning. It allows us to determine it's type and other information
// about it.
struct Basic {

  // If the type of this object is String, this determines wether it's a short string
  uint8_t shortstring_set : 1 = 0;

  // Used by the Garbage Collector during the Mark & Sweep Cycle
  uint8_t mark_set : 1 = =;

  // Holds the type of the heap allocated struct
  uint8_t type : 5 = kTypeDead;
};

// Describes an object type
//
// It contains an unordered map which holds the objects values
// The klass field is a VALUE containing the class the object was constructed from
struct Object {
  Basic basic;
  VALUE klass;
  std::unordered_map<VALUE, VALUE>* container;

  void inline clean() {
    delete this->container;
  }
};

// Array type
struct Array {
  Basic basic;
  std::vector<VALUE>* data;

  void inline clean() {
    delete this->data;
  }
};

// String type
//
// A field inside the Basic structure tells the VM which representation is currently active
// +- Basic field
// |
// v
// 0x00 0x00 0x00 0x00 0x00
//      ^                 ^
//      |                 |
//      +-----------------+- Long string length
//      |
//      +- Short string length
static const uint32_t kShortStringMaxSize = 62;
struct String {
  Basic basic;

  union {
    struct {
      uint32_t length;
      char* data;
    } lbuf;
    struct {
      uint8_t length;
      char data[kShortStringMaxSize];
    } sbuf;
  };

  inline char* data() {
    return basic.short_string() ? sbuf.data : lbuf.data;
  }
  inline uint32_t length() {
    return basic.short_string() ? sbuf.length : lbuf.length;
  }
  inline void clean() {
    if (!basic.short_string()) {
      std::free(lbuf.data);
    }
  }
};

// Frames introduce new environments
struct Frame {
  Basic basic;
  Frame* parent;
  Frame* parent_environment_frame;
  CatchTable* last_active_catchtable;
  Function* function;
  std::vector<VALUE>* environment;
  VALUE self;
  uint8_t* return_address;
  bool halt_after_return;
};

// Catchtable used for exception handling
struct CatchTable {
  Basic basic;
  uint8_t* address;
  size_t stacksize;
  Frame* frame;
  CatchTable* parent;
};

// Normal functions defined inside the virtual machine.
struct Function {
  Basic basic;
  VALUE name;
  uint32_t argc;
  uint32_t lvarcount;
  Frame* context;
  uint8_t* body_address;
  bool anonymous;
  bool bound_self_set;
  VALUE bound_self;
  std::unordered_map<VALUE, VALUE>* container;

  inline void clean() {
    delete this->container;
  }

  // TODO: Bound argumentlist
};

// Function type used for including external functions from C-Land into the virtual machine
// These are basically just a function pointer with some metadata associated to them
struct CFunction {
  Basic basic;
  VALUE name;
  uintptr_t pointer;
  uint32_t argc;
  bool bound_self_set;
  VALUE bound_self;
  std::unordered_map<VALUE, VALUE>* container;

  inline void clean() {
    delete this->container;
  }

  // TODO: Bound argumentlist
};

// Classes defined inside the virtual machine
struct Class {
  Basic basic;
  VALUE name;
  VALUE constructor;
  std::vector<VALUE>* member_properties;
  VALUE prototype;
  VALUE parent_class;
  std::unordered_map<VALUE, VALUE>* container;

  inline void clean() {
    delete this->member_properties;
    delete this->container;
  }
};

// clang-format off

// An IEEE 754 double-precision float is a regular 64-bit value. The bits are laid out as follows:
//
// 1 Sign bit
// | 11 Exponent bits
// | |            52 Mantissa bits
// v v            v
// S[Exponent---][Mantissa--------------------------------------------]
//
// The exact details of how these parts store a float value is not important here, we just
// have to ensure not to mess with them if they represent a valid value.
//
// The IEEE 754 standard defines a way to encode NaN (not a number) values.
// A NaN is any value where all exponent bits are set:
//
//  +- If these bits are set, it's a NaN value
//  v
// -11111111111----------------------------------------------------
//
// NaN values come in two variants: "signalling" and "quiet". The former is
// intended to cause an exception, while the latter silently flows through any
// arithmetic operation.
//
// A quiet NaN is indicated by setting the highest mantissa bit:
//
//               +- This bit signals a quiet NaN
//               v
// -[NaN        ]1---------------------------------------------------
//
// This gives us 52 bits to play with. Even 64-bit machines only use the
// lower 48 bits for addresses, so we can store full pointer in there.
// In the future this might change but that's a discussion for another time.
//
// +- If set, denotes an encoded pointer
// |              + Stores the type id of the encoded value
// |              | These are only useful if the encoded value is not a pointer
// v              v
// S[NaN        ]1TTT------------------------------------------------
//
// The type bits map to the following values
// 000: NaN
// 001: false
// 010: true
// 011: null
// 100: integers
// 101: symbols:
// 110: string (full)
// 111: string (most significant payload byte stores the length)
//
// Note: Documentation for this section of the code was inspired by:
//       https://github.com/munificent/wren/blob/master/src/vm/wren_value.h

// Masks for the VALUE type
const uint64_t kMaskSignBit       = 0x8000000000000000; // Sign bit
const uint64_t kMaskExponentBits  = 0x7ff0000000000000; // Exponent bits
const uint64_t kMaskQuietBit      = 0x0008000000000000; // Quiet bit
const uint64_t kMaskTypeBits      = 0x0007000000000000; // Type bits
const uint64_t kMaskSignature     = 0xffff000000000000; // Signature bits
const uint64_t kMaskPayloadBits   = 0x0000ffffffffffff; // Payload bits

// Types that are encoded in the type field
const uint64_t kITypeNaN          = 0x0000000000000000;
const uint64_t kITypeFalse        = 0x0001000000000000;
const uint64_t kITypeTrue         = 0x0002000000000000;
const uint64_t kITypeNull         = 0x0003000000000000;
const uint64_t kITypeInteger      = 0x0004000000000000;
const uint64_t kITypeSymbol       = 0x0005000000000000;
const uint64_t kITypePString      = 0x0006000000000000;
const uint64_t kITypeIString      = 0x0007000000000000;

// Shorthand values
const uint64_t kNaN               = kMaskSignBit | kMaskExponentBits | kMaskQuietBit;
const uint64_t kFalse             = kNaN | kITypeFalse;
const uint64_t kTrue              = kNaN | kITypeTrue;
const uint64_t kNull              = kNaN | kITypeNull;

// Signatures of complex encoded types
const uint64_t kSignaturePointer  = kMaskSignBit | kNaN;
const uint64_t kSignatureInteger  = kNaN | kITypeInteger;
const uint64_t kSignatureSymbol   = kNaN | kITypeSymbol;
const uint64_t kSignaturePString  = kNaN | kITypePString;
const uint64_t kSignatureIString  = kNaN | kITypeIString;

// Masks for the immediate encoded types
const uint64_t kMaskPointer       = 0x0000ffffffffffff;
const uint64_t kMaskInteger       = 0x0000ffffffffffff;
const uint64_t kMaskIntegerSign   = 0x0000800000000000;
const uint64_t kMaskSymbol        = 0x0000ffffffffffff;
const uint64_t kMaskPString       = 0x0000ffffffffffff;
const uint64_t kMaskIString       = 0x000000ffffffffff;
const uint64_t kMaskIStringLength = 0x0000ff0000000000;

// Offsets to positions inside the payload section
const size_t kOffsetPString       = 0x02;
const size_t kOffsetIStringLength = 0x02;
const size_t kOffsetIString       = 0x03;

// Constants used when converting between different representations
const uint64_t kSignBlock         = 0xFFFF000000000000;

// Type casting functions
inline void* charly_as_pointer(VALUE value)           { return reinterpret_cast<void*>(value & kMaskPointer); }
inline Basic* charly_as_basic(VALUE value)            { return reinterpret_cast<Basic*>(value & kMaskPointer); }
inline Class* charly_as_class(VALUE value)            { return reinterpret_cast<Class*>(value & kMaskPointer); }
inline Object* charly_as_object(VALUE value)          { return reinterpret_cast<Object*>(value & kMaskPointer); }
inline Array* charly_as_array(VALUE value)            { return reinterpret_cast<Array*>(value & kMaskPointer); }
inline String* charly_as_hstring(VALUE value)         { return reinterpret_cast<String*>(value & kMaskPointer); }
inline Function* charly_as_function(VALUE value)      { return reinterpret_cast<Function*>(value & kMaskPointer); }
inline CFunction* charly_as_cfunction(VALUE value)    { return reinterpret_cast<CFunction*>(value & kMaskPointer); }
inline Frame* charly_as_frame(VALUE value)            { return reinterpret_cast<Frame*>(value & kMaskPointer); }
inline CatchTable* charly_as_catchtable(VALUE value)  { return reinterpret_cast<CatchTable*>(value & kMaskPointer); }

// Type checking functions
inline bool charly_is_false(VALUE value)     { return value == kFalse; }
inline bool charly_is_true(VALUE value)      { return value == kTrue; }
inline bool charly_is_boolean(VALUE value)   { return charly_is_false(value) || charly_is_true(value); }
inline bool charly_is_null(VALUE value)      { return value == kNull; }
inline bool charly_is_float(VALUE value)     { return (value & kMaskExponentBits) != kMaskExponentBits; }
inline bool charly_is_int(VALUE value)       { return (value & kMaskSignature) == kSignatureInteger; }
inline bool charly_is_numeric(VALUE value)   { return charly_is_int(value) || charly_is_float(value); }
inline bool charly_is_symbol(VALUE value)    { return (value & kMaskSignature) == kSignatureSymbol; }
inline bool charly_is_pstring(VALUE value)   { return (value & kMaskSignature) == kSignaturePString; }
inline bool charly_is_istring(VALUE value)   { return (value & kMaskSignature) == kSignatureString; }
inline bool charly_is_ptr(VALUE value)       { return (value & kMaskSignature) == kSignaturePointer; }

// Heap allocated types
inline bool charly_is_type(VALUE value, uint8_t type) { return charly_is_ptr(value) && as_basic(value)->type == type; }
inline bool charly_is_class(VALUE value) { return charly_is_type(value, kTypeClass); }
inline bool charly_is_object(VALUE value) { return charly_is_type(value, kTypeObject); }
inline bool charly_is_array(VALUE value) { return charly_is_type(value, kTypeArray); }
inline bool charly_is_hstring(VALUE value) { return charly_is_type(value, kTypeString); }
inline bool charly_is_string(VALUE value) {
  return charly_is_pstring(value) || charly_is_istring(value) || charly_is_hstring(value);
}
inline bool charly_is_function(VALUE value) { return charly_is_type(value, kTypeFunction); }
inline bool charly_is_cfunction(VALUE value) { return charly_is_type(value, kTypeCFunction); }
inline bool charly_is_frame(VALUE value) { return charly_is_type(value, kTypeFrame); }
inline bool charly_is_catchtable(VALUE value) { return charly_is_type(value, kTypeCatchTable); }

// Convert an immediate integer to other integer or float types
//
// Warning: These methods don't perform any type checks and assume
// the caller made sure that the input value is an immediate integer
//
// Because we only use 48 bits to store an integer, the sign bit is stored at the 47th bit.
// When converting, we need to sign extend the value to retain correctness

inline int64_t charly_int_to_int64(VALUE value) {
  return (value & kMaskInteger) | ((value & kMaskSignBit) ? kSignBlock : 0x00);
}
inline int64_t charly_int_to_int64(VALUE value)   { return charly_int_to_int64(value); }
inline uint64_t charly_int_to_uint64(VALUE value) { return charly_int_to_int64(value); }
inline int32_t charly_int_to_int32(VALUE value)   { return charly_int_to_int64(value); }
inline uint32_t charly_int_to_uint32(VALUE value) { return charly_int_to_int64(value); }
inline int16_t charly_int_to_int16(VALUE value)   { return charly_int_to_int64(value); }
inline uint16_t charly_int_to_uint16(VALUE value) { return charly_int_to_int64(value); }
inline int8_t charly_int_to_int8(VALUE value)     { return charly_int_to_int64(value); }
inline uint8_t charly_int_to_uint8(VALUE value)   { return charly_int_to_int64(value); }
inline float charly_int_to_float(VALUE value)     { return charly_int_to_int64(value); }
inline double charly_int_to_double(VALUE value)   { return charly_int_to_int64(value); }

// Convert an immediate double to other integer or float types
//
// Warning: These methods don't perform any type checks and assume
// the caller made sure that the input value is an immediate double
inline int64_t charly_double_to_int64(VALUE value)   { return value; }
inline uint64_t charly_double_to_uint64(VALUE value) { return value; }
inline int32_t charly_double_to_int32(VALUE value)   { return value; }
inline uint32_t charly_double_to_uint32(VALUE value) { return value; }
inline int16_t charly_double_to_int16(VALUE value)   { return value; }
inline uint16_t charly_double_to_uint16(VALUE value) { return value; }
inline int8_t charly_double_to_int8(VALUE value)     { return value; }
inline uint8_t charly_double_to_uint8(VALUE value)   { return value; }
inline float charly_double_to_float(VALUE value)     { return value; }
inline double charly_double_to_double(VALUE value)   { return value; }

// Convert an immediate number to other integer or float types
//
// Note: Assumes the caller doesn't know what exact numeric type the value has,
// only that it is a number.
//
// Note: Methods which return an integer, return 0 if the value is not a number
// Note: Methods which return a float, return NaN if the value is not a number

// Get a pointer to the data of a string
// Returns a nullptr if value is not a string
inline char* charly_string_data(VALUE value) {
  if (charly_is_pstring(value)) {
    return static_cast<char*>(charly_as_pointer(value) + kOffsetPString);
  }

  if (charly_is_istring(value)) {
    return static_cast<char*>(charly_as_pointer(value) + kOffsetIString);
  }

  if (charly_is_hstring(value)) {
    return charly_as_hstring(value)->data();
  }

  return nullptr;
}

// Get the length of a string
// Returns -1 (0xffffffff) if value is not a string
inline uint32_t charly_string_length(VALUE value) {
  if (charly_is_pstring(value)) {
    return 6;
  }

  if (charly_is_istring(value)) {
    return *reinterpret_cast<uint8_t*>(charly_as_pointer(value) + kOffsetIStringLength);
  }

  if (charly_is_hstring(value)) {
    return charly_as_hstring(value)->length();
  }

  return 0xFFFFFFFF;
}

// Create an immediate integer
//
// Warning: Doesn't perform any overflow checks. If the integer doesn't fit into 48 bits
// the value is going to be truncated.
template <typename T>
inline VALUE charly_create_integer(T value) {
  return kSignatureInteger | (static_cast<int64_t>(value) & ~kMaskInteger);
}

// Create an immediate double
template <typename T>
inline VALUE charly_create_double(T value) {
  return static_cast<double>(value);
}

// Convert a numeric type into an immediate charly value
//
// This method assumes the caller doesn't care what format the resulting number has,
// so it might return an immediate integer or double
inline VALUE charly_create_number(int64_t value) {
  if (reinterpret_cast<uint64_t>(value) >= kMaskInteger) return charly_create_double(value);
  return charly_create_integer(value);
}
inline VALUE charly_create_number(uint64_t value) {
  if (reinterpret_cast<uint64_t>(value) >= kMaskInteger) return charly_create_double(value);
  return charly_create_integer(value);
}
inline VALUE charly_create_number(int8_t value)   { return charly_create_integer(value); }
inline VALUE charly_create_number(uint8_t value)  { return charly_create_integer(value); }
inline VALUE charly_create_number(int16_t value)  { return charly_create_integer(value); }
inline VALUE charly_create_number(uint16_t value) { return charly_create_integer(value); }
inline VALUE charly_create_number(int32_t value)  { return charly_create_integer(value); }
inline VALUE charly_create_number(uint32_t value) { return charly_create_integer(value); }
inline VALUE charly_create_number(double value)   { return charly_create_double(value); }
inline VALUE charly_create_number(float value)    { return charly_create_double(value); }

// Convert types into symbols
template <typename T>
inline constexpr VALUE charly_create_symbol(T& input) {
  size_t val = std::hash<T>{}(input);
  return kSignatureSymbol | (val & ~kMaskSymbol);
}

// clang-format on

}  // namespace Charly
