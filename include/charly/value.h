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
#include <limits>

#include "charly.h"
#include "symbol.h"

#pragma once

namespace charly::core::runtime {

// types which are encoded in a tagged pointer
#define IMMEDIATE_TYPE_NAMES(V) \
  V(Int)                        \
  V(Float)                      \
  V(Bool)                       \
  V(Symbol)                     \
  V(Null)                       \
  V(SmallString)                \
  V(SmallBytes)

// types which do not have their own object type but serve as a superclass
// for child types
//
// e.g. a 'String' can be either a SmallString, LargeString or HugeString
#define SUPER_TYPE_NAMES(V) \
  V(Value)                  \
  V(Object)                 \
  V(Instance)               \
  V(Data)                   \
  V(Bytes)                  \
  V(String)

// types which store raw bytes on the heap
#define DATA_TYPE_NAMES(V) \
  V(LargeBytes)            \
  V(LargeString)

// types which store their data via the shape system
#define INSTANCE_TYPE_NAMES(V) \
  V(HugeBytes)                 \
  V(HugeString)                \
  V(Tuple)                     \
  V(Fiber)                     \
  V(Function)                  \
  V(Shape)                     \
  V(Type)

// exception types
#define EXCEPTION_TYPE_NAMES(V) V(Exception)

#define TYPE_NAMES(V)     \
  IMMEDIATE_TYPE_NAMES(V) \
  SUPER_TYPE_NAMES(V)     \
  DATA_TYPE_NAMES(V)      \
  INSTANCE_TYPE_NAMES(V)  \
  EXCEPTION_TYPE_NAMES(V)

enum class ShapeId : uint32_t {

  // immediate types
  //
  // the shape id of any immediate type can be determined by checking the
  // lowest 4 bits.
  kInt = 0,
  kFloat = 3,
  kBool = 5,
  kSymbol = 7,
  kNull = 11,
  kSmallString = 13,
  kSmallBytes = 15,
  kLastImmediateShape = kSmallBytes,

  // non-instance heap types
  // clang-format off
#define SHAPE_ID(name) k##name,
#define GET_FIRST(name) k##name + 0 *
#define GET_LAST(name) 0 + k##name *
  DATA_TYPE_NAMES(SHAPE_ID)
  kFirstNonInstance = DATA_TYPE_NAMES(GET_FIRST) 1,
  kLastNonInstance = DATA_TYPE_NAMES(GET_LAST) 1,

  INSTANCE_TYPE_NAMES(SHAPE_ID)
  EXCEPTION_TYPE_NAMES(SHAPE_ID)

  kFirstException = EXCEPTION_TYPE_NAMES(GET_FIRST) 0,
  kLastException = EXCEPTION_TYPE_NAMES(GET_LAST) 1,
#undef GET_LAST
#undef GET_FIRST
#undef SHAPE_ID
  // clang-format on

  kLastBuiltinShapeId = kLastException,
  kMaxShapeId = (uint32_t{1} << 20) - 1,
  kMaxShapeCount = uint32_t{1} << 20
};

// maps the lowest 4 bits of an immediate value to a shape id
// this table is consulted after the runtime has determined that the value
// is not a pointer to a heap object
static const ShapeId kShapeImmediateTagMapping[16] = {
  /* 0b0000 */ ShapeId::kInt,
  /* 0b0001 */ ShapeId::kMaxShapeCount,   // heap objects
  /* 0b0010 */ ShapeId::kInt,
  /* 0b0011 */ ShapeId::kFloat,
  /* 0b0100 */ ShapeId::kInt,
  /* 0b0101 */ ShapeId::kBool,
  /* 0b0110 */ ShapeId::kInt,
  /* 0b0111 */ ShapeId::kSymbol,
  /* 0b1000 */ ShapeId::kInt,
  /* 0b1001 */ ShapeId::kMaxShapeCount,   // heap objects
  /* 0b1010 */ ShapeId::kInt,
  /* 0b1011 */ ShapeId::kNull,
  /* 0b1100 */ ShapeId::kInt,
  /* 0b1101 */ ShapeId::kSmallString,
  /* 0b1110 */ ShapeId::kInt,
  /* 0b1111 */ ShapeId::kSmallBytes
};

// check if a shape belongs to a certain group
bool is_immediate_shape(ShapeId shape_id);
bool is_object_shape(ShapeId shape_id);
bool is_data_shape(ShapeId shape_id);
bool is_instance_shape(ShapeId shape_id);
bool is_exception_shape(ShapeId shape_id);
bool is_builtin_shape(ShapeId shape_id);
bool is_user_shape(ShapeId shape_id);

// align a size to some alignment
size_t align_to_size(size_t size, size_t alignment);

// internal error values used only internally in the runtime
// this value is encoded in the null value
enum class ErrorId : uint8_t {
  kErrorNone = 0,
  kErrorOk,
  kErrorNotFound,
  kErrorOutOfBounds,
  kErrorException
};

// forward declare Raw types
#define FORWARD_DECL(name) class Raw##name;
TYPE_NAMES(FORWARD_DECL)
#undef FORWARD_DECL

// every heap allocated object gets prefixed with a object header
class ObjectHeader {
public:
  CHARLY_NON_HEAP_ALLOCATABLE(ObjectHeader);

