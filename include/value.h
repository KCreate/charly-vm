/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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
#include <cmath>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <utf8/utf8.h>

#include "common.h"
#include "defines.h"

#pragma once

namespace Charly {

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
static constexpr uint64_t kMaskSignBit      = 0x8000000000000000; // Sign bit
static constexpr uint64_t kMaskExponentBits = 0x7ff0000000000000; // Exponent bits
static constexpr uint64_t kMaskQuietBit     = 0x0008000000000000; // Quiet bit
static constexpr uint64_t kMaskTypeBits     = 0x0007000000000000; // Type bits
static constexpr uint64_t kMaskSignature    = 0xffff000000000000; // Signature bits
static constexpr uint64_t kMaskPayloadBits  = 0x0000ffffffffffff; // Payload bits

// Types that are encoded in the type field
static constexpr uint64_t kITypeNaN     = 0x0000000000000000;
static constexpr uint64_t kITypeFalse   = 0x0001000000000000;
static constexpr uint64_t kITypeTrue    = 0x0002000000000000;
static constexpr uint64_t kITypeNull    = 0x0003000000000000;
static constexpr uint64_t kITypeInteger = 0x0004000000000000;
static constexpr uint64_t kITypeSymbol  = 0x0005000000000000;
static constexpr uint64_t kITypePString = 0x0006000000000000;
static constexpr uint64_t kITypeIString = 0x0007000000000000;

// Shorthand values
static constexpr uint64_t kBitsNaN = kMaskExponentBits | kMaskQuietBit;
static constexpr uint64_t kNaN     = kBitsNaN;
static constexpr uint64_t kFalse   = kBitsNaN | kITypeFalse; // 0x7ff9000000000000
static constexpr uint64_t kTrue    = kBitsNaN | kITypeTrue;  // 0x7ffa000000000000
static constexpr uint64_t kNull    = kBitsNaN | kITypeNull;  // 0x7ffb000000000000

// Signatures of complex encoded types
static constexpr uint64_t kSignaturePointer = kMaskSignBit | kBitsNaN;
static constexpr uint64_t kSignatureInteger = kBitsNaN | kITypeInteger;
static constexpr uint64_t kSignatureSymbol  = kBitsNaN | kITypeSymbol;
static constexpr uint64_t kSignaturePString = kBitsNaN | kITypePString;
static constexpr uint64_t kSignatureIString = kBitsNaN | kITypeIString;

// Masks for the immediate encoded types
static constexpr uint64_t kMaskPointer       = 0x0000ffffffffffff;
static constexpr uint64_t kMaskInteger       = 0x0000ffffffffffff;
static constexpr uint64_t kMaskIntegerSign   = 0x0000800000000000;
static constexpr uint64_t kMaskSymbol        = 0x0000ffffffffffff;
static constexpr uint64_t kMaskPString       = 0x0000ffffffffffff;
static constexpr uint64_t kMaskIString       = 0x000000ffffffffff;
static constexpr uint64_t kMaskIStringLength = 0x0000ff0000000000;

// Constants used when converting between different representations
static constexpr int64_t kMaxInt     = (static_cast<int64_t>(1) << 47) - 1;
static constexpr int64_t kMaxUInt    = (static_cast<int64_t>(1) << 48) - 1;
static constexpr int64_t kMinInt     = -(static_cast<int64_t>(1) << 47);
static const void* const kMaxPointer = (void*)0x0000FFFFFFFFFFFF;
static constexpr uint64_t kSignBlock = 0xFFFF000000000000;

// Misc. constants
static constexpr uint32_t kMaxIStringLength = 5;
static constexpr uint32_t kMaxPStringLength = 6;
static constexpr int64_t kMaxStringLength   = 0xffffffff;

// Human readable types of all data types
const std::string kHumanReadableTypes[] = {"dead",     "class",     "object", "array",      "string",
                                           "function", "cfunction", "frame",  "catchtable", "cpointer",
                                           "number",   "boolean",   "null",   "symbol",     "unknown"};

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
  kTypeCPointer,

