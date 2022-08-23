/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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
#include <vector>

#include "charly.h"
#include "symbol.h"

#include "charly/utils/buffer.h"

#pragma once

namespace charly::core::compiler {
struct CompilationUnit;
}

namespace charly::core::runtime {

class Thread;
struct HeapRegion;
struct SharedFunctionInfo;

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
  V(Fiber)                     \
  V(Future)                    \
  V(Exception)                 \
  V(ImportException)

#define TYPE_NAMES(V)     \
  IMMEDIATE_TYPE_NAMES(V) \
  SUPER_TYPE_NAMES(V)     \
  DATA_TYPE_NAMES(V)      \
  INSTANCE_TYPE_NAMES(V)

// clang-format off
enum class ShapeId : uint32_t {
  // immediate types
  //
  // the shape id of any immediate type can be determined by checking the
  // lowest 4 bits.
  kInt = 0,
  kFloat = 5,
  kBool = 7,
  kSymbol = 9,
  kNull = 11,
  kSmallString = 13,
  kSmallBytes = 15,

  // object types
#define SHAPE_ID(name) k##name,
  DATA_TYPE_NAMES(SHAPE_ID)
  INSTANCE_TYPE_NAMES(SHAPE_ID)
#undef SHAPE_ID

  kFirstUserDefinedShapeId,
  kMaxShapeId = (uint32_t{ 1 } << 20) - 1,
  kMaxShapeCount = uint32_t{ 1 } << 20,
  kLastImmediateShape = kSmallBytes,
#define GET_FIRST(name) k##name + 0 *
#define GET_LAST(name) 0 + k##name*
  kFirstDataObject = DATA_TYPE_NAMES(GET_FIRST) 1,
  kLastDataObject = DATA_TYPE_NAMES(GET_LAST) 1,
  kFirstBuiltinShapeId = INSTANCE_TYPE_NAMES(GET_FIRST) 1,
  kLastBuiltinShapeId = INSTANCE_TYPE_NAMES(GET_LAST) 1,
#undef GET_LAST
#undef GET_FIRST
};
// clang-format on

static_assert((size_t)ShapeId::kFirstBuiltinShapeId == 19, "invalid first user-defined shape id");
static_assert((size_t)ShapeId::kLastBuiltinShapeId == 29, "invalid first user-defined shape id");
static_assert((size_t)ShapeId::kFirstUserDefinedShapeId == 30, "invalid first user-defined shape id");

// maps the lowest 4 bits of an immediate value to a shape id
// this table is consulted after the runtime has determined that the value
// is not a pointer to a heap object
constexpr ShapeId kShapeImmediateTagMapping[16] = {
  /* 0b0000 */ ShapeId::kInt,
  /* 0b0001 */ ShapeId::kMaxShapeCount,  // old heap objects
  /* 0b0010 */ ShapeId::kInt,
  /* 0b0011 */ ShapeId::kMaxShapeCount,  // young heap objects
  /* 0b0100 */ ShapeId::kInt,
  /* 0b0101 */ ShapeId::kFloat,
  /* 0b0110 */ ShapeId::kInt,
  /* 0b0111 */ ShapeId::kBool,
  /* 0b1000 */ ShapeId::kInt,
  /* 0b1001 */ ShapeId::kSymbol,
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
bool is_shape_with_external_heap_pointers(ShapeId shape_id);

// align a size to some alignment
size_t align_to_size(size_t size, size_t alignment);

// internal error values used only internally in the runtime
// this value is encoded in the null value
// limit of 16 error codes
enum class ErrorId : uint8_t {
  kErrorNone = 0,
  kErrorOk,
  kErrorNotFound,
  kErrorOutOfBounds,
  kErrorException,
  kErrorReadOnly,
  kErrorNoBaseClass
};

static std::string kErrorCodeNames[] = {
  "None", "Ok", "NotFound", "OutOfBounds", "Exception", "ReadOnly", "NoBaseClass"
};

// forward declare Raw types
#define FORWARD_DECL(name) class Raw##name;
TYPE_NAMES(FORWARD_DECL)
#undef FORWARD_DECL

// every heap allocated object gets prefixed with a object header
class ObjectHeader {
  friend class RawObject;

public:
  enum Flag : uint8_t {
    kReachable = 1,        // object is reachable by GC
    kHasHashcode = 2,      // object has a cached hashcode
    kYoungGeneration = 4,  // object is in an eden or intermediate region
  };

  static void initialize_header(uintptr_t address, ShapeId shape_id, uint16_t count);

  static ObjectHeader* header_at_address(uintptr_t address) {
    auto* header = bitcast<ObjectHeader*>(address);
    DCHECK(header->validate_magic_number());
    return header;
  }

  HeapRegion* heap_region() const;
  RawObject object() const;
  uint32_t alloc_size() const;  // header + object + padding

  ShapeId shape_id() const;

  uint16_t count() const;
  bool cas_count(uint16_t old_count, uint16_t new_count);

  SYMBOL hashcode();
  bool cas_hashcode(SYMBOL old_hashcode, SYMBOL new_hashcode);

  uint8_t survivor_count() const;
  void increment_survivor_count();
  void clear_survivor_count();

  RawObject forward_target() const;
  bool has_forward_target() const;
  void set_forward_target(RawObject object);

  Flag flags() const;
  bool is_reachable() const;
  bool has_cached_hashcode() const;
  bool is_young_generation() const;
  void set_is_reachable();
  void set_has_cached_hashcode();
  void set_is_young_generation();
  void clear_is_reachable();
  void clear_has_cached_hashcode();
  void clear_is_young_generation();

private:
  static uint32_t encode_shape_and_survivor_count(ShapeId shape_id, uint8_t survivor_count);

private:
  // object header encoding
  //
  // name           bytes     bits      total
  // [ shape id ] : 3 bytes : 24 bits :  3 bytes
  // [ gc count ] : 1 byte  :  8 bits :  4 bytes
  // [ count    ] : 2 bytes : 16 bits :  6 bytes
  // [ lock     ] : 1 byte  :  8 bits :  7 bytes
  // [ flags    ] : 1 byte  :  8 bits :  8 bytes
  // [ hashcode ] : 4 bytes : 32 bits : 12 bytes
  // [ forward  ] : 4 bytes : 32 bits : 16 bytes

  static constexpr uint32_t kMaskShape = 0x00FFFFFF;
  static constexpr uint32_t kMaskSurvivorCount = 0xFF000000;
  static constexpr uint32_t kShiftSurvivorCount = 24;

  atomic<uint32_t> m_shape_id_and_survivor_count;  // shape id and survivor count
  atomic<uint16_t> m_count;                        // count field
  utils::TinyLock m_lock;                          // per-object small lock
  atomic<Flag> m_flags;                            // flags
  atomic<SYMBOL> m_hashcode;                       // cached hashcode of object / string
  atomic<uint32_t> m_forward_target;               // forwarded heap offset (multiple of 16 bytes)

#ifndef NDEBUG
  atomic<size_t> m_magic1;
  atomic<size_t> m_magic2;

  static constexpr size_t kMagicNumber1 = 0xcafebeefdeadbeef;
  static constexpr size_t kMagicNumber2 = 0x1234abcd5678a1a1;
#endif

public:
  bool validate_magic_number() const {
#ifdef NDEBUG
    return true;
#else
    return m_magic1 == kMagicNumber1 && m_magic2 == kMagicNumber2;
#endif
  }
};

constexpr size_t kObjectAlignment = 16;
constexpr size_t kObjectHeaderMaxCount = 0xffff;
constexpr size_t kObjectHeaderMaxSurvivorCount = 0xff;

static_assert((sizeof(ObjectHeader) % kObjectAlignment == 0), "invalid object header size");

#define COMMON_RAW_OBJECT(name)                                                       \
  static Raw##name cast(RawValue value) {                                             \
    DCHECK(value.is##name(), "invalid object type, got %, expected %", value, #name); \
    return value.rawCast<Raw##name>();                                                \
  }                                                                                   \
  static Raw##name cast(const RawValue* value) {                                      \
    return cast(*value);                                                              \
  }                                                                                   \
  static Raw##name cast(uintptr_t value) {                                            \
    return cast(RawValue(value));                                                     \
  }                                                                                   \
  static Raw##name unsafe_cast(RawValue value) {                                      \
    return value.rawCast<Raw##name>();                                                \
  }                                                                                   \
  static Raw##name unsafe_cast(const RawValue* value) {                               \
    return unsafe_cast(*value);                                                       \
  }                                                                                   \
  static Raw##name unsafe_cast(uintptr_t value) {                                     \
    return unsafe_cast(RawValue(value));                                              \
  }                                                                                   \
  static bool value_is_type(RawValue value) {                                         \
    return value.is##name();                                                          \
  }                                                                                   \
  Raw##name& operator=(RawValue other) {                                              \
    m_raw = other.rawCast<Raw##name>().raw();                                         \
    return *this;                                                                     \
  }

