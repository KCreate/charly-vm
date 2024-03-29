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

#include <iomanip>

#include "charly/utils/buffer.h"
#include "charly/utils/colorwriter.h"

#include "charly/utf8.h"
#include "charly/value.h"

#include "charly/core/compiler/compiler.h"
#include "charly/core/runtime/compiled_module.h"
#include "charly/core/runtime/heap.h"
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"

namespace charly::core::runtime {

using Color = utils::Color;

bool is_immediate_shape(ShapeId shape_id) {
  auto id = static_cast<uint32_t>(shape_id);
  return id <= static_cast<uint32_t>(ShapeId::kLastImmediateShape);
}

bool is_object_shape(ShapeId shape_id) {
  auto id = static_cast<uint32_t>(shape_id);
  return id > static_cast<uint32_t>(ShapeId::kLastImmediateShape);
}

bool is_data_shape(ShapeId shape_id) {
  auto id = static_cast<uint32_t>(shape_id);
  auto first_data_shape = static_cast<uint32_t>(ShapeId::kFirstDataObject);
  auto last_data_shape = static_cast<uint32_t>(ShapeId::kLastDataObject);
  return (id >= first_data_shape) && (id <= last_data_shape);
}

bool is_instance_shape(ShapeId shape_id) {
  auto id = static_cast<uint32_t>(shape_id);
  auto last_non_instance = static_cast<uint32_t>(ShapeId::kLastDataObject);
  return id > last_non_instance;
}

bool is_builtin_shape(ShapeId shape_id) {
  auto id = static_cast<uint32_t>(shape_id);
  auto last_builtin_shape = static_cast<uint32_t>(ShapeId::kLastBuiltinShapeId);
  return id <= last_builtin_shape;
}

bool is_user_shape(ShapeId shape_id) {
  return !is_builtin_shape(shape_id);
}

size_t align_to_size(size_t size, size_t alignment) {
  DCHECK(alignment > 0, "invalid alignment (%)", alignment);

  if (size % alignment == 0) {
    return size;
  }

  size_t remaining_until_aligned = alignment - (size % alignment);
  return size + remaining_until_aligned;
}

int64_t wrap_negative_indices(int64_t index, size_t size) {
  return index < 0 ? index + size : index;
}

void ObjectHeader::initialize_header(uintptr_t address, ShapeId shape_id, uint16_t count) {
  auto* header = bitcast<ObjectHeader*>(address);
  header->m_shape_id_and_survivor_count = encode_shape_and_survivor_count(shape_id, 0);
  header->m_count = count;
  header->m_lock = 0;
  header->m_flags = Flag::kYoungGeneration;
  header->m_hashcode = 0;
  header->m_forward_target = 0;

#ifndef NDEBUG
  header->m_magic1 = ObjectHeader::kMagicNumber1;
  header->m_magic2 = ObjectHeader::kMagicNumber2;
#endif
}

ShapeId ObjectHeader::shape_id() const {
  uint32_t tmp = m_shape_id_and_survivor_count;
  ShapeId id = static_cast<ShapeId>(tmp & kMaskShape);
  DCHECK(id > ShapeId::kLastImmediateShape);
  return id;
}

uint8_t ObjectHeader::survivor_count() const {
  uint32_t tmp = m_shape_id_and_survivor_count;
  return (tmp & kMaskSurvivorCount) >> kShiftSurvivorCount;
}

uint16_t ObjectHeader::count() const {
  return m_count;
}

ObjectHeader::Flag ObjectHeader::flags() const {
  return m_flags;
}

SYMBOL ObjectHeader::hashcode() {
  if (has_cached_hashcode()) {
    return m_hashcode;
  } else {
    ShapeId shape = shape_id();

    switch (shape) {
      case ShapeId::kLargeString:
      case ShapeId::kLargeBytes: {
        SYMBOL hash = crc32::hash_block(bitcast<const char*>(payload_pointer()), count());
        cas_hashcode(0, hash);
        set_has_cached_hashcode();
        return hash;
      }
      case ShapeId::kHugeString: {
        auto huge_string = RawHugeString::cast(object());
        const char* data = huge_string.data();
        size_t size = huge_string.byte_length();
        SYMBOL hash = crc32::hash_block(data, size);
        cas_hashcode(0, hash);
        set_has_cached_hashcode();
        return hash;
      }
      case ShapeId::kHugeBytes: {
        auto huge_bytes = RawHugeBytes::cast(object());
        const char* data = bitcast<const char*>(huge_bytes.data());
        size_t size = huge_bytes.length();
        SYMBOL hash = crc32::hash_block(data, size);
        cas_hashcode(0, hash);
        set_has_cached_hashcode();
        return hash;
      }
      default: {
        FAIL("Unexpected object type");
      }
    }
  }
}

bool ObjectHeader::is_reachable() const {
  return flags() & kReachable;
}

bool ObjectHeader::has_cached_hashcode() const {
  return flags() & kHasHashcode;
}

bool ObjectHeader::is_young_generation() const {
  return flags() & kYoungGeneration;
}

void ObjectHeader::set_is_reachable() {
  Flag old_flags = flags();
  while (!m_flags.cas(old_flags, static_cast<Flag>(old_flags | kReachable))) {
    old_flags = flags();
  }
}

void ObjectHeader::set_has_cached_hashcode() {
  Flag old_flags = flags();
  while (!m_flags.cas(old_flags, static_cast<Flag>(old_flags | kHasHashcode))) {
    old_flags = flags();
  }
}

void ObjectHeader::set_is_young_generation() {
  Flag old_flags = flags();
  while (!m_flags.cas(old_flags, static_cast<Flag>(old_flags | kYoungGeneration))) {
    old_flags = flags();
  }
}

void ObjectHeader::clear_is_reachable() {
  Flag old_flags = flags();
  while (!m_flags.cas(old_flags, static_cast<Flag>(old_flags & ~kReachable))) {
    old_flags = flags();
  }
}

void ObjectHeader::clear_has_cached_hashcode() {
  Flag old_flags = flags();
  while (!m_flags.cas(old_flags, static_cast<Flag>(old_flags & ~kHasHashcode))) {
    old_flags = flags();
  }
}

void ObjectHeader::clear_is_young_generation() {
  Flag old_flags = flags();
  while (!m_flags.cas(old_flags, static_cast<Flag>(old_flags & ~kYoungGeneration))) {
    old_flags = flags();
  }
}

HeapRegion* ObjectHeader::heap_region() const {
  auto base = bitcast<uintptr_t>(this);
  auto region = bitcast<HeapRegion*>(base & kHeapRegionPointerMask);
  // DCHECK(region->magic == kHeapRegionMagicNumber);
  return region;
}

RawObject ObjectHeader::object() const {
  return RawObject::create_from_ptr(payload_pointer(), is_young_generation());
}

uintptr_t ObjectHeader::payload_pointer() const {
  return bitcast<uintptr_t>(this) + sizeof(ObjectHeader);
}

uint32_t ObjectHeader::alloc_size() const {
  DCHECK(bitcast<uintptr_t>(this) % kObjectAlignment == 0);
  ShapeId id = shape_id();
  if (id == ShapeId::kTuple || is_instance_shape(id)) {
    return align_to_size(sizeof(ObjectHeader) + count() * kPointerSize, kObjectAlignment);
  } else {
    return align_to_size(sizeof(ObjectHeader) + count(), kObjectAlignment);
  }
}

void ObjectHeader::increment_survivor_count() {
  uint32_t old_value = m_shape_id_and_survivor_count;
  ShapeId old_shape_id = shape_id();
  uint8_t old_survivor_count = survivor_count();
  uint32_t new_value = encode_shape_and_survivor_count(old_shape_id, old_survivor_count + 1);
  m_shape_id_and_survivor_count.acas(old_value, new_value);
}

void ObjectHeader::clear_survivor_count() {
  uint32_t old_value = m_shape_id_and_survivor_count;
  ShapeId old_shape_id = shape_id();
  uint32_t new_value = encode_shape_and_survivor_count(old_shape_id, 0);
  m_shape_id_and_survivor_count.acas(old_value, new_value);
}

bool ObjectHeader::cas_count(uint16_t old_count, uint16_t new_count) {
  return m_count.cas(old_count, new_count);
}

bool ObjectHeader::cas_hashcode(SYMBOL old_hashcode, SYMBOL new_hashcode) {
  return m_hashcode.cas(old_hashcode, new_hashcode);
}

bool ObjectHeader::has_forward_target() const {
  return m_forward_target != 0;
}

RawObject ObjectHeader::forward_target() const {
  DCHECK(has_forward_target());
  size_t offset = m_forward_target * kObjectAlignment;
  auto heap_base = bitcast<uintptr_t>(heap_region()->heap->heap_base());
  auto pointer = heap_base + offset;
  auto* header = ObjectHeader::header_at_address(pointer);
  auto* region = header->heap_region();
  DCHECK(region->magic == kHeapRegionMagicNumber);
  return header->object();
}

void ObjectHeader::set_forward_target(RawObject object) {
  auto* region = heap_region();
  auto* heap = region->heap;
  auto* header = object.header();
  size_t heap_offset = bitcast<uintptr_t>(header) - bitcast<uintptr_t>(heap->heap_base());
  size_t multiple_of_alignment = heap_offset / kObjectAlignment;
  DCHECK(multiple_of_alignment <= kUInt32Max);
  m_forward_target.acas(0, multiple_of_alignment);
}

uint32_t ObjectHeader::encode_shape_and_survivor_count(ShapeId shape_id, uint8_t survivor_count) {
  DCHECK(shape_id < ShapeId::kMaxShapeId);
  DCHECK(survivor_count <= kGCObjectMaxSurvivorCount);
  return static_cast<uint32_t>(shape_id) | ((uint32_t)survivor_count << kShiftSurvivorCount);
}

bool RawValue::is_error() const {
  return isNull() && RawNull::cast(this).error_code() != ErrorId::kErrorNone;
}

bool RawValue::is_error_ok() const {
  return (m_raw & kMaskLowByte) == kTagErrorOk;
}

bool RawValue::is_error_exception() const {
  return (m_raw & kMaskLowByte) == kTagErrorException;
}

bool RawValue::is_error_not_found() const {
  return (m_raw & kMaskLowByte) == kTagErrorNotFound;
}

bool RawValue::is_error_out_of_bounds() const {
  return (m_raw & kMaskLowByte) == kTagErrorOutOfBounds;
}

bool RawValue::is_error_read_only() const {
  return (m_raw & kMaskLowByte) == kTagErrorReadOnly;
}

bool RawValue::is_error_no_base_class() const {
  return (m_raw & kMaskLowByte) == kTagErrorNoBaseClass;
}

ShapeId RawValue::shape_id() const {
  if (isObject()) {
    RawObject object = RawObject::cast(this);
    ObjectHeader* header = object.header();
    return header->shape_id();
  } else {
    return shape_id_not_object_int();
  }
}

ShapeId RawValue::shape_id_not_object_int() const {
  size_t table_index = m_raw & kMaskImmediate;
  DCHECK(table_index < 16);
  return kShapeImmediateTagMapping[table_index];
}

bool RawValue::truthyness() const {
  if (isNull()) {
    return false;
  } else if (*this == kTrue) {
    return true;
  } else if (*this == kFalse) {
    return false;
  } else if (*this == kNaN) {
    return false;
  } else if (*this == kZero) {
    return false;
  } else if (*this == kFloatZero) {
    return false;
  }

  return true;
}

bool RawValue::is_old_pointer() const {
  return (m_raw & kMaskImmediate) == kTagOldObject;
}

bool RawValue::is_young_pointer() const {
  return (m_raw & kMaskImmediate) == kTagYoungObject;
}

RawClass RawValue::klass(Thread* thread) const {
  if (isInstance()) {
    auto instance = RawInstance::cast(this);
    auto klass_field = instance.klass_field();
    DCHECK(!klass_field.isNull());
    return RawClass::cast(klass_field);
  }

  return thread->runtime()->get_builtin_class(shape_id());
}

RawString RawValue::klass_name(Thread* thread) const {
  return klass(thread).name();
}

RawValue RawValue::load_attr(Thread* thread, RawValue attribute) const {
  if (attribute.isNumber()) {
    return load_attr_number(thread, attribute.int_value());
  } else if (attribute.isString()) {
    SYMBOL symbol = RawString::cast(attribute).hashcode();
    return load_attr_symbol(thread, symbol);
  } else if (attribute.isSymbol()) {
    return load_attr_symbol(thread, RawSymbol::cast(attribute));
  } else {
    return thread->throw_message("Expected index attribute to be a number or a string, got '%'",
                                 attribute.klass_name(thread));
  }
}

RawValue RawValue::load_attr_number(Thread* thread, int64_t index) const {
  if (isTuple()) {
    auto tuple = RawTuple::cast(this);
    auto length = tuple.length();
    auto real_index = wrap_negative_indices(index, length);

    if (real_index < 0 || real_index >= length) {
      return thread->throw_message("Tuple index % is out of range", index);
    }

    return tuple.field_at(real_index);
  } else if (isList()) {
    auto list = RawList::cast(this);
    return list.read_at(thread, index);
  } else if (isString()) {
    // TODO: optimize string index lookup
    //
    // current algorithm is O(N) since we need to traverse the entire string once to
    // get the codepoint length and then another time to find the nth character
    //
    // if user code iterates over each character, this becomes O(N^2)
    auto string = RawString::cast(this);
    auto size = string.codepoint_length();
    auto real_index = wrap_negative_indices(index, size);

    if (real_index < 0 || (size_t)real_index >= size) {
      return thread->throw_message("String index % is out of range", index);
    }

    const char* begin_ptr = RawString::data(&string);
    const char* end_ptr = begin_ptr + string.byte_length();
    CHECK(utf8::advance_to_nth_codepoint(begin_ptr, end_ptr, real_index));

    uint32_t cp;
    CHECK(utf8::peek_next(begin_ptr, end_ptr, cp));
    CHECK(utf8::is_valid_codepoint(cp));
    return RawSmallString::create_from_cp(cp);
  } else if (isBytes()) {
    auto bytes = RawBytes::cast(this);
    auto size = bytes.length();
    auto real_index = wrap_negative_indices(index, size);

    if (real_index < 0 || (size_t)real_index >= size) {
      return thread->throw_message("Bytes index % is out of range", index);
    }

    return RawInt::create(RawBytes::data(&bytes)[real_index]);
  } else {
    return thread->throw_message("Cannot perform index access on value of type '%'", klass_name(thread));
  }
}

RawValue RawValue::load_attr_symbol(Thread* thread, SYMBOL symbol) const {
  // builtin attributes
  switch (symbol) {
    case SYM("klass"): return klass(thread);
    case SYM("length"): {
      if (isTuple()) {
        auto tuple = RawTuple::cast(this);
        return RawInt::create(tuple.length());
      } else if (isString()) {
        auto string = RawString::cast(this);
        return RawInt::create(string.codepoint_length());
      } else if (isBytes()) {
        auto bytes = RawBytes::cast(this);
        return RawInt::create(bytes.length());
      }
      break;
    }
  }

  // shape property access
  auto klass = this->klass(thread);
  if (isInstance()) {
    auto runtime = thread->runtime();
    auto instance = RawInstance::cast(this);

    // TODO: cache result via inline cache
    auto shape = runtime->lookup_shape(instance.shape_id());
    auto result = shape.lookup_symbol(symbol);
    if (result.found) {
      // TODO: allow accessing private member of same class
      if (result.is_private() && runtime->check_private_access_permitted(thread, instance) <= result.offset) {
        return thread->throw_message("Cannot read private property '%' of class '%'", RawSymbol::create(symbol),
                                     klass.name());
      }

      return instance.field_at(result.offset);
    }
  }

  // function lookup
  // TODO: cache via inline cache
  RawValue lookup = klass.lookup_function(symbol);
  if (lookup.isFunction()) {
    auto function = RawFunction::cast(lookup);
    // TODO: allow accessing private member of same class
    if (function.shared_info()->ir_info.private_function && (*this != thread->frame()->self)) {
      return thread->throw_message("Cannot call private function '%' of class '%'", RawSymbol::create(symbol),
                                   klass.name());
    }

    return lookup;
  }

  return thread->throw_message("Object of type '%' has no attribute '%'", klass.name(), RawSymbol::create(symbol));
}

RawValue RawValue::set_attr(Thread* thread, RawValue attribute, RawValue value) const {
  if (attribute.isNumber()) {
    return set_attr_number(thread, attribute.int_value(), value);
  } else if (attribute.isString()) {
    SYMBOL symbol = RawString::cast(attribute).hashcode();
    return set_attr_symbol(thread, symbol, value);
  } else if (attribute.isSymbol()) {
    return set_attr_symbol(thread, RawSymbol::cast(attribute), value);
  } else {
    return thread->throw_message("Expected index attribute to be a number or a string, got '%'",
                                 attribute.klass_name(thread));
  }
}

RawValue RawValue::set_attr_number(Thread* thread, int64_t index, RawValue value) const {
  if (isList()) {
    auto list = RawList::cast(this);
    return list.write_at(thread, index, value);
  } else if (isTuple()) {
    return thread->throw_message("Cannot write to tuple");
  } else {
    return thread->throw_message("Cannot perform index write on value of type '%'", klass_name(thread));
  }
}

RawValue RawValue::set_attr_symbol(Thread* thread, SYMBOL symbol, RawValue value) const {
  // shape property write
  // TODO: cache result via inline cache
  auto klass = this->klass(thread);
  if (isInstance()) {
    auto runtime = thread->runtime();
    auto instance = RawInstance::cast(this);
    auto shape = runtime->lookup_shape(instance.shape_id());
    auto result = shape.lookup_symbol(symbol);
    if (result.found) {
      if (result.is_read_only()) {
        return thread->throw_message("Property '%' of type '%' is read-only", RawSymbol::create(symbol), klass.name());
      }

      if (result.is_private() && runtime->check_private_access_permitted(thread, instance) <= result.offset) {
        return thread->throw_message("Cannot assign to private property '%' of class '%'", RawSymbol::create(symbol),
                                     klass.name());
      }

      instance.set_field_at(result.offset, value);
      return value;
    }
  }

  return thread->throw_message("Object of type '%' has no writeable attribute '%'", klass.name(),
                               RawSymbol::create(symbol));
}

RawValue RawValue::cast_to_bool() const {
  return RawBool::create(truthyness());
}

RawValue RawValue::cast_to_string(Thread* thread) const {
  if (isString()) {
    return *this;
  }

  utils::Buffer buffer;
  to_string(buffer);

  if (buffer.size() > RawString::kMaxByteLength) {
    return thread->throw_message("String exceeds maximum allowed size");
  }

  return RawString::acquire(thread, buffer);
}

RawValue RawValue::op_add(Thread* thread, RawValue other) const {
  if (isInt() && other.isInt()) {
    return RawInt::create(RawInt::cast(*this).value() + RawInt::cast(other).value());
  }

  if (isNumber() && other.isNumber()) {
    return RawFloat::create(double_value() + other.double_value());
  }

  if (isString() && other.isString()) {
    return RawString::cast(this).concat(thread, RawString::cast(other));
  }

  return kNaN;
}

RawValue RawValue::op_sub(RawValue other) const {
  if (isInt() && other.isInt()) {
    return RawInt::create(RawInt::cast(*this).value() - RawInt::cast(other).value());
  }

  if (isNumber() && other.isNumber()) {
    return RawFloat::create(double_value() - other.double_value());
  }

  return kNaN;
}

RawValue RawValue::op_mul(Thread* thread, RawValue other) const {
  if (isInt() && other.isInt()) {
    return RawInt::create(RawInt::cast(*this).value() * RawInt::cast(other).value());
  }

  if (isNumber() && other.isNumber()) {
    return RawFloat::create(double_value() * other.double_value());
  }

  if ((isString() && other.isNumber()) || (isNumber() && other.isString())) {
    RawString string;
    int64_t count;

    if (isString()) {
      string = RawString::cast(this);
      count = other.int_value();
    } else {
      string = RawString::cast(other);
      count = int_value();
    }

    return string.op_mul(thread, count);
  }

  if ((isTuple() && other.isNumber()) || (isNumber() && other.isTuple())) {
    RawTuple tuple;
    int64_t count;

    if (isTuple()) {
      tuple = RawTuple::cast(this);
      count = other.int_value();
    } else {
      tuple = RawTuple::cast(other);
      count = int_value();
    }

    return tuple.op_mul(thread, count);
  }

  if ((isList() && other.isNumber()) || (isNumber() && other.isList())) {
    RawList list;
    int64_t count;

    if (isList()) {
      list = RawList::cast(this);
      count = other.int_value();
    } else {
      list = RawList::cast(other);
      count = int_value();
    }

    return list.op_mul(thread, count);
  }

  return kNaN;
}

RawValue RawValue::op_div(RawValue other) const {
  if (isNumber() && other.isNumber()) {
    return RawFloat::create(double_value() / other.double_value());
  }

  return kNaN;
}

// NOTE: Update this method together with Node::compares_equal
RawValue RawValue::op_eq(Thread* thread, RawValue other, uint32_t depth) const {
  if (depth >= kMaxComparisonRecursionDepth) {
    return thread->throw_message("Maximum recursion depth exceeded");
  }

  if (*this == other) {
    return kTrue;
  }

  if (isInt() && other.isInt()) {
    int64_t left_num = RawInt::cast(this).value();
    int64_t right_num = RawInt::cast(other).value();
    return RawBool::create(left_num == right_num);
  }

  if (isNumber() && other.isNumber()) {
    double left_num = double_value();
    double right_num = other.double_value();
    return RawBool::create(double_fuzzy_equal(left_num, right_num));
  }

  if (isString() && other.isString()) {
    auto left = RawString::cast(this);
    auto right = RawString::cast(other);

    // compare character by character, since there might be a hash collision
    if (left.hashcode() == right.hashcode()) {
      return RawBool::create(left.view() == right.view());
    }

    return kFalse;
  }

  if (isString() && other.isSymbol()) {
    return RawBool::create(RawString::cast(this).hashcode() == RawSymbol::cast(other).value());
  } else if (isSymbol() && other.isString()) {
    return RawBool::create(RawSymbol::cast(this).value() == RawString::cast(other).hashcode());
  }

  if (isBytes() && other.isBytes()) {
    auto left_bytes = RawBytes::cast(this);
    auto right_bytes = RawBytes::cast(other);

    if (left_bytes.length() != right_bytes.length()) {
      return kFalse;
    }

    // compare byte by byte, since there might be a hash collision
    if (left_bytes.hashcode() == right_bytes.hashcode()) {
      auto* left_ptr = RawBytes::data(&left_bytes);
      auto* right_ptr = RawBytes::data(&right_bytes);
      return RawBool::create(std::memcmp(left_ptr, right_ptr, left_bytes.length()) == 0);
    }

    return kFalse;
  }

  if (isTuple() && other.isTuple()) {
    auto left_tuple = RawTuple::cast(this);
    auto right_tuple = RawTuple::cast(other);

    if (left_tuple.length() != right_tuple.length()) {
      return kFalse;
    }

    size_t length = left_tuple.length();
    for (size_t i = 0; i < length; i++) {
      auto left_value = left_tuple.field_at(i);
      auto right_value = right_tuple.field_at(i);
      auto result = left_value.op_eq(thread, right_value, depth + 1);
      if (result.isFalse()) {
        return kFalse;
      } else if (result.is_error_exception()) {
        return result;
      }
    }

    return kTrue;
  }

  if (isList() && other.isList()) {
    auto left_list = RawList::cast(this);
    auto right_list = RawList::cast(other);

    if (left_list.length() != right_list.length()) {
      return kFalse;
    }

    size_t length = left_list.length();
    for (size_t i = 0; i < length; i++) {
      bool lists_have_same_length = left_list.length() == right_list.length();
      bool lists_have_original_length = left_list.length() == length;
      if (!(lists_have_same_length && lists_have_original_length)) {
        return thread->throw_message("List length changed during comparison");
      }

      auto left_value = left_list.read_at(thread, i);
      if (left_value.is_error_exception()) {
        return kErrorException;
      }

      auto right_value = right_list.read_at(thread, i);
      if (right_value.is_error_exception()) {
        return kErrorException;
      }

      auto result = left_value.op_eq(thread, right_value, depth + 1);
      if (result.isFalse()) {
        return kFalse;
      } else if (result.is_error_exception()) {
        return result;
      }
    }

    return kTrue;
  }

  return kFalse;
}

RawValue RawValue::op_neq(Thread* thread, RawValue other, uint32_t depth) const {
  // TODO: implement opposite version of op_eq here, could take some shortcuts while comparing
  RawValue result = op_eq(thread, other, depth);
  if (result.isBool()) {
    return RawBool::create(!RawBool::cast(result).value());
  }

  return result;
}

RawValue RawValue::op_lt(RawValue other) const {
  if (isInt() && other.isInt()) {
    int64_t left_num = RawInt::cast(this).value();
    int64_t right_num = RawInt::cast(other).value();
    return RawBool::create(left_num < right_num);
  }

  if (isNumber() && other.isNumber()) {
    double left_num = double_value();
    double right_num = other.double_value();
    return RawBool::create(std::isless(left_num, right_num));
  }

  if (isString() && other.isString()) {
    auto left = RawString::cast(this);
    auto right = RawString::cast(other);
    return RawBool::create(left.codepoint_length() < right.codepoint_length());
  }

  return kFalse;
}

RawValue RawValue::op_gt(RawValue other) const {
  if (isInt() && other.isInt()) {
    int64_t left_num = RawInt::cast(this).value();
    int64_t right_num = RawInt::cast(other).value();
    return RawBool::create(left_num > right_num);
  }

  if (isNumber() && other.isNumber()) {
    double left_num = double_value();
    double right_num = other.double_value();
    return RawBool::create(std::isgreater(left_num, right_num));
  }

  if (isString() && other.isString()) {
    auto left = RawString::cast(this);
    auto right = RawString::cast(other);
    return RawBool::create(left.codepoint_length() > right.codepoint_length());
  }

  return kFalse;
}

RawValue RawValue::op_le(RawValue other) const {
  if (isInt() && other.isInt()) {
    int64_t left_num = RawInt::cast(this).value();
    int64_t right_num = RawInt::cast(other).value();
    return RawBool::create(left_num <= right_num);
  }

  if (isNumber() && other.isNumber()) {
    double left_num = double_value();
    double right_num = other.double_value();
    return RawBool::create(std::islessequal(left_num, right_num));
  }

  if (isString() && other.isString()) {
    auto left = RawString::cast(this);
    auto right = RawString::cast(other);
    return RawBool::create(left.codepoint_length() <= right.codepoint_length());
  }

  return kFalse;
}

RawValue RawValue::op_ge(RawValue other) const {
  if (isInt() && other.isInt()) {
    int64_t left_num = RawInt::cast(this).value();
    int64_t right_num = RawInt::cast(other).value();
    return RawBool::create(left_num >= right_num);
  }

  if (isNumber() && other.isNumber()) {
    double left_num = double_value();
    double right_num = other.double_value();
    return RawBool::create(std::isgreaterequal(left_num, right_num));
  }

  if (isString() && other.isString()) {
    auto left = RawString::cast(this);
    auto right = RawString::cast(other);
    return RawBool::create(left.codepoint_length() >= right.codepoint_length());
  }

  return kFalse;
}

RawValue RawValue::op_spaceship(RawValue other) const {
  if (isInt() && other.isInt()) {
    int64_t left = RawInt::cast(this).value();
    int64_t right = RawInt::cast(other).value();

    if (left == right) {
      return RawInt::create(0);
    } else if (left < right) {
      return RawInt::create(-1);
    } else {
      return RawInt::create(1);
    }
  }

  if (isNumber() && other.isNumber()) {
    double left = double_value();
    double right = other.double_value();

    if (std::isgreater(left, right)) {
      return RawInt::create(1);
    } else if (std::isless(left, right)) {
      return RawInt::create(-1);
    } else {
      return RawInt::create(0);
    }
  }

  if (isString() && other.isString()) {
    size_t left_length = RawString::cast(this).codepoint_length();
    size_t right_length = RawString::cast(other).codepoint_length();

    if (left_length == right_length) {
      return RawInt::create(0);
    } else if (left_length < right_length) {
      return RawInt::create(-1);
    } else {
      return RawInt::create(1);
    }
  }

  // All other types compare equal with the spaceship operator
  // TODO: implement spaceship operator for lists and tuples
  return RawInt::create(0);
}

RawValue RawValue::op_usub() const {
  if (isInt()) {
    return RawInt::create(-RawInt::cast(this).value());
  } else if (isFloat()) {
    return RawFloat::create(-RawFloat::cast(this).value());
  } else if (isBool()) {
    return RawBool::create(!RawBool::cast(this).value());
  }

  return kNaN;
}

RawValue RawValue::op_unot() const {
  return truthyness() ? kFalse : kTrue;
}

bool RawValue::isInt() const {
  return (m_raw & kMaskInt) == kTagInt;
}

bool RawValue::isFloat() const {
  return (m_raw & kMaskImmediate) == kTagFloat;
}

bool RawValue::isBool() const {
  return (m_raw & kMaskImmediate) == kTagBool;
}

bool RawValue::isSymbol() const {
  return (m_raw & kMaskImmediate) == kTagSymbol;
}

bool RawValue::isNull() const {
  return (m_raw & kMaskImmediate) == kTagNull;
}

bool RawValue::isSmallString() const {
  return (m_raw & kMaskImmediate) == kTagSmallString;
}

bool RawValue::isSmallBytes() const {
  return (m_raw & kMaskImmediate) == kTagSmallBytes;
}

bool RawValue::isValue() const {
  return true;
}

bool RawValue::isObject() const {
  return is_old_pointer() || is_young_pointer();
}

bool RawValue::isInstance() const {
  if (isObject()) {
    return is_instance_shape(RawObject::cast(this).shape_id());
  } else {
    return false;
  }
}

bool RawValue::isData() const {
  if (isObject()) {
    return is_data_shape(RawObject::cast(this).shape_id());
  } else {
    return false;
  }
}

bool RawValue::isBytes() const {
  if (isObject()) {
    ShapeId shape_id = RawObject::cast(this).shape_id();
    switch (shape_id) {
      case ShapeId::kLargeBytes:
      case ShapeId::kHugeBytes: {
        return true;
      }
      default: {
        return false;
      }
    }
  } else {
    return isSmallBytes();
  }
}

bool RawValue::isString() const {
  if (isObject()) {
    ShapeId shape_id = RawObject::cast(this).shape_id();
    switch (shape_id) {
      case ShapeId::kLargeString:
      case ShapeId::kHugeString: {
        return true;
      }
      default: {
        return false;
      }
    }
  } else {
    return isSmallString();
  }
}

bool RawValue::isLargeBytes() const {
  return shape_id() == ShapeId::kLargeBytes;
}

bool RawValue::isLargeString() const {
  return shape_id() == ShapeId::kLargeString;
}

bool RawValue::isHugeBytes() const {
  return shape_id() == ShapeId::kHugeBytes;
}

bool RawValue::isHugeString() const {
  return shape_id() == ShapeId::kHugeString;
}

bool RawValue::isTuple() const {
  return shape_id() == ShapeId::kTuple;
}

bool RawValue::isList() const {
  return shape_id() == ShapeId::kList;
}

bool RawValue::isClass() const {
  if (isInstance()) {
    return RawInstance::cast(this).is_instance_of(ShapeId::kClass);
  }
  return false;
}

bool RawValue::isShape() const {
  return shape_id() == ShapeId::kShape;
}

bool RawValue::isFunction() const {
  return shape_id() == ShapeId::kFunction;
}

bool RawValue::isBuiltinFunction() const {
  return shape_id() == ShapeId::kBuiltinFunction;
}

bool RawValue::isFiber() const {
  return shape_id() == ShapeId::kFiber;
}

bool RawValue::isFuture() const {
  return shape_id() == ShapeId::kFuture;
}

bool RawValue::isException() const {
  if (isInstance()) {
    return RawInstance::cast(this).is_instance_of(ShapeId::kException);
  }
  return false;
}

bool RawValue::isImportException() const {
  if (isInstance()) {
    return RawInstance::cast(this).is_instance_of(ShapeId::kImportException);
  }
  return false;
}

bool RawValue::isAssertionException() const {
  if (isInstance()) {
    return RawInstance::cast(this).is_instance_of(ShapeId::kAssertionException);
  }
  return false;
}

bool RawValue::isTrue() const {
  return *this == kTrue;
}

bool RawValue::isFalse() const {
  return *this == kFalse;
}

bool RawValue::isNumber() const {
  return isInt() || isFloat();
}

int64_t RawValue::int_value() const {
  if (isInt()) {
    return RawInt::cast(this).value();
  } else if (isFloat()) {
    auto value = RawFloat::cast(this);

    if (value.isfinite()) {
      return value.value();
    }

    if (value.isnan()) {
      return 0;
    } else if (value.isinf()) {
      if (value.isgreater(0)) {
        return RawInt::kMaxValue;
      } else {
        return RawInt::kMinValue;
      }
    }

    UNREACHABLE();
  }

  UNREACHABLE();
}

double RawValue::double_value() const {
  if (isInt()) {
    return RawInt::cast(this).value();
  } else if (isFloat()) {
    return RawFloat::cast(this).value();
  }

  UNREACHABLE();
}

void RawValue::to_string(std::ostream& out) const {
  if (isString()) {
    RawString value = RawString::cast(this);
    out << value.view();
    return;
  }

  dump(out);
}

void RawValue::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);

  // FIXME: build some mechanism to automatically print <...> for
  //        values that have already been printed before (cyclic data structures)

  if (isInt()) {
    writer.fg(Color::Cyan, RawInt::cast(this).value());
    return;
  }

  if (isString()) {
    auto value = RawString::cast(this);
    writer.fg(Color::Red, "\"", value.view(), "\"");
    return;
  }

  if (isBytes()) {
    auto value = RawBytes::cast(this);
    writer.fg(Color::Grey, "[");
    for (size_t i = 0; i < value.length(); i++) {
      uint8_t byte = *(RawBytes::data(&value) + i);

      if (i) {
        out << " ";
      }

      writer.fg(Color::Red, std::hex, byte, std::dec);
    }
    writer.fg(Color::Grey, "]");
    return;
  }

  if (isObject()) {
    if (isTuple()) {
      auto tuple = RawTuple::cast(this);

      writer.fg(Color::Grey, "(");

      if (tuple.length()) {
        tuple.field_at(0).dump(out);
        for (uint32_t i = 1; i < tuple.length(); i++) {
          writer << ", ";
          tuple.field_at(i).dump(out);
        }
      }

      writer.fg(Color::Grey, ")");

      return;
    }

    if (isList()) {
      auto list = RawList::cast(this);

      writer.fg(Color::Grey, "[");

      if (list.is_locked()) {
        writer.fg(Color::Grey, "...");
      } else {
        // TODO: this lock needs to be taken because we do not want the list to change while we're printing it
        std::lock_guard locker(list);
        RawValue* data = list.data();
        size_t length = list.length();

        if (length) {
          DCHECK(data);
          data[0].dump(out);
          for (uint32_t i = 1; i < length; i++) {
            writer << ", ";
            data[i].dump(out);
          }
        }
      }

      writer.fg(Color::Grey, "]");

      return;
    }

    if (isFunction()) {
      auto function = RawFunction::cast(this);

      const SharedFunctionInfo& shared_info = *function.shared_info();
      const core::compiler::ir::FunctionInfo& ir_info = shared_info.ir_info;
      bool arrow = ir_info.arrow_function;
      const std::string& name = shared_info.name;

      if (arrow) {
        writer.fg(Color::Magenta, "->()");
      } else {
        if (function.host_class().isClass()) {
          auto klass = RawClass::cast(function.host_class());
          if (klass.flags() & RawClass::kFlagStatic) {
            writer.fg(Color::Yellow, "static func ", klass.name(), "::", name, "()");
          } else {
            writer.fg(Color::Yellow, "func ", klass.name(), "::", name, "()");
          }
        } else {
          writer.fg(Color::Yellow, "func ", name, "()");
        }
      }

      return;
    }

    if (isBuiltinFunction()) {
      auto builtin_function = RawBuiltinFunction::cast(this);
      RawString name = builtin_function.name();
      writer.fg(Color::Magenta, "builtin func ", name, "()");
      return;
    }

    if (isFuture()) {
      auto future = RawFuture::cast(this);
      writer.fg(Color::Red, "<Future ");
      if (future.wait_queue() == nullptr) {
        if (future.exception().isNull()) {
          writer.fg(Color::Red, "resolved>");
        } else {
          writer.fg(Color::Red, "rejected>");
        }
      } else {
        writer.fg(Color::Red, "pending>");
      }
      return;
    }

    if (isShape()) {
      auto shape = RawShape::cast(this);
      writer.fg(Color::Cyan, "<Shape #", static_cast<uint32_t>(shape.own_shape_id()), ":(");

      RawTuple keys = shape.keys();
      for (uint32_t i = 0; i < keys.length(); i++) {
        auto encoded = keys.field_at<RawInt>(i);
        SYMBOL prop_name;
        uint8_t prop_flags;
        RawShape::decode_shape_key(encoded, prop_name, prop_flags);
        writer.fg(Color::Cyan, RawSymbol::create(prop_name));

        if (i < keys.length() - 1) {
          writer << ", ";
        }
      }

      writer.fg(Color::Cyan, ")>");
      return;
    }

    if (isClass()) {
      auto klass = RawClass::cast(this);
      writer.fg(Color::Yellow, "<Class ", klass.name(), ">");
      return;
    }

    if (isException()) {
      if (isImportException()) {
        auto exception = RawImportException::cast(this);
        auto errors = exception.errors();
        auto message = RawString::cast(exception.message());

        writer << message.view() << "\n";

        for (uint32_t i = 0; i < errors.length(); i++) {
          auto error = errors.field_at<RawTuple>(i);
          auto type = error.field_at<RawString>(0);
          auto filepath = error.field_at<RawString>(1);
          auto error_message = error.field_at<RawString>(2);
          auto source = error.field_at<RawString>(3);
          auto location = error.field_at<RawString>(4);

          if (type.view() == "error") {
            writer.fg(Color::Red, type.view(), ": ");
          } else if (type.view() == "warning") {
            writer.fg(Color::Yellow, type.view(), ": ");
          } else if (type.view() == "info") {
            writer.fg(Color::Blue, type.view(), ": ");
          } else {
            FAIL("unknown error type");
          }

          writer << filepath.view() << ":" << location.view() << ": " << error_message.view() << "\n";
          writer << source.view();
        }
        return;
      } else {
        auto exception = RawException::cast(this);
        auto klass = RawClass::cast(exception.klass_field());
        RawValue message = exception.message();
        RawTuple backtrace = exception.backtrace();
        writer.fg(Color::Green, termcolor::bold, klass.name());
        writer << ": ";
        writer.fg(Color::White, message);

        for (size_t i = 0; i < backtrace.length(); i++) {
          auto entry = backtrace.field_at<RawTuple>(i);
          switch (entry.length()) {
            case 1: {
              auto function = entry.field_at<RawBuiltinFunction>(0);
              writer << "\n    at " << function;
              break;
            }
            case 4: {
              auto function = entry.field_at<RawFunction>(0);
              auto filename = entry.field_at<RawString>(1);
              auto row = entry.field_at<RawInt>(2).value();
              auto col = entry.field_at<RawInt>(3).value();
              writer << "\n    at " << function;

//              if (function.shared_info()->ir_info.arrow_function) {
//                writer.fg(Color::Red, function.name());
//              } else {
//                writer.fg(Color::Yellow, function.name());
//              }

              writer.fg(Color::Grey, " ", filename, ":", row, ":", col);
              break;
            }
            default: UNREACHABLE();
          }
        }

        return;
      }
    }

    if (isInstance()) {
      auto instance = RawInstance::cast(this);
      auto klass = RawClass::cast(instance.klass_field());
      writer.fg(Color::Green, "<", klass.name(), " ", instance.address_voidptr(), ">");
      return;
    }

    auto object = RawObject::cast(this);
    auto shape_id = object.header()->shape_id();
    writer.fg(Color::Green, "<?? #", (uint32_t)shape_id, " ", object.address_voidptr(), ">");
    return;
  }

  if (isFloat()) {
    auto old_flags = out.flags();
    out << std::defaultfloat;
    out << std::setprecision(10);
    writer.fg(Color::Red, RawFloat::cast(this).value());
    out.flags(old_flags);
    return;
  }

  if (isBool()) {
    if (RawBool::cast(this).value()) {
      writer.fg(Color::Green, "true");
    } else {
      writer.fg(Color::Red, "false");
    }
    return;
  }

  if (isSymbol()) {
    RawSymbol symbol = RawSymbol::cast(this);

    if (auto* thread = Thread::current()) {
      auto sym_value = thread->lookup_symbol(symbol.value());
      if (sym_value.isString()) {
        auto string = RawString::cast(sym_value);
        out << string.view();
        return;
      }
    }

    out << std::hex << symbol.value() << std::dec;
    return;
  }

  if (isNull()) {
    RawNull null_value = RawNull::cast(this);
    writer.fg(Color::Grey, "null");
    if (null_value.is_error()) {
      writer.fg(Color::Yellow, " (", kErrorCodeNames[(uint8_t)null_value.error_code()], ")");
    }
    return;
  }

  UNREACHABLE();
}

