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

#include <cmath>
#include <cstdint>
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
  V(Data)                   \
  V(Bytes)                  \
  V(String)

// types which store raw bytes on the heap
#define DATA_TYPE_NAMES(V) \
  V(LargeString)           \
  V(LargeBytes)            \
  V(Tuple)

// types which store their data via the shape system
#define INSTANCE_TYPE_NAMES(V) \
  V(Instance)                  \
  V(HugeBytes)                 \
  V(HugeString)                \
  V(Class)                     \
  V(Shape)                     \
  V(Function)                  \
  V(BuiltinFunction)           \
  V(Fiber)

// types which store their data via the user class system
#define USER_INSTANCE_TYPE_NAMES(V) V(Exception)

#define TYPE_NAMES(V)     \
  IMMEDIATE_TYPE_NAMES(V) \
  SUPER_TYPE_NAMES(V)     \
  DATA_TYPE_NAMES(V)      \
  INSTANCE_TYPE_NAMES(V)  \
  USER_INSTANCE_TYPE_NAMES(V)

enum class ShapeId : uint32_t
{
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
  kFirstDataObject = DATA_TYPE_NAMES(GET_FIRST) 1,
  kLastDataObject = DATA_TYPE_NAMES(GET_LAST) 1,
  INSTANCE_TYPE_NAMES(SHAPE_ID)
  USER_INSTANCE_TYPE_NAMES(SHAPE_ID)
  kFirstBuiltinShapeId = INSTANCE_TYPE_NAMES(GET_FIRST) 1,
  kFirstKnownUserShapeId = USER_INSTANCE_TYPE_NAMES(GET_FIRST) 1,
  kLastKnownUserShapeId = USER_INSTANCE_TYPE_NAMES(GET_LAST) 1,
  kLastBuiltinShapeId = USER_INSTANCE_TYPE_NAMES(GET_LAST) 1,
#undef GET_LAST
#undef GET_FIRST
#undef SHAPE_ID
  // clang-format on

  kMaxShapeId = (uint32_t{ 1 } << 20) - 1,
  kMaxShapeCount = uint32_t{ 1 } << 20
};

// maps the lowest 4 bits of an immediate value to a shape id
// this table is consulted after the runtime has determined that the value
// is not a pointer to a heap object
static const ShapeId kShapeImmediateTagMapping[16] = {
  /* 0b0000 */ ShapeId::kInt,
  /* 0b0001 */ ShapeId::kMaxShapeCount,  // heap objects
  /* 0b0010 */ ShapeId::kInt,
  /* 0b0011 */ ShapeId::kFloat,
  /* 0b0100 */ ShapeId::kInt,
  /* 0b0101 */ ShapeId::kBool,
  /* 0b0110 */ ShapeId::kInt,
  /* 0b0111 */ ShapeId::kSymbol,
  /* 0b1000 */ ShapeId::kInt,
  /* 0b1001 */ ShapeId::kMaxShapeCount,  // heap objects
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
bool is_builtin_shape(ShapeId shape_id);
bool is_user_shape(ShapeId shape_id);

// align a size to some alignment
size_t align_to_size(size_t size, size_t alignment);

// internal error values used only internally in the runtime
// this value is encoded in the null value
// limit of 16 error codes
enum class ErrorId : uint8_t
{
  kErrorNone = 0,
  kErrorOk,
  kErrorNotFound,
  kErrorOutOfBounds,
  kErrorException,
  kErrorReadOnly,
  kErrorNoBaseClass = kErrorNotFound
};

// forward declare Raw types
#define FORWARD_DECL(name) class Raw##name;
TYPE_NAMES(FORWARD_DECL)
#undef FORWARD_DECL

// every heap allocated object gets prefixed with a object header
class ObjectHeader {
  friend class RawObject;

public:
  CHARLY_NON_HEAP_ALLOCATABLE(ObjectHeader);

  enum Flag : uint8_t
  {
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
  utils::TinyLock m_lock;                          // per-object small lock
  atomic<Flag> m_flags;                            // flags
  atomic<SYMBOL> m_hashcode;                       // cached hashcode of object / string
  atomic<uint32_t> m_forward_offset;               // contains offset of forwarded copy during concurrent GC
};
static_assert((sizeof(ObjectHeader) == 16), "invalid object header size");

static const size_t kObjectAlignment = 8;
static const size_t kObjectHeaderMaxCount = 65535;
static const size_t kObjectHeaderMaxSurvivorCount = 15;

#define COMMON_RAW_OBJECT(name)                                         \
  static Raw##name cast(RawValue value) {                               \
    DCHECK(value.is##name(), "invalid object type, expected %", #name); \
    return value.rawCast<Raw##name>();                                  \
  }                                                                     \
  static Raw##name cast(const RawValue* value) {                        \
    return cast(*value);                                                \
  }                                                                     \
  static Raw##name cast(uintptr_t value) {                              \
    return cast(RawValue(value));                                       \
  }                                                                     \
  static Raw##name unsafe_cast(RawValue value) {                        \
    return value.rawCast<Raw##name>();                                  \
  }                                                                     \
  static Raw##name unsafe_cast(const RawValue* value) {                 \
    return unsafe_cast(*value);                                         \
  }                                                                     \
  static Raw##name unsafe_cast(uintptr_t value) {                       \
    return unsafe_cast(RawValue(value));                                \
  }                                                                     \
  static bool value_is_type(RawValue value) {                           \
    return value.is##name();                                            \
  }                                                                     \
  Raw##name& operator=(RawValue other) {                                \
    m_raw = other.rawCast<Raw##name>().raw();                           \
    return *this;                                                       \
  }                                                                     \
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

  uintptr_t raw() const;
  bool is_error() const;
  bool is_error_ok() const;
  bool is_error_exception() const;
  bool is_error_not_found() const;
  bool is_error_out_of_bounds() const;
  bool is_error_read_only() const;
  bool is_error_no_base_class() const;
  ShapeId shape_id() const;
  ShapeId shape_id_not_object_int() const;
  bool truthyness() const;