  enum Flag : uint8_t {
    kReachable = 1,        // object is reachable by GC
    kHasHashcode = 2,      // object has a cached hashcode
    kYoungGeneration = 4,  // object is in a young generation
  };

  static void initialize_header(uintptr_t address, ShapeId shape_id, uint16_t count);

  // getter
  ShapeId shape_id() const;
  uint8_t survivor_count() const;
  uint16_t count() const;
  Flag flags() const;
  SYMBOL hashcode();
  uint32_t forward_offset() const;

  RawObject object() const;

  // returns the size of the object (including padding, excluding the header)
  uint32_t object_size() const;

  bool cas_shape_id(ShapeId old_shape_id, ShapeId new_shape_id);
  bool cas_survivor_count(uint8_t old_count, uint8_t new_count);
  bool cas_count(uint16_t old_count, uint16_t new_count);
  bool cas_flags(Flag old_flags, Flag new_flags);
  bool cas_hashcode(SYMBOL old_hashcode, SYMBOL new_hashcode);
  bool cas_forward_offset(uint32_t old_offset, uint32_t new_offset);

private:
  static uint32_t encode_shape_and_survivor_count(ShapeId shape_id, uint8_t survivor_count);

private:
  // object header encoding
  //
  // [ shape id ] : 3 bytes : 24 bits :  3 bytes
  // [ gc count ] : 1 byte  :  8 bits :  4 bytes
  // [ lock     ] : 1 byte  :  8 bits :  5 bytes
  // [ flags    ] : 1 byte  :  8 bits :  6 bytes
  // [ count    ] : 2 bytes : 16 bits :  8 bytes
  // [ hashcode ] : 4 bytes : 32 bits : 12 bytes
  // [ forward  ] : 4 bytes : 32 bits : 16 bytes

  static const uint32_t kMaskShape = 0x00FFFFFF;
  static const uint32_t kMaskSurvivorCount = 0xFF000000;
  static const uint32_t kShiftSurvivorCount = 24;

  atomic<uint32_t> m_shape_id_and_survivor_count;  // shape id and survivor count
  atomic<uint16_t> m_count;                        // count field
  atomic<uint8_t> m_lock;                          // per-object small lock
  atomic<Flag> m_flags;                            // flags
  atomic<SYMBOL> m_hashcode;                       // cached hashcode of object / string
  atomic<uint32_t> m_forward_offset;               // contains offset of forwarded copy during concurrent GC
};
static_assert((sizeof(ObjectHeader) == 16), "invalid object header size");

static const size_t kObjectAlignment = 8;
static const size_t kObjectHeaderMaxCount = 65535;
static const size_t kObjectHeaderMaxSurvivorCount = 15;

#define COMMON_RAW_OBJECT(name)                                        \
  static Raw##name cast(RawValue value) {                              \
    DCHECK(value.is##name(), "invalid object type, expected %", #name); \
    return value.rawCast<Raw##name>();                                 \
  }                                                                    \
  static Raw##name cast(const RawValue* value) {                       \
    return cast(*value);                                               \
  }                                                                    \
  static Raw##name cast(uintptr_t value) {                             \
    return cast(RawValue(value));                                      \
  }                                                                    \
  static bool value_is_type(RawValue value) {                          \
    return value.is##name();                                           \
  }                                                                    \
  CHARLY_NON_HEAP_ALLOCATABLE(name)

