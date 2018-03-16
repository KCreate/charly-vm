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

#include <cmath>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <utf8/utf8.h>

#include "common.h"
#include "defines.h"

#pragma once

namespace Charly {

// Human readable types of all data types
const std::string kHumanReadableTypes[] = {"dead",     "class",     "object",    "array",  "string",
                                           "function", "cfunction", "generator", "frame",  "catchtable",
                                           "number",   "boolean",   "null",      "symbol", "unknown"};

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
  kTypeGenerator,
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
  // Unreserved flags which can be used by the datatypes for anything they want
  // This is mostly an optimisation so that data types don't need to allocate
  // a byte for their own flags
  union {
    struct {
      bool f1 : 1;
      bool f2 : 1;
    };
    uint8_t flags : 2;
  };

  // Used by the Garbage Collector during the Mark & Sweep Cycle
  bool mark : 1;

  // Holds the type of the heap allocated struct
  uint8_t type : 5;

  Basic() : f1(false), f2(false), mark(false), type(kTypeDead) {
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

  inline void clean() {
    delete this->container;
  }
};

// Array type
struct Array {
  Basic basic;
  std::vector<VALUE>* data;

  inline void clean() {
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
//
// Uses the f1 flag of the basic structure to differentiate between short and heap strings
static constexpr uint32_t kShortStringMaxSize = 118;
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
    return basic.f1 ? sbuf.data : lbuf.data;
  }
  inline uint32_t length() {
    return basic.f1 ? sbuf.length : lbuf.length;
  }
  inline void set_shortstring(bool f) {
    this->basic.f1 = f;
  }
  inline bool is_shortstring() {
    return basic.f1;
  }
  inline void clean() {
    if (!basic.f1) {
      std::free(lbuf.data);
    }
  }
};

// Frames introduce new environments
//
// Uses the f1 flag of the basic structure to differentiate between small and regular frames
// Uses the f2 flag of the basic structure to store the machine should halt after this frame
static constexpr uint32_t kSmallFrameLocalCount = 6;
struct Frame {
  Basic basic;
  Frame* parent;
  Frame* parent_environment_frame;
  CatchTable* last_active_catchtable;
  VALUE caller_value;
  uint32_t stacksize_at_entry;
  union {
    std::vector<VALUE>* lenv;
    struct {
      VALUE data[kSmallFrameLocalCount];
      uint8_t lvarcount;
    } senv;
  };
  VALUE self;
  uint8_t* return_address;

  inline bool halt_after_return() {
    return this->basic.f2;
  }

  inline void set_halt_after_return(bool f) {
    this->basic.f2 = f;
  }

  // Read the local variable at a given index
  //
  // This method performs no overflow checks
  inline VALUE read_local(uint32_t index) {
    if (this->basic.f1) {
      return this->senv.data[index];
    } else {
      return (* this->lenv)[index];
    }
  }

  // Set the local variable at a given index
  //
  // This method performs no overflow checks
  inline void write_local(uint32_t index, VALUE value) {
    if (this->basic.f1) {
      this->senv.data[index] = value;
    } else {
      (* this->lenv)[index] = value;
    }
  }

  // Returns the amount of local variables this frame currently holds
  inline size_t lvarcount() {
    if (this->basic.f1) {
      return this->senv.lvarcount;
    } else {
      if (this->lenv) return this->lenv->size();
      return 0;
    }
  }

  inline bool is_smallframe() {
    return this->basic.f1;
  }

  inline void set_smallframe(bool f) {
    this->basic.f1 = f;
  }

  inline void clean() {
    if (!this->basic.f1) {
      delete this->lenv;
    }
  }
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
//
// Stores anonymous and needs_arguments inside f1 and f2
struct Function {
  Basic basic;
  VALUE name;
  uint32_t argc;
  uint32_t lvarcount;
  Frame* context;
  uint8_t* body_address;
  bool bound_self_set;
  VALUE bound_self;
  std::unordered_map<VALUE, VALUE>* container;

  inline bool anonymous() { return this->basic.f1; }
  inline bool needs_arguments() { return this->basic.f2; }
  inline void set_anonymous(bool f) { this->basic.f1 = f; }
  inline void set_needs_arguments(bool f) { this->basic.f2 = f; }

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
  std::unordered_map<VALUE, VALUE>* container;

  inline void clean() {
    delete this->container;
  }