#define TYPECHECK(name) bool is##name() const;
  TYPE_NAMES(TYPECHECK)
#undef TYPECHECK

  // tag masks
  static const uintptr_t kMaskInt = 0b00000001;
  static const uintptr_t kMaskPointer = 0b00000111;
  static const uintptr_t kMaskImmediate = 0b00001111;
  static const uintptr_t kMaskLowByte = 0b11111111;
  static const uintptr_t kMaskLength = 0b11110000;

  // tag bits
  static const uintptr_t kTagInt = 0b00000000;
  static const uintptr_t kTagObject = 0b00000001;
  static const uintptr_t kTagFloat = 0b00000011;
  static const uintptr_t kTagBool = 0b00000101;
  static const uintptr_t kTagSymbol = 0b00000111;
  static const uintptr_t kTagNull = 0b00001011;
  static const uintptr_t kTagErrorOk = ((int)ErrorId::kErrorOk << 4) | kTagNull;
  static const uintptr_t kTagErrorException = ((int)ErrorId::kErrorException << 4) | kTagNull;
  static const uintptr_t kTagErrorNotFound = ((int)ErrorId::kErrorNotFound << 4) | kTagNull;
  static const uintptr_t kTagErrorOutOfBounds = ((int)ErrorId::kErrorOutOfBounds << 4) | kTagNull;
  static const uintptr_t kTagErrorReadOnly = ((int)ErrorId::kErrorReadOnly << 4) | kTagNull;
  static const uintptr_t kTagErrorNoBaseClass = ((int)ErrorId::kErrorNoBaseClass << 4) | kTagNull;
  static const uintptr_t kTagSmallString = 0b00001101;
  static const uintptr_t kTagSmallBytes = 0b00001111;

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

  void to_string(std::ostream& out) const;

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

  inline static const int64_t kMinValue = -(int64_t{ 1 } << 62);
  inline static const int64_t kMaxValue = (int64_t{ 1 } << 62) - 1;
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

  operator SYMBOL() const {
    return value();
  }

  static RawSymbol make(SYMBOL symbol);
  static RawSymbol make(const char* string) {
    return make(SYM(string));
  }
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
static const RawNull kErrorReadOnly = RawNull::make_error(ErrorId::kErrorReadOnly);
static const RawNull kErrorNoBaseClass = RawNull::make_error(ErrorId::kErrorNoBaseClass);

// common super class for RawSmallString, LargeString and HugeString
class RawString : public RawValue {
public:
  COMMON_RAW_OBJECT(String);

  size_t length() const;
  static const char* data(const RawString* value);
  SYMBOL hashcode() const;

  std::string str() const;
  std::string_view view() const;

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
  void* address_voidptr() const;
  uintptr_t base_address() const;
  ShapeId shape_id() const;

  ObjectHeader* header() const;

  void lock() const;
  void unlock() const;
  bool is_locked() const;

  static RawObject make_from_ptr(uintptr_t address);
};

// base class for all objects that store immediate data
class RawData : public RawObject {
public:
  COMMON_RAW_OBJECT(Data);

  size_t length() const;
  const uint8_t* data() const;
  SYMBOL hashcode() const;