std::ostream& operator<<(std::ostream& out, const RawValue& value) {
  value.to_string(out);
  return out;
}

RawValue RawValue::build_spread_size_table(
  Thread* thread, const RawValue* segments, uint32_t segment_count, size_t* segment_sizes, size_t* total_size) {
  for (uint32_t i = 0; i < segment_count; i++) {
    const RawValue& segment = segments[i];

    if (segment.isTuple()) {
      size_t size = RawTuple::cast(segment).length();
      segment_sizes[i] = size;
      *total_size += size;
    } else if (segment.isList()) {
      size_t size = RawList::cast(segment).length();
      segment_sizes[i] = size;
      *total_size += size;
    } else if (segment.isString()) {
      size_t size = RawString::cast(segment).codepoint_length();
      segment_sizes[i] = size;
      *total_size += size;
    } else {
      return thread->throw_message("Cannot spread unpack value of type '%'", segment.klass_name(thread));
    }
  }

  return kNull;
}

RawValue RawValue::unpack_spread_segments_to_buffer(Thread* thread,
                                                    const RawValue* segments,
                                                    uint32_t segment_count,
                                                    size_t* segment_sizes,
                                                    RawValue* destination_buffer,
                                                    size_t total_size) {
  uint32_t next_write_index = 0;
  for (uint32_t i = 0; i < segment_count; i++) {
    const RawValue* segment = &segments[i];

    if (segment->isTuple()) {
      auto seg_tuple = RawTuple::cast(segment);
      size_t seg_length = seg_tuple.length();
      DCHECK(seg_length == segment_sizes[i]);
      std::memcpy(destination_buffer + next_write_index, seg_tuple.data(), seg_length * kPointerSize);
      next_write_index += seg_length;
    } else if (segment->isList()) {
      auto seg_list = RawList::cast(segment);
      std::unique_lock locker(seg_list);
      size_t seg_length = seg_list.length();

      if (seg_length != segment_sizes[i]) {
        locker.unlock();
        return thread->throw_message("List length changed during segment unpack");
      }

      std::memcpy(destination_buffer + next_write_index, seg_list.data(), seg_length * kPointerSize);
      next_write_index += seg_length;
    } else if (segment->isString()) {
      auto seg_string = RawString::cast(segment);
      size_t seg_length = seg_string.codepoint_length();
      DCHECK(seg_length == segment_sizes[i]);

      seg_string.each_codepoint([&](uint32_t cp, size_t index) {
        destination_buffer[next_write_index + index] = RawSmallString::create_from_cp(cp);
      });

      next_write_index += seg_length;
    } else {
      FAIL("Unexpected object type");
    }
  }

  DCHECK(next_write_index == total_size);

  return kNull;
}