// the RawValue class represents a single pointer-tagged value.
// the tagging scheme supports the following value types:
//
// ******** ******** ******** ******** ******** ******** ******** ******* 0  int
// ******** ******** ******** ******** ******** ******** ******** **** 0001  old heap object
// ******** ******** ******** ******** ******** ******** ******** **** 0011  young heap object
// DDDDDDDD DDDDDDDD DDDDDDDD DDDDDDDD DDDDDDDD DDDDDDDD DDDDDDDD DDDD 0101  float
// ******** ******** ******** ******** ******** ******** *******B **** 0111  bool
// SSSSSSSS SSSSSSSS SSSSSSSS SSSSSSSS ******** ******** ******** **** 1001  symbol
// ******** ******** ******** ******** ******** ******** ******** EEEE 1011  null (or internal error / header)
// SSSSSSSS SSSSSSSS SSSSSSSS SSSSSSSS SSSSSSSS SSSSSSSS SSSSSSSS LLLL 1101  small string
// BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB LLLL 1111  small bytes
//
// this scheme supports immediate 63-bit signed integers. the integer itself is encoded
// and decoded by shifting it to the left or right by 1 bit. if the LSB of the value
// is set to 0, it's always an immediate encoded int.
//
// values that have their LSB set to 1 can be either pointers to heap objects or another
// immediately encoded value. testing the last 4 bits allows us to determine if the value
// is a pointer or another immediate. encoded pointers must always be aligned to at least 16 bytes.
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
  bool is_old_pointer() const;
  bool is_young_pointer() const;

  RawClass klass(Thread*) const;
  RawSymbol klass_name(Thread*) const;