  // FIXME: remove magic number
  // 1024 bytes - 16 bytes for the header
  static const size_t kMaxLength = 1008;
};

// string stored on managed heap
class RawLargeString : public RawData {
public:
  COMMON_RAW_OBJECT(LargeString);
  const char* data() const;
};

// bytes stored on managed heap
class RawLargeBytes : public RawData {
public:
  COMMON_RAW_OBJECT(LargeBytes);
};

// tuple
class RawTuple : public RawData {
public:
  COMMON_RAW_OBJECT(Tuple);

  uint32_t size() const;

  template <typename R = RawValue>
  R field_at(uint32_t index) const {
    DCHECK(index < size());
    RawValue* data = bitcast<RawValue*>(address());
    return R::cast(data[index]);
  }

  template <typename T = RawValue>
  void set_field_at(uint32_t index, T value) {
    DCHECK(index < size());
    RawValue* data = bitcast<RawValue*>(address());
    data[index] = RawValue::cast(value);
  }
};

// base type for all types that use the shape system
class RawInstance : public RawObject {
public:
  COMMON_RAW_OBJECT(Instance);

  uint32_t field_count() const;

  // check wether this instance is or inherits from a given shape
  bool is_instance_of(ShapeId id);

  RawValue klass_field() const;
  void set_klass_field(RawValue klass);

  template <typename R = RawValue>
  R field_at(uint32_t index) const {
    DCHECK(index < field_count());
    RawValue* data = bitcast<RawValue*>(address());
    return R::cast(data[index]);
  }

  template <typename T = RawValue>
  void set_field_at(uint32_t index, T value) {
    DCHECK(index < field_count());
    RawValue* data = bitcast<RawValue*>(address());
    data[index] = RawValue::cast(value);
  }

  uintptr_t pointer_at(int64_t index) const;
  void set_pointer_at(int64_t index, uintptr_t pointer);
  void set_pointer_at(int64_t index, const void* pointer) {
    set_pointer_at(index, bitcast<uintptr_t>(const_cast<void*>(pointer)));
  }

  int64_t int_at(int64_t index) const;
  void set_int_at(int64_t index, int64_t value);

  enum
  {
    kKlassOffset = 0,
    kFieldCount
  };

  static const size_t kMaximumFieldCount = 256;
  static const size_t kSize = kFieldCount * kPointerSize;
};

// bytes stored on c++ heap
class RawHugeBytes : public RawInstance {
public:
  COMMON_RAW_OBJECT(HugeBytes);

  SYMBOL hashcode() const;

  const uint8_t* data() const;
  void set_data(const uint8_t* data);

  size_t length() const;
  void set_length(size_t length);

  enum
  {
    kDataPointerOffset = RawInstance::kFieldCount,
    kDataLengthOffset,
    kFieldCount
  };
  static const size_t kSize = RawInstance::kSize + kFieldCount * kPointerSize;
};

// string stored on c++ heap
class RawHugeString : public RawInstance {
public:
  COMMON_RAW_OBJECT(HugeString);

  SYMBOL hashcode() const;

  const char* data() const;
  void set_data(const char* data);

  size_t length() const;
  void set_length(size_t length);

  enum
  {
    kDataPointerOffset = RawInstance::kFieldCount,
    kDataLengthOffset,
    kFieldCount
  };
  static const size_t kSize = RawInstance::kSize + kFieldCount * kPointerSize;
};

// class
class RawClass : public RawInstance {
public:
  COMMON_RAW_OBJECT(Class);

  enum
  {
    kFlagNone = 0,
    kFlagFinal = 1,
    kFlagNonConstructable = 2
  };

  uint8_t flags() const;
  void set_flags(uint8_t flags);

  RawTuple ancestor_table() const;
  void set_ancestor_table(RawTuple ancestor_table);

  RawSymbol name() const;
  void set_name(RawSymbol name);

  RawValue parent() const;
  void set_parent(RawValue parent);

  RawShape shape_instance() const;
  void set_shape_instance(RawShape shape);

  RawTuple function_table() const;
  void set_function_table(RawTuple function_table);

  RawValue constructor() const;
  void set_constructor(RawValue constructor);

  RawValue lookup_function(SYMBOL name) const;

  enum
  {
    kFlagsOffset = RawInstance::kFieldCount,
    kAncestorTableOffset,
    kNameOffset,
    kParentOffset,
    kShapeOffset,
    kFunctionTableOffset,
    kConstructorOffset,
    kFieldCount
  };
  static const size_t kSize = RawInstance::kSize + kFieldCount * kPointerSize;
};