int64_t RawInt::value() const {
  auto tmp = static_cast<int64_t>(m_raw);
  return tmp >> kShiftInt;
}

uintptr_t RawInt::external_pointer_value() const {
  return static_cast<uintptr_t>(value());
}

RawInt RawInt::create(int64_t value) {
  CHECK(is_valid(value));
  return create_truncate(value);
}

RawInt RawInt::create_truncate(int64_t value) {
  return RawInt::cast((static_cast<uintptr_t>(value) << kShiftInt) | kTagInt);
}

RawInt RawInt::create_from_external_pointer(uintptr_t value) {
  DCHECK((value & kExternalPointerValidationMask) == 0);
  return create(static_cast<int64_t>(value));
}

bool RawInt::is_valid(int64_t value) {
  return value >= kMinValue && value <= kMaxValue;
}

double RawFloat::value() const {
  uintptr_t doublepart = m_raw & (~kMaskImmediate);
  return bitcast<double>(doublepart);
}

RawFloat RawFloat::create(double value) {
  auto value_bits = bitcast<uintptr_t>(value);
  return RawFloat::cast((value_bits & ~kMaskImmediate) | kTagFloat);
}

bool RawBool::value() const {
  return (m_raw >> kShiftBool) != 0;
}

RawBool RawBool::create(bool value) {
  if (value) {
    return RawBool::cast((uintptr_t{ 1 } << kShiftBool) | kTagBool);
  } else {
    return RawBool::cast(kTagBool);
  }
}