#define TYPECHECK(name) bool is##name() const;
  TYPE_NAMES(TYPECHECK)
#undef TYPECHECK

  bool isNumber() const;
  int64_t int_value() const;
  double double_value() const;

  // tag masks
  static constexpr uintptr_t kMaskInt = 0x1;
  static constexpr uintptr_t kMaskImmediate = 0x0f;
  static constexpr uintptr_t kMaskLowByte = 0xff;
  static constexpr uintptr_t kMaskLength = 0xf0;

  // tag bits
  static constexpr uintptr_t kTagInt = 0;
  static constexpr uintptr_t kTagOldObject = 0b0001;
  static constexpr uintptr_t kTagYoungObject = 0b0011;
  static constexpr uintptr_t kTagFloat = 0b0101;
  static constexpr uintptr_t kTagBool = 0b0111;
  static constexpr uintptr_t kTagSymbol = 0b1001;
  static constexpr uintptr_t kTagNull = 0b1011;
  static constexpr uintptr_t kTagErrorOk = ((int)ErrorId::kErrorOk << 4) | kTagNull;
  static constexpr uintptr_t kTagErrorException = ((int)ErrorId::kErrorException << 4) | kTagNull;
  static constexpr uintptr_t kTagErrorNotFound = ((int)ErrorId::kErrorNotFound << 4) | kTagNull;
  static constexpr uintptr_t kTagErrorOutOfBounds = ((int)ErrorId::kErrorOutOfBounds << 4) | kTagNull;
  static constexpr uintptr_t kTagErrorReadOnly = ((int)ErrorId::kErrorReadOnly << 4) | kTagNull;
  static constexpr uintptr_t kTagErrorNoBaseClass = ((int)ErrorId::kErrorNoBaseClass << 4) | kTagNull;
  static constexpr uintptr_t kTagSmallString = 0b1101;
  static constexpr uintptr_t kTagSmallBytes = 0b1111;

  // right shift amounts to decode values
  static constexpr int kShiftInt = 1;
  static constexpr int kShiftBool = 8;
  static constexpr int kShiftSymbol = 32;
  static constexpr int kShiftError = 4;
  static constexpr int kShiftLength = 4;

  template <typename T>
  T rawCast() const {
    const RawValue* self = this;
    return *reinterpret_cast<const T*>(self);
  }

  void to_string(std::ostream& out) const;
  void dump(std::ostream& out) const;

  std::string to_string() const {
    utils::Buffer buffer;
    to_string(buffer);
    return buffer.str();
  }

  std::string dump() const {
    utils::Buffer buffer;
    dump(buffer);
    return buffer.str();
  }

  friend std::ostream& operator<<(std::ostream& out, const RawValue& value);