  // TODO: Bound argumentlist
};

// Generators allow pausing and resuming execution of their block
//
// A generator allows you to pause execution of a function at an arbitrary location
// and resume it at a later time
//
// The generator saves enough state to be able to resume from the last position
//
// Relevant fields which differ from the Function struct
//
// context_frame: Stores the frame that is active in the generator
// context_catchtable: Stores the catchtable that is active in the generator
// context_stack: Stores all values on the stack which belong to the generator
// resume_address: Stores the address at which execution should continue the next time it is called
// finished: Wether the generator has finished, if true, calling it will throw an exception
//
// The context_frame field contains the frame which was created for the function.
// Nested functions also have a reference to that same frame, but we can freely modify it because
// child functions only require the environment for variable lookups
// This means we can freely modify the parent_frame pointer to correctly handle generator returns
//
// Uses f1 and f2 to store finished and started flags
struct Generator {
  Basic basic;
  VALUE name;
  Frame* context_frame;
  CatchTable* context_catchtable;
  std::vector<VALUE>* context_stack;
  uint8_t* resume_address;
  bool owns_catchtable;
  bool running;
  bool bound_self_set;
  VALUE bound_self;
  std::unordered_map<VALUE, VALUE>* container;

  inline bool finished() { return this->basic.f1; }
  inline bool started() { return this->basic.f2; }
  inline void set_finished(bool f) { this->basic.f1 = f; }
  inline void set_started(bool f) { this->basic.f2 = f; }