  // Types which are immediate encoded using nan-boxing
  kTypeNumber,
  kTypeBoolean,
  kTypeNull,
  kTypeSymbol,

  // This should never appear anywhere
  kTypeUnknown
};

// Print a more detailed output of charly values and data structures
void charly_debug_print(std::ostream& io, VALUE value, std::vector<VALUE>& trace);
inline void charly_debug_print(std::ostream& io, VALUE value) {
  std::vector<VALUE> vec;
  charly_debug_print(io, value, vec);
}

// Print a user-friendly version of charly values and data structures
void charly_to_string(std::ostream& io, VALUE value, std::vector<VALUE>& trace);
inline void charly_to_string(std::ostream& io, VALUE value) {
  std::vector<VALUE> vec;
  charly_to_string(io, value, vec);
}

// Metadata which is stores in every heap value
class Header {
  friend class GarbageCollector;
  template<typename T> friend class Immortal;
public:
  void init(ValueType type);
  void clean();

  ValueType get_type();

  VALUE as_value();

protected:
  ValueType type;   // the type of this heap value
  bool mark;        // set by the GC to mark reachable values
  bool immortal;    // wether this value is immortal (e.g. should never be deleted by the GC)
};

// Underlying type of every value which has its own
// value container (e.g. Object, Function, Class)
class Container : public Header {
  friend class GarbageCollector;
public:
  using ContainerType = std::unordered_map<VALUE, VALUE>;

  void init(ValueType type, uint32_t initial_capacity = 4);
  void init(Container* source);
  void clean();

  bool read(VALUE key, VALUE* result);              // reads a value from the container
  VALUE read_or(VALUE key, VALUE fallback = kNull); // reads a value from the container or returns fallback
  bool contains(VALUE key);                         // check wether the container contains some key
  uint32_t keycount();                              // returns amount of keys inside container
  bool erase(VALUE key);                            // erases a key from the container, returns true on success
  void write(VALUE key, VALUE value);               // insert or assign to some key
  bool assign(VALUE key, VALUE value);              // assign to some key, returns false if key did not exist

  // Access the internal container data structure
  // via a callback function
  void access_container(std::function<void(ContainerType*)> cb);
  void access_container_shared(std::function<void(ContainerType*)> cb);

protected:
  ContainerType* container;
};

// Object type
class Object : public Container {
  friend class GarbageCollector;
public:
  void init(uint32_t initial_capacity = 4, Class* klass = nullptr);
  void init(Object* source); // copy constructor

  Class* get_klass();

protected:
  Class* klass; // The class this object was constructed from
};

// Array type
class Array : public Header {
  friend class GarbageCollector;
public:
  using VectorType = std::vector<VALUE>;

  void init(uint32_t initial_capacity = 4);
  void init(Array* source); // copy constructor
  void clean();

  uint32_t size();                         // returns the amount of values stored
  VALUE read(int64_t index);               // read the value at index, returns null if out-of-bounds
  VALUE write(int64_t index, VALUE value); // write value to index, returns value if successful, null if not
  void insert(int64_t index, VALUE value); // insert a value at some index, nop if out-of-bounds
  void push(VALUE value);                  // append value to the end of the array
  void remove(int64_t index);              // remove a alue at index, nop if out-of-bounds
  void clear();                            // remove all elements from the array
  void fill(VALUE value, uint32_t count);  // fill the array with count instances of value

  bool contains_only(ValueType type);

  // Access to internal vector via a callback function
  void access_vector(std::function<void(VectorType*)> cb);
  void access_vector_shared(std::function<void(VectorType*)> cb);

protected:
  VectorType* data;
};

// String type
class String : public Header {
  friend class GarbageCollector;
public:
  void init(const char* data, uint32_t length);
  void init(char* data, uint32_t length, bool copy = true);
  void init(const std::string& source);
  void init(String* source); // copy constructor
  void clean();

  char* get_data();
  uint32_t get_length();
protected:
  char* data;
  uint32_t length;
};