protected:
  uintptr_t m_raw;
};

// immediate int
class RawInt : public RawValue {
public:
  COMMON_RAW_OBJECT(Int);

  int64_t value() const;
  uintptr_t external_pointer_value() const;

  static RawInt create(int64_t value);
  static RawInt create_truncate(int64_t value);
  static RawInt create_from_external_pointer(uintptr_t value);

  static bool is_valid(int64_t value);

  static constexpr int64_t kMinValue = -(int64_t{ 1 } << 62);
  static constexpr int64_t kMaxValue = (int64_t{ 1 } << 62) - 1;

  // uppermost bit of an external pointer must be 0
  static constexpr size_t kExternalPointerValidationMask = 0x8000000000000000;
};
static const RawInt kZero = RawInt::create(0);
static const RawInt kOne = RawInt::create(1);
static const RawInt kTwo = RawInt::create(2);
static const RawInt kThree = RawInt::create(3);
static const RawInt kFour = RawInt::create(4);

// immediate double
class RawFloat : public RawValue {
public:
  COMMON_RAW_OBJECT(Float);

  double value() const;

  bool close_to(double other, double precision = std::numeric_limits<double>::epsilon()) const;
  bool close_to(RawFloat other, double precision = std::numeric_limits<double>::epsilon()) const {
    return close_to(other.value(), precision);
  }

  static RawFloat create(double value);
};
static const RawFloat kNaN = RawFloat::create(NAN);
static const RawFloat kFloatZero = RawFloat::create(0.0);
static const RawFloat kInfinity = RawFloat::create(INFINITY);
static const RawFloat kNegInfinity = RawFloat::create(-INFINITY);

// immediate bool
class RawBool : public RawValue {
public:
  COMMON_RAW_OBJECT(Bool);

  bool value() const;

  static RawBool create(bool value);
};
static const RawBool kTrue = RawBool::create(true);
static const RawBool kFalse = RawBool::create(false);

// immediate symbol
class RawSymbol : public RawValue {
public:
  COMMON_RAW_OBJECT(Symbol);

  SYMBOL value() const;

  operator SYMBOL() const {
    return value();
  }

  static RawSymbol create(SYMBOL symbol);
  static RawSymbol create(const char* string) {
    return create(SYM(string));
  }
};

