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
#include <cmath>

#include "defines.h"
#include "common.h"

#pragma once

namespace Charly {

// Human readable types of all data types
const std::string kHumanReadableTypes[] = {
  "dead",
  "class",
  "object",
  "array",
  "string",
  "function",
  "cfunction",
  "frame",
  "catchtable",
  "number",
  "boolean",
  "null",
  "symbol",
  "unknown"
};

// Identifies which type a VALUE points to
enum ValueType : uint8_t {

  // Types which are allocated on the heap
  kTypeDead,
  kTypeClass,
  kTypeObject,
  kTypeArray,
  kTypeString,
  kTypeFunction,
  kTypeCFunction,
  kTypeFrame,
  kTypeCatchTable,

  // Types which are immediate encoded using nan-boxing
  kTypeNumber,
  kTypeBoolean,
  kTypeNull,
  kTypeSymbol,

  // This should never appear anywhere
  kTypeUnknown
};

// Every heap allocated structure in Charly contains this structure at
// the beginning. It allows us to determine it's type and other information
// about it.
struct Basic {
  // If the type of this object is String, this determines wether it's a short string
  bool shortstring_set = false;

  // Used by the Garbage Collector during the Mark & Sweep Cycle
  bool mark_set = 0;

  // Holds the type of the heap allocated struct
  uint8_t type : 5;