// instance shape
//
// describes the shape of an object
class RawShape : public RawInstance {
public:
  COMMON_RAW_OBJECT(Shape);

  ShapeId own_shape_id() const;
  void set_own_shape_id(ShapeId id);

  RawValue parent() const;
  void set_parent(RawValue parent);

  RawTuple keys() const;
  void set_keys(RawTuple keys);

  enum
  {
    kKeyFlagNone = 0,
    kKeyFlagInternal = 1,  // keys that cannot get accessed via charly code
    kKeyFlagReadOnly = 2,  // keys that cannot be assigned to
    kKeyFlagPrivate = 4    // keys that can only be accessed from within class functions
  };
  static RawInt encode_shape_key(SYMBOL symbol, uint8_t flags = kKeyFlagNone);
  static void decode_shape_key(RawInt encoded, SYMBOL& symbol_out, uint8_t& flags_out);

  enum : uint8_t
  {
    kAdditionsKeyOffset = 0,
    kAdditionsNextOffset
  };

  RawTuple additions() const;
  void set_additions(RawTuple additions);

  // returns the found offset of a symbol
  // returns -1 if the symbol could not be found
  struct LookupResult {
    bool found;
    uint32_t offset;
    SYMBOL key;
    uint8_t flags;

    bool is_internal() const {
      return flags & kKeyFlagInternal;
    }

    bool is_read_only() const {
      return flags & kKeyFlagReadOnly;
    }

    bool is_private() const {
      return flags & kKeyFlagPrivate;
    }
  };
  LookupResult lookup_symbol(SYMBOL symbol) const;

  enum
  {
    kOwnShapeIdOffset = RawInstance::kFieldCount,
    kParentShapeOffset,
    kKeysOffset,
    kAdditionsOffset,
    kFieldCount
  };
  static const size_t kSize = RawInstance::kSize + kFieldCount * kPointerSize;
};

// function with bound context and bytecode
struct SharedFunctionInfo;
class RawFunction : public RawInstance {
public:
  COMMON_RAW_OBJECT(Function);

  RawSymbol name() const;
  void set_name(RawSymbol name);

  RawValue context() const;
  void set_context(RawValue context);

  RawValue saved_self() const;
  void set_saved_self(RawValue context);

  RawValue host_class() const;
  void set_host_class(RawValue host_class);

  SharedFunctionInfo* shared_info() const;
  void set_shared_info(SharedFunctionInfo* function);

  enum
  {
    kContextParentOffset = 0,
    kContextSelfOffset,
    kContextHeapVariablesOffset
  };

  enum
  {
    kNameOffset = RawInstance::kFieldCount,
    kFrameContextOffset,
    kSavedSelfOffset,
    kHostClassOffset,
    kSharedInfoOffset,
    kFieldCount
  };
  static const size_t kSize = RawInstance::kSize + kFieldCount * kPointerSize;
};

// builtin function implemented in c++
class Thread;
typedef RawValue (*BuiltinFunctionType)(Thread*, const RawValue*, uint8_t);
class RawBuiltinFunction : public RawInstance {
public:
  COMMON_RAW_OBJECT(BuiltinFunction);

  BuiltinFunctionType function() const;
  void set_function(BuiltinFunctionType function);

  RawSymbol name() const;
  void set_name(RawSymbol symbol);

  uint8_t argc() const;
  void set_argc(uint8_t argc);

  enum
  {
    kFunctionPtrOffset = RawInstance::kFieldCount,
    kNameOffset,
    kArgcOffset,
    kFieldCount
  };
  static const size_t kSize = RawInstance::kSize + kFieldCount * kPointerSize;
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

  RawValue context() const;
  void set_context(RawValue context);

  RawValue arguments() const;
  void set_arguments(RawValue arguments);

  RawValue result() const;
  void set_result(RawValue result);

  bool has_finished() const;

  enum
  {
    kThreadPointerOffset = RawInstance::kFieldCount,
    kFunctionOffset,
    kSelfOffset,
    kArgumentsOffset,
    kResultOffset,
    kFieldCount
  };
  static const size_t kSize = RawInstance::kSize + kFieldCount * kPointerSize;
};

// user exception instance
class RawException : public RawInstance {
public:
  COMMON_RAW_OBJECT(Exception);

  RawValue message() const;
  void set_message(RawValue value);

  RawTuple stack_trace() const;
  void set_stack_trace(RawTuple stack_trace);

  enum
  {
    kMessageOffset = RawInstance::kFieldCount,
    kStackTraceOffset,
    kFieldCount
  };
  static const size_t kSize = kFieldCount * kPointerSize;
};

}  // namespace charly::core::runtime