// immediate string
class RawSmallString : public RawValue {
public:
  COMMON_RAW_OBJECT(SmallString);

  size_t byte_length() const;
  static const char* data(const RawSmallString* value);
  SYMBOL hashcode() const;

  static RawSmallString create_from_cp(uint32_t cp);
  static RawSmallString create_from_str(const std::string& string);
  static RawSmallString create_from_cstr(const char* value);
  static RawSmallString create_from_memory(const char* value, size_t length);
  static RawSmallString create_empty();

  static constexpr size_t kMaxLength = 7;
};
static const RawSmallString kEmptyString = RawSmallString::create_empty();

// immediate bytes
class RawSmallBytes : public RawValue {
public:
  COMMON_RAW_OBJECT(SmallBytes);

  size_t length() const;
  static const uint8_t* data(const RawSmallBytes* value);
  SYMBOL hashcode() const;

  static RawSmallBytes create_from_memory(const uint8_t* value, size_t length);
  static RawSmallBytes create_empty();

  static constexpr size_t kMaxLength = 7;
};
static const RawSmallBytes kEmptyBytes = RawSmallBytes::create_empty();

// immediate null
class RawNull : public RawValue {
public:
  COMMON_RAW_OBJECT(Null);

  ErrorId error_code() const;

  static RawNull create();
  static RawNull create_error(ErrorId id);
};
static const RawNull kNull = RawNull::create();
static const RawNull kErrorNone = RawNull::create_error(ErrorId::kErrorNone);
static const RawNull kErrorOk = RawNull::create_error(ErrorId::kErrorOk);
static const RawNull kErrorException = RawNull::create_error(ErrorId::kErrorException);
static const RawNull kErrorNotFound = RawNull::create_error(ErrorId::kErrorNotFound);
static const RawNull kErrorOutOfBounds = RawNull::create_error(ErrorId::kErrorOutOfBounds);
static const RawNull kErrorReadOnly = RawNull::create_error(ErrorId::kErrorReadOnly);
static const RawNull kErrorNoBaseClass = RawNull::create_error(ErrorId::kErrorNoBaseClass);

// common super class for RawSmallString, LargeString and HugeString
class RawString : public RawValue {
public:
  COMMON_RAW_OBJECT(String);

  static RawString create(Thread*, const char* data, size_t size, SYMBOL hash);
  static RawString create(Thread*, const std::string& string);

  static RawString format(Thread* thread, const char* str) {
    return RawString::create(thread, str);
  }
  template <typename... T>
  static RawString format(Thread* thread, const char* str, const T&... args) {
    utils::Buffer buffer;
    buffer.write_formatted(str, std::forward<const T>(args)...);
    return RawString::acquire(thread, buffer);
  }

  // create a new string by acquiring ownership over an existing allocation
  static RawString acquire(Thread*, char* cstr, size_t size, SYMBOL hash);
  static RawString acquire(Thread*, utils::Buffer& buffer);

  size_t byte_length() const;
  static const char* data(const RawString* value);
  SYMBOL hashcode() const;

  std::string str() const;
  std::string_view view() const&;
  std::string_view view() const&& = delete;

  static int32_t compare(RawString base, const char* data, size_t length);
  static int32_t compare(RawString base, const char* data) {
    return compare(base, data, std::strlen(data));
  }
  static int32_t compare(RawString base, RawString other) {
    return compare(base, RawString::data(&other), other.byte_length());
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
  size_t count() const;

  bool contains_external_heap_pointers() const;

  ObjectHeader* header() const;

  void lock() const;
  void unlock() const;
  bool is_locked() const;

  template <typename R = RawValue>
  R& field_at(uint32_t index) const {
    DCHECK(isInstance() || isTuple());
    DCHECK(index < count());
    RawValue* data = bitcast<RawValue*>(address());
    RawValue* value = &data[index];
    R::value_is_type(*value);
    return *bitcast<R*>(value);
  }

  template <typename R = RawValue>
  R& first_field() const {
    return field_at<R>(0);
  }

  template <typename R = RawValue>
  R& last_field() const {
    DCHECK(count() > 0);
    return field_at<R>(count() - 1);
  }

  void set_field_at(uint32_t index, RawValue value) const;

  static RawObject create_from_ptr(uintptr_t address, bool is_young = true);
};

// base class for all objects that store immediate data
class RawData : public RawObject {
public:
  COMMON_RAW_OBJECT(Data);