// Frames introduce new environments
class Frame : public Header {
  friend class GarbageCollector;
public:
  using VectorType = std::vector<VALUE>;

  void init(Frame* parent, CatchTable* catchtable, Function* function, uint8_t* origin, VALUE self, bool halt = false);
  void clean();

  Frame* get_parent();
  Frame* get_environment();
  CatchTable* get_catchtable();
  Function* get_function();
  VALUE get_self();
  uint8_t* get_origin_address();
  uint8_t* get_return_address();  // calculate the return address of this frame
  bool get_halt_after_return();

  bool read_local(uint32_t index, VALUE* result);               // read value at index and store into result
  VALUE read_local_or(uint32_t index, VALUE fallback = kNull);  // read value at index or return fallback

  bool write_local(uint32_t index, VALUE value);  // write value to index, returns false if out-of-bounds

  // Access to internal vector via a callback function
  void access_locals(std::function<void(VectorType*)> cb);
  void access_locals_shared(std::function<void(VectorType*)> cb);

protected:
  Frame* parent;              // parent frame
  Frame* environment;         // parent environment frame (used by closures)
  CatchTable* catchtable;     // last active catchtable on frame entry
  Function* function;         // the function which pushed this frame
  VALUE self;                 // the object this function was invoked on
  uint8_t* origin_address;    // the address where this call originated
  std::vector<VALUE>* locals; // local variables
  bool halt_after_return;     // wether the machine should halt after returning
};

// Catchtable used for exception handling
class CatchTable : public Header {
  friend class GarbageCollector;
public:
  void init(CatchTable* parent, Frame* frame, uint8_t* address, size_t stacksize);

  CatchTable* get_parent();
  Frame* get_frame();
  uint8_t* get_address();
  size_t get_stacksize();

protected:
  CatchTable* parent;   // the parent catchtable
  Frame* frame;         // the frame in which this created was created
  uint8_t* address;     // the address of the exception handler
  size_t stacksize;     // the amount of values on the stack when this table was created
};

// Contains a data pointer and a destructor method to deallocate c library resources
class CPointer : public Header {
  friend class GarbageCollector;
public:
  using DestructorType = void (*)(void*);

  void init(void* data, DestructorType destructor);
  void clean();

  void set_data(void* data);
  void set_destructor(DestructorType destructor);

  void* get_data();
  DestructorType get_destructor();

protected:
  void* data;                 // arbitrary data pointer used by native libraries
  DestructorType destructor;  // destructor function pointer
};

// Normal functions defined inside the virtual machine.
class Function : public Container {
  friend class GarbageCollector;
public:
  void init(VALUE name, Frame* context, uint8_t* body, uint32_t argc, uint32_t minimum_argc,
            uint32_t lvarcount, bool anonymous, bool needs_arguments);
  void init(Function* source); // copy constructor

  void set_context(Frame* context);
  void set_host_class(Class* host_class);
  void set_bound_self(VALUE bound_self);  // set the bound_self value
  void clear_bound_self();                // remove the bound_self value

  VALUE get_name();
  Frame* get_context();
  uint8_t* get_body_address();
  Class* get_host_class();
  bool get_bound_self(VALUE* result);     // load bound_self into result, returns false if not set
  uint32_t get_argc();
  uint32_t get_minimum_argc();
  uint32_t get_lvarcount();
  bool get_anonymous();
  bool get_needs_arguments();

  VALUE get_self(VALUE fallback); // determines the self value passed to the function
  VALUE get_self(); // determines the self value passed to the function

protected:
  VALUE name;             // symbol encoded name
  Frame* context;         // frame this function was defined in
  uint8_t* body_address;  // address of the body
  Class* host_class;      // host class of this function, nullptr if none
  VALUE bound_self;       // the bound self value
  uint32_t argc;          // amount of named arguments
  uint32_t minimum_argc;  // minimum amount of arguments needed to call
  uint32_t lvarcount;     // amount of local variable slots required
  bool bound_self_set;    // wether a bound self value is set, see bound_self above
  bool anonymous;         // wether this function is anonymous (-> syntax)
  bool needs_arguments;   // wether this function needs the arguments special value
};