// the RawValue class represents a single pointer-tagged value.
// the tagging scheme supports the following value types:
//
// ******** ******** ******** ******** ******** ******** ******** ******* 0  int
// ******** ******** ******** ******** ******** ******** ******** ***** 001  heap object
// DDDDDDDD DDDDDDDD DDDDDDDD DDDDDDDD DDDDDDDD DDDDDDDD DDDDDDDD DDDD 0011  float
// ******** ******** ******** ******** ******** ******** *******B **** 0101  bool
// SSSSSSSS SSSSSSSS SSSSSSSS SSSSSSSS ******** ******** ******** **** 0111  symbol
// ******** ******** ******** ******** ******** ******** ******** EEEE 1011  null (or internal error / header)
// SSSSSSSS SSSSSSSS SSSSSSSS SSSSSSSS SSSSSSSS SSSSSSSS SSSSSSSS LLLL 1101  small string
// BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB LLLL 1111  small bytes
//
// this scheme supports immediate 63-bit signed integers. the integer itself is encoded
// and decoded by shifting it to the left or right by 1 bit. if the LSB of the value
// is set to 0, it's always an immediate encoded int.
//
// values that have their LSB set to 1 can be either pointers to heap objects or another
// immediately encoded value. testing the last 3 bits allows us to determine if the value
// is a pointer or another immediate. encoded pointers must always be aligned to at least 8 bytes.
//
// if the value isn't an int or a heap pointer then it must be one of the following immediate types:
//
//         float: floats are stored as 60-bit IEEE 754 double-precision floating-point numbers
//                the last 4 bits of the mantissa are trimmed off when encoding a double value,
//                resulting in a small loss of precision, but no loss in range.
//
//          bool: bools store either true or false within a single bit
//
//        symbol: symbols store a SYMBOL value in the upper 4 bytes of the value.
//
//          null: encodes the singleton null type. can also carry a 1 byte error code
//                that is only visible to the runtime when explicitly checking for it.
//                this allows the null value to be used to pass internal status codes,
//                without interfering with the values that are visible to the user.
//
//  small string: small strings encode a string value of up to 7 bytes in the upper part of the value.
//                the remaining 4 bits next to the tag are used to store the amount of characters
//                in the string. small strings can only store data that is valid utf-8 text.
//                length values above 7 are invalid.
//
//   small bytes: same as a small-string except data must not be valid utf-8.
class RawValue {
public:
  COMMON_RAW_OBJECT(Value);

  RawValue() : m_raw(kTagNull) {}
  explicit RawValue(uintptr_t raw) : m_raw(raw) {}
  RawValue(const RawValue& other) : m_raw(other.raw()) {}

  bool operator==(const RawValue& other) const {
    return m_raw == other.raw();
  }
  bool operator!=(const RawValue& other) const {
    return m_raw != other.raw();
  }

  RawValue& operator=(const RawValue& other) {
    m_raw = other.raw();
    return *this;
  }

  uintptr_t raw() const;
  bool is_error_ok() const;
  bool is_error_exception() const;
  bool is_error_not_found() const;
  bool is_error_out_of_bounds() const;
  ShapeId shape_id() const;
  ShapeId shape_id_not_object_int() const;
  bool truthyness() const;

#define TYPECHECK(name) bool is##name() const;
  TYPE_NAMES(TYPECHECK)
#undef TYPECHECK

  // tag masks
  static const uintptr_t kMaskInt       = 0b00000001;
  static const uintptr_t kMaskPointer   = 0b00000111;
  static const uintptr_t kMaskImmediate = 0b00001111;
  static const uintptr_t kMaskLowByte   = 0b11111111;
  static const uintptr_t kMaskLength    = 0b11110000;

  // tag bits
  static const uintptr_t kTagInt              = 0b00000000;
  static const uintptr_t kTagObject           = 0b00000001;
  static const uintptr_t kTagFloat            = 0b00000011;
  static const uintptr_t kTagBool             = 0b00000101;
  static const uintptr_t kTagSymbol           = 0b00000111;
  static const uintptr_t kTagNull             = 0b00001011;
  static const uintptr_t kTagErrorOk          = 0b00011011;
  static const uintptr_t kTagErrorException   = 0b00101011;
  static const uintptr_t kTagErrorNotFound    = 0b00111011;
  static const uintptr_t kTagErrorOutOfBounds = 0b01001011;
  static const uintptr_t kTagSmallString      = 0b00001101;
  static const uintptr_t kTagSmallBytes       = 0b00001111;

  // right shift amounts to decode values
  static const int kShiftInt = 1;
  static const int kShiftBool = 8;
  static const int kShiftSymbol = 32;
  static const int kShiftError = 4;
  static const int kShiftLength = 4;