SYMBOL RawSymbol::value() const {
  return m_raw >> kShiftSymbol;
}

RawSymbol RawSymbol::create(SYMBOL symbol) {
  return RawSymbol::cast((uintptr_t{ symbol } << kShiftSymbol) | kTagSymbol);
}

size_t RawSmallString::byte_length() const {
  return (m_raw & kMaskLength) >> kShiftLength;
}

size_t RawSmallString::codepoint_length() const {
  return RawString::cast(this).codepoint_length();
}

const char* RawSmallString::data(const RawSmallString* value) {
  uintptr_t address = bitcast<uintptr_t>(value) + 1;
  return bitcast<const char*>(address);
}

SYMBOL RawSmallString::hashcode() const {
  return crc32::hash_block(RawSmallString::data(this), byte_length());
}

RawSmallString RawSmallString::create_from_cp(uint32_t cp) {
  utils::Buffer buf(4);
  buf.write_utf8_cp(cp);
  DCHECK(buf.size() <= 4);
  return create_from_memory(buf.data(), utf8::sequence_length(cp));
}

RawSmallString RawSmallString::create_from_str(const std::string& string) {
  return create_from_memory(string.data(), string.size());
}

RawSmallString RawSmallString::create_from_cstr(const char* value) {
  return create_from_memory(value, std::strlen(value));
}

RawSmallString RawSmallString::create_from_memory(const char* value, size_t length) {
  CHECK(length <= kMaxLength);

  uintptr_t immediate = 0;
  char* base_ptr = bitcast<char*>(&immediate) + 1;
  std::memcpy(bitcast<void*>(base_ptr), value, length);

  immediate |= (length << kShiftLength) & kMaskLowByte;
  immediate |= kTagSmallString;

  return RawSmallString::cast(immediate);
}

RawSmallString RawSmallString::create_empty() {
  return RawSmallString::cast(kTagSmallString);
}

size_t RawSmallBytes::length() const {
  return (m_raw & kMaskLength) >> kShiftLength;
}

const uint8_t* RawSmallBytes::data(const RawSmallBytes* value) {
  auto base = bitcast<uintptr_t>(value);
  return bitcast<const uint8_t*>(base + 1);
}

SYMBOL RawSmallBytes::hashcode() const {
  return crc32::hash_block(bitcast<const char*>(RawSmallBytes::data(this)), length());
}

RawSmallBytes RawSmallBytes::create_from_memory(const uint8_t* value, size_t length) {
  CHECK(length <= kMaxLength);

  uintptr_t immediate = 0;
  char* base_ptr = bitcast<char*>(&immediate) + 1;
  std::memcpy(bitcast<void*>(base_ptr), value, length);

  immediate |= (length << kShiftLength) & kMaskLowByte;
  immediate |= kTagSmallBytes;

  return RawSmallBytes::cast(immediate);
}

RawSmallBytes RawSmallBytes::create_empty() {
  return RawSmallBytes::cast(kTagSmallBytes);
}

ErrorId RawNull::error_code() const {
  return static_cast<ErrorId>(raw() >> kShiftError);
}

RawNull RawNull::create() {
  return RawNull::cast(kTagNull);
}

RawNull RawNull::create_error(ErrorId id) {
  return RawNull::cast((static_cast<uint8_t>(id) << kShiftError) | kTagNull);
}

RawString RawString::create(Thread* thread, const char* data, size_t size, SYMBOL hash) {
  if (size <= RawSmallString::kMaxLength) {
    return RawString::cast(RawSmallString::create_from_memory(data, size));
  } else if (size <= kHeapRegionUsableSizeForPayload) {
    return RawString::cast(RawLargeString::create(thread, data, size, hash));
  } else {
    return RawString::cast(RawHugeString::create(thread, data, size, hash));
  }
}

RawString RawString::create(Thread* thread, const char* data, size_t size) {
  if (size <= RawSmallString::kMaxLength) {
    return RawString::cast(RawSmallString::create_from_memory(data, size));
  } else if (size <= kHeapRegionUsableSizeForPayload) {
    return RawString::cast(RawLargeString::create(thread, data, size));
  } else {
    return RawString::cast(RawHugeString::create(thread, data, size));
  }
}

RawString RawString::create(Thread* thread, const std::string& string) {
  return RawString::create(thread, string.data(), string.size());
}

RawString RawString::acquire(Thread* thread, char* cstr, size_t size, SYMBOL hash) {
  if (size <= RawSmallString::kMaxLength) {
    auto result = RawString::cast(RawSmallString::create_from_memory(cstr, size));
    utils::Allocator::free(cstr);
    return result;
  } else {
    return RawString::cast(RawHugeString::acquire(thread, cstr, size, hash));
  }
}

RawString RawString::acquire(Thread* thread, char* cstr, size_t size) {
  if (size <= RawSmallString::kMaxLength) {
    auto result = RawString::cast(RawSmallString::create_from_memory(cstr, size));
    utils::Allocator::free(cstr);
    return result;
  } else {
    return RawString::cast(RawHugeString::acquire(thread, cstr, size));
  }
}

RawString RawString::acquire(Thread* thread, utils::Buffer& buffer) {
  size_t size = buffer.size();
  return RawString::acquire(thread, buffer.release_buffer(), size);
}

size_t RawString::byte_length() const {
  if (isSmallString()) {
    return RawSmallString::cast(this).byte_length();
  } else {
    if (isLargeString()) {
      return RawLargeString::cast(this).byte_length();
    }

    if (isHugeString()) {
      return RawHugeString::cast(this).byte_length();
    }
  }

  UNREACHABLE();
}

size_t RawString::codepoint_length() const {
  const char* begin = RawString::data(this);
  const char* end = begin + byte_length();
  return utf8::codepoint_count(begin, end);
}

const char* RawString::data(const RawString* value) {
  if (value->isSmallString()) {
    return RawSmallString::data(bitcast<const RawSmallString*>(value));
  } else {
    if (value->isLargeString()) {
      return RawLargeString::cast(value).data();
    }

    if (value->isHugeString()) {
      return RawHugeString::cast(value).data();
    }
  }

  UNREACHABLE();
}

SYMBOL RawString::hashcode() const {
  if (isSmallString()) {
    return RawSmallString::cast(this).hashcode();
  } else {
    if (isLargeString()) {
      return RawLargeString::cast(this).hashcode();
    }

    if (isHugeString()) {
      return RawHugeString::cast(this).hashcode();
    }
  }

  UNREACHABLE();
}

std::string RawString::str() const {
  return { data(this), byte_length() };
}

std::string_view RawString::view() const& {
  return { data(this), byte_length() };
}

int32_t RawString::compare(RawString base, const char* data, size_t length) {
  std::string_view base_view = base.view();
  std::string_view other_view(data, length);
  return base_view.compare(other_view);
}

RawValue RawString::concat(Thread* thread, RawString other) const {
  size_t self_size = byte_length();
  size_t other_size = other.byte_length();
  size_t total_size = self_size + other_size;

  if (total_size > RawString::kMaxByteLength) {
    return thread->throw_message("String exceeds maximum allowed size");
  }

  utils::Buffer buffer(total_size);
  buffer.write(RawString::data(this), self_size);
  buffer.write(RawString::data(&other), other_size);
  return RawString::acquire(thread, buffer);
}

RawValue RawString::op_mul(Thread* thread, int64_t count) const {
  if (count <= 0) {
    return RawString::cast(kEmptyString);
  }

  if (count == 1) {
    return *this;
  }

  size_t byte_length = this->byte_length();
  if (byte_length == 0) {
    return *this;
  }

  if ((size_t)count > RawString::kMaxByteLength) {
    return thread->throw_message("String exceeds maximum allowed size");
  }

  size_t new_length = byte_length * count;
  if (new_length > RawString::kMaxByteLength) {
    return thread->throw_message("String exceeds maximum allowed size");
  }

  constexpr int64_t kCachedCopyThreshold = 1024;

  utils::Buffer buffer(new_length);
  utils::Buffer value_buf;
  to_string(value_buf);
  if (count <= kCachedCopyThreshold) {
    for (int64_t i = 0; i < count; i++) {
      buffer.write_buffer(value_buf);
    }
  } else {
    // if we're multiplying by a sufficiently big number (>kCachedCopyThreshold)
    // we're splitting the multiplication into multiple steps
    // copying a very large buffer a few times is faster than copying a small buffer many times
    size_t tmp_count = std::floor(std::sqrt(count));
    size_t remaining = count - (tmp_count * tmp_count);

    utils::Buffer tmp_buffer(tmp_count * byte_length);
    for (size_t i = 0; i < tmp_count; i++) {
      tmp_buffer.write_buffer(value_buf);
    }

    for (size_t i = 0; i < tmp_count; i++) {
      buffer.write_buffer(tmp_buffer);
    }

    for (size_t i = 0; i < remaining; i++) {
      buffer.write_buffer(value_buf);
    }
  }

  return RawString::acquire(thread, buffer);
}

void RawString::each_codepoint(std::function<void(uint32_t, size_t)> callback) const {
  RawString string = *this;
  const char* data = RawString::data(&string);
  size_t byte_length = this->byte_length();

  const char* it = data;
  const char* end_it = it + byte_length;
  size_t i = 0;
  while (it < end_it) {
    uint32_t cp;
    bool result = utf8::next(it, end_it, cp);
    CHECK(result, "invalid utf8");
    callback(cp, i);
    i++;
  }
}

size_t RawBytes::length() const {
  if (isSmallBytes()) {
    return RawSmallBytes::cast(this).length();
  } else {
    if (isLargeBytes()) {
      return RawLargeBytes::cast(this).length();
    }

    if (isHugeBytes()) {
      return RawHugeBytes::cast(this).length();
    }
  }

  UNREACHABLE();
}

const uint8_t* RawBytes::data(const RawBytes* value) {
  if (value->isSmallBytes()) {
    return RawSmallBytes::data(bitcast<const RawSmallBytes*>(value));
  } else {
    if (value->isLargeBytes()) {
      return RawLargeBytes::cast(value).data();
    }

    if (value->isHugeBytes()) {
      return RawHugeBytes::cast(value).data();
    }
  }

  UNREACHABLE();
}