// Thread policies describing what thread a native function is allowed to run on
enum ThreadPolicy : uint8_t {
  ThreadPolicyMain    = 0b00000001,
  ThreadPolicyWorker  = 0b00000010,
  ThreadPolicyBoth    = ThreadPolicyMain | ThreadPolicyWorker
};

// Function type used for including external functions from C-Land into the virtual machine
// These are basically just a function pointer with some metadata associated to them
class CFunction : public Container {
  friend class GarbageCollector;
public:
  void init(VALUE name, void* pointer, uint32_t argc, ThreadPolicy thread_policy);
  void init(CFunction* source); // copy constructor

  void set_push_return_value(bool value);
  void set_halt_after_return(bool value);

  VALUE get_name();
  void* get_pointer();
  uint32_t get_argc();
  ThreadPolicy get_thread_policy();
  bool get_push_return_value();
  bool get_halt_after_return();

  bool allowed_on_main_thread();
  bool allowed_on_worker_thread();

protected:
  VALUE name;
  void* pointer;
  uint32_t argc;
  ThreadPolicy thread_policy;
  bool push_return_value;
  bool halt_after_return;
};

// Classes defined inside the virtual machine
class Class : public Container {
  friend class GarbageCollector;
public:
  using VectorType = std::vector<VALUE>;

  void init(VALUE name, Function* constructor, Class* parent_class);
  void clean();

  void set_prototype(Object* prototype);

  VALUE get_name();
  Class* get_parent_class();
  Function* get_constructor();
  Object* get_prototype();

  uint32_t get_member_property_count();

  bool find_value(VALUE symbol, VALUE* result);       // find method/value in class hierarchy
  bool find_super_value(VALUE symbol, VALUE* result); // find method/value in class hierarchy, starting at parent

  Function* find_constructor();       // find the first constructor in the class hierarchy
  Function* find_super_constructor(); // find the first constructor in the class hierarchy, starting at parent

  void initialize_member_properties(Object* object); // Initialize an object as an instance of this class

  // Access to member_properties vector via callback
  void access_member_properties(std::function<void(VectorType*)> cb);
  void access_member_properties_shared(std::function<void(VectorType*)> cb);

protected:
  VALUE name;
  Class* parent_class;
  Function* constructor;
  Object* prototype;
  VectorType* member_properties;
};

// clang-format off
__attribute__((always_inline))
inline VALUE charly_create_pointer(void* ptr) {
  if (ptr == nullptr) return kNull;
  if (ptr > kMaxPointer) return kSignaturePointer; // null pointer
  return kSignaturePointer | (kMaskPointer & reinterpret_cast<int64_t>(ptr));
}

// Type casting functions
template <typename T>
__attribute__((always_inline))
T* charly_as_pointer_to(VALUE value)                  { return reinterpret_cast<T*>(value & kMaskPointer); }
__attribute__((always_inline))
inline void* charly_as_pointer(VALUE value)           { return charly_as_pointer_to<void>(value); }
__attribute__((always_inline))
inline Header* charly_as_header(VALUE value)          { return charly_as_pointer_to<Header>(value); }
__attribute__((always_inline))
inline Container* charly_as_container(VALUE value)    { return charly_as_pointer_to<Container>(value); }
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
inline Frame* charly_as_frame(VALUE value)            { return charly_as_pointer_to<Frame>(value); }
__attribute__((always_inline))
inline CatchTable* charly_as_catchtable(VALUE value)  { return charly_as_pointer_to<CatchTable>(value); }
__attribute__((always_inline))
inline CPointer* charly_as_cpointer(VALUE value)      { return charly_as_pointer_to<CPointer>(value); }

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
inline bool charly_is_nan(VALUE value)       { return value == kNaN; }
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
  return charly_is_on_heap(value) && charly_as_header(value)->get_type() == type;
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
inline bool charly_is_callable(VALUE value) {
  return charly_is_function(value) || charly_is_cfunction(value);
}
__attribute__((always_inline))
inline bool charly_is_frame(VALUE value) { return charly_is_heap_type(value, kTypeFrame); }
__attribute__((always_inline))
inline bool charly_is_catchtable(VALUE value) { return charly_is_heap_type(value, kTypeCatchTable); }
__attribute__((always_inline))
inline bool charly_is_cpointer(VALUE value) { return charly_is_heap_type(value, kTypeCPointer); }