  inline void clean() {
    delete this->container;
    delete this->context_stack;
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
static constexpr uint64_t kMaskSignBit       = 0x8000000000000000; // Sign bit
static constexpr uint64_t kMaskExponentBits  = 0x7ff0000000000000; // Exponent bits
static constexpr uint64_t kMaskQuietBit      = 0x0008000000000000; // Quiet bit
static constexpr uint64_t kMaskTypeBits      = 0x0007000000000000; // Type bits
static constexpr uint64_t kMaskSignature     = 0xffff000000000000; // Signature bits
static constexpr uint64_t kMaskPayloadBits   = 0x0000ffffffffffff; // Payload bits

// Types that are encoded in the type field
static constexpr uint64_t kITypeNaN          = 0x0000000000000000;
static constexpr uint64_t kITypeFalse        = 0x0001000000000000;
static constexpr uint64_t kITypeTrue         = 0x0002000000000000;
static constexpr uint64_t kITypeNull         = 0x0003000000000000;
static constexpr uint64_t kITypeInteger      = 0x0004000000000000;
static constexpr uint64_t kITypeSymbol       = 0x0005000000000000;
static constexpr uint64_t kITypePString      = 0x0006000000000000;
static constexpr uint64_t kITypeIString      = 0x0007000000000000;

// Shorthand values
static constexpr uint64_t kBitsNaN           = kMaskExponentBits | kMaskQuietBit;
static constexpr uint64_t kFalse             = kBitsNaN | kITypeFalse; // 0x7ff9000000000000
static constexpr uint64_t kTrue              = kBitsNaN | kITypeTrue;  // 0x7ffa000000000000
static constexpr uint64_t kNull              = kBitsNaN | kITypeNull;  // 0x7ffb000000000000

// Signatures of complex encoded types
static constexpr uint64_t kSignaturePointer  = kMaskSignBit | kBitsNaN;
static constexpr uint64_t kSignatureInteger  = kBitsNaN | kITypeInteger;
static constexpr uint64_t kSignatureSymbol   = kBitsNaN | kITypeSymbol;
static constexpr uint64_t kSignaturePString  = kBitsNaN | kITypePString;
static constexpr uint64_t kSignatureIString  = kBitsNaN | kITypeIString;

// Masks for the immediate encoded types
static constexpr uint64_t kMaskPointer       = 0x0000ffffffffffff;
static constexpr uint64_t kMaskInteger       = 0x0000ffffffffffff;
static constexpr uint64_t kMaskIntegerSign   = 0x0000800000000000;
static constexpr uint64_t kMaskSymbol        = 0x0000ffffffffffff;
static constexpr uint64_t kMaskPString       = 0x0000ffffffffffff;
static constexpr uint64_t kMaskIString       = 0x000000ffffffffff;
static constexpr uint64_t kMaskIStringLength = 0x0000ff0000000000;

// Constants used when converting between different representations
static constexpr int64_t kMaxInt             = (static_cast<int64_t>(1) << 47) - 1;
static constexpr int64_t kMaxUInt            = (static_cast<int64_t>(1) << 48) - 1;
static constexpr int64_t kMinInt             = -(static_cast<int64_t>(1) << 47);
static const void* const kMaxPointer         = (void*)0x0000FFFFFFFFFFFF;
static constexpr uint64_t kSignBlock         = 0xFFFF000000000000;

// Misc. constants
static constexpr uint32_t kMaxIStringLength  = 5;
static constexpr uint32_t kMaxPStringLength  = 6;
static constexpr int64_t kMaxStringLength    = 0xffffffff;

// Type casting functions
template <typename T>
__attribute__((always_inline))
T* charly_as_pointer_to(VALUE value)                  { return reinterpret_cast<T*>(value & kMaskPointer); }
__attribute__((always_inline))
inline void* charly_as_pointer(VALUE value)           { return charly_as_pointer_to<void>(value); }
__attribute__((always_inline))
inline Basic* charly_as_basic(VALUE value)            { return charly_as_pointer_to<Basic>(value); }
__attribute__((always_inline))
inline Class* charly_as_class(VALUE value)            { return charly_as_pointer_to<Class>(value); }
__attribute__((always_inline))
inline Object* charly_as_object(VALUE value)          { return charly_as_pointer_to<Object>(value); }
__attribute__((always_inline))
inline Array* charly_as_array(VALUE value)            { return charly_as_pointer_to<Array>(value); }
__attribute__((always_inline))
inline String* charly_as_hstring(VALUE value)         { return charly_as_pointer_to<String>(value); }
__attribute__((always_inline))
inline Function* charly_as_function(VALUE value)      { return charly_as_pointer_to<Function>(value); }
__attribute__((always_inline))
inline CFunction* charly_as_cfunction(VALUE value)    { return charly_as_pointer_to<CFunction>(value); }
__attribute__((always_inline))
inline Generator* charly_as_generator(VALUE value)    { return charly_as_pointer_to<Generator>(value); }
__attribute__((always_inline))
inline Frame* charly_as_frame(VALUE value)            { return charly_as_pointer_to<Frame>(value); }
__attribute__((always_inline))
inline CatchTable* charly_as_catchtable(VALUE value)  { return charly_as_pointer_to<CatchTable>(value); }

// Type checking functions
__attribute__((always_inline))
inline bool charly_is_false(VALUE value)     { return value == kFalse; }
__attribute__((always_inline))
inline bool charly_is_true(VALUE value)      { return value == kTrue; }
__attribute__((always_inline))
inline bool charly_is_boolean(VALUE value)   { return charly_is_false(value) || charly_is_true(value); }
__attribute__((always_inline))
inline bool charly_is_null(VALUE value)      { return value == kNull; }
__attribute__((always_inline))
inline bool charly_is_nan(VALUE value)       { return value == kBitsNaN; }
__attribute__((always_inline))
inline bool charly_is_float(VALUE value)     { return charly_is_nan(value) || ((~value & kMaskExponentBits) != 0); }
__attribute__((always_inline))
inline bool charly_is_int(VALUE value)       { return (value & kMaskSignature) == kSignatureInteger; }
__attribute__((always_inline))
inline bool charly_is_number(VALUE value)    { return charly_is_int(value) || charly_is_float(value); }
__attribute__((always_inline))
inline bool charly_is_symbol(VALUE value)    { return (value & kMaskSignature) == kSignatureSymbol; }
__attribute__((always_inline))
inline bool charly_is_pstring(VALUE value)   { return (value & kMaskSignature) == kSignaturePString; }
__attribute__((always_inline))
inline bool charly_is_istring(VALUE value)   { return (value & kMaskSignature) == kSignatureIString; }
__attribute__((always_inline))
inline bool charly_is_immediate_string(VALUE value) { return charly_is_istring(value) || charly_is_pstring(value); }
__attribute__((always_inline))
inline bool charly_is_ptr(VALUE value)       { return (value & kMaskSignature) == kSignaturePointer; }

// Heap allocated types
__attribute__((always_inline))
inline bool charly_is_on_heap(VALUE value) { return charly_is_ptr(value); }
__attribute__((always_inline))
inline bool charly_is_heap_type(VALUE value, uint8_t type) {
  return charly_is_on_heap(value) && charly_as_basic(value)->type == type;
}
__attribute__((always_inline))
inline bool charly_is_dead(VALUE value) { return charly_is_heap_type(value, kTypeDead); }
__attribute__((always_inline))
inline bool charly_is_class(VALUE value) { return charly_is_heap_type(value, kTypeClass); }
__attribute__((always_inline))
inline bool charly_is_object(VALUE value) { return charly_is_heap_type(value, kTypeObject); }
__attribute__((always_inline))
inline bool charly_is_array(VALUE value) { return charly_is_heap_type(value, kTypeArray); }
__attribute__((always_inline))
inline bool charly_is_hstring(VALUE value) { return charly_is_heap_type(value, kTypeString); }
__attribute__((always_inline))
inline bool charly_is_string(VALUE value) {
  return charly_is_istring(value) || charly_is_pstring(value) || charly_is_hstring(value);
}
__attribute__((always_inline))
inline bool charly_is_function(VALUE value) { return charly_is_heap_type(value, kTypeFunction); }
__attribute__((always_inline))
inline bool charly_is_cfunction(VALUE value) { return charly_is_heap_type(value, kTypeCFunction); }
__attribute__((always_inline))
inline bool charly_is_generator(VALUE value) { return charly_is_heap_type(value, kTypeGenerator); }
__attribute__((always_inline))
inline bool charly_is_callable(VALUE value) {
  return charly_is_function(value) || charly_is_cfunction(value) || charly_is_class(value);
}
__attribute__((always_inline))
inline bool charly_is_frame(VALUE value) { return charly_is_heap_type(value, kTypeFrame); }
__attribute__((always_inline))
inline bool charly_is_catchtable(VALUE value) { return charly_is_heap_type(value, kTypeCatchTable); }

// Return the ValueType representation of the type of the value
__attribute__((always_inline))
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
__attribute__((always_inline))
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
__attribute__((always_inline))
inline int64_t charly_int_to_int64(VALUE value) {
  return (value & kMaskInteger) | ((value & kMaskIntegerSign) ? kSignBlock : 0x00);
}
__attribute__((always_inline))
inline uint64_t charly_int_to_uint64(VALUE value) { return charly_int_to_int64(value); }
__attribute__((always_inline))
inline int32_t charly_int_to_int32(VALUE value)   { return charly_int_to_int64(value); }
__attribute__((always_inline))
inline uint32_t charly_int_to_uint32(VALUE value) { return charly_int_to_int64(value); }
__attribute__((always_inline))
inline int16_t charly_int_to_int16(VALUE value)   { return charly_int_to_int64(value); }
__attribute__((always_inline))
inline uint16_t charly_int_to_uint16(VALUE value) { return charly_int_to_int64(value); }
__attribute__((always_inline))
inline int8_t charly_int_to_int8(VALUE value)     { return charly_int_to_int64(value); }
__attribute__((always_inline))
inline uint8_t charly_int_to_uint8(VALUE value)   { return charly_int_to_int64(value); }
__attribute__((always_inline))
inline float charly_int_to_float(VALUE value)     { return charly_int_to_int64(value); }
__attribute__((always_inline))
inline double charly_int_to_double(VALUE value)   { return charly_int_to_int64(value); }

// Convert an immediate double to other integer or float types
//
// Warning: These methods don't perform any type checks and assume
// the caller made sure that the input value is an immediate double

// The purpose of this method is to replace INFINITY, -INFINITY, NAN with 0
// Conversion from these values to int is undefined behaviour and will result
// in random gibberish being returned
__attribute__((always_inline))
inline double charly_double_to_safe_double(VALUE value) { return FP_STRIP_INF(FP_STRIP_NAN(BITCAST_DOUBLE(value))); }
__attribute__((always_inline))
inline int64_t charly_double_to_int64(VALUE value)      { return charly_double_to_safe_double(value); }
__attribute__((always_inline))
inline uint64_t charly_double_to_uint64(VALUE value)    { return charly_double_to_safe_double(value); }
__attribute__((always_inline))
inline int32_t charly_double_to_int32(VALUE value)      { return charly_double_to_safe_double(value); }
__attribute__((always_inline))
inline uint32_t charly_double_to_uint32(VALUE value)    { return charly_double_to_safe_double(value); }
__attribute__((always_inline))
inline int16_t charly_double_to_int16(VALUE value)      { return charly_double_to_safe_double(value); }
__attribute__((always_inline))
inline uint16_t charly_double_to_uint16(VALUE value)    { return charly_double_to_safe_double(value); }
__attribute__((always_inline))
inline int8_t charly_double_to_int8(VALUE value)        { return charly_double_to_safe_double(value); }
__attribute__((always_inline))
inline uint8_t charly_double_to_uint8(VALUE value)      { return charly_double_to_safe_double(value); }
__attribute__((always_inline))
inline float charly_double_to_float(VALUE value)        { return BITCAST_DOUBLE(value); }
__attribute__((always_inline))
inline double charly_double_to_double(VALUE value)      { return BITCAST_DOUBLE(value); }
__attribute__((always_inline))

// Convert an immediate number to other integer or float types
//
// Note: Assumes the caller doesn't know what exact number type the value has,
// only that it is a number.
__attribute__((always_inline))
inline int64_t charly_number_to_int64(VALUE value)   {
  if (charly_is_float(value)) return charly_double_to_int64(value);
  return charly_int_to_int64(value);
}
__attribute__((always_inline))
inline uint64_t charly_number_to_uint64(VALUE value) {
  if (charly_is_float(value)) return charly_double_to_uint64(value);
  return charly_int_to_uint64(value);
}
__attribute__((always_inline))
inline int32_t charly_number_to_int32(VALUE value)   {
  if (charly_is_float(value)) return charly_double_to_int32(value);
  return charly_int_to_int32(value);
}
__attribute__((always_inline))
inline uint32_t charly_number_to_uint32(VALUE value) {
  if (charly_is_float(value)) return charly_double_to_uint32(value);
  return charly_int_to_uint32(value);
}
__attribute__((always_inline))
inline int16_t charly_number_to_int16(VALUE value)   {
  if (charly_is_float(value)) return charly_double_to_int16(value);
  return charly_int_to_int16(value);
}
__attribute__((always_inline))
inline uint16_t charly_number_to_uint16(VALUE value) {
  if (charly_is_float(value)) return charly_double_to_uint16(value);
  return charly_int_to_uint16(value);
}
__attribute__((always_inline))
inline int8_t charly_number_to_int8(VALUE value)     {
  if (charly_is_float(value)) return charly_double_to_int8(value);
  return charly_int_to_int8(value);
}
__attribute__((always_inline))
inline uint8_t charly_number_to_uint8(VALUE value)   {
  if (charly_is_float(value)) return charly_double_to_uint8(value);
  return charly_int_to_uint8(value);
}
__attribute__((always_inline))
inline float charly_number_to_float(VALUE value)     {
  if (charly_is_float(value)) return charly_double_to_float(value);
  return charly_int_to_float(value);
}
__attribute__((always_inline))
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
__attribute__((always_inline))
inline char* charly_string_data(VALUE& value) {
  if (charly_is_hstring(value)) {
    return charly_as_hstring(value)->data();
  }

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

  return nullptr;
}

// Get the length of a string
// Returns -1 (0xffffffff) if value is not a string
__attribute__((always_inline))
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

// Returns a pointer to the length field of an immediate string
// Returns a null pointer if the value is not a istring
inline uint8_t* charly_istring_length_field(VALUE* value) {
  if (IS_BIG_ENDIAN()) {
    return reinterpret_cast<uint8_t*>(value) + 2;
  } else {
    return reinterpret_cast<uint8_t*>(value) + 5;
  }
  return nullptr;
}

// Create immediate encoded strings of size 0 - 5
//
// Note: Because char* should always contain a null terminator at the end, we check for <= 6 bytes
// instead of <= 5.
template <size_t N>
__attribute__((always_inline))
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

__attribute__((always_inline))
inline VALUE charly_create_istring(char const (& input)[7]) {
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

__attribute__((always_inline))
inline VALUE charly_create_empty_string() {
  return kSignatureIString;
}

// Create an istring from data pointed to by ptr
//
// Returns kNull if the string doesn't fit into the immediate encoded format
inline VALUE charly_create_istring(const char* ptr, uint8_t length) {
  if (length == 6) {
    // Construct packed string if we have exactly 6 bytes of data
    VALUE val = kSignaturePString;
    char* buf = (char*)&val;

    // Copy the string buffer
    if (IS_BIG_ENDIAN()) {
      buf[2] = ptr[0];
      buf[3] = ptr[1];
      buf[4] = ptr[2];
      buf[5] = ptr[3];
      buf[6] = ptr[4];
      buf[7] = ptr[5];
    } else {
      buf[0] = ptr[0];
      buf[1] = ptr[1];
      buf[2] = ptr[2];
      buf[3] = ptr[3];
      buf[4] = ptr[4];
      buf[5] = ptr[5];
    }

    return val;
  } else if (length <= 5) {
    // Construct string with length if we have 0-5 bytes of data
    VALUE val = kSignatureIString;
    char* buf = (char*)&val;

    // Copy the string buffer
    if (IS_BIG_ENDIAN()) {
      if (length >= 1) buf[3] = ptr[0];
      if (length >= 2) buf[4] = ptr[1];
      if (length >= 3) buf[5] = ptr[2];
      if (length >= 4) buf[6] = ptr[3];
      if (length >= 5) buf[7] = ptr[4];
      buf[2] = length;
    } else {
      if (length >= 1) buf[0] = ptr[0];
      if (length >= 2) buf[1] = ptr[1];
      if (length >= 3) buf[2] = ptr[2];
      if (length >= 4) buf[3] = ptr[3];
      if (length >= 5) buf[4] = ptr[4];
      buf[5] = length;
    }

    return val;
  } else {
    return kNull;
  }
}

// Get the amount of utf8 codepoints inside a string
__attribute__((always_inline))
inline uint32_t charly_string_utf8_length(VALUE value) {
  uint32_t length = charly_string_length(value);
  char* start = charly_string_data(value);
  char* end = start + length;
  return utf8::distance(start, end);
}

// Get the utf8 codepoint at a given index into the string
// The index indexes over utf8 codepoints, not bytes
//
// Return value will be an immediate string
// If the index is out-of-bounds, kNull will be returned
inline VALUE charly_string_cp_at_index(VALUE value, int32_t index) {
  uint32_t length = charly_string_length(value);
  uint32_t utf8length = charly_string_utf8_length(value);
  char* start = charly_string_data(value);
  char* start_it = start;
  char* end = start + length;

  // Wrap negative index and do bounds check
  if (index < 0) index += utf8length;
  if (index < 0 || index >= static_cast<int32_t>(utf8length)) return kNull;

  // Advance to our index
  while (index--) {
    if (start_it >= end) return kNull;
    utf8::advance(start_it, 1, end);
  }

  // Calculate the length of current codepoint
  char* cp_begin = start_it;
  utf8::advance(start_it, 1, end);
  uint32_t cp_length = start_it - cp_begin;

  return charly_create_istring(cp_begin, cp_length);
}

// Convert a string to a int64
__attribute__((always_inline))
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
__attribute__((always_inline))
inline double charly_string_to_double(VALUE value) {
  size_t length = charly_string_length(value);
  char* buffer = charly_string_data(value);
  char* buffer_end = buffer + length;

  double interpreted = strtod(buffer, &buffer_end);

  // HUGE_VAL gets returned when the converted value
  // doesn't fit inside a double value
  // In this case we just return NAN
  if (interpreted == HUGE_VAL) {
    return NAN;
  }

  // If strtod could not perform a conversion it returns 0
  // and sets str_end to str
  // We just return NAN in this case
  if (buffer_end == buffer) {
    return NAN;
  }

  return interpreted;
}

// Create an immediate integer
//
// Warning: Doesn't perform any overflow checks. If the integer doesn't fit into 48 bits
// the value is going to be truncated.
template <typename T>
__attribute__((always_inline))
VALUE charly_create_integer(T value) {
  return kSignatureInteger | (value & kMaskInteger);
}

// Create an immediate double
__attribute__((always_inline))
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
__attribute__((always_inline))
inline VALUE charly_value_to_number(VALUE value) {
  if (charly_is_float(value)) return value;
  if (charly_is_int(value)) return value;
  if (charly_is_boolean(value)) return value == kTrue ? charly_create_integer(1) : charly_create_integer(0);
  if (charly_is_null(value)) return charly_create_integer(0);
  if (charly_is_symbol(value)) return charly_create_integer(0);
  if (charly_is_string(value)) return charly_string_to_double(value);
  return charly_create_double(NAN);
}
__attribute__((always_inline))
inline int64_t charly_value_to_int64(VALUE value) {
  if (charly_is_number(value)) return charly_number_to_int64(value);
  if (charly_is_boolean(value)) return value == kTrue ? 1 : 0;
  if (charly_is_null(value)) return 0;
  if (charly_is_symbol(value)) return 0;
  if (charly_is_string(value)) return charly_string_to_int64(value);
  return 0;
}
__attribute__((always_inline))
inline double charly_value_to_double(VALUE value) {
  if (charly_is_number(value)) return charly_number_to_double(value);
  if (charly_is_boolean(value)) return value == kTrue ? 1 : 0;
  if (charly_is_null(value)) return 0;
  if (charly_is_symbol(value)) return 0;
  if (charly_is_string(value)) return charly_string_to_double(value);
  return 0;
}
__attribute__((always_inline))
inline uint64_t charly_value_to_uint64(VALUE value) { return charly_value_to_int64(value); }
__attribute__((always_inline))
inline int32_t charly_value_to_int32(VALUE value)   { return charly_value_to_int64(value); }
__attribute__((always_inline))
inline uint32_t charly_value_to_uint32(VALUE value) { return charly_value_to_int64(value); }
__attribute__((always_inline))
inline int16_t charly_value_to_int16(VALUE value)   { return charly_value_to_int64(value); }
__attribute__((always_inline))
inline uint16_t charly_value_to_uint16(VALUE value) { return charly_value_to_int64(value); }
__attribute__((always_inline))
inline int8_t charly_value_to_int8(VALUE value)     { return charly_value_to_int64(value); }
__attribute__((always_inline))
inline uint8_t charly_value_to_uint8(VALUE value)   { return charly_value_to_int64(value); }
__attribute__((always_inline))
inline float charly_value_to_float(VALUE value)     { return charly_value_to_double(value); }

// Convert a number type into an immediate charly value
//
// This method assumes the caller doesn't care what format the resulting number has,
// so it might return an immediate integer or double
__attribute__((always_inline))
inline VALUE charly_create_number(int64_t value) {
  if (value >= kMaxInt || value <= kMinInt) return charly_create_double(value);
  return charly_create_integer(value);
}
__attribute__((always_inline))
inline VALUE charly_create_number(uint64_t value) {
  if (value >= kMaxUInt) return charly_create_double(value);
  return charly_create_integer(value);
}
__attribute__((always_inline))
inline VALUE charly_create_number(int32_t value)  { return charly_create_integer(value); }
__attribute__((always_inline))
inline VALUE charly_create_number(uint32_t value) { return charly_create_integer(value); }
__attribute__((always_inline))
inline VALUE charly_create_number(int16_t value)  { return charly_create_integer(value); }
__attribute__((always_inline))
inline VALUE charly_create_number(uint16_t value) { return charly_create_integer(value); }
__attribute__((always_inline))
inline VALUE charly_create_number(int8_t value)   { return charly_create_integer(value); }
__attribute__((always_inline))
inline VALUE charly_create_number(uint8_t value)  { return charly_create_integer(value); }
__attribute__((always_inline))
inline VALUE charly_create_number(double value)   {
  double intpart;
  if (modf(value, &intpart) == 0.0 && value <= kMaxInt && value >= kMinInt) {
    return charly_create_integer((int64_t)value);
  }
  return charly_create_double(value);
}
__attribute__((always_inline))
inline VALUE charly_create_number(float value)    { return charly_create_number((double)value); }

// Binary arithmetic methods
//
// Note: These methods assume the caller made sure that left and right are of a number type
__attribute__((always_inline))
inline VALUE charly_add_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) + charly_int_to_int64(right));
  }
  return charly_create_number(charly_number_to_double(left) + charly_number_to_double(right));
}
__attribute__((always_inline))
inline VALUE charly_sub_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) - charly_int_to_int64(right));
  }
  return charly_create_number(charly_number_to_double(left) - charly_number_to_double(right));
}
__attribute__((always_inline))
inline VALUE charly_mul_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) * charly_int_to_int64(right));
  }
  return charly_create_number(charly_number_to_double(left) * charly_number_to_double(right));
}
__attribute__((always_inline))
inline VALUE charly_div_number(VALUE left, VALUE right) {
  return charly_create_number(charly_number_to_double(left) / charly_number_to_double(right));
}
__attribute__((always_inline))
inline VALUE charly_mod_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(charly_int_to_int64(left) % charly_int_to_int64(right));
  }
  return charly_create_number(std::fmod(charly_number_to_double(left), charly_number_to_double(right)));
}
__attribute__((always_inline))
inline VALUE charly_pow_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_create_number(std::pow(charly_int_to_int64(left), charly_int_to_int64(right)));
  }
  return charly_create_number(std::pow(charly_number_to_double(left), charly_number_to_double(right)));
}
__attribute__((always_inline))
inline VALUE charly_lt_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_int_to_int64(left) < charly_int_to_int64(right) ? kTrue : kFalse;
  }
  return std::isless(charly_number_to_double(left), charly_number_to_double(right)) ? kTrue : kFalse;
}
__attribute__((always_inline))
inline VALUE charly_gt_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_int_to_int64(left) > charly_int_to_int64(right) ? kTrue : kFalse;
  }
  return std::isgreater(charly_number_to_double(left), charly_number_to_double(right)) ? kTrue : kFalse;
}
__attribute__((always_inline))
inline VALUE charly_le_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_int_to_int64(left) <= charly_int_to_int64(right) ? kTrue : kFalse;
  }
  return std::islessequal(charly_number_to_double(left), charly_number_to_double(right)) ? kTrue : kFalse;
}
__attribute__((always_inline))
inline VALUE charly_ge_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_int_to_int64(left) >= charly_int_to_int64(right) ? kTrue : kFalse;
  }
  return std::isgreaterequal(charly_number_to_double(left), charly_number_to_double(right)) ? kTrue : kFalse;
}
__attribute__((always_inline))
inline VALUE charly_eq_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_int_to_int64(left) == charly_int_to_int64(right) ? kTrue : kFalse;
  }
  return FP_ARE_EQUAL(charly_number_to_double(left), charly_number_to_double(right)) ? kTrue : kFalse;
}
__attribute__((always_inline))
inline VALUE charly_neq_number(VALUE left, VALUE right) {
  if (charly_is_int(left) && charly_is_int(right)) {
    return charly_int_to_int64(left) != charly_int_to_int64(right) ? kTrue : kFalse;
  }
  return FP_ARE_EQUAL(charly_number_to_double(left), charly_number_to_double(right)) ? kFalse : kTrue;
}
__attribute__((always_inline))
inline VALUE charly_shl_number(VALUE left, VALUE right) {
  int32_t num = charly_number_to_int32(left);
  int32_t amount = charly_number_to_int32(right);
  return charly_create_number(num << amount);
}
__attribute__((always_inline))
inline VALUE charly_shr_number(VALUE left, VALUE right) {
  int32_t num = charly_number_to_int32(left);
  int32_t amount = charly_number_to_int32(right);
  return charly_create_number(num >> amount);
}
__attribute__((always_inline))
inline VALUE charly_and_number(VALUE left, VALUE right) {
  int32_t num = charly_number_to_int32(left);
  int32_t amount = charly_number_to_int32(right);
  return charly_create_number(num & amount);
}
__attribute__((always_inline))
inline VALUE charly_or_number(VALUE left, VALUE right) {
  int32_t num = charly_number_to_int32(left);
  int32_t amount = charly_number_to_int32(right);
  return charly_create_number(num | amount);
}
__attribute__((always_inline))
inline VALUE charly_xor_number(VALUE left, VALUE right) {
  int32_t num = charly_number_to_int32(left);
  int32_t amount = charly_number_to_int32(right);
  return charly_create_number(num ^ amount);
}