  static RawData create(Thread*, ShapeId shape_id, size_t size);

  size_t length() const;
  const uint8_t* data() const;
  SYMBOL hashcode() const;
};

// string stored on managed heap
class RawLargeString : public RawData {
public:
  COMMON_RAW_OBJECT(LargeString);

  static RawLargeString create(Thread*, const char* data, size_t size, SYMBOL hash);

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

  static RawTuple create_empty(Thread*);
  static RawTuple create(Thread*, uint32_t count);
  static RawTuple create(Thread*, RawValue value);
  static RawTuple create(Thread*, RawValue value1, RawValue value2);
  static RawTuple concat_value(Thread*, RawTuple left, RawValue value);

  const RawValue* data() const;
  uint32_t size() const;
};

// base type for all types that use the shape system
class RawInstance : public RawObject {
public:
  COMMON_RAW_OBJECT(Instance);

  static RawInstance create(Thread*, ShapeId shape_id, size_t field_count, RawValue klass = kNull);
  static RawInstance create(Thread*, RawShape shape, RawValue klass = kNull);
  static RawInstance create(Thread*, RawClass klass);

  uint32_t field_count() const;

  // check wether this instance is or inherits from a given shape
  bool is_instance_of(ShapeId id) const;

  RawValue klass_field() const;
  void set_klass_field(RawValue klass) const;

  uintptr_t pointer_at(int64_t index) const;
  void set_pointer_at(int64_t index, const void* pointer) const;

  enum {
    kKlassOffset = 0,
    kFieldCount
  };

  static constexpr size_t kMaximumFieldCount = 256;
};

// bytes stored on c++ heap
class RawHugeBytes : public RawInstance {
public:
  COMMON_RAW_OBJECT(HugeBytes);

  SYMBOL hashcode() const;

  const uint8_t* data() const;
  void set_data(const uint8_t* data) const;

  size_t length() const;
  void set_length(size_t length) const;

  enum {
    kDataPointerOffset = RawInstance::kFieldCount,
    kDataLengthOffset,
    kFieldCount
  };
};

// string stored on c++ heap
class RawHugeString : public RawInstance {
public:
  COMMON_RAW_OBJECT(HugeString);

  static RawHugeString create(Thread*, const char* data, size_t size, SYMBOL hash);
  static RawHugeString acquire(Thread*, char* data, size_t size, SYMBOL hash);

  SYMBOL hashcode() const;

  const char* data() const;
  void set_data(const char* data) const;

  size_t byte_length() const;
  void set_byte_length(size_t length) const;

  enum {
    kDataPointerOffset = RawInstance::kFieldCount,
    kDataLengthOffset,
    kFieldCount
  };
};

// class
class RawClass : public RawInstance {
public:
  COMMON_RAW_OBJECT(Class);

  static RawValue create(Thread* thread,
                         SYMBOL name,
                         RawValue parent,
                         RawFunction constructor,
                         RawTuple member_props,
                         RawTuple member_funcs,
                         RawTuple static_prop_keys,
                         RawTuple static_prop_values,
                         RawTuple static_funcs,
                         uint32_t flags = 0);

  enum {
    kFlagNone = 0,
    kFlagFinal = 1,
    kFlagNonConstructable = 2,
    kFlagStatic = 4
  };

  uint8_t flags() const;
  void set_flags(uint8_t flags) const;