SYMBOL RawBytes::hashcode() const {
  if (isSmallBytes()) {
    return RawSmallBytes::cast(this).hashcode();
  } else {
    if (isLargeBytes()) {
      return RawLargeBytes::cast(this).hashcode();
    }

    if (isHugeBytes()) {
      return RawHugeBytes::cast(this).hashcode();
    }
  }

  UNREACHABLE();
}

bool RawBytes::compare(RawBytes base, const uint8_t* data, size_t length) {
  if (base.length() != length) {
    return false;
  }

  return std::memcmp(RawBytes::data(&base), data, length) == 0;
}

uintptr_t RawObject::address() const {
  return m_raw & ~kMaskImmediate;
}

void* RawObject::address_voidptr() const {
  return bitcast<void*>(address());
}

uintptr_t RawObject::base_address() const {
  return address() - sizeof(ObjectHeader);
}

ShapeId RawObject::shape_id() const {
  return header()->shape_id();
}

size_t RawObject::count() const {
  return header()->count();
}

bool RawObject::contains_external_heap_pointers() const {
  return is_shape_with_external_heap_pointers(shape_id());
}

ObjectHeader* RawObject::header() const {
  return ObjectHeader::header_at_address(base_address());
}

void RawObject::lock() const {
  header()->m_lock.lock();
}

void RawObject::unlock() const {
  header()->m_lock.unlock();
}

bool RawObject::is_locked() const {
  return header()->m_lock.is_locked();
}

void RawObject::set_field_at(uint32_t index, RawValue value) const {
  field_at(index) = value;
  gc_write_barrier(value);
}

void RawObject::gc_write_barrier(RawValue written_value) const {
  if (written_value.is_young_pointer() && this->is_old_pointer()) {
    HeapRegion* region = header()->heap_region();
    DCHECK(region->type == HeapRegion::Type::Old);
    size_t span_index = region->span_get_index_for_pointer(base_address());
    region->span_set_dirty_flag(span_index);
  }
}

RawObject RawObject::create_from_ptr(uintptr_t address, bool is_young) {
  CHECK((address % kObjectAlignment) == 0, "invalid pointer alignment");
  auto object = RawObject::cast(address | (is_young ? kTagYoungObject : kTagOldObject));
  return object;
}

RawData RawData::create(Thread* thread, ShapeId shape_id, size_t size) {
  DCHECK(size <= kHeapRegionUsableSizeForPayload);

  // determine the total allocation size
  size_t header_size = sizeof(ObjectHeader);
  size_t total_size = align_to_size(header_size + size, kObjectAlignment);
  DCHECK(total_size <= kHeapRegionUsableSize);

  uintptr_t memory = thread->allocate(total_size);
  DCHECK(memory);

  // initialize header
  ObjectHeader::initialize_header(memory, shape_id, size);
  ObjectHeader* header = bitcast<ObjectHeader*>(memory);

  if (shape_id == ShapeId::kTuple) {
    CHECK(header->cas_count(size, size / kPointerSize));
  }

  return RawData::cast(RawObject::create_from_ptr(memory + header_size));
}

size_t RawData::length() const {
  return count();
}

const uint8_t* RawData::data() const {
  return bitcast<const uint8_t*>(address());
}

SYMBOL RawData::hashcode() const {
  return header()->hashcode();
}

RawLargeString RawLargeString::create(Thread* thread, const char* data, size_t size, SYMBOL hash) {
  auto string = RawLargeString::create(thread, data, size);
  string.header()->cas_hashcode((SYMBOL)0, hash);
  string.header()->set_has_cached_hashcode();
  return string;
}

RawLargeString RawLargeString::create(Thread* thread, const char* data, size_t size) {
  auto string = RawLargeString::cast(RawData::create(thread, ShapeId::kLargeString, size));
  std::memcpy((char*)string.address(), data, size);
  return string;
}

size_t RawLargeString::byte_length() const {
  return length();
}

const char* RawLargeString::data() const {
  return bitcast<const char*>(address());
}

RawTuple RawTuple::create_empty(Thread* thread) {
  return RawTuple::create(thread, 0);
}

RawTuple RawTuple::create(Thread* thread, uint32_t count) {
  return RawTuple::cast(RawData::create(thread, ShapeId::kTuple, count * kPointerSize));
}

RawTuple RawTuple::create(Thread* thread, RawValue value) {
  HandleScope scope(thread);
  Value v1(scope, value);
  Tuple tuple(scope, RawTuple::create(thread, 1));
  tuple.set_field_at(0, v1);
  return *tuple;
}

RawTuple RawTuple::create(Thread* thread, RawValue value1, RawValue value2) {
  HandleScope scope(thread);
  Value v1(scope, value1);
  Value v2(scope, value2);
  Tuple tuple(scope, RawTuple::create(thread, 2));
  tuple.set_field_at(0, v1);
  tuple.set_field_at(1, v2);
  return *tuple;
}

RawTuple RawTuple::create(Thread* thread, RawValue value1, RawValue value2, RawValue value3) {
  HandleScope scope(thread);
  Value v1(scope, value1);
  Value v2(scope, value2);
  Value v3(scope, value3);
  Tuple tuple(scope, RawTuple::create(thread, 3));
  tuple.set_field_at(0, v1);
  tuple.set_field_at(1, v2);
  tuple.set_field_at(2, v3);
  return *tuple;
}

RawTuple RawTuple::create(Thread* thread, RawValue value1, RawValue value2, RawValue value3, RawValue value4) {
  HandleScope scope(thread);
  Value v1(scope, value1);
  Value v2(scope, value2);
  Value v3(scope, value3);
  Value v4(scope, value4);
  Tuple tuple(scope, RawTuple::create(thread, 4));
  tuple.set_field_at(0, v1);
  tuple.set_field_at(1, v2);
  tuple.set_field_at(2, v3);
  tuple.set_field_at(3, v4);
  return *tuple;
}

RawTuple RawTuple::concat_value(Thread* thread, RawTuple left, RawValue value) {
  uint32_t left_length = left.length();
  uint64_t new_length = left_length + 1;
  CHECK(new_length <= kInt32Max);

  HandleScope scope(thread);
  Value new_value(scope, value);
  Tuple old_tuple(scope, left);
  Tuple new_tuple(scope, RawTuple::create(thread, new_length));
  for (uint32_t i = 0; i < left_length; i++) {
    new_tuple.set_field_at(i, left.field_at(i));
  }
  new_tuple.set_field_at(new_length - 1, new_value);

  return *new_tuple;
}

RawValue RawTuple::create_spread(Thread* thread, const RawValue* segments, uint32_t segment_count) {
  size_t segment_sizes[segment_count];
  size_t total_size = 0;

  if (build_spread_size_table(thread, segments, segment_count, segment_sizes, &total_size).is_error_exception()) {
    return kErrorException;
  }

  if (total_size > kHeapRegionMaximumObjectFieldCount) {
    return thread->throw_message("Tuple exceeded max size");
  }

  auto tuple = RawTuple::create(thread, total_size);
  if (total_size == 0) {
    return tuple;
  }

  auto* destination = tuple.data();
  if (unpack_spread_segments_to_buffer(thread, segments, segment_count, segment_sizes, destination, total_size)
        .is_error_exception()) {
    return kErrorException;
  }

  return tuple;
}

RawValue* RawTuple::data() const {
  return bitcast<RawValue*>(address());
}

uint32_t RawTuple::length() const {
  return count();
}

RawValue RawTuple::op_mul(Thread* thread, int64_t count) const {
  HandleScope scope(thread);
  Tuple tuple(scope, *this);

  if (count < 0) {
    count = 0;
  }

  size_t old_length = length();
  size_t new_length = count * old_length;
  if (new_length > kHeapRegionMaximumObjectFieldCount) {
    return thread->throw_message("Tuple exceeded max size");
  }

  Tuple new_tuple(scope, RawTuple::create(thread, new_length));

  RawValue* source = tuple.data();
  RawValue* destination = new_tuple.data();

  for (size_t i = 0; i < (size_t)count; i++) {
    std::memcpy(destination + (i * old_length), source, old_length * kPointerSize);
  }

  return new_tuple;
}

RawInstance RawInstance::create(Thread* thread, ShapeId shape_id, size_t field_count, RawValue _klass) {
  HandleScope scope(thread);
  Value klass(scope, _klass);

  // determine the allocation size
  DCHECK(field_count >= 1);
  DCHECK(field_count <= RawInstance::kMaximumFieldCount);
  size_t object_size = field_count * kPointerSize;
  size_t header_size = sizeof(ObjectHeader);
  size_t total_size = align_to_size(header_size + object_size, kObjectAlignment);

  uintptr_t memory = thread->allocate(total_size, is_shape_with_external_heap_pointers(shape_id));
  DCHECK(memory);

  // initialize header
  ObjectHeader::initialize_header(memory, shape_id, field_count);

  // initialize fields to null
  uintptr_t object = memory + header_size;
  auto* object_fields = bitcast<RawValue*>(object);
  for (uint32_t i = 0; i < field_count; i++) {
    object_fields[i] = kNull;
  }

  auto instance = RawInstance::cast(RawObject::create_from_ptr(object));
  instance.set_klass_field(klass);
  return instance;
}

RawInstance RawInstance::create(Thread* thread, RawShape shape, RawValue klass) {
  return RawInstance::create(thread, shape.own_shape_id(), shape.keys().length(), klass);
}

RawInstance RawInstance::create(Thread* thread, RawClass klass) {
  return RawInstance::create(thread, klass.shape_instance(), klass);
}

uint32_t RawInstance::field_count() const {
  return count();
}

bool RawInstance::is_instance_of(ShapeId id) const {
  ShapeId own_id = shape_id();

  if (own_id == id) {
    return true;
  }

  // check inheritance chain
  // TODO: optimize this with an ancestor table
  auto klass = RawClass::cast(klass_field());
  RawValue search = klass.shape_instance();
  while (search.isShape()) {
    auto shape = RawShape::cast(search);

    if (shape.own_shape_id() == id) {
      return true;
    }

    search = shape.parent();
  }

  return false;
}

uintptr_t RawInstance::pointer_at(int64_t index) const {
  return field_at<RawInt>(index).external_pointer_value();
}

void RawInstance::set_pointer_at(int64_t index, const void* pointer) const {
  uintptr_t raw = bitcast<uintptr_t>(const_cast<void*>(pointer));
  set_field_at(index, RawInt::create_from_external_pointer(raw));
}

RawValue RawInstance::klass_field() const {
  return field_at(kKlassOffset);
}

void RawInstance::set_klass_field(RawValue klass) const {
  set_field_at(kKlassOffset, klass);
}

SYMBOL RawHugeBytes::hashcode() const {
  return header()->hashcode();
}

const uint8_t* RawHugeBytes::data() const {
  return bitcast<const uint8_t*>(pointer_at(kDataPointerOffset));
}

void RawHugeBytes::set_data(const uint8_t* data) const {
  set_pointer_at(kDataPointerOffset, data);
}

size_t RawHugeBytes::length() const {
  return field_at<RawInt>(kDataLengthOffset).value();
}

void RawHugeBytes::set_length(size_t length) const {
  set_field_at(kDataLengthOffset, RawInt::create(length));
}

RawHugeString RawHugeString::create(Thread* thread, const char* data, size_t size, SYMBOL hash) {
  DCHECK(size <= RawString::kMaxByteLength);
  char* copy = static_cast<char*>(utils::Allocator::alloc(size));
  std::memcpy(copy, data, size);
  return RawHugeString::acquire(thread, copy, size, hash);
}

RawHugeString RawHugeString::create(Thread* thread, const char* data, size_t size) {
  DCHECK(size <= RawString::kMaxByteLength);
  char* copy = static_cast<char*>(utils::Allocator::alloc(size));
  std::memcpy(copy, data, size);
  return RawHugeString::acquire(thread, copy, size);
}

RawHugeString RawHugeString::acquire(Thread* thread, char* data, size_t size, SYMBOL hash) {
  DCHECK(size <= RawString::kMaxByteLength);
  auto string = RawHugeString::acquire(thread, data, size);
  string.header()->cas_hashcode((SYMBOL)0, hash);
  string.header()->set_has_cached_hashcode();
  return string;
}

RawHugeString RawHugeString::acquire(Thread* thread, char* data, size_t size) {
  DCHECK(size <= RawString::kMaxByteLength);
  auto* runtime = thread->runtime();
  auto huge_string_class = runtime->get_builtin_class(ShapeId::kHugeString);
  auto string = RawHugeString::cast(
    RawInstance::create(thread, ShapeId::kHugeString, RawHugeString::kFieldCount, huge_string_class));
  string.set_data(data);
  string.set_byte_length(size);
  return string;
}

SYMBOL RawHugeString::hashcode() const {
  return header()->hashcode();
}

const char* RawHugeString::data() const {
  return bitcast<const char*>(pointer_at(kDataPointerOffset));
}

void RawHugeString::set_data(const char* data) const {
  set_pointer_at(kDataPointerOffset, data);
}

size_t RawHugeString::byte_length() const {
  return field_at<RawInt>(kDataLengthOffset).value();
}

void RawHugeString::set_byte_length(size_t length) const {
  auto length_int = bitcast<int64_t>(length);
  DCHECK(length_int >= 0);
  set_field_at(kDataLengthOffset, RawInt::create(length_int));
}

RawList RawList::create(Thread* thread, size_t initial_capacity) {
  DCHECK(initial_capacity <= kMaximumCapacity);

  auto* runtime = thread->runtime();
  auto list_class = runtime->get_builtin_class(ShapeId::kList);
  auto list = RawList::cast(RawInstance::create(thread, ShapeId::kList, RawList::kFieldCount, list_class));

  list.set_data(nullptr);
  list.set_length(0);
  list.set_capacity(0);
  std::lock_guard locker(list);
  RawValue reserve_result = list.reserve_capacity(thread, initial_capacity);
  DCHECK(!reserve_result.is_error_exception());

  return list;
}

