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

#include <iomanip>

#include "charly/utils/colorwriter.h"
#include "charly/utils/buffer.h"

#include "charly/value.h"

#include "charly/core/runtime/heap.h"
#include "charly/core/runtime/compiled_module.h"

namespace charly::core::runtime {

using Color = utils::Color;

bool is_immediate_shape(ShapeId shape_id) {
  uint32_t id = static_cast<uint32_t>(shape_id);
  return id <= static_cast<uint32_t>(ShapeId::kLastImmediateShape);
}

bool is_object_shape(ShapeId shape_id) {
  uint32_t id = static_cast<uint32_t>(shape_id);
  return id > static_cast<uint32_t>(ShapeId::kLastImmediateShape);
}

bool is_data_shape(ShapeId shape_id) {
  uint32_t id = static_cast<uint32_t>(shape_id);
  uint32_t first_data_shape = static_cast<uint32_t>(ShapeId::kFirstNonInstance);
  uint32_t last_data_shape = static_cast<uint32_t>(ShapeId::kLastNonInstance);
  return (id >= first_data_shape) && (id <= last_data_shape);
}

bool is_instance_shape(ShapeId shape_id) {
  uint32_t id = static_cast<uint32_t>(shape_id);
  uint32_t last_non_instance = static_cast<uint32_t>(ShapeId::kLastNonInstance);
  return id > last_non_instance;
}

bool is_exception_shape(ShapeId shape_id) {
  uint32_t id = static_cast<uint32_t>(shape_id);
  uint32_t first_exception_shape = static_cast<uint32_t>(ShapeId::kFirstException);
  uint32_t last_exception_shape = static_cast<uint32_t>(ShapeId::kLastException);
  return (id >= first_exception_shape) && (id <= last_exception_shape);
}

bool is_builtin_shape(ShapeId shape_id) {
  uint32_t id = static_cast<uint32_t>(shape_id);
  uint32_t last_builtin_shape = static_cast<uint32_t>(ShapeId::kLastBuiltinShapeId);
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

void ObjectHeader::initialize_header(uintptr_t address, ShapeId shape_id, uint16_t count) {
  ObjectHeader* header = bitcast<ObjectHeader*>(address);
  header->m_shape_id_and_survivor_count = static_cast<uint32_t>(shape_id); // survivor count initialized to 0
  header->m_count = count;
  header->m_lock = 0;
  header->m_flags = Flag::kYoungGeneration;
  header->m_hashcode = 0;
  header->m_forward_offset = 0;
}

ShapeId ObjectHeader::shape_id() const {
  uint32_t tmp = m_shape_id_and_survivor_count;
  return static_cast<ShapeId>(tmp & kMaskShape);
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
  Flag current_flags = flags();

  if (current_flags & Flag::kHasHashcode) {
    return m_hashcode;
  } else {
    uintptr_t address = bitcast<uintptr_t>(this);
    uint32_t offset_in_heap = address % kHeapTotalSize;

    if (cas_hashcode(0, offset_in_heap)) {
      bool result = cas_flags(current_flags, static_cast<Flag>(current_flags | Flag::kHasHashcode));
      DCHECK(result);
      return m_hashcode;
    } else {
      return m_hashcode;
    }
  }
}

uint32_t ObjectHeader::forward_offset() const {
  return m_forward_offset;
}

RawObject ObjectHeader::object() const {
  uintptr_t object_address = bitcast<uintptr_t>(this) + sizeof(ObjectHeader);
  return RawObject::make_from_ptr(object_address);
}

uint32_t ObjectHeader::object_size() const {
  if (is_data_shape(shape_id())) {
    return align_to_size(count(), kObjectAlignment);
  } else {
    return align_to_size(count() * kPointerSize, kObjectAlignment);
  }
}

bool ObjectHeader::cas_shape_id(ShapeId old_shape_id, ShapeId new_shape_id) {
  uint8_t old_survivor_count = survivor_count();
  uint32_t old_value = encode_shape_and_survivor_count(old_shape_id, old_survivor_count);
  uint32_t new_value = encode_shape_and_survivor_count(new_shape_id, old_survivor_count);
  return m_shape_id_and_survivor_count.cas(old_value, new_value);
}

bool ObjectHeader::cas_survivor_count(uint8_t old_count, uint8_t new_count) {
  ShapeId old_shape_id = shape_id();
  uint32_t old_value = encode_shape_and_survivor_count(old_shape_id, old_count);
  uint32_t new_value = encode_shape_and_survivor_count(old_shape_id, new_count);
  return m_shape_id_and_survivor_count.cas(old_value, new_value);
}

bool ObjectHeader::cas_count(uint16_t old_count, uint16_t new_count) {
  return m_count.cas(old_count, new_count);
}

bool ObjectHeader::cas_flags(Flag old_flags, Flag new_flags) {
  return m_flags.cas(old_flags, new_flags);
}

bool ObjectHeader::cas_hashcode(SYMBOL old_hashcode, SYMBOL new_hashcode) {
  return m_hashcode.cas(old_hashcode, new_hashcode);
}

bool ObjectHeader::cas_forward_offset(uint32_t old_offset, uint32_t new_offset) {
  return m_forward_offset.cas(old_offset, new_offset);
}

uint32_t ObjectHeader::encode_shape_and_survivor_count(ShapeId shape_id, uint8_t survivor_count) {
  return static_cast<uint32_t>(shape_id) | ((uint32_t)survivor_count << kShiftSurvivorCount);
}

uintptr_t RawValue::raw() const {
  return m_raw;
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
  uint64_t table_index = m_raw & kMaskImmediate;
  DCHECK(table_index < 16);
  return kShapeImmediateTagMapping[table_index];
}

bool RawValue::truthyness() const {
  if (isInt()) {
    return RawInt::cast(this) != kZero;
  } else if (isObject()) {
    return RawObject::cast(this).address() != 0;
  } else {
    switch (shape_id_not_object_int()) {
      case ShapeId::kFloat: {
        double v = RawFloat::cast(this).value();
        if (v == 0.0) return false;
        if (std::isnan(v)) return false;
        return true;
      }
      case ShapeId::kBool: {
        return RawBool::cast(this) == kTrue;
      }
      case ShapeId::kSymbol: {
        return true;
      }
      case ShapeId::kNull: {
        return false;
      }
      default: {
        return true;
      }
    }
  }
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
  return (m_raw & kMaskPointer) == kTagObject;
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

bool RawValue::isFiber() const {
  return shape_id() == ShapeId::kFiber;
}

bool RawValue::isFunction() const {
  return shape_id() == ShapeId::kFunction;
}

bool RawValue::isShape() const {
  return shape_id() == ShapeId::kShape;
}

bool RawValue::isType() const {
  return shape_id() == ShapeId::kType;
}

bool RawValue::isException() const {
  return shape_id() == ShapeId::kException;
}

void RawValue::to_string(std::ostream& out) const {
  if (isString()) {
    RawString value = RawString::cast(this);
    const char* data = RawString::data(&value);
    size_t length = value.length();
    out << std::string_view(data, length);
    return;
  }

  dump(out);
}

void RawValue::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);

  if (isInt()) {
    writer.fg(Color::Cyan, RawInt::cast(this).value());
    return;
  }

  if (isString()) {
    RawString value = RawString::cast(this);
    const char* data = RawString::data(&value);
    size_t length = value.length();
    writer.fg(Color::Red, "\"", std::string_view(data, length), "\"");
    return;
  }

  if (isBytes()) {
    RawBytes value = RawBytes::cast(this);
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
    RawObject object(RawObject::cast(this));

    ShapeId shape_id = object.shape_id();
    switch (shape_id) {
      case ShapeId::kTuple: {
        RawTuple tuple(RawTuple::cast(object));

        writer.fg(Color::Grey, "(");

        {
          tuple.field_at(0).dump(out);
          for (int64_t i = 1; i < tuple.size(); i++) {
            writer.fg(Color::Grey, ", ");
            tuple.field_at(i).dump(out);
          }
        }

        writer.fg(Color::Grey, ")");

        return;
      }
      case ShapeId::kFunction: {
        RawFunction function(RawFunction::cast(object));

        const SharedFunctionInfo& shared_info = *function.shared_info();
        const compiler::ir::FunctionInfo& ir_info = shared_info.ir_info;
        uint8_t minargc = ir_info.minargc;
        bool arrow = ir_info.arrow_function;
        const std::string& name = shared_info.name;

        if (arrow) {
          writer.fg(Color::Magenta, "->(", (uint32_t)minargc, ")");
        } else {
          writer.fg(Color::Yellow, "func ", name, "(", (uint32_t)minargc, ")");
        }

        return;
      }
      default: {
        writer.fg(Color::Cyan, (uint32_t)shape_id, "%", bitcast<void*>(RawObject::cast(this).address()));
        return;
      }
    }
  }

  if (isFloat()) {
    out << std::defaultfloat;
    out << std::setprecision(10);
    writer.fg(Color::Red, RawFloat::cast(this).value());
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
    writer.fg(Color::Red, std::hex, RawSymbol::cast(this).value(), std::dec);
    return;
  }

  if (isNull()) {
    writer.fg(Color::Grey, "null");
    return;
  }

  writer.fg(Color::Grey, "<?>");
}

std::ostream& operator<<(std::ostream& out, const RawValue& value) {
  value.dump(out);
  return out;
}

int64_t RawInt::value() const {
  int64_t tmp = static_cast<int64_t>(m_raw);
  return tmp >> kShiftInt;
}

RawInt RawInt::make(int64_t value) {
  CHECK(is_valid(value));
  return make_truncate(value);
}

RawInt RawInt::make_truncate(int64_t value) {
  return RawInt::cast((static_cast<uintptr_t>(value) << kShiftInt) | kTagInt);
}

bool RawInt::is_valid(int64_t value) {
  return value >= kMinValue && value <= kMaxValue;
}

double RawFloat::value() const {
  uintptr_t doublepart = m_raw & (~kMaskImmediate);
  return bitcast<double>(doublepart);
}

bool RawFloat::close_to(double other, double precision) const {
  return std::fabs(value() - other) <= precision;
}

RawFloat RawFloat::make(double value) {
  uintptr_t value_bits = bitcast<uintptr_t>(value);
  return RawFloat::cast((value_bits & ~kMaskImmediate) | kTagFloat);
}

bool RawBool::value() const {
  return (m_raw >> kShiftBool) != 0;
}

RawBool RawBool::make(bool value) {
  if (value) {
    return RawBool::cast((uintptr_t{1} << kShiftBool) | kTagBool);
  } else {
    return RawBool::cast(kTagBool);
  }
}

SYMBOL RawSymbol::value() const {
  return m_raw >> kShiftSymbol;
}

RawSymbol RawSymbol::make(SYMBOL value) {
  return RawSymbol::cast((uintptr_t{value} << kShiftSymbol) | kTagSymbol);
}

size_t RawSmallString::length() const {
  return (m_raw & kMaskLength) >> kShiftLength;
}

const char* RawSmallString::data(const RawSmallString* value) {
  uintptr_t address = bitcast<uintptr_t>(value) + 1;
  return bitcast<const char*>(address);
}

SYMBOL RawSmallString::hashcode() const {
  return crc32_block(RawSmallString::data(this), length());
}

RawSmallString RawSmallString::make_from_cp(uint32_t cp) {
  utils::Buffer buf(4);
  buf.emit_utf8_cp(cp);
  DCHECK(buf.size() <= 4);
  return make_from_memory(buf.data(), utils::Buffer::utf8_cp_length(cp));
}

RawSmallString RawSmallString::make_from_str(const std::string& string) {
  return make_from_memory(string.c_str(), string.size());
}

RawSmallString RawSmallString::make_from_cstr(const char* value) {
  return make_from_memory(value, std::strlen(value));
}

RawSmallString RawSmallString::make_from_memory(const char* value, size_t length) {
  CHECK(length <= kMaxLength);

  uintptr_t immediate = 0;
  char* base_ptr = bitcast<char*>(&immediate) + 1;
  std::memcpy(bitcast<void*>(base_ptr), value, length);

  immediate |= (length << kShiftLength) & kMaskLowByte;
  immediate |= kTagSmallString;

  return RawSmallString::cast(immediate);
}

RawSmallString RawSmallString::make_empty() {
  return RawSmallString::cast(kTagSmallString);
}

size_t RawSmallBytes::length() const {
  return (m_raw & kMaskLength) >> kShiftLength;
}

const uint8_t* RawSmallBytes::data(const RawSmallBytes* value) {
  uintptr_t base = bitcast<uintptr_t>(value);
  return bitcast<const uint8_t*>(base + 1);
}

SYMBOL RawSmallBytes::hashcode() const {
  return crc32_block(bitcast<const char*>(RawSmallBytes::data(this)), length());
}

RawSmallBytes RawSmallBytes::make_from_memory(const uint8_t* value, size_t length) {
  CHECK(length <= kMaxLength);

  uintptr_t immediate = 0;
  char* base_ptr = bitcast<char*>(&immediate) + 1;
  std::memcpy(bitcast<void*>(base_ptr), value, length);

  immediate |= (length << kShiftLength) & kMaskLowByte;
  immediate |= kTagSmallBytes;

  return RawSmallBytes::cast(immediate);
}

RawSmallBytes RawSmallBytes::make_empty() {
  return RawSmallBytes::cast(kTagSmallBytes);
}

ErrorId RawNull::error_code() const {
  return static_cast<ErrorId>((raw() >> kShiftError) & kMaskImmediate);
}

RawNull RawNull::make() {
  return RawNull::cast(kTagNull);
}

RawNull RawNull::make_error(ErrorId id) {
  return RawNull::cast((static_cast<uint8_t>(id) << kShiftError) | kTagNull);
}

size_t RawString::length() const {
  if (isSmallString()) {
    return RawSmallString::cast(this).length();
  } else {
    if (isLargeString()) {
      return RawLargeString::cast(this).length();
    }

    if (isHugeString()) {
      return RawHugeString::cast(this).length();
    }
  }

  UNREACHABLE();
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

int32_t RawString::compare(RawString base, const char* data, size_t length) {
  std::string_view base_view(RawString::data(&base), base.length());
  std::string_view other_view(data, length);
  return base_view.compare(other_view);
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
  return m_raw & ~kMaskPointer;
}

uintptr_t RawObject::base_address() const {
  return address() - sizeof(ObjectHeader);
}

size_t RawObject::size() const {
  uint16_t count = header()->count();

  if (is_data_shape(shape_id())) {
    return align_to_size(count, kObjectAlignment);
  } else {
    return align_to_size(count * kPointerSize, kObjectAlignment);
  }
}

ShapeId RawObject::shape_id() const {
  return header()->shape_id();
}

ObjectHeader* RawObject::header() const {
  return bitcast<ObjectHeader*>(base_address());
}

RawObject RawObject::make_from_ptr(uintptr_t address) {
  CHECK((address % kObjectAlignment) == 0, "invalid pointer alignment");
  return RawObject::cast(address | kTagObject);
}

size_t RawData::length() const {
  return header()->count();
}

const uint8_t* RawData::data() const {
  return bitcast<const uint8_t*>(address());
}

SYMBOL RawData::hashcode() const {
  return crc32_block(bitcast<const char*>(data()), length());
}

const char* RawLargeString::data() const {
  return bitcast<const char*>(address());
}

int64_t RawInstance::size() const {
  return header()->count();
}

RawValue RawInstance::field_at(int64_t index) const {
  DCHECK(index >= 0 && index < size());
  RawValue* base = bitcast<RawValue*>(address());
  return *(base + index);
}

void RawInstance::set_field_at(int64_t index, RawValue value) {
  DCHECK(index >= 0 && index < size());
  RawValue* base = bitcast<RawValue*>(address());
  *(base + index) = value;
}

uintptr_t RawInstance::pointer_at(int64_t index) const {
  RawValue value = field_at(index);
  DCHECK(value.isObject());
  return RawObject::cast(value).address();
}

void RawInstance::set_pointer_at(int64_t index, uintptr_t pointer) {
  set_field_at(index, RawObject::make_from_ptr(pointer));
}

int64_t RawInstance::int_at(int64_t index) const {
  RawValue value = field_at(index);
  DCHECK(value.isInt());
  return RawInt::cast(value).value();
}

void RawInstance::set_int_at(int64_t index, int64_t value) {
  set_field_at(index, RawInt::make(value));
}

size_t RawHugeBytes::length() const {
  int64_t value = int_at(kDataLengthOffset);
  DCHECK(value >= 0);
  return value;
}

const uint8_t* RawHugeBytes::data() const {
  return bitcast<const uint8_t*>(pointer_at(kDataPointerOffset));
}

SYMBOL RawHugeBytes::hashcode() const {
  return SYM(data(), length());
}

size_t RawHugeString::length() const {
  int64_t value = int_at(kDataLengthOffset);
  DCHECK(value >= 0);
  return value;
}

const char* RawHugeString::data() const {
  return bitcast<const char*>(pointer_at(kDataPointerOffset));
}

SYMBOL RawHugeString::hashcode() const {
  return SYM(data(), length());
}

RawValue RawFunction::context() const {
  return field_at(kFrameContextOffset);
}

void RawFunction::set_context(RawValue context) {
  set_field_at(kFrameContextOffset, context);
}

SharedFunctionInfo* RawFunction::shared_info() const {
  return bitcast<SharedFunctionInfo*>(pointer_at(kSharedInfoOffset));
}

void RawFunction::set_shared_info(SharedFunctionInfo* function) {
  set_pointer_at(kSharedInfoOffset, function);
}

Thread* RawFiber::thread() const {
  return bitcast<Thread*>(pointer_at(kThreadPointerOffset));
}

void RawFiber::set_thread(Thread* thread) {
  set_pointer_at(kThreadPointerOffset, thread);
}

RawFunction RawFiber::function() const {
  return RawFunction::cast(field_at(kFunctionOffset));
}

void RawFiber::set_function(RawFunction function) {
  set_field_at(kFunctionOffset, function);
}

}  // namespace charly::core::runtime