  RawTuple ancestor_table() const;
  void set_ancestor_table(RawTuple ancestor_table) const;

  RawSymbol name() const;
  void set_name(RawSymbol name) const;

  RawValue parent() const;
  void set_parent(RawValue parent) const;

  RawShape shape_instance() const;
  void set_shape_instance(RawShape shape) const;

  RawTuple function_table() const;
  void set_function_table(RawTuple function_table) const;

  RawValue constructor() const;
  void set_constructor(RawValue constructor) const;

  RawValue lookup_function(SYMBOL name) const;

  // checks if this class is the same as or inherits from another class
  bool is_instance_of(RawClass other) const;

  enum {
    kFlagsOffset = RawInstance::kFieldCount,
    kAncestorTableOffset,
    kNameOffset,
    kParentOffset,
    kShapeOffset,
    kFunctionTableOffset,
    kConstructorOffset,
    kFieldCount
  };
};

// instance shape
//
// describes the shape of an object
class RawShape : public RawInstance {
public:
  COMMON_RAW_OBJECT(Shape);

  static RawShape create(Thread*, RawValue parent, RawTuple key_table);
  static RawShape create(Thread*, RawValue parent, std::initializer_list<std::tuple<std::string, uint8_t>> keys);

  ShapeId own_shape_id() const;
  void set_own_shape_id(ShapeId id) const;

  RawValue parent() const;
  void set_parent(RawValue parent) const;

  RawTuple keys() const;
  void set_keys(RawTuple keys) const;

  enum {
    kKeyFlagNone = 0,
    kKeyFlagInternal = 1,  // keys that cannot get accessed via charly code
    kKeyFlagReadOnly = 2,  // keys that cannot be assigned to
    kKeyFlagPrivate = 4    // keys that can only be accessed from within class functions
  };
  static RawInt encode_shape_key(SYMBOL symbol, uint8_t flags = kKeyFlagNone);
  static void decode_shape_key(RawInt encoded, SYMBOL& symbol_out, uint8_t& flags_out);

  enum : uint8_t {
    kAdditionsKeyOffset = 0,
    kAdditionsNextOffset
  };

  RawTuple additions() const;
  void set_additions(RawTuple additions) const;

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

  enum {
    kOwnShapeIdOffset = RawInstance::kFieldCount,
    kParentShapeOffset,
    kKeysOffset,
    kAdditionsOffset,
    kFieldCount
  };
};

// function with bound context and bytecode
class RawFunction : public RawInstance {
public:
  COMMON_RAW_OBJECT(Function);

  static RawFunction create(Thread*, RawValue context, SharedFunctionInfo* shared_info, RawValue saved_self = kNull);

  RawSymbol name() const;
  void set_name(RawSymbol name) const;

  RawValue context() const;
  void set_context(RawValue context) const;

  RawValue saved_self() const;
  void set_saved_self(RawValue context) const;

  RawValue host_class() const;
  void set_host_class(RawValue host_class) const;

  RawValue overload_table() const;
  void set_overload_table(RawValue overload_table) const;

  SharedFunctionInfo* shared_info() const;
  void set_shared_info(SharedFunctionInfo* function) const;

  // check wether this specific function instance (ignoring overloads) would accept *argc* arguments
  bool check_accepts_argc(uint32_t argc) const;

  enum {
    kContextParentOffset = 0,
    kContextSelfOffset,
    kContextHeapVariablesOffset
  };

  enum {
    kNameOffset = RawInstance::kFieldCount,
    kFrameContextOffset,
    kSavedSelfOffset,
    kHostClassOffset,
    kOverloadTableOffset,
    kSharedInfoOffset,
    kFieldCount
  };
};

// builtin function implemented in c++
typedef RawValue (*BuiltinFunctionType)(Thread*, const RawValue*, uint8_t);
class RawBuiltinFunction : public RawInstance {
public:
  COMMON_RAW_OBJECT(BuiltinFunction);