// Return the ValueType representation of the type of the value
__attribute__((always_inline))
inline uint8_t charly_get_type(VALUE value) {
  if (charly_is_on_heap(value)) return charly_as_header(value)->get_type();
  if (charly_is_float(value)) return kTypeNumber;
  if (charly_is_int(value)) return kTypeNumber;
  if (charly_is_null(value)) return kTypeNull;
  if (charly_is_pstring(value) || charly_is_istring(value)) return kTypeString;
  if (charly_is_boolean(value)) return kTypeBoolean;
  if (charly_is_symbol(value)) return kTypeSymbol;
  return kTypeUnknown;
}

// Check wether an array consists only of certain elements
template <ValueType T>
__attribute__((always_inline))
inline bool charly_is_array_of(VALUE value) {
  assert(charly_is_array(value));
  return charly_as_array(value)->contains_only(T);
}

// Checks wether this value is a container
__attribute__((always_inline))
inline bool charly_is_container(VALUE value) {
  switch(charly_get_type(value)) {
    case kTypeObject:
    case kTypeClass:
    case kTypeFunction:
    case kTypeCFunction: return true;
  }

  return false;
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

// The purpose of this method is to replace INFINITY, -INFINITY, NAN with 0
// Conversion from these values to int is undefined behaviour and will result
// in random gibberish being returned
__attribute__((always_inline))
inline double charly_double_to_safe_double(VALUE value) { return FP_STRIP_INF(FP_STRIP_NAN(BITCAST_DOUBLE(value))); }

// Convert an immediate double to other integer or float types
//
// Warning: These methods don't perform any type checks and assume
// the caller made sure that the input value is an immediate double
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
    return charly_as_hstring(value)->get_data();
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
    return charly_as_hstring(value)->get_length();
  }

  return 0xFFFFFFFF;
}