RawList RawList::create_with(Thread* thread, size_t length, RawValue _value) {
  DCHECK(length <= kMaximumCapacity);

  HandleScope scope(thread);
  Value value(scope, _value);

  auto list = RawList::create(thread, length);
  DCHECK(list.capacity() >= length);
  list.set_length(length);

  auto* data = list.data();
  thread->runtime()->memset_value(data, value, length);

  return list;
}

RawValue RawList::create_spread(Thread* thread, const RawValue* segments, uint32_t segment_count) {
  size_t segment_sizes[segment_count];
  size_t total_size = 0;

  if (build_spread_size_table(thread, segments, segment_count, segment_sizes, &total_size).is_error_exception()) {
    return kErrorException;
  }

  if (total_size > RawList::kMaximumCapacity) {
    return thread->throw_message("List exceeded max size");
  }

  auto list = RawList::create(thread, total_size);
  if (total_size == 0) {
    return list;
  }

  list.set_length(total_size);

  auto* destination = list.data();
  if (unpack_spread_segments_to_buffer(thread, segments, segment_count, segment_sizes, destination, total_size)
        .is_error_exception()) {
    return kErrorException;
  }

  return list;
}

RawValue* RawList::data() const {
  return bitcast<RawValue*>(pointer_at(kDataOffset));
}

void RawList::set_data(RawValue* pointer) const {
  set_pointer_at(kDataOffset, pointer);
}

size_t RawList::length() const {
  return field_at<RawInt>(kLengthOffset).value();
}

void RawList::set_length(size_t length) const {
  set_field_at(kLengthOffset, RawInt::create(length));
}

size_t RawList::capacity() const {
  return field_at<RawInt>(kCapacityOffset).value();
}

void RawList::set_capacity(size_t capacity) const {
  set_field_at(kCapacityOffset, RawInt::create(capacity));
}

RawValue RawList::op_mul(Thread* thread, int64_t count) const {
  HandleScope scope(thread);
  List list(scope, *this);

  if (count < 0) {
    count = 0;
  }

  size_t old_length = list.length();
  size_t new_length = count * old_length;

  if (new_length == 0) {
    return RawList::create(thread, 0);
  }

  if (new_length > kMaximumCapacity) {
    return thread->throw_message("List exceeded max size");
  }

  List new_list(scope, RawList::create(thread, new_length));
  new_list.set_length(new_length);
  {
    std::unique_lock locker(list);
    if (list.length() != old_length) {
      locker.unlock();
      return thread->throw_message("List length changed during multiplication");
    }
    RawValue* source = list.data();
    RawValue* destination = new_list.data();
    for (size_t i = 0; i < (size_t)count; i++) {
      std::memcpy(destination + (i * old_length), source, old_length * kPointerSize);
    }
  }

  return new_list;
}

RawValue RawList::reserve_capacity(Thread* thread, size_t expected_size) const {
  DCHECK(is_locked());

  if (expected_size > kMaximumCapacity) {
    return thread->throw_message("List exceeded max size");
  }

  if (expected_size > capacity()) {
    size_t new_capacity = kInitialCapacity;
    while (new_capacity < expected_size) {
      new_capacity *= 2;
    }

    DCHECK(new_capacity <= kMaximumCapacity);

    auto* new_buffer = static_cast<RawValue*>(utils::Allocator::alloc(new_capacity * kPointerSize));
    DCHECK(new_buffer);

    // copy old values and free old buffer
    if (auto* buffer = data()) {
      std::memcpy(new_buffer, buffer, this->length() * kPointerSize);
      utils::Allocator::free(buffer);
    }

    set_data(new_buffer);
    set_capacity(new_capacity);
  }

  return kNull;
}

RawValue RawList::read_at(Thread* thread, int64_t index) const {
  std::unique_lock locker(*this);

  int64_t real_index = wrap_negative_indices(index, length());
  if (real_index < 0 || (size_t)real_index >= length()) {
    locker.unlock();
    return thread->throw_message("List index % is out of range", index);
  }
  DCHECK((size_t)real_index < capacity());

  return data()[real_index];
}

RawValue RawList::write_at(Thread* thread, int64_t index, RawValue value) const {
  std::unique_lock locker(*this);

  size_t length = this->length();
  int64_t real_index = wrap_negative_indices(index, length);
  if (real_index < 0 || (size_t)real_index >= length) {
    locker.unlock();
    return thread->throw_message("List index % is out of range", index);
  }
  DCHECK((size_t)real_index < capacity());

  data()[real_index] = value;
  gc_write_barrier(value);
  return value;
}

RawValue RawList::insert_at(Thread* thread, int64_t index, RawValue value) const {
  std::unique_lock locker(*this);

  size_t length = this->length();
  int64_t real_index = wrap_negative_indices(index, length);
  if (real_index < 0 || (size_t)real_index > length) {
    locker.unlock();
    return thread->throw_message("List index % is out of range", index);
  }

  if (reserve_capacity(thread, length + 1).is_error_exception()) {
    return kErrorException;
  }

  auto* data = this->data();
  DCHECK(data);

  size_t bytes_to_move = (length - real_index) * kPointerSize;
  std::memmove(data + real_index + 1, data + real_index, bytes_to_move);
  set_length(length + 1);
  data[real_index] = value;
  gc_write_barrier(value);
  return value;
}

RawValue RawList::erase_at(Thread* thread, int64_t start, int64_t count) const {
  std::unique_lock locker(*this);

  size_t length = this->length();
  int64_t real_start = wrap_negative_indices(start, length);
  if (real_start < 0 || (size_t)real_start >= length) {
    locker.unlock();
    return thread->throw_message("List index % is out of range", start);
  }
  if (count < 0) {
    locker.unlock();
    return thread->throw_message("Expected count to be greater than 0");
  }
  if ((size_t)(real_start + count) > length) {
    locker.unlock();
    return thread->throw_message("Not enough values in list to erase");
  }

  if (count == 0) {
    return kNull;
  }

  auto* data = this->data();
  DCHECK(data);

  // shift back values after the erased values
  size_t bytes_after_erase = (length - real_start - count) * kPointerSize;
  std::memmove(data + real_start, data + real_start + count, bytes_after_erase);
  set_length(length - count);
  // TODO: shrink list if a lot of space got freed up?
  return kNull;
}

RawValue RawList::push_value(Thread* thread, RawValue value) const {
  std::lock_guard locker(*this);
  size_t length = this->length();

  if (reserve_capacity(thread, length + 1).is_error_exception()) {
    return kErrorException;
  }

  set_length(length + 1);
  data()[length] = value;
  return value;
}

RawValue RawList::pop_value(Thread* thread) const {
  std::unique_lock locker(*this);
  size_t length = this->length();

  if (length == 0) {
    locker.unlock();
    return thread->throw_message("List is empty");
  }

  RawValue result = data()[length - 1];
  set_length(length - 1);
  return result;
}

RawValue RawClass::create(Thread* thread,
                          RawString name,
                          RawValue _parent_value,
                          RawFunction _constructor,
                          RawTuple _member_props,
                          RawTuple _member_funcs,
                          RawTuple _static_prop_keys,
                          RawTuple _static_prop_values,
                          RawTuple _static_funcs,
                          uint32_t flags) {
  auto runtime = thread->runtime();

  HandleScope scope(thread);
  Value parent_value(scope, _parent_value);
  Function constructor(scope, _constructor);
  Tuple member_props(scope, _member_props);
  Tuple member_funcs(scope, _member_funcs);
  Tuple static_prop_keys(scope, _static_prop_keys);
  Tuple static_prop_values(scope, _static_prop_values);
  Tuple static_funcs(scope, _static_funcs);

  if (parent_value.is_error_no_base_class()) {
    parent_value = runtime->get_builtin_class(ShapeId::kInstance);
  }

  if (!parent_value.isClass()) {
    return thread->throw_message("Extended value is not a class");
  }

  Class parent_class(scope, parent_value);
  Shape parent_shape(scope, parent_class.shape_instance());

  if (parent_class.flags() & RawClass::kFlagFinal) {
    return thread->throw_message("Cannot extend final class '%'", parent_class.name());
  }

  // check for duplicate member properties
  Tuple parent_keys_tuple(scope, parent_shape.keys());
  for (uint8_t i = 0; i < member_props.length(); i++) {
    RawInt encoded = member_props.field_at<RawInt>(i);
    SYMBOL prop_name;
    uint8_t prop_flags;
    RawShape::decode_shape_key(encoded, prop_name, prop_flags);
    for (uint32_t pi = 0; pi < parent_keys_tuple.length(); pi++) {
      SYMBOL parent_key_symbol;
      uint8_t parent_key_flags;
      RawShape::decode_shape_key(parent_keys_tuple.field_at<RawInt>(pi), parent_key_symbol, parent_key_flags);
      if (parent_key_symbol == prop_name) {
        return thread->throw_message("Cannot redeclare property '%', parent class '%' already contains it",
                                     RawSymbol::create(prop_name), parent_class.name());
      }
    }
  }

  // ensure new class doesn't exceed the class member property limit
  size_t new_member_count = parent_shape.field_count() + member_props.length();
  if (new_member_count > RawInstance::kMaximumFieldCount) {
    // for some reason, RawInstance::kMaximumFieldCount needs to be casted to its own
    // type, before it can be used in here. this is some weird template thing...
    return thread->throw_message("Newly created class '%' has % properties, but the limit is %", name, new_member_count,
                                 (size_t)RawInstance::kMaximumFieldCount);
  }

  Shape object_shape(scope, RawShape::create(thread, parent_shape, member_props));

  Tuple parent_function_table(scope, parent_class.function_table());
  Tuple new_function_table(scope);

  // can simply reuse parent function table if no new functions are being added
  if (member_funcs.length() == 0) {
    new_function_table = parent_function_table;
  } else {
    std::unordered_map<SYMBOL, uint32_t> parent_function_indices;
    for (uint32_t j = 0; j < parent_function_table.length(); j++) {
      RawFunction parent_function = parent_function_table.field_at<RawFunction>(j);
      parent_function_indices[parent_function.name().hashcode()] = j;
    }

    // calculate the amount of functions being added that are not already present in the parent class
    std::unordered_set<SYMBOL> new_function_names;
    size_t newly_added_functions = 0;
    for (uint32_t i = 0; i < member_funcs.length(); i++) {
      RawString method_name = member_funcs.field_at<RawTuple>(i).field_at<RawFunction>(0).name();
      new_function_names.insert(method_name.hashcode());

      if (parent_function_indices.count(method_name.hashcode()) == 0) {
        newly_added_functions++;
      }
    }

    size_t new_function_table_length = parent_function_table.length() + newly_added_functions;
    new_function_table = RawTuple::create(thread, new_function_table_length);

    // functions which are not being overriden in the new class can be copied to the new table
    size_t new_function_table_offset = 0;
    for (uint32_t j = 0; j < parent_function_table.length(); j++) {
      RawFunction entry = parent_function_table.field_at<RawFunction>(j);
      if (new_function_names.count(entry.name().hashcode()) == 0) {
        if (entry.overload_table().isTuple()) {
          // create copy of the overload table and all functions included
          Tuple overload_table(scope, entry.overload_table());
          Tuple new_overload_table(scope, RawTuple::create(thread, overload_table.length()));

          for (uint32_t i = 0; i < overload_table.length(); i++) {
            Function parent(scope, overload_table.field_at<RawFunction>(i));
            Function method_copy(
              scope, RawFunction::create(thread, parent.context(), parent.shared_info(), parent.saved_self()));
            method_copy.set_overload_table(new_overload_table);
            new_overload_table.set_field_at(i, method_copy);
          }

          new_function_table.set_field_at(new_function_table_offset++, new_overload_table.first_field());
        } else {
          Function method_copy(scope,
                               RawFunction::create(thread, entry.context(), entry.shared_info(), entry.saved_self()));
          new_function_table.set_field_at(new_function_table_offset++, method_copy);
        }
      }
    }

    // insert new methods into function table
    // the overload tables of functions that override functions from the parent class must be merged
    // with the overload tables from their parents
    for (uint32_t i = 0; i < member_funcs.length(); i++) {
      Tuple new_overload_table(scope, member_funcs.field_at(i));
      Function first_overload(scope, new_overload_table.first_field());
      RawString new_overload_name = first_overload.name();

      // new function does not override any parent functions
      if (parent_function_indices.count(new_overload_name.hashcode()) == 0) {
        for (uint32_t j = 0; j < new_overload_table.length(); j++) {
          new_overload_table.field_at<RawFunction>(j).set_overload_table(new_overload_table);
        }
        new_function_table.set_field_at(new_function_table_offset++, first_overload);
        continue;
      }

      uint32_t parent_function_index = parent_function_indices[new_overload_name.hashcode()];
      Function parent_function(scope, parent_function_table.field_at(parent_function_index));

      Tuple parent_overload_table(scope);
      if (parent_function.overload_table().isTuple()) {
        parent_overload_table = parent_function.overload_table();
      } else {
        parent_overload_table = RawTuple::create(thread, parent_function);
      }

      Function max_parent_overload(scope, parent_overload_table.last_field());
      Function max_new_overload(scope, new_overload_table.last_field());

      uint32_t parent_max_argc = max_parent_overload.shared_info()->ir_info.argc;
      uint32_t new_max_argc = max_new_overload.shared_info()->ir_info.argc;
      uint32_t max_argc = std::max(parent_max_argc, new_max_argc);
      uint32_t merged_size = max_argc + 2;
      Tuple merged_table(scope, RawTuple::create(thread, merged_size));

      // merge the parents and the new functions overload tables
      Function highest_overload(scope);
      Function first_new_overload(scope, new_overload_table.first_field());
      for (uint32_t argc = 0; argc < merged_size; argc++) {
        uint32_t parent_table_index = std::min(parent_overload_table.length() - 1, argc);
        uint32_t new_table_index = std::min(new_overload_table.length() - 1, argc);

        Function parent_method(scope, parent_overload_table.field_at(parent_table_index));
        Function new_method(scope, new_overload_table.field_at(new_table_index));

        if (new_method.check_accepts_argc(argc)) {
          new_method.set_overload_table(merged_table);
          merged_table.set_field_at(argc, new_method);
          highest_overload = new_method;
          continue;
        }

        if (parent_method.check_accepts_argc(argc)) {
          Function method_copy(scope, RawFunction::create(thread, parent_method.context(), parent_method.shared_info(),
                                                          parent_method.saved_self()));

          method_copy.set_overload_table(merged_table);
          method_copy.set_host_class(parent_method.host_class());
          merged_table.set_field_at(argc, method_copy);
          highest_overload = method_copy;
          continue;
        }

        merged_table.set_field_at(argc, kNull);
      }

      // patch holes in the merged table with their next highest applicable overload
      // trailing holes are filled with the next lowest overload
      Value next_highest_method(scope, kNull);
      for (int64_t k = merged_size - 1; k >= 0; k--) {
        RawValue field = merged_table.field_at(k);
        if (field.isNull()) {
          if (next_highest_method.isNull()) {
            merged_table.set_field_at(k, highest_overload);
          } else {
            merged_table.set_field_at(k, next_highest_method);
          }
        } else {
          next_highest_method = field;
        }
      }

      // remove overload tuple if overload table is homogenic
      // TODO: ideally the overload tuple shouldn't be constructed
      //       in the first place if this is the case

      Function merged_first_overload(scope, merged_table.first_field());
      bool overload_tuple_is_homogenic = true;
      for (uint32_t oi = 0; oi < merged_table.length(); oi++) {
        if (merged_table.field_at(oi) != merged_first_overload) {
          overload_tuple_is_homogenic = false;
          break;
        }
      }
      if (merged_table.length() == 1 || overload_tuple_is_homogenic) {
        merged_first_overload.set_overload_table(kNull);
        new_function_table.set_field_at(new_function_table_offset++, merged_first_overload);
      } else {
        new_function_table.set_field_at(new_function_table_offset++, first_new_overload);
      }
    }
  }

  // build overload tables for static functions
  Tuple static_function_table(scope, RawTuple::create(thread, static_funcs.length()));
  for (uint32_t i = 0; i < static_funcs.length(); i++) {
    auto overload_tuple = static_funcs.field_at<RawTuple>(i);
    DCHECK(overload_tuple.length() >= 1);

    // remove overload tuple if only one function is present
    auto first_overload = overload_tuple.first_field<RawFunction>();
    bool overload_tuple_is_homogenic = true;
    for (uint32_t oi = 0; oi < overload_tuple.length(); oi++) {
      if (overload_tuple.field_at(oi) != first_overload) {
        overload_tuple_is_homogenic = false;
        break;
      }
    }
    if (overload_tuple.length() == 1 || overload_tuple_is_homogenic) {
      first_overload.set_overload_table(kNull);
      static_function_table.set_field_at(i, first_overload);
      continue;
    }

    // point functions to the same overload tuple
    for (uint32_t j = 0; j < overload_tuple.length(); j++) {
      auto func = overload_tuple.field_at<RawFunction>(j);
      func.set_overload_table(overload_tuple);
    }

    // put the lowest overload into the functions lookup table
    // since every function part of the overload has a reference to the overload table
    // it doesn't really matter which function we put in there
    static_function_table.set_field_at(i, first_overload);
  }

  // if there are any static properties or functions in this class
  // a special intermediate class needs to be created that contains those static properties
  // the class instance returned is an instance of that intermediate class
  DCHECK(static_prop_keys.length() == static_prop_values.length());

  Class builtin_class_instance(scope, runtime->get_builtin_class(ShapeId::kClass));
  Class constructed_class(scope);
  if (static_prop_keys.length() || static_funcs.length()) {
    Shape builtin_class_shape(scope, builtin_class_instance.shape_instance());
    Shape static_shape(scope, RawShape::create(thread, builtin_class_shape, static_prop_keys));
    Class static_class(scope, RawInstance::create(thread, builtin_class_instance));
    static_class.set_flags(flags | RawClass::kFlagStatic);
    static_class.set_ancestor_table(
      RawTuple::concat_value(thread, builtin_class_instance.ancestor_table(), builtin_class_instance));
    static_class.set_name(name);
    static_class.set_parent(builtin_class_instance);
    static_class.set_shape_instance(static_shape);
    static_class.set_function_table(static_function_table);
    static_class.set_constructor(kNull);

    // build instance of newly created static shape
    Class actual_class(scope, RawInstance::create(thread, static_shape, static_class));
    actual_class.set_flags(flags);
    actual_class.set_ancestor_table(RawTuple::concat_value(thread, parent_class.ancestor_table(), parent_class));
    actual_class.set_name(name);
    actual_class.set_parent(parent_class);
    actual_class.set_shape_instance(object_shape);
    actual_class.set_function_table(new_function_table);
    actual_class.set_constructor(constructor);

    // initialize static properties
    for (uint32_t i = 0; i < static_prop_values.length(); i++) {
      actual_class.set_field_at(RawClass::kFieldCount + i, static_prop_values.field_at(i));
    }

    // patch host class field on static functions
    for (uint32_t i = 0; i < static_function_table.length(); i++) {
      auto entry = static_function_table.field_at<RawFunction>(i);
      auto overloads_value = entry.overload_table();
      if (overloads_value.isTuple()) {
        auto overloads = RawTuple::cast(overloads_value);
        for (uint32_t j = 0; j < overloads.length(); j++) {
          auto func = overloads.field_at<RawFunction>(j);
          if (func.host_class().isNull()) {
            func.set_host_class(static_class);
          }
        }
      }
      if (entry.host_class().isNull()) {
        entry.set_host_class(static_class);
      }
    }

    constructed_class = actual_class;
  } else {
    Class klass(scope, RawInstance::create(thread, builtin_class_instance));
    klass.set_flags(flags);
    klass.set_ancestor_table(RawTuple::concat_value(thread, parent_class.ancestor_table(), parent_class));
    klass.set_name(name);
    klass.set_parent(parent_class);
    klass.set_shape_instance(object_shape);
    klass.set_function_table(new_function_table);
    klass.set_constructor(constructor);
    constructed_class = klass;
  }

  // set host class fields on functions
  constructor.set_host_class(constructed_class);
  for (uint32_t i = 0; i < new_function_table.length(); i++) {
    auto entry = new_function_table.field_at<RawFunction>(i);
    auto overloads_value = entry.overload_table();
    if (overloads_value.isTuple()) {
      auto overloads = RawTuple::cast(overloads_value);
      for (uint32_t j = 0; j < overloads.length(); j++) {
        auto func = overloads.field_at<RawFunction>(j);
        if (func.host_class().isNull()) {
          func.set_host_class(constructed_class);
        }
      }
    }
    if (entry.host_class().isNull()) {
      entry.set_host_class(constructed_class);
    }
  }

  return constructed_class;
}