// Unary arithmetic methods
__attribute__((always_inline))
inline VALUE charly_uadd_number(VALUE value) {
  return value;
}
__attribute__((always_inline))
inline VALUE charly_usub_number(VALUE value) {
  if (charly_is_int(value)) return charly_create_number(-charly_int_to_int64(value));
  return charly_create_double(-charly_double_to_double(value));
}
__attribute__((always_inline))
inline VALUE charly_unot_number(VALUE value) {
  if (charly_is_int(value)) return charly_int_to_int8(value) == 0 ? kTrue : kFalse;
  return charly_double_to_double(value) == 0.0 ? kTrue : kFalse;
}
__attribute__((always_inline))
inline VALUE charly_ubnot_number(VALUE value) {
  if (charly_is_int(value)) return charly_create_number(~charly_int_to_int32(value));
  return charly_create_integer(~charly_double_to_int32(value));
}
__attribute__((always_inline))
inline bool charly_truthyness(VALUE value) {
  if (value == kBitsNaN) return false;
  if (value == kNull) return false;
  if (value == kFalse) return false;
  if (charly_is_int(value)) return charly_int_to_int64(value) != 0;
  if (charly_is_float(value)) return charly_double_to_double(value) != 0;
  if (charly_is_generator(value)) return !charly_as_generator(value)->finished();
  return true;
}