  template <typename T>
  T rawCast() const {
    const RawValue* self = this;
    return *reinterpret_cast<const T*>(self);
  }

  void dump(std::ostream& out) const;

  friend std::ostream& operator<<(std::ostream& out, const RawValue& value);

protected:
  uintptr_t m_raw;
};

// immediate int
class RawInt : public RawValue {
public:
  COMMON_RAW_OBJECT(Int);

  int64_t value() const;

  static RawInt make(int64_t value);
  static RawInt make_truncate(int64_t value);

  static bool is_valid(int64_t value);

  inline static const int64_t kMinValue = -(int64_t{1} << 62);
  inline static const int64_t kMaxValue = (int64_t{1} << 62) - 1;
};
static const RawInt kZero = RawInt::make(0);
static const RawInt kOne = RawInt::make(1);
static const RawInt kTwo = RawInt::make(2);
static const RawInt kThree = RawInt::make(3);
static const RawInt kFour = RawInt::make(4);

// immediate double
class RawFloat : public RawValue {
public:
  COMMON_RAW_OBJECT(Float);

  double value() const;

  bool close_to(double other, double precision = std::numeric_limits<double>::epsilon()) const;
  bool close_to(RawFloat other, double precision = std::numeric_limits<double>::epsilon()) const {
    return close_to(other.value(), precision);
  }

  static RawFloat make(double value);
};
static const RawFloat kNaN = RawFloat::make(NAN);
static const RawFloat kInfinity = RawFloat::make(INFINITY);
static const RawFloat kNegInfinity = RawFloat::make(-INFINITY);

// immediate bool
class RawBool : public RawValue {
public:
  COMMON_RAW_OBJECT(Bool);

  bool value() const;

  static RawBool make(bool value);
};
static const RawBool kTrue = RawBool::make(true);
static const RawBool kFalse = RawBool::make(false);

// immediate symbol
class RawSymbol : public RawValue {
public:
  COMMON_RAW_OBJECT(Symbol);

  SYMBOL value() const;

  static RawSymbol make(SYMBOL symbol);
};

// immediate string
class RawSmallString : public RawValue {
public:
  COMMON_RAW_OBJECT(SmallString);

  size_t length() const;
  static const char* data(const RawSmallString* value);
  SYMBOL hashcode() const;

  static RawSmallString make_from_cp(uint32_t cp);
  static RawSmallString make_from_str(const std::string& string);
  static RawSmallString make_from_cstr(const char* value);
  static RawSmallString make_from_memory(const char* value, size_t length);
  static RawSmallString make_empty();

  static const size_t kMaxLength = 7;
};
static const RawSmallString kEmptyString = RawSmallString::make_empty();

// immediate bytes
class RawSmallBytes : public RawValue {
public:
  COMMON_RAW_OBJECT(SmallBytes);

  size_t length() const;
  static const uint8_t* data(const RawSmallBytes* value);
  SYMBOL hashcode() const;

  static RawSmallBytes make_from_memory(const uint8_t* value, size_t length);
  static RawSmallBytes make_empty();

  static const size_t kMaxLength = 7;
};
static const RawSmallBytes kEmptyBytes = RawSmallBytes::make_empty();

// immediate null
class RawNull : public RawValue {
public:
  COMMON_RAW_OBJECT(Null);

  ErrorId error_code() const;

  static RawNull make();
  static RawNull make_error(ErrorId id);
};
static const RawNull kNull = RawNull::make();
static const RawNull kErrorNone = RawNull::make_error(ErrorId::kErrorNone);
static const RawNull kErrorOk = RawNull::make_error(ErrorId::kErrorOk);
static const RawNull kErrorException = RawNull::make_error(ErrorId::kErrorException);
static const RawNull kErrorNotFound = RawNull::make_error(ErrorId::kErrorNotFound);
static const RawNull kErrorOutOfBounds = RawNull::make_error(ErrorId::kErrorOutOfBounds);

// common super class for RawSmallString, LargeString and HugeString
class RawString : public RawValue {
public:
  COMMON_RAW_OBJECT(String);

  size_t length() const;
  static const char* data(const RawString* value);
  SYMBOL hashcode() const;

  static int32_t compare(RawString base, const char* data, size_t length);
  static int32_t compare(RawString base, const char* data) {
    return compare(base, data, std::strlen(data));
  }
  static int32_t compare(RawString base, RawString other) {
    return compare(base, RawString::data(&other), other.length());
  }
};

// common super class for RawSmallBytes, LargeBytes and HugeBytes
class RawBytes : public RawValue {
public:
  COMMON_RAW_OBJECT(Bytes);