uint8_t RawClass::flags() const {
  return field_at<RawInt>(kFlagsOffset).value();
}

void RawClass::set_flags(uint8_t flags) const {
  set_field_at(kFlagsOffset, RawInt::create(flags));
}

RawTuple RawClass::ancestor_table() const {
  return field_at<RawTuple>(kAncestorTableOffset);
}

void RawClass::set_ancestor_table(RawTuple ancestor_table) const {
  set_field_at(kAncestorTableOffset, ancestor_table);
}

RawString RawClass::name() const {
  return field_at<RawString>(kNameOffset);
}

void RawClass::set_name(RawString name) const {
  set_field_at(kNameOffset, name);
}

RawValue RawClass::parent() const {
  return field_at(kParentOffset);
}

void RawClass::set_parent(RawValue parent) const {
  set_field_at(kParentOffset, parent);
}

RawShape RawClass::shape_instance() const {
  return field_at<RawShape>(kShapeOffset);
}

void RawClass::set_shape_instance(RawShape shape) const {
  set_field_at(kShapeOffset, shape);
}

RawTuple RawClass::function_table() const {
  return field_at<RawTuple>(kFunctionTableOffset);
}

void RawClass::set_function_table(RawTuple function_table) const {
  set_field_at(kFunctionTableOffset, function_table);
}

RawValue RawClass::constructor() const {
  return field_at(kConstructorOffset);
}

void RawClass::set_constructor(RawValue constructor) const {
  set_field_at(kConstructorOffset, constructor);
}

RawValue RawClass::lookup_function(SYMBOL name) const {
  // search function table
  auto functions = function_table();
  for (uint32_t i = 0; i < functions.length(); i++) {
    auto function = functions.field_at<RawFunction>(i);
    if (function.shared_info()->name_symbol == name) {
      return function;
    }
  }

  // search in parent class
  auto p = parent();
  if (p.isClass()) {
    return RawClass::cast(p).lookup_function(name);
  }

  return kErrorNotFound;
}

bool RawClass::is_instance_of(RawClass other) const {
  // same class
  if (*this == other) {
    return true;
  }

  // traverse class hierarchy
  RawValue search = parent();
  while (!search.isNull()) {
    if (search.raw() == other.raw()) {
      return true;
    }

    search = RawClass::cast(search).parent();
  }

  return false;
}

RawShape RawShape::create(Thread* thread, RawValue parent, RawTuple key_table) {
  auto runtime = thread->runtime();
  HandleScope scope(thread);
  Shape parent_shape(scope, parent);
  Shape target_shape(scope, parent_shape);

  // find preexisting shape with same layout or create new shape
  for (uint32_t i = 0; i < key_table.length(); i++) {
    RawInt encoded = key_table.field_at<RawInt>(i);
    Shape next_shape(scope);
    {
      std::lock_guard lock(target_shape);

      // find the shape to transition to when adding the new key
      Tuple additions(scope, target_shape.additions());
      bool found_next = false;
      for (uint32_t ai = 0; ai < additions.length(); ai++) {
        Tuple entry(scope, additions.field_at(ai));
        RawInt entry_encoded_key = entry.field_at<RawInt>(RawShape::kAdditionsKeyOffset);
        if (encoded == entry_encoded_key) {
          next_shape = entry.field_at<RawShape>(RawShape::kAdditionsNextOffset);
          found_next = true;
          break;
        }
      }

      // create new shape for added key
      if (!found_next) {
        Value klass(scope, kNull);

        // RawShape::create gets called during runtime initialization
        // at that point no builtin classes are registered yet
        // the klass field gets patched once runtime initialization has finished
        if (runtime->builtin_class_is_registered(ShapeId::kShape)) {
          klass = runtime->get_builtin_class(ShapeId::kShape);
        }
        next_shape = RawInstance::create(thread, ShapeId::kShape, RawShape::kFieldCount, klass);
        next_shape.set_parent(target_shape);
        next_shape.set_keys(RawTuple::concat_value(thread, target_shape.keys(), encoded));
        next_shape.set_additions(RawTuple::create_empty(thread));
        runtime->register_shape(next_shape);

        // add new shape to additions table of previous base
        target_shape.set_additions(
          RawTuple::concat_value(thread, additions, RawTuple::create(thread, encoded, next_shape)));
      }
    }

    target_shape = next_shape;
  }

  return *target_shape;
}

RawShape RawShape::create(Thread* thread,
                          RawValue _parent,
                          std::initializer_list<std::tuple<std::string, uint8_t>> keys) {
  auto runtime = thread->runtime();
  HandleScope scope(thread);
  Value parent(scope, _parent);
  Tuple key_tuple(scope, RawTuple::create(thread, keys.size()));
  for (size_t i = 0; i < keys.size(); i++) {
    auto& entry = std::data(keys)[i];
    auto& name = std::get<0>(entry);
    auto flags = std::get<1>(entry);

    auto name_string = runtime->declare_symbol(thread, name);
    auto name_symbol = RawSymbol::create(name_string.hashcode());
    key_tuple.set_field_at(i, RawShape::encode_shape_key(name_symbol, flags));
  }
  return RawShape::create(thread, parent, key_tuple);
}

ShapeId RawShape::own_shape_id() const {
  return static_cast<ShapeId>(field_at<RawInt>(kOwnShapeIdOffset).value());
}

void RawShape::set_own_shape_id(ShapeId id) const {
  set_field_at(kOwnShapeIdOffset, RawInt::create(static_cast<uint32_t>(id)));
}

RawValue RawShape::parent() const {
  return field_at(kParentShapeOffset);
}

void RawShape::set_parent(RawValue parent) const {
  set_field_at(kParentShapeOffset, parent);
}

RawTuple RawShape::keys() const {
  return field_at<RawTuple>(kKeysOffset);
}

void RawShape::set_keys(RawTuple keys) const {
  set_field_at(kKeysOffset, keys);
}

RawTuple RawShape::additions() const {
  return field_at<RawTuple>(kAdditionsOffset);
}

void RawShape::set_additions(RawTuple additions) const {
  set_field_at(kAdditionsOffset, additions);
}

RawShape::LookupResult RawShape::lookup_symbol(SYMBOL symbol) const {
  RawTuple keys = this->keys();

  for (uint32_t i = 0; i < keys.length(); i++) {
    auto encoded = keys.field_at<RawInt>(i);
    SYMBOL key_symbol;
    uint8_t key_flags;
    RawShape::decode_shape_key(encoded, key_symbol, key_flags);

    // skip internal fields
    if (key_flags & RawShape::kKeyFlagInternal) {
      continue;
    }

    if (key_symbol == symbol) {
      return LookupResult{ .found = true, .offset = i, .key = key_symbol, .flags = key_flags };
    }
  }

  return LookupResult{ .found = false };
}

RawInt RawShape::encode_shape_key(SYMBOL symbol, uint8_t flags) {
  size_t encoded = ((size_t)symbol << 8) | (size_t)flags;
  return RawInt::create(encoded);
}