__attribute__((always_inline))
inline std::string charly_string_std(VALUE value) {
  char* data = charly_string_data(value);
  uint32_t length = charly_string_length(value);

  if (!data) return "not a string";
  return std::string(data, length);
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

// Create immediate encoded strings of size 0 - 5
//
// Note: Because char* should always contain a null terminator at the end, we check for <= 6 bytes
// instead of <= 5.
template <size_t N>
__attribute__((always_inline))
VALUE charly_create_istring(char const (& input)[N]) {
  static_assert(N <= 6, "charly_create_istring can only create strings of length <= 5 (excluding null-terminator)");
  return charly_create_istring(input, N - 1);
}

__attribute__((always_inline))
inline VALUE charly_create_istring(char const (& input)[7]) {
  return charly_create_istring(input, 6);
}

inline VALUE charly_create_istring(const std::string& input) {
  return charly_create_istring(input.c_str(), input.size());
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
  char* source_buffer = charly_string_data(value);
  size_t length = charly_string_length(value);

  // copy string into local null-terminated buffer
  char buffer[length + 1];
  std::memset(buffer, 0, length + 1);
  std::memcpy(buffer, source_buffer, length);

  char* end_ptr;
  int64_t interpreted = strtol(buffer, &end_ptr, 0);

  // strtol sets errno to ERANGE if the result value doesn't fit
  // into the return type
  if (errno == ERANGE) {
    errno = 0;
    return 0;
  }

  if (end_ptr == buffer) {
    return 0;
  }

  return interpreted;
}

// Convert a string to a double
__attribute__((always_inline))
inline double charly_string_to_double(VALUE value) {
  char* source_buffer = charly_string_data(value);
  size_t length = charly_string_length(value);

  // copy string into local null-terminated buffer
  char buffer[length + 1];
  std::memset(buffer, 0, length + 1);
  std::memcpy(buffer, source_buffer, length);

  char* end_ptr;
  double interpreted = strtod(buffer, &end_ptr);

  // filter out incorrect or unsupported values
  if (interpreted == HUGE_VAL || interpreted == INFINITY) {
    return NAN;
  }

  // If strtod could not perform a conversion it returns 0
  // and sets str_end to str
  if (end_ptr == buffer) {
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

#ifdef __APPLE__
__attribute__((always_inline))
inline VALUE charly_create_number(size_t value) {
  if (value >= kMaxUInt) return charly_create_double(value);
  return charly_create_integer(value);
}
#endif

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
    if (charly_int_to_int64(right) == 0) return kNaN;
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
  if (amount < 0) amount = 0;
  return charly_create_number(num << amount);
}
__attribute__((always_inline))
inline VALUE charly_shr_number(VALUE left, VALUE right) {
  int32_t num = charly_number_to_int32(left);
  int32_t amount = charly_number_to_int32(right);
  if (amount < 0) amount = 0;
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
  if (value == kNaN) return false;
  if (value == kNull) return false;
  if (value == kFalse) return false;
  if (charly_is_int(value)) return charly_int_to_int64(value) != 0;
  if (charly_is_float(value)) return charly_double_to_double(value) != 0;
  return true;
}

#define VALUE_01 VALUE
#define VALUE_02 VALUE_01, VALUE
#define VALUE_03 VALUE_02, VALUE
#define VALUE_04 VALUE_03, VALUE
#define VALUE_05 VALUE_04, VALUE
#define VALUE_06 VALUE_05, VALUE
#define VALUE_07 VALUE_06, VALUE
#define VALUE_08 VALUE_07, VALUE
#define VALUE_09 VALUE_08, VALUE
#define VALUE_10 VALUE_09, VALUE
#define VALUE_11 VALUE_10, VALUE
#define VALUE_12 VALUE_11, VALUE
#define VALUE_13 VALUE_12, VALUE
#define VALUE_14 VALUE_13, VALUE
#define VALUE_15 VALUE_14, VALUE
#define VALUE_16 VALUE_15, VALUE
#define VALUE_17 VALUE_16, VALUE
#define VALUE_18 VALUE_17, VALUE
#define VALUE_19 VALUE_18, VALUE
#define VALUE_20 VALUE_19, VALUE

#define ARG_01 argv[0]
#define ARG_02 ARG_01, argv[1]
#define ARG_03 ARG_02, argv[2]
#define ARG_04 ARG_03, argv[3]
#define ARG_05 ARG_04, argv[4]
#define ARG_06 ARG_05, argv[5]
#define ARG_07 ARG_06, argv[6]
#define ARG_08 ARG_07, argv[7]
#define ARG_09 ARG_08, argv[8]
#define ARG_10 ARG_09, argv[9]
#define ARG_11 ARG_10, argv[10]
#define ARG_12 ARG_11, argv[11]
#define ARG_13 ARG_12, argv[12]
#define ARG_14 ARG_13, argv[13]
#define ARG_15 ARG_14, argv[14]
#define ARG_16 ARG_15, argv[15]
#define ARG_17 ARG_16, argv[16]
#define ARG_18 ARG_17, argv[17]
#define ARG_19 ARG_18, argv[18]
#define ARG_20 ARG_19, argv[19]

__attribute__((always_inline))
inline VALUE charly_call_cfunction(VM* vm_handle, CFunction* cfunc, uint32_t argc, VALUE* argv) {
  if (argc < cfunc->get_argc()) return kNull;

  void* pointer = cfunc->get_pointer();
  switch (cfunc->get_argc()) {
    case  0: return reinterpret_cast<VALUE (*)(VM&)>           (pointer)(*vm_handle);
    case  1: return reinterpret_cast<VALUE (*)(VM&, VALUE_01)> (pointer)(*vm_handle, ARG_01);
    case  2: return reinterpret_cast<VALUE (*)(VM&, VALUE_02)> (pointer)(*vm_handle, ARG_02);
    case  3: return reinterpret_cast<VALUE (*)(VM&, VALUE_03)> (pointer)(*vm_handle, ARG_03);
    case  4: return reinterpret_cast<VALUE (*)(VM&, VALUE_04)> (pointer)(*vm_handle, ARG_04);
    case  5: return reinterpret_cast<VALUE (*)(VM&, VALUE_05)> (pointer)(*vm_handle, ARG_05);
    case  6: return reinterpret_cast<VALUE (*)(VM&, VALUE_06)> (pointer)(*vm_handle, ARG_06);
    case  7: return reinterpret_cast<VALUE (*)(VM&, VALUE_07)> (pointer)(*vm_handle, ARG_07);
    case  8: return reinterpret_cast<VALUE (*)(VM&, VALUE_08)> (pointer)(*vm_handle, ARG_08);
    case  9: return reinterpret_cast<VALUE (*)(VM&, VALUE_09)> (pointer)(*vm_handle, ARG_09);
    case 10: return reinterpret_cast<VALUE (*)(VM&, VALUE_10)> (pointer)(*vm_handle, ARG_10);
    case 11: return reinterpret_cast<VALUE (*)(VM&, VALUE_11)> (pointer)(*vm_handle, ARG_11);
    case 12: return reinterpret_cast<VALUE (*)(VM&, VALUE_12)> (pointer)(*vm_handle, ARG_12);
    case 13: return reinterpret_cast<VALUE (*)(VM&, VALUE_13)> (pointer)(*vm_handle, ARG_13);
    case 14: return reinterpret_cast<VALUE (*)(VM&, VALUE_14)> (pointer)(*vm_handle, ARG_14);
    case 15: return reinterpret_cast<VALUE (*)(VM&, VALUE_15)> (pointer)(*vm_handle, ARG_15);
    case 16: return reinterpret_cast<VALUE (*)(VM&, VALUE_16)> (pointer)(*vm_handle, ARG_16);
    case 17: return reinterpret_cast<VALUE (*)(VM&, VALUE_17)> (pointer)(*vm_handle, ARG_17);
    case 18: return reinterpret_cast<VALUE (*)(VM&, VALUE_18)> (pointer)(*vm_handle, ARG_18);
    case 19: return reinterpret_cast<VALUE (*)(VM&, VALUE_19)> (pointer)(*vm_handle, ARG_19);
    case 20: return reinterpret_cast<VALUE (*)(VM&, VALUE_20)> (pointer)(*vm_handle, ARG_20);
  }

  return kNull;
}

#undef VALUE_01
#undef VALUE_02
#undef VALUE_03
#undef VALUE_04
#undef VALUE_05
#undef VALUE_06
#undef VALUE_07
#undef VALUE_08
#undef VALUE_09
#undef VALUE_10
#undef VALUE_11
#undef VALUE_12
#undef VALUE_13
#undef VALUE_14
#undef VALUE_15
#undef VALUE_16
#undef VALUE_17
#undef VALUE_18
#undef VALUE_19
#undef VALUE_20

#undef ARG_01
#undef ARG_02
#undef ARG_03
#undef ARG_04
#undef ARG_05
#undef ARG_06
#undef ARG_07
#undef ARG_08
#undef ARG_09
#undef ARG_10
#undef ARG_11
#undef ARG_12
#undef ARG_13
#undef ARG_14
#undef ARG_15
#undef ARG_16
#undef ARG_17
#undef ARG_18
#undef ARG_19
#undef ARG_20

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

// Compile-time CRC32 hashing
// Source: https://stackoverflow.com/questions/28675727/using-crc32-algorithm-to-hash-string-at-compile-time
namespace CRC32 {
  template <unsigned c, int k = 8>
  struct f : f<((c & 1) ? 0xedb88320 : 0) ^ (c >> 1), k - 1> {};
  template <unsigned c> struct f<c, 0>{enum {value = c};};

#define CRCA(x) CRCB(x) CRCB(x + 128)
#define CRCB(x) CRCC(x) CRCC(x +  64)
#define CRCC(x) CRCD(x) CRCD(x +  32)
#define CRCD(x) CRCE(x) CRCE(x +  16)
#define CRCE(x) CRCF(x) CRCF(x +   8)
#define CRCF(x) CRCG(x) CRCG(x +   4)
#define CRCG(x) CRCH(x) CRCH(x +   2)
#define CRCH(x) CRCI(x) CRCI(x +   1)
#define CRCI(x) f<x>::value ,
  constexpr unsigned crc_table[] = { CRCA(0) };
#undef CRCA
#undef CRCB
#undef CRCC
#undef CRCD
#undef CRCE
#undef CRCF
#undef CRCG
#undef CRCH
#undef CRCI

  constexpr uint32_t crc32_impl(const char* p, size_t len, uint32_t crc) {
    return len ?
            crc32_impl(p + 1,len - 1,(crc >> 8) ^ crc_table[(crc & 0xFF) ^ *p])
            : crc;
  }

  constexpr uint32_t crc32(const char* data, size_t length) {
    return ~crc32_impl(data, length, ~0);
  }

  constexpr uint32_t crc32(const std::string& str) {
    return crc32(str.c_str(), str.size());
  }

  constexpr size_t strlen_c(const char* str) {
    return *str ? 1 + strlen_c(str + 1) : 0;
  }

  constexpr uint32_t crc32(const char* str) {
    return crc32(str, strlen_c(str));
  }

  template <size_t N>
  constexpr uint32_t crc(char const (& input)[N]) {
    return crc32(&input, N);
  }

  constexpr VALUE crc32_to_symbol(uint32_t value) {
    uint64_t val = value;
    return kSignatureSymbol | (val & kMaskSymbol);
  }
}

#define SYM(X) CRC32::crc32_to_symbol(CRC32::crc32(X))

__attribute__((always_inline))
inline VALUE charly_create_symbol(const char* data, size_t length) {
  return CRC32::crc32_to_symbol(CRC32::crc32(data, length));
}

__attribute__((always_inline))
inline VALUE charly_create_symbol(const std::string& input) {
  return CRC32::crc32_to_symbol(CRC32::crc32(input));
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
      return charly_create_symbol(str_data, str_length);
    }

    case kTypeNumber: {
      if (charly_is_float(value)) {
        double val = charly_double_to_double(value);
        std::string str = std::to_string(val);
        return charly_create_symbol(std::move(str));
      } else {
        int64_t val = charly_int_to_int64(value);
        std::string str = std::to_string(val);
        return charly_create_symbol(std::move(str));
      }
    }

    case kTypeBoolean: {
      return SYM(value == kTrue ? "true" : "false");
    }

    case kTypeNull: {
      return SYM("null");
    }

    case kTypeSymbol: {
      return value;
    }

    default: {
      static const std::string symbol_type_symbol_table[] = {
        "<dead>",      "<class>",     "<object>", "<array>",      "<string>",   "<function>",
        "<cfunction>", "<frame>",  "<catchtable>", "<cpointer>", "<number>",
        "<boolean>",   "<null>",      "<symbol>", "<unknown>"
      };

      const std::string& symbol = symbol_type_symbol_table[type];
      return charly_create_symbol(symbol);
    }
  }
}

// External libs interface
typedef std::tuple<std::string, uint32_t, ThreadPolicy> CharlyLibSignature;
struct CharlyLibSignatures {
  std::vector<CharlyLibSignature> signatures;
};

// Shorthand for declaring charly api methods to export
#define CHARLY_API(N) \
  extern "C" VALUE N

// Shorthands for defining the signatures
#define F(N, A, P) {#N, A, P},
#define CHARLY_MANIFEST(P) \
  extern "C" { \
    CharlyLibSignatures __charly_signatures = {{ \
      P \
    }}; \
  }

// clang-format on

}  // namespace Charly