  static RawBuiltinFunction create(Thread*, BuiltinFunctionType function, SYMBOL name, uint8_t argc);

  BuiltinFunctionType function() const;
  void set_function(BuiltinFunctionType function) const;

  RawSymbol name() const;
  void set_name(RawSymbol symbol) const;

  uint8_t argc() const;
  void set_argc(uint8_t argc) const;

  enum {
    kFunctionPtrOffset = RawInstance::kFieldCount,
    kNameOffset,
    kArgcOffset,
    kFieldCount
  };
};

// runtime fiber
class RawFiber : public RawInstance {
public:
  COMMON_RAW_OBJECT(Fiber);

  static RawFiber create(Thread*, RawFunction function, RawValue self, RawValue arguments);

  Thread* thread() const;
  void set_thread(Thread* thread) const;

  RawFunction function() const;
  void set_function(RawFunction function) const;

  RawValue context() const;
  void set_context(RawValue context) const;

  RawValue arguments() const;
  void set_arguments(RawValue arguments) const;

  RawFuture result_future() const;
  void set_result_future(RawFuture result_future) const;

  RawValue await(Thread*) const;

  enum {
    kThreadPointerOffset = RawInstance::kFieldCount,
    kFunctionOffset,
    kSelfOffset,
    kArgumentsOffset,
    kResultFutureOffset,
    kFieldCount
  };
};

class RawFuture : public RawInstance {
public:
  COMMON_RAW_OBJECT(Future);

  static RawFuture create(Thread*);

  // instead of using a regular std::vector for the wait queue, a custom struct is used
  // this is so that the GC can simply call Allocator::free on the queue pointer, instead of needing
  // to call 'delete' on it. This makes the GC deallocation logic a lot simpler
  struct WaitQueue {
    size_t capacity;
    size_t used;
    Thread* buffer[];

    // allocate a new wait queue instance with a given initial capacity
    static WaitQueue* alloc(size_t initial_capacity = 4);

    // insert a thread into the wait queue
    // if the new size exceeds the current capacity, the queue gets reallocated and the new pointer is returned
    // if the queue wasn't reallocated, the old pointer gets returned
    static WaitQueue* append_thread(WaitQueue* queue, Thread* thread);
  };

  WaitQueue* wait_queue() const;
  void set_wait_queue(WaitQueue* wait_queue) const;

  RawValue result() const;
  void set_result(RawValue result) const;

  RawValue exception() const;
  void set_exception(RawValue value) const;

  bool has_finished() const {
    return wait_queue() == nullptr;
  }

  RawValue await(Thread*) const;
  RawValue resolve(Thread*, RawValue value) const;
  RawValue reject(Thread*, RawException exception) const;

  enum {
    kWaitQueueOffset = RawInstance::kFieldCount,
    kResultOffset,
    kExceptionOffset,
    kFieldCount
  };

private:
  void wake_waiting_threads(Thread*) const;
};

// user exception instance
class RawException : public RawInstance {
public:
  COMMON_RAW_OBJECT(Exception);

  static RawException create(Thread*, RawString message);

  RawString message() const;
  void set_message(RawString value) const;

  RawTuple stack_trace() const;
  void set_stack_trace(RawTuple stack_trace) const;

  RawValue cause() const;
  void set_cause(RawValue stack_trace) const;

  enum {
    kMessageOffset = RawInstance::kFieldCount,
    kStackTraceOffset,
    kCauseOffset,
    kFieldCount
  };
};

// import exception instance
class RawImportException : public RawException {
public:
  COMMON_RAW_OBJECT(ImportException);

  static RawImportException create(Thread*,
                                   const std::string& module_path,
                                   const ref<core::compiler::CompilationUnit>& unit);

  RawTuple errors() const;
  void set_errors(RawTuple errors) const;

  enum {
    kErrorsOffset = RawException::kFieldCount,
    kFieldCount
  };
};

}  // namespace charly::core::runtime