void RawShape::decode_shape_key(RawInt encoded, SYMBOL& symbol_out, uint8_t& flags_out) {
  size_t encoded_value = encoded.value();
  symbol_out = encoded_value >> 8;
  flags_out = (encoded_value & 0xff);
}

RawFunction RawFunction::create(Thread* thread,
                                RawValue _context,
                                SharedFunctionInfo* shared_info,
                                RawValue _saved_self) {
  auto runtime = thread->runtime();
  HandleScope scope(thread);
  Value context(scope, _context);
  Value saved_self(scope, _saved_self);
  Function func(scope, RawInstance::create(thread, runtime->get_builtin_class(ShapeId::kFunction)));
  func.set_name(RawString::create(thread, shared_info->name));
  func.set_context(context);
  func.set_saved_self(saved_self);
  func.set_host_class(kNull);
  func.set_overload_table(kNull);
  func.set_shared_info(shared_info);
  return *func;
}

RawString RawFunction::name() const {
  return field_at<RawString>(kNameOffset);
}

void RawFunction::set_name(RawString name) const {
  set_field_at(kNameOffset, name);
}

RawValue RawFunction::context() const {
  return field_at(kFrameContextOffset);
}

void RawFunction::set_context(RawValue context) const {
  set_field_at(kFrameContextOffset, context);
}

RawValue RawFunction::saved_self() const {
  return field_at(kSavedSelfOffset);
}

void RawFunction::set_saved_self(RawValue value) const {
  set_field_at(kSavedSelfOffset, value);
}

RawValue RawFunction::host_class() const {
  return field_at(kHostClassOffset);
}

void RawFunction::set_host_class(RawValue host_class) const {
  set_field_at(kHostClassOffset, host_class);
}

RawValue RawFunction::overload_table() const {
  return field_at(kOverloadTableOffset);
}

void RawFunction::set_overload_table(RawValue overload_table) const {
  set_field_at(kOverloadTableOffset, overload_table);
}

SharedFunctionInfo* RawFunction::shared_info() const {
  return bitcast<SharedFunctionInfo*>(pointer_at(kSharedInfoOffset));
}

void RawFunction::set_shared_info(SharedFunctionInfo* function) const {
  set_pointer_at(kSharedInfoOffset, function);
}

bool RawFunction::check_accepts_argc(uint32_t argc) const {
  auto& ir_info = shared_info()->ir_info;
  bool direct_match = argc >= ir_info.minargc && argc <= ir_info.argc;
  bool spread_match = argc > ir_info.argc && ir_info.spread_argument;
  return direct_match || spread_match;
}

RawBuiltinFunction RawBuiltinFunction::create(Thread* thread, BuiltinFunctionType function, RawString name, int64_t argc) {
  auto runtime = thread->runtime();
  auto func =
    RawBuiltinFunction::cast(RawInstance::create(thread, runtime->get_builtin_class(ShapeId::kBuiltinFunction)));
  func.set_function(function);
  func.set_name(name);
  func.set_argc(argc);
  return func;
}

BuiltinFunctionType RawBuiltinFunction::function() const {
  return (BuiltinFunctionType)pointer_at(kFunctionPtrOffset);
}

void RawBuiltinFunction::set_function(BuiltinFunctionType function) const {
  set_pointer_at(kFunctionPtrOffset, (const void*)(function));
}

RawString RawBuiltinFunction::name() const {
  return field_at<RawString>(kNameOffset);
}

void RawBuiltinFunction::set_name(RawString symbol) const {
  return set_field_at(kNameOffset, symbol);
}

int64_t RawBuiltinFunction::argc() const {
  return field_at<RawInt>(kArgcOffset).value();
}

void RawBuiltinFunction::set_argc(int64_t argc) const {
  return set_field_at(kArgcOffset, RawInt::create(argc));
}

RawFiber RawFiber::create(Thread* thread, RawFunction _function, RawValue _self, RawValue _arguments) {
  auto runtime = thread->runtime();
  auto scheduler = runtime->scheduler();
  HandleScope scope(thread);
  Function function(scope, _function);
  Value self(scope, _self);
  Value arguments(scope, _arguments);
  Fiber fiber(scope, RawInstance::create(thread, runtime->get_builtin_class(ShapeId::kFiber)));
  Thread* fiber_thread = scheduler->get_free_thread();
  fiber_thread->init_fiber_thread(fiber);
  fiber.set_thread(fiber_thread);
  fiber.set_function(function);
  fiber.set_context(self);
  fiber.set_arguments(arguments);
  fiber.set_result_future(RawFuture::create(thread));

  // schedule the fiber for execution
  fiber_thread->wake_from_wait();
  scheduler->schedule_thread(fiber_thread, thread->worker()->processor());

  return *fiber;
}

Thread* RawFiber::thread() const {
  return bitcast<Thread*>(pointer_at(kThreadPointerOffset));
}

void RawFiber::set_thread(Thread* thread) const {
  set_pointer_at(kThreadPointerOffset, thread);
}

RawFunction RawFiber::function() const {
  return field_at<RawFunction>(kFunctionOffset);
}

void RawFiber::set_function(RawFunction function) const {
  set_field_at(kFunctionOffset, function);
}

RawValue RawFiber::context() const {
  return field_at(kSelfOffset);
}

void RawFiber::set_context(RawValue context) const {
  set_field_at(kSelfOffset, context);
}

RawValue RawFiber::arguments() const {
  return field_at(kArgumentsOffset);
}

void RawFiber::set_arguments(RawValue arguments) const {
  set_field_at(kArgumentsOffset, arguments);
}

RawFuture RawFiber::result_future() const {
  return field_at<RawFuture>(kResultFutureOffset);
}

void RawFiber::set_result_future(RawFuture result_future) const {
  set_field_at(kResultFutureOffset, result_future);
}

RawValue RawFiber::await(Thread* thread) const {
  return result_future().await(thread);
}

RawFuture RawFuture::create(Thread* thread) {
  auto runtime = thread->runtime();
  auto future = RawFuture::cast(RawInstance::create(thread, runtime->get_builtin_class(ShapeId::kFuture)));
  future.set_wait_queue(RawFuture::WaitQueue::alloc());
  future.set_result(kNull);
  future.set_exception(kNull);
  return future;
}

RawFuture::WaitQueue* RawFuture::WaitQueue::alloc(size_t initial_capacity) {
  DCHECK(initial_capacity <= kUInt32Max);
  size_t alloc_size = sizeof(WaitQueue) + kPointerSize * initial_capacity;
  auto* queue = static_cast<WaitQueue*>(utils::Allocator::alloc(alloc_size));
  queue->capacity = initial_capacity;
  queue->used = 0;
  return queue;
}

RawFuture::WaitQueue* RawFuture::WaitQueue::append_thread(RawFuture::WaitQueue* queue, Thread* thread) {
  DCHECK(queue->used <= queue->capacity);
  if (queue->used == queue->capacity) {
    WaitQueue* new_queue = WaitQueue::alloc(queue->capacity * 2);
    for (size_t i = 0; i < queue->used; i++) {
      WaitQueue::append_thread(new_queue, queue->buffer[i]);
    }
    WaitQueue::append_thread(new_queue, thread);
    utils::Allocator::free(queue);
    return new_queue;
  }

  queue->buffer[queue->used] = thread;
  queue->used++;
  return queue;
}

RawFuture::WaitQueue* RawFuture::wait_queue() const {
  return bitcast<WaitQueue*>(pointer_at(kWaitQueueOffset));
}

void RawFuture::set_wait_queue(RawFuture::WaitQueue* wait_queue) const {
  set_pointer_at(kWaitQueueOffset, wait_queue);
}

RawValue RawFuture::result() const {
  return field_at(kResultOffset);
}

void RawFuture::set_result(RawValue result) const {
  set_field_at(kResultOffset, result);
}

RawValue RawFuture::exception() const {
  return field_at(kExceptionOffset);
}

void RawFuture::set_exception(RawValue value) const {
  set_field_at(kExceptionOffset, value);
}

RawValue RawFuture::await(Thread* thread) const {
  HandleScope scope(thread);
  Future future(scope, *this);
  thread->await_future(future);

  auto exception = future.exception();
  if (exception.isNull()) {
    return future.result();
  }

  return thread->throw_exception(RawException::cast(exception));
}

RawValue RawFuture::resolve(Thread* thread, RawValue value) const {
  std::lock_guard lock(*this);
  if (wait_queue() == nullptr) {
    return thread->throw_message("Future has already completed");
  }

  set_result(value);
  set_exception(kNull);
  wake_waiting_threads(thread);
  return *this;
}

RawValue RawFuture::reject(Thread* thread, RawException exception) const {
  std::lock_guard lock(*this);
  if (wait_queue() == nullptr) {
    return thread->throw_message("Future has already completed");
  }

  set_result(kNull);
  set_exception(exception);
  wake_waiting_threads(thread);
  return *this;
}

void RawFuture::wake_waiting_threads(Thread* thread) const {
  DCHECK(is_locked());
  auto* scheduler = thread->runtime()->scheduler();
  auto* processor = thread->worker()->processor();
  auto* queue = wait_queue();

  for (size_t i = 0; i < queue->used; i++) {
    Thread* waiting_thread = queue->buffer[i];
    waiting_thread->wake_from_wait();
    scheduler->schedule_thread(waiting_thread, processor);
  }

  utils::Allocator::free(queue);
  set_wait_queue(nullptr);
}

RawException RawException::create(Thread* thread, RawString _message) {
  auto runtime = thread->runtime();
  HandleScope scope(thread);
  String message(scope, _message);
  Exception exception(scope, RawInstance::create(thread, runtime->get_builtin_class(ShapeId::kException)));
  exception.set_message(message);
  exception.set_backtrace(thread->create_backtrace());
  exception.set_cause(kNull);
  return *exception;
}

RawString RawException::message() const {
  return field_at<RawString>(kMessageOffset);
}

void RawException::set_message(RawString value) const {
  set_field_at(kMessageOffset, value);
}

RawTuple RawException::backtrace() const {
  return field_at<RawTuple>(kBacktraceOffset);
}

void RawException::set_backtrace(RawTuple backtrace) const {
  set_field_at(kBacktraceOffset, backtrace);
}

RawValue RawException::cause() const {
  return field_at(kCauseOffset);
}

void RawException::set_cause(RawValue cause) const {
  set_field_at(kCauseOffset, cause);
}

RawImportException RawImportException::create(Thread* thread,
                                              const std::string& module_path,
                                              const ref<core::compiler::CompilationUnit>& unit) {
  auto runtime = thread->runtime();
  const auto& messages = unit->console.messages();
  size_t size = messages.size();

  HandleScope scope(thread);
  Tuple error_tuple(scope, RawTuple::create(thread, size));
  for (size_t i = 0; i < size; i++) {
    auto& msg = messages[i];
    std::string type;
    switch (msg.type) {
      case core::compiler::DiagnosticType::Info: type = "info"; break;
      case core::compiler::DiagnosticType::Warning: type = "warning"; break;
      case core::compiler::DiagnosticType::Error: type = "error"; break;
    }
    auto filepath = msg.filepath;
    auto message = msg.message;

    utils::Buffer location_buffer;
    utils::Buffer annotated_source_buffer;
    if (msg.location.valid) {
      location_buffer << msg.location;
      unit->console.write_annotated_source(annotated_source_buffer, msg);
    }

    Tuple message_tuple(scope, RawTuple::create(thread, 5));
    message_tuple.set_field_at(0, RawString::create(thread, type));                      // type
    message_tuple.set_field_at(1, RawString::create(thread, filepath));                  // filepath
    message_tuple.set_field_at(2, RawString::create(thread, message));                   // message
    message_tuple.set_field_at(3, RawString::acquire(thread, annotated_source_buffer));  // source
    message_tuple.set_field_at(4, RawString::acquire(thread, location_buffer));          // location
    error_tuple.set_field_at(i, message_tuple);
  }

  ImportException exception(scope, RawInstance::create(thread, runtime->get_builtin_class(ShapeId::kImportException)));
  exception.set_message(RawString::format(thread, "Encountered an error while importing '%'", module_path));
  exception.set_backtrace(thread->create_backtrace());
  exception.set_cause(kNull);
  exception.set_errors(error_tuple);
  return *exception;
}

RawTuple RawImportException::errors() const {
  return field_at<RawTuple>(kErrorsOffset);
}

void RawImportException::set_errors(RawTuple errors) const {
  set_field_at(kErrorsOffset, errors);
}

RawAssertionException RawAssertionException::create(
  Thread* thread, RawString _message, RawValue _left, RawString _operation, RawValue _right) {
  auto runtime = thread->runtime();
  HandleScope scope(thread);
  String message(scope, _message);
  Value left(scope, _left);
  String operation_name(scope, _operation);
  Value right(scope, _right);

  auto builtin_class = runtime->get_builtin_class(ShapeId::kAssertionException);
  AssertionException exception(scope, RawInstance::create(thread, builtin_class));

  exception.set_message(RawString::format(thread, "% (% % %)", message, left, operation_name, right));
  exception.set_backtrace(thread->create_backtrace());
  exception.set_cause(kNull);

  Tuple components(scope, RawTuple::create(thread, left, operation_name, right));
  exception.set_components(components);
  return *exception;
}

RawAssertionException RawAssertionException::create(Thread* thread, RawString _message, RawValue _value) {
  auto runtime = thread->runtime();
  HandleScope scope(thread);
  String message(scope, _message);
  Value value(scope, _value);

  auto builtin_class = runtime->get_builtin_class(ShapeId::kAssertionException);
  AssertionException exception(scope, RawInstance::create(thread, builtin_class));

  exception.set_message(message);
  exception.set_backtrace(thread->create_backtrace());
  exception.set_cause(kNull);

  Tuple components(scope, RawTuple::create(thread, value));
  exception.set_components(components);
  return *exception;
}

RawTuple RawAssertionException::components() const {
  return field_at<RawTuple>(kComponentsOffset);
}

void RawAssertionException::set_components(RawTuple components) const {
  set_field_at(kComponentsOffset, components);
}

}  // namespace charly::core::runtime