  Basic() : type(kTypeDead) {
  }
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
// Strings which are <= 62 bytes long, are stored inside the String structure itself. Most strings should fall
// into this category.
//
// If a string exceeds this limit, the string is allocated separately on the heap. The String structure
// now only stores a pointer and a length to the allocated memory
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
    return basic.shortstring_set ? sbuf.data : lbuf.data;
  }
  inline uint32_t length() {
    return basic.shortstring_set ? sbuf.length : lbuf.length;
  }
  inline void clean() {
    if (!basic.shortstring_set) {
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
// lower 48 bits for addresses, so we can store a full pointer in there.
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
const uint64_t kBitsNaN           = kMaskExponentBits | kMaskQuietBit;
const double kNaN                 = BITCAST_DOUBLE(kMaskExponentBits | kMaskQuietBit);
const uint64_t kFalse             = kBitsNaN | kITypeFalse;
const uint64_t kTrue              = kBitsNaN | kITypeTrue;
const uint64_t kNull              = kBitsNaN | kITypeNull;

// Signatures of complex encoded types
const uint64_t kSignaturePointer  = kMaskSignBit | kBitsNaN;
const uint64_t kSignatureInteger  = kBitsNaN | kITypeInteger;
const uint64_t kSignatureSymbol   = kBitsNaN | kITypeSymbol;
const uint64_t kSignaturePString  = kBitsNaN | kITypePString;
const uint64_t kSignatureIString  = kBitsNaN | kITypeIString;

// Masks for the immediate encoded types
const uint64_t kMaskPointer       = 0x0000ffffffffffff;
const uint64_t kMaskInteger       = 0x0000ffffffffffff;
const uint64_t kMaskIntegerSign   = 0x0000800000000000;
const uint64_t kMaskSymbol        = 0x0000ffffffffffff;
const uint64_t kMaskPString       = 0x0000ffffffffffff;
const uint64_t kMaskIString       = 0x000000ffffffffff;
const uint64_t kMaskIStringLength = 0x0000ff0000000000;

// Constants used when converting between different representations
const int64_t kMaxInt             = (static_cast<int64_t>(1) << 47) - 1;
const int64_t kMaxUInt            = (static_cast<int64_t>(1) << 48) - 1;
const int64_t kMinInt             = -(static_cast<int64_t>(1) << 47);
const void* kMaxPointer           = reinterpret_cast<void*>((static_cast<int64_t>(1) << 48) - 1);
const uint64_t kSignBlock         = 0xFFFF000000000000;

// Type casting functions

template <typename T>
T* charly_as_pointer_to(VALUE value)                  { return reinterpret_cast<T*>(value & kMaskPointer); }
inline void* charly_as_pointer(VALUE value)           { return charly_as_pointer_to<void>(value); }
inline Basic* charly_as_basic(VALUE value)            { return charly_as_pointer_to<Basic>(value); }
inline Class* charly_as_class(VALUE value)            { return charly_as_pointer_to<Class>(value); }
inline Object* charly_as_object(VALUE value)          { return charly_as_pointer_to<Object>(value); }
inline Array* charly_as_array(VALUE value)            { return charly_as_pointer_to<Array>(value); }
inline String* charly_as_hstring(VALUE value)         { return charly_as_pointer_to<String>(value); }
inline Function* charly_as_function(VALUE value)      { return charly_as_pointer_to<Function>(value); }
inline CFunction* charly_as_cfunction(VALUE value)    { return charly_as_pointer_to<CFunction>(value); }
inline Frame* charly_as_frame(VALUE value)            { return charly_as_pointer_to<Frame>(value); }
inline CatchTable* charly_as_catchtable(VALUE value)  { return charly_as_pointer_to<CatchTable>(value); }

// Type checking functions
inline bool charly_is_false(VALUE value)     { return value == kFalse; }
inline bool charly_is_true(VALUE value)      { return value == kTrue; }
inline bool charly_is_boolean(VALUE value)   { return charly_is_false(value) || charly_is_true(value); }
inline bool charly_is_null(VALUE value)      { return value == kNull; }
inline bool charly_is_nan(VALUE value)       { return value == kBitsNaN; }
inline bool charly_is_float(VALUE value)     { return charly_is_nan(value) || ((~value & kMaskExponentBits) != 0); }
inline bool charly_is_int(VALUE value)       { return (value & kMaskSignature) == kSignatureInteger; }
inline bool charly_is_number(VALUE value)    { return charly_is_int(value) || charly_is_float(value); }
inline bool charly_is_symbol(VALUE value)    { return (value & kMaskSignature) == kSignatureSymbol; }
inline bool charly_is_pstring(VALUE value)   { return (value & kMaskSignature) == kSignaturePString; }
inline bool charly_is_istring(VALUE value)   { return (value & kMaskSignature) == kSignatureIString; }
inline bool charly_is_ptr(VALUE value)       { return (value & kMaskSignature) == kSignaturePointer; }

// Heap allocated types
inline bool charly_is_on_heap(VALUE value) { return charly_is_ptr(value); }
inline bool charly_is_heap_type(VALUE value, uint8_t type) {
  return charly_is_on_heap(value) && charly_as_basic(value)->type == type;
}
inline bool charly_is_class(VALUE value) { return charly_is_heap_type(value, kTypeClass); }
inline bool charly_is_object(VALUE value) { return charly_is_heap_type(value, kTypeObject); }
inline bool charly_is_array(VALUE value) { return charly_is_heap_type(value, kTypeArray); }
inline bool charly_is_hstring(VALUE value) { return charly_is_heap_type(value, kTypeString); }
inline bool charly_is_string(VALUE value) {
  return charly_is_pstring(value) || charly_is_istring(value) || charly_is_hstring(value);
}
inline bool charly_is_function(VALUE value) { return charly_is_heap_type(value, kTypeFunction); }
inline bool charly_is_cfunction(VALUE value) { return charly_is_heap_type(value, kTypeCFunction); }
inline bool charly_is_callable(VALUE value) {
  return charly_is_function(value) || charly_is_cfunction(value) || charly_is_class(value);
}
inline bool charly_is_frame(VALUE value) { return charly_is_heap_type(value, kTypeFrame); }
inline bool charly_is_catchtable(VALUE value) { return charly_is_heap_type(value, kTypeCatchTable); }

// Return the ValueType representation of the type of the value
inline uint8_t charly_get_type(VALUE value) {
  if (charly_is_on_heap(value)) return charly_as_basic(value)->type;
  if (charly_is_float(value)) return kTypeNumber;
  if (charly_is_int(value)) return kTypeNumber;
  if (charly_is_null(value)) return kTypeNull;
  if (charly_is_pstring(value) || charly_is_istring(value)) return kTypeString;
  if (charly_is_boolean(value)) return kTypeBoolean;
  if (charly_is_symbol(value)) return kTypeSymbol;
  return kTypeUnknown;
}

// Return a human readable string of the type of value
inline const std::string& charly_get_typestring(VALUE value) {
  return kHumanReadableTypes[charly_get_type(value)];
}

// Convert an immediate integer to other integer or float types
//
// Warning: These methods don't perform any type checks and assume
// the caller made sure that the input value is an immediate integer
//
// Because we only use 48 bits to store an integer, the sign bit is stored at the 47th bit.
// When converting, we need to sign extend the value to retain correctness

inline int64_t charly_int_to_int64(VALUE value) {
  return (value & kMaskInteger) | ((value & kMaskIntegerSign) ? kSignBlock : 0x00);
}
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

// The purpose of this method is to replace INFINITY, -INFINITY, NAN with 0
// Conversion from these values to int is undefined behaviour and will result
// in random gibberish being returned
inline double charly_double_to_safe_double(VALUE value) { return FP_STRIP_INF(FP_STRIP_NAN(BITCAST_DOUBLE(value))); }
inline int64_t charly_double_to_int64(VALUE value)      { return charly_double_to_safe_double(value); }
inline uint64_t charly_double_to_uint64(VALUE value)    { return charly_double_to_safe_double(value); }
inline int32_t charly_double_to_int32(VALUE value)      { return charly_double_to_safe_double(value); }
inline uint32_t charly_double_to_uint32(VALUE value)    { return charly_double_to_safe_double(value); }
inline int16_t charly_double_to_int16(VALUE value)      { return charly_double_to_safe_double(value); }
inline uint16_t charly_double_to_uint16(VALUE value)    { return charly_double_to_safe_double(value); }
inline int8_t charly_double_to_int8(VALUE value)        { return charly_double_to_safe_double(value); }
inline uint8_t charly_double_to_uint8(VALUE value)      { return charly_double_to_safe_double(value); }
inline float charly_double_to_float(VALUE value)        { return BITCAST_DOUBLE(value); }
inline double charly_double_to_double(VALUE value)      { return BITCAST_DOUBLE(value); }

// Convert an immediate number to other integer or float types
//
// Note: Assumes the caller doesn't know what exact number type the value has,
// only that it is a number.
inline int64_t charly_number_to_int64(VALUE value)   {
  if (charly_is_float(value)) return charly_double_to_int64(value);
  return charly_int_to_int64(value);
}
inline uint64_t charly_number_to_uint64(VALUE value) {
  if (charly_is_float(value)) return charly_double_to_uint64(value);
  return charly_int_to_uint64(value);
}
inline int32_t charly_number_to_int32(VALUE value)   {
  if (charly_is_float(value)) return charly_double_to_int32(value);
  return charly_int_to_int32(value);
}
inline uint32_t charly_number_to_uint32(VALUE value) {
  if (charly_is_float(value)) return charly_double_to_uint32(value);
  return charly_int_to_uint32(value);
}
inline int16_t charly_number_to_int16(VALUE value)   {
  if (charly_is_float(value)) return charly_double_to_int16(value);
  return charly_int_to_int16(value);
}
inline uint16_t charly_number_to_uint16(VALUE value) {
  if (charly_is_float(value)) return charly_double_to_uint16(value);
  return charly_int_to_uint16(value);
}
inline int8_t charly_number_to_int8(VALUE value)     {
  if (charly_is_float(value)) return charly_double_to_int8(value);
  return charly_int_to_int8(value);
}
inline uint8_t charly_number_to_uint8(VALUE value)   {
  if (charly_is_float(value)) return charly_double_to_uint8(value);
  return charly_int_to_uint8(value);
}
inline float charly_number_to_float(VALUE value)     {
  if (charly_is_float(value)) return charly_double_to_float(value);
  return charly_int_to_float(value);
}
inline double charly_number_to_double(VALUE value)   {
  if (charly_is_float(value)) return charly_double_to_double(value);
  return charly_int_to_double(value);
}

// Get a pointer to the data of a string
// Returns a nullptr if value is not a string
//
// Because charly_string_data has to return a pointer to a char buffer
// We can't take the value argument by value, as that version of the value
// is being destroyed once the function exits.
inline char* charly_string_data(VALUE& value) {

  // If this machine is little endian, the buffer is already conventiently layed out at the
  // beginning of the value
  if (!IS_BIG_ENDIAN()) {
    return reinterpret_cast<char*>(&value);
  } else {
    if (charly_is_pstring(value)) {
      return reinterpret_cast<char*>(&value) + 2;
    } else if (charly_is_istring(value)) {
      return reinterpret_cast<char*>(&value) + 3;
    }
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
    if (IS_BIG_ENDIAN()) {
      return *(reinterpret_cast<uint8_t*>(&value) + 2);
    } else {
      return *(reinterpret_cast<uint8_t*>(&value) + 5);
    }
  }

  if (charly_is_hstring(value)) {
    return charly_as_hstring(value)->length();
  }

  return 0xFFFFFFFF;
}

// Convert a string to a int64
inline int64_t charly_string_to_int64(VALUE value) {
  size_t length = charly_string_length(value);
  char* buffer = charly_string_data(value);
  char* buffer_end = buffer + length;

  int64_t interpreted = strtol(buffer, &buffer_end, 0);

  // strtol sets errno to ERANGE if the result value doesn't fit
  // into the return type
  if (errno == ERANGE) {
    errno = 0;
    return 0;
  }

  if (buffer_end == buffer) {
    return 0;
  }

  return interpreted;
}

// Convert a string to a double
inline double charly_string_to_double(VALUE value) {
  size_t length = charly_string_length(value);
  char* buffer = charly_string_data(value);
  char* buffer_end = buffer + length;

  double interpreted = strtod(buffer, &buffer_end);

  // HUGE_VAL gets returned when the converted value
  // doesn't fit inside a double value
  // In this case we just return NAN
  if (interpreted == HUGE_VAL) {
    return kNaN;
  }

  // If strtod could not perform a conversion it returns 0
  // and sets str_end to str
  // We just return NAN in this case
  if (buffer_end == buffer) {
    return kNaN;
  }

  return interpreted;
}

// Create an immediate integer
//
// Warning: Doesn't perform any overflow checks. If the integer doesn't fit into 48 bits
// the value is going to be truncated.
template <typename T>
VALUE charly_create_integer(T value) {
  return kSignatureInteger | (value & kMaskInteger);
}

// Create an immediate double
inline VALUE charly_create_double(double value) {
  int64_t bits = *reinterpret_cast<int64_t*>(&value);

  // Strip sign bit and payload bits if value is NaN
  if ((bits & kMaskExponentBits) == kMaskExponentBits) return kBitsNaN;
  return *reinterpret_cast<VALUE*>(&value);
}

// Convert any type of value to a number type
// Floats stay floats
// Integers stay integers
// Any other type is converter to an integer
inline VALUE charly_value_to_number(VALUE value) {
  if (charly_is_float(value)) return value;
  if (charly_is_int(value)) return value;
  if (charly_is_boolean(value)) return value == kTrue ? charly_create_integer(1) : charly_create_integer(0);
  if (charly_is_null(value)) return charly_create_integer(0);
  if (charly_is_symbol(value)) return charly_create_integer(0);
  if (charly_is_string(value)) return charly_string_to_double(value);
  return charly_create_double(kNaN);
}
inline int64_t charly_value_to_int64(VALUE value) {
  if (charly_is_number(value)) return charly_number_to_int64(value);
  if (charly_is_boolean(value)) return value == kTrue ? 1 : 0;
  if (charly_is_null(value)) return 0;
  if (charly_is_symbol(value)) return 0;
  if (charly_is_string(value)) return charly_string_to_int64(value);
  return 0;
}
inline double charly_value_to_double(VALUE value) {
  if (charly_is_number(value)) return charly_number_to_double(value);
  if (charly_is_boolean(value)) return value == kTrue ? 1 : 0;
  if (charly_is_null(value)) return 0;
  if (charly_is_symbol(value)) return 0;
  if (charly_is_string(value)) return charly_string_to_double(value);
  return 0;
}
inline uint64_t charly_value_to_uint64(VALUE value) { return charly_value_to_int64(value); }
inline int32_t charly_value_to_int32(VALUE value)   { return charly_value_to_int64(value); }
inline uint32_t charly_value_to_uint32(VALUE value) { return charly_value_to_int64(value); }
inline int16_t charly_value_to_int16(VALUE value)   { return charly_value_to_int64(value); }
inline uint16_t charly_value_to_uint16(VALUE value) { return charly_value_to_int64(value); }
inline int8_t charly_value_to_int8(VALUE value)     { return charly_value_to_int64(value); }
inline uint8_t charly_value_to_uint8(VALUE value)   { return charly_value_to_int64(value); }
inline float charly_value_to_float(VALUE value)     { return charly_value_to_double(value); }

// Convert a number type into an immediate charly value
//
// This method assumes the caller doesn't care what format the resulting number has,
// so it might return an immediate integer or double
inline VALUE charly_create_number(int64_t value) {
  if (value >= kMaxInt || value <= kMinInt) return charly_create_double(value);
  return charly_create_integer(value);
}
inline VALUE charly_create_number(uint64_t value) {
  if (value >= kMaxUInt) return charly_create_double(value);
  return charly_create_integer(value);
}
inline VALUE charly_create_number(double value) { return charly_create_double(value); }

// Binary arithmetic methods
inline VALUE charly_add_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) + charly_int_to_int64(right));
  }
  return charly_create_number(charly_number_to_double(left) + charly_number_to_double(right));
}
inline VALUE charly_sub_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) - charly_int_to_int64(right));
  }
  return charly_create_number(charly_number_to_double(left) - charly_number_to_double(right));
}
inline VALUE charly_mul_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) * charly_int_to_int64(right));
  }
  return charly_create_number(charly_number_to_double(left) * charly_number_to_double(right));
}
inline VALUE charly_div_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) / charly_int_to_int64(right));
  }
  return charly_create_number(charly_number_to_double(left) / charly_number_to_double(right));
}
inline VALUE charly_mod_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) % charly_int_to_int64(right));
  }
  return charly_create_number(std::fmod(charly_number_to_double(left), charly_number_to_double(right)));
}
inline VALUE charly_pow_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(std::pow(charly_int_to_int64(left), charly_int_to_int64(right)));
  }
  return charly_create_number(std::pow(charly_number_to_double(left), charly_number_to_double(right)));
}
inline VALUE charly_lt_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) < charly_int_to_int64(right));
  }
  return charly_create_number(std::isless(charly_number_to_double(left), charly_number_to_double(right)));
}
inline VALUE charly_gt_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) > charly_int_to_int64(right));
  }
  return charly_create_number(std::isgreater(charly_number_to_double(left), charly_number_to_double(right)));
}
inline VALUE charly_le_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) <= charly_int_to_int64(right));
  }
  return charly_create_number(std::islessequal(charly_number_to_double(left), charly_number_to_double(right)));
}
inline VALUE charly_ge_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) >= charly_int_to_int64(right));
  }
  return charly_create_number(std::isgreaterequal(charly_number_to_double(left), charly_number_to_double(right)));
}
inline VALUE charly_eq_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) == charly_int_to_int64(right));
  }
  return charly_create_number(FP_ARE_EQUAL(charly_number_to_double(left), charly_number_to_double(right)));
}
inline VALUE charly_neq_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) != charly_int_to_int64(right));
  }
  return charly_create_number(!FP_ARE_EQUAL(charly_number_to_double(left), charly_number_to_double(right)));
}
inline VALUE charly_shl_number(VALUE left, VALUE right) {
  int32_t num = charly_number_to_int32(left);
  int32_t amount = charly_number_to_int32(right);
  return charly_create_number(num << amount);
}
inline VALUE charly_shr_number(VALUE left, VALUE right) {
  int32_t num = charly_number_to_int32(left);
  int32_t amount = charly_number_to_int32(right);
  return charly_create_number(num >> amount);
}
inline VALUE charly_and_number(VALUE left, VALUE right) {
  int32_t num = charly_number_to_int32(left);
  int32_t amount = charly_number_to_int32(right);
  return charly_create_number(num & amount);
}
inline VALUE charly_or_number(VALUE left, VALUE right) {
  int32_t num = charly_number_to_int32(left);
  int32_t amount = charly_number_to_int32(right);
  return charly_create_number(num | amount);
}
inline VALUE charly_xor_number(VALUE left, VALUE right) {
  int32_t num = charly_number_to_int32(left);
  int32_t amount = charly_number_to_int32(right);
  return charly_create_number(num ^ amount);
}