// Concatenate two strings into a packed encoded string
//
// Assumes the caller made sure both strings fit into exactly 6 bytes
__attribute__((always_inline))
inline VALUE charly_string_concat_into_packed(VALUE left, VALUE right) {
  VALUE result = kSignaturePString;
  char* buf = charly_string_data(result);
  uint32_t left_length = charly_string_length(left);
  std::memcpy(buf, charly_string_data(left), left_length);
  std::memcpy(buf + left_length, charly_string_data(right), 6 - left_length);
  return result;
}

// Concatenate two strings into an immediate encoded string
//
// Assumes the caller made sure the string fits exactly into 5 or less bytes
__attribute__((always_inline))
inline VALUE charly_string_concat_into_immediate(VALUE left, VALUE right) {
  VALUE result = kSignatureIString;
  char* buf = charly_string_data(result);
  uint32_t left_length = charly_string_length(left);
  uint32_t right_length = charly_string_length(right);
  std::memcpy(buf, charly_string_data(left), left_length);
  std::memcpy(buf + left_length, charly_string_data(right), right_length);
  *charly_istring_length_field(&result) = left_length + right_length;
  return result;
}

// Multiply a string by an integer
//
// Assumes the caller made sure that left is a string and right a number
// Assumes the caller made sure the result fits into exactly 6 bytes
__attribute__((always_inline))
inline VALUE charly_string_mul_into_packed(VALUE left, int64_t amount) {
  VALUE result = kSignaturePString;
  char* buf = charly_string_data(result);

  char* str_data = charly_string_data(left);
  uint32_t str_length = charly_string_length(left);

  uint32_t offset = 0;
  while (amount--) {
    std::memcpy(buf + offset, str_data, str_length);
    offset += str_length;
  }

  return result;
}