  size_t length() const;
  static const uint8_t* data(const RawBytes* value);
  SYMBOL hashcode() const;

  static bool compare(RawBytes base, const uint8_t* data, size_t length);
  static bool compare(RawBytes base, RawBytes other) {
    return compare(base, RawBytes::data(&other), other.length());
  }
};

// heap object base type
class RawObject : public RawValue {
public:
  COMMON_RAW_OBJECT(Object);

  uintptr_t address() const;
  uintptr_t base_address() const;
  size_t size() const;
  ShapeId shape_id() const;

  ObjectHeader* header() const;

  static RawObject make_from_ptr(uintptr_t address);
};

// base class for all objects that store immediate data
class RawData : public RawObject {
public:
  COMMON_RAW_OBJECT(Data);

  size_t length() const;
  const uint8_t* data() const;
  SYMBOL hashcode() const;
};

// bytes stored on managed heap
class RawLargeBytes : public RawData {
public:
  COMMON_RAW_OBJECT(LargeBytes);
};

// string stored on managed heap
class RawLargeString : public RawData {
public:
  COMMON_RAW_OBJECT(LargeString);
  const char* data() const;
};

// base type for all types that use the shape system
class RawInstance : public RawObject {
public:
  COMMON_RAW_OBJECT(Instance);

  int64_t size() const;

  RawValue field_at(int64_t index) const;
  void set_field_at(int64_t index, RawValue value);

  uintptr_t pointer_at(int64_t index) const;
  void set_pointer_at(int64_t index, uintptr_t pointer);
  void set_pointer_at(int64_t index, void* pointer) {
    set_pointer_at(index, bitcast<uintptr_t>(pointer));
  }

  int64_t int_at(int64_t index) const;
  void set_int_at(int64_t index, int64_t value);

  static const size_t kFieldCount = 0;
  static const size_t kSize = kFieldCount * kPointerSize;
};

// bytes stored on c++ heap
class RawHugeBytes : public RawInstance {
public:
  COMMON_RAW_OBJECT(HugeBytes);

  size_t length() const;
  const uint8_t* data() const;
  SYMBOL hashcode() const;

  static const size_t kDataPointerOffset = 0;
  static const size_t kDataLengthOffset = 1;
  static const size_t kFieldCount = RawInstance::kFieldCount + 2;
  static const size_t kSize = kFieldCount * kPointerSize;
};

// string stored on c++ heap
class RawHugeString : public RawInstance {
public:
  COMMON_RAW_OBJECT(HugeString);

  size_t length() const;
  const char* data() const;
  SYMBOL hashcode() const;

  static const size_t kDataPointerOffset = 0;
  static const size_t kDataLengthOffset = 1;
  static const size_t kFieldCount = RawInstance::kFieldCount + 2;
  static const size_t kSize = kFieldCount * kPointerSize;
};

// tuple
class RawTuple : public RawInstance {
public:
  COMMON_RAW_OBJECT(Tuple);
};

// type
class RawType : public RawInstance {
public:
  COMMON_RAW_OBJECT(Type);
};

// instance shape
//
// describes the shape of an object
class RawShape : public RawInstance {
public:
  COMMON_RAW_OBJECT(Shape);
};

// function with bound context and bytecode
struct SharedFunctionInfo;
class RawFunction : public RawInstance {
public:
  COMMON_RAW_OBJECT(Function);

  RawValue context() const;
  void set_context(RawValue context);

  SharedFunctionInfo* shared_info() const;
  void set_shared_info(SharedFunctionInfo* function);

  static const size_t kFrameContextOffset = 0;
  static const size_t kSharedInfoOffset = 1;
  static const size_t kFieldCount = RawInstance::kFieldCount + 2;
  static const size_t kSize = kFieldCount * kPointerSize;
};

// runtime fiber
class Thread;
class RawFiber : public RawInstance {
public:
  COMMON_RAW_OBJECT(Fiber);

  Thread* thread() const;
  void set_thread(Thread* thread);

  RawFunction function() const;
  void set_function(RawFunction function);

  static const size_t kThreadPointerOffset = 0;
  static const size_t kFunctionOffset = 1;
  static const size_t kFieldCount = RawInstance::kFieldCount + 2;
  static const size_t kSize = kFieldCount * kPointerSize;
};

// base class of all exceptions
class RawException : public RawInstance {
public:
  COMMON_RAW_OBJECT(Exception);
};

}