// Unary arithmetic methods
inline VALUE charly_uadd_number(VALUE value) {
  return value;
}
inline VALUE charly_usub_number(VALUE value) {
  if (charly_is_int(value)) return charly_create_number(-charly_int_to_int64(value));
  return charly_create_double(-charly_double_to_double(value));
}
inline VALUE charly_unot_number(VALUE value) {
  if (charly_is_int(value)) return charly_int_to_int8(value) == 0 ? kTrue : kFalse;
  return charly_double_to_double(value) == 0.0 ? kTrue : kFalse;
}
inline VALUE charly_ubnot_number(VALUE value) {
  if (charly_is_int(value)) return charly_create_number(~charly_int_to_int32(value));
  return charly_create_integer(~charly_double_to_int32(value));
}

// Convert types into symbols
inline VALUE charly_create_symbol(const std::string& input) {
  size_t val = std::hash<std::string>{}(input);
  return kSignatureSymbol | (val & kMaskSymbol);
}

// Create immediate encoded strings of size 0 - 5
//
// Note: Because char* should always contain a null terminator at the end, we check for <= 6 bytes
// instead of <= 5.
template <size_t N>
VALUE charly_create_istring(char const (& input)[N]) {
  static_assert(N <= 6, "charly_create_istring can only create strings of length <= 5 (excluding null-terminator)");

  VALUE val = kSignatureIString;
  char* buf = (char*)&val;

  // Copy the string buffer
  if (IS_BIG_ENDIAN()) {
    if constexpr (N >= 1) buf[3] = input[0];
    if constexpr (N >= 2) buf[4] = input[1];
    if constexpr (N >= 3) buf[5] = input[2];
    if constexpr (N >= 4) buf[6] = input[3];
    if constexpr (N >= 5) buf[7] = input[4];
    buf[2] = N - 1;
  } else {
    if constexpr (N >= 1) buf[0] = input[0];
    if constexpr (N >= 2) buf[1] = input[1];
    if constexpr (N >= 3) buf[2] = input[2];
    if constexpr (N >= 4) buf[3] = input[3];
    if constexpr (N >= 5) buf[4] = input[4];
    buf[5] = N - 1;
  }

  return val;
}

template <>
VALUE charly_create_istring(char const (& input)[7]) {
  VALUE val = kSignaturePString;
  char* buf = (char*)&val;

  // Copy the string buffer
  if (IS_BIG_ENDIAN()) {
    buf[2] = input[0];
    buf[3] = input[1];
    buf[4] = input[2];
    buf[5] = input[3];
    buf[6] = input[4];
    buf[7] = input[5];
  } else {
    buf[0] = input[0];
    buf[1] = input[1];
    buf[2] = input[2];
    buf[3] = input[3];
    buf[4] = input[4];
    buf[5] = input[5];
  }

  return val;
}

// Create a VALUE from a ptr
inline VALUE charly_create_pointer(void* ptr) {
  if (ptr > kMaxPointer) return kSignaturePointer; // null pointer
  return kSignaturePointer | (~kSignaturePointer & reinterpret_cast<int64_t>(ptr));
}

// clang-format on

}  // namespace Charly