// Multiply a string by an integer
//
// Assumes the caller made sure that left is a string and right a number
// Assumes the caller made sure the result fits into exactly 5 or less bytes
__attribute__((always_inline))
inline VALUE charly_string_mul_into_immediate(VALUE left, int64_t amount) {
  VALUE result = kSignatureIString;
  char* buf = charly_string_data(result);

  char* str_data = charly_string_data(left);
  uint32_t str_length = charly_string_length(left);

  uint32_t offset = 0;
  while (amount--) {
    std::memcpy(buf + offset, str_data, str_length);
    offset += str_length;
  }

  *charly_istring_length_field(&result) = offset;

  return result;
}

// Convert types into symbols
__attribute__((always_inline))
inline VALUE charly_create_symbol(const std::string& input) {
  size_t val = std::hash<std::string>{}(input);
  return kSignatureSymbol | (val & kMaskSymbol);
}

// TODO: Refactor this...
// It creates too many copies of data and std::stringstream is not acceptable
__attribute__((always_inline))
inline VALUE charly_create_symbol(VALUE value) {
  uint8_t type = charly_get_type(value);
  switch (type) {
    case kTypeString: {
      char* str_data = charly_string_data(value);
      uint32_t str_length = charly_string_length(value);
      return charly_create_symbol(std::string(str_data, str_length));
    }
    case kTypeNumber: {
      if (charly_is_float(value)) {
        double val = charly_double_to_double(value);
        std::stringstream io;
        io << val;
        return charly_create_symbol(io.str());
      } else {
        int64_t val = charly_int_to_int64(value);
        std::stringstream io;
        io << val;
        return charly_create_symbol(io.str());
      }
    }
    default: {
      return charly_create_symbol(charly_get_typestring(type));
    }
  }
}

// Create a VALUE from a ptr
__attribute__((always_inline))
inline VALUE charly_create_pointer(void* ptr) {
  if (ptr > kMaxPointer) return kSignaturePointer; // null pointer
  return kSignaturePointer | (kMaskPointer & reinterpret_cast<int64_t>(ptr));
}

// clang-format on

}  // namespace Charly
