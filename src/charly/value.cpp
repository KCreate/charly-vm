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

#include "charly/utils/buffer.h"
#include "charly/utils/colorwriter.h"

#include "charly/utf8.h"
#include "charly/value.h"

#include "charly/core/runtime/compiled_module.h"
#include "charly/core/runtime/heap.h"
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

void ObjectHeader::initialize_header(uintptr_t address, ShapeId shape_id, uint16_t count) {
  auto* header = bitcast<ObjectHeader*>(address);
  header->m_shape_id_and_survivor_count = static_cast<uint32_t>(shape_id);  // survivor count initialized to 0
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
    auto address = bitcast<uintptr_t>(this);
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
        if (v == 0.0)
          return false;
        if (std::isnan(v))
          return false;
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

bool RawValue::isClass() const {
  if (isInstance()) {
    return RawInstance::cast(this).is_instance_of(ShapeId::kClass);
  }
  return shape_id() == ShapeId::kClass;
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

bool RawValue::isException() const {
  if (isInstance()) {
    return RawInstance::cast(this).is_instance_of(ShapeId::kException);
  }
  return shape_id() == ShapeId::kException;
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
  Thread* thread = Thread::current();

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

      if (tuple.size()) {
        tuple.field_at(0).dump(out);
        for (uint32_t i = 1; i < tuple.size(); i++) {
          writer.fg(Color::Grey, ", ");
          tuple.field_at(i).dump(out);
        }
      }

      writer.fg(Color::Grey, ")");

      return;
    }

    if (isFunction()) {
      auto function = RawFunction::cast(this);

      const SharedFunctionInfo& shared_info = *function.shared_info();
      const compiler::ir::FunctionInfo& ir_info = shared_info.ir_info;
      uint8_t minargc = ir_info.minargc;
      bool arrow = ir_info.arrow_function;
      const std::string& name = shared_info.name;

      if (arrow) {
        writer.fg(Color::Magenta, "->(", (uint32_t)minargc, ")");
      } else {
        if (function.host_class().isClass()) {
          auto klass = RawClass::cast(function.host_class());
          writer.fg(Color::Yellow, "func ", klass.name(), "::", name, "(", (uint32_t)minargc, ")");
        } else {
          writer.fg(Color::Yellow, "func ", name, "(", (uint32_t)minargc, ")");
        }
      }

      return;
    }

    if (isBuiltinFunction()) {
      auto builtin_function = RawBuiltinFunction::cast(this);
      RawSymbol name = builtin_function.name();
      uint8_t argc = builtin_function.argc();
      writer.fg(Color::Yellow, "builtin ", name, "(", (uint32_t)argc, ")");
      return;
    }

    if (isFiber()) {
      auto fiber = RawFiber::cast(this);
      writer.fg(Color::Red, "<Fiber ", fiber.address_voidptr(), ">");
      return;
    }

    if (isShape()) {
      auto shape = RawShape::cast(this);
      writer.fg(Color::Cyan, "<Shape #", static_cast<uint32_t>(shape.own_shape_id()), ":", shape.keys(), ">");
      return;
    }

    if (isClass()) {
      auto klass = RawClass::cast(this);
      writer.fg(Color::Yellow, "<Class ", klass.name(), ">");
      return;
    }

    if (isException()) {
      auto exception = RawException::cast(this);
      auto klass = thread->runtime()->lookup_class(thread, exception);
      RawValue message = exception.message();
      RawTuple stack_trace = exception.stack_trace();
      writer.fg(Color::Green, "<", klass.name(), " ", message, ", ", stack_trace, ">");
      return;
    }

    auto instance = RawInstance::cast(this);
    RawClass klass = thread->runtime()->lookup_class(thread, instance);
    writer.fg(Color::Green, "<", klass.name(), " ", instance.address_voidptr(), ">");
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

    HandleScope scope(thread);
    Value sym_value(scope, thread->lookup_symbol(symbol.value()));

    if (sym_value.isString()) {
      RawString string = RawString::cast(sym_value);
      out << string.view();
    } else {
      out << std::hex << symbol.value() << std::dec;
    }

    return;
  }

  if (isNull()) {
    writer.fg(Color::Grey, "null");
    return;
  }

  UNREACHABLE();
}

std::ostream& operator<<(std::ostream& out, const RawValue& value) {
  value.dump(out);
  return out;
}

int64_t RawInt::value() const {
  auto tmp = static_cast<int64_t>(m_raw);
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
  auto value_bits = bitcast<uintptr_t>(value);
  return RawFloat::cast((value_bits & ~kMaskImmediate) | kTagFloat);
}

bool RawBool::value() const {
  return (m_raw >> kShiftBool) != 0;
}

RawBool RawBool::make(bool value) {
  if (value) {
    return RawBool::cast((uintptr_t{ 1 } << kShiftBool) | kTagBool);
  } else {
    return RawBool::cast(kTagBool);
  }
}

SYMBOL RawSymbol::value() const {
  return m_raw >> kShiftSymbol;
}

RawSymbol RawSymbol::make(SYMBOL value) {
  return RawSymbol::cast((uintptr_t{ value } << kShiftSymbol) | kTagSymbol);
}

size_t RawSmallString::length() const {
  return (m_raw & kMaskLength) >> kShiftLength;
}

const char* RawSmallString::data(const RawSmallString* value) {
  uintptr_t address = bitcast<uintptr_t>(value) + 1;
  return bitcast<const char*>(address);
}

SYMBOL RawSmallString::hashcode() const {
  return crc32::hash_block(RawSmallString::data(this), length());
}

RawSmallString RawSmallString::make_from_cp(uint32_t cp) {
  utils::Buffer buf(4);
  buf.write_utf8_cp(cp);
  DCHECK(buf.size() <= 4);
  return make_from_memory(buf.data(), utf8::sequence_length(cp));
}

RawSmallString RawSmallString::make_from_str(const std::string& string) {
  return make_from_memory(string.data(), string.size());
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
  auto base = bitcast<uintptr_t>(value);
  return bitcast<const uint8_t*>(base + 1);
}

SYMBOL RawSmallBytes::hashcode() const {
  return crc32::hash_block(bitcast<const char*>(RawSmallBytes::data(this)), length());
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

std::string RawString::str() const {
  return { data(this), length() };
}

std::string_view RawString::view() const {
  return { data(this), length() };
}

int32_t RawString::compare(RawString base, const char* data, size_t length) {
  std::string_view base_view = base.view();
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

void* RawObject::address_voidptr() const {
  return bitcast<void*>(address());
}

uintptr_t RawObject::base_address() const {
  return address() - sizeof(ObjectHeader);
}

ShapeId RawObject::shape_id() const {
  return header()->shape_id();
}

ObjectHeader* RawObject::header() const {
  return bitcast<ObjectHeader*>(base_address());
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
  return crc32::hash_block(bitcast<const char*>(data()), length());
}

const char* RawLargeString::data() const {
  return bitcast<const char*>(address());
}

uint32_t RawTuple::size() const {
  return header()->count();
}

uint32_t RawInstance::field_count() const {
  return header()->count();
}

bool RawInstance::is_instance_of(ShapeId id) {
  ShapeId own_id = shape_id();

  if (own_id == id) {
    return true;
  }

  // check inheritance chain
  // TODO: optimize this with an ancestor table
  if (shape_id() > ShapeId::kLastBuiltinShapeId) {
    auto klass = RawClass::cast(klass_field());
    RawValue search = klass.shape_instance();
    while (search.isShape()) {
      auto shape = RawShape::cast(search);

      if (shape.own_shape_id() == id) {
        return true;
      }

      search = shape.parent();
    }
  }

  return false;
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

RawValue RawInstance::klass_field() const {
  return field_at(kKlassOffset);
}

void RawInstance::set_klass_field(RawValue klass) {
  set_field_at(kKlassOffset, klass);
}

SYMBOL RawHugeBytes::hashcode() const {
  return crc32::hash_block(bitcast<const char*>(data()), length());
}

const uint8_t* RawHugeBytes::data() const {
  return bitcast<const uint8_t*>(pointer_at(kDataPointerOffset));
}

void RawHugeBytes::set_data(const uint8_t* data) {
  set_pointer_at(kDataPointerOffset, data);
}

size_t RawHugeBytes::length() const {
  int64_t value = int_at(kDataLengthOffset);
  DCHECK(value >= 0);
  return value;
}

void RawHugeBytes::set_length(size_t length) {
  set_int_at(kDataLengthOffset, length);
}

SYMBOL RawHugeString::hashcode() const {
  return crc32::hash_block(data(), length());
}

const char* RawHugeString::data() const {
  return bitcast<const char*>(pointer_at(kDataPointerOffset));
}

void RawHugeString::set_data(const char* data) {
  set_pointer_at(kDataPointerOffset, data);
}

size_t RawHugeString::length() const {
  int64_t value = int_at(kDataLengthOffset);
  DCHECK(value >= 0);
  return value;
}

void RawHugeString::set_length(size_t length) {
  auto length_int = bitcast<int64_t>(length);
  DCHECK(length_int >= 0);
  set_int_at(kDataLengthOffset, length_int);
}

uint8_t RawClass::flags() const {
  return int_at(kFlagsOffset);
}

void RawClass::set_flags(uint8_t flags) {
  set_int_at(kFlagsOffset, flags);
}

RawTuple RawClass::ancestor_table() const {
  return field_at<RawTuple>(kAncestorTableOffset);
}

void RawClass::set_ancestor_table(RawTuple ancestor_table) {
  set_field_at(kAncestorTableOffset, ancestor_table);
}

RawSymbol RawClass::name() const {
  return field_at<RawSymbol>(kNameOffset);
}

void RawClass::set_name(RawSymbol name) {
  set_field_at(kNameOffset, name);
}

RawValue RawClass::parent() const {
  return field_at(kParentOffset);
}

void RawClass::set_parent(RawValue parent) {
  set_field_at(kParentOffset, parent);
}

RawShape RawClass::shape_instance() const {
  return field_at<RawShape>(kShapeOffset);
}

void RawClass::set_shape_instance(RawShape shape) {
  set_field_at(kShapeOffset, shape);
}

RawTuple RawClass::function_table() const {
  return field_at<RawTuple>(kFunctionTableOffset);
}

void RawClass::set_function_table(RawTuple function_table) {
  set_field_at(kFunctionTableOffset, function_table);
}

RawValue RawClass::constructor() const {
  return field_at(kConstructorOffset);
}

void RawClass::set_constructor(RawValue constructor) {
  set_field_at(kConstructorOffset, constructor);
}

RawValue RawClass::lookup_function(SYMBOL name) const {
  // search function table
  auto functions = function_table();
  for (uint32_t i = 0; i < functions.size(); i++) {
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

ShapeId RawShape::own_shape_id() const {
  return static_cast<ShapeId>(field_at<RawInt>(kOwnShapeIdOffset).value());
}

void RawShape::set_own_shape_id(ShapeId id) {
  set_field_at(kOwnShapeIdOffset, RawInt::make(static_cast<uint32_t>(id)));
}

RawValue RawShape::parent() const {
  return field_at(kParentShapeOffset);
}

void RawShape::set_parent(RawValue parent) {
  set_field_at(kParentShapeOffset, parent);
}

RawTuple RawShape::keys() const {
  return field_at<RawTuple>(kKeysOffset);
}

void RawShape::set_keys(RawTuple keys) {
  set_field_at(kKeysOffset, keys);
}

RawTuple RawShape::additions() const {
  return field_at<RawTuple>(kAdditionsOffset);
}

void RawShape::set_additions(RawTuple additions) {
  set_field_at(kAdditionsOffset, additions);
}

RawShape::LookupResult RawShape::lookup_symbol(SYMBOL symbol) const {
  RawTuple keys = this->keys();

  for (uint32_t i = 0; i < keys.size(); i++) {
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
  return RawInt::make(encoded);
}

void RawShape::decode_shape_key(RawInt encoded, SYMBOL& symbol_out, uint8_t& flags_out) {
  size_t encoded_value = encoded.value();
  symbol_out = encoded_value >> 8;
  flags_out = (encoded_value & 0xff);
}

RawSymbol RawFunction::name() const {
  return field_at<RawSymbol>(kNameOffset);
}

void RawFunction::set_name(RawSymbol name) {
  set_field_at(kNameOffset, name);
}

RawValue RawFunction::context() const {
  return field_at(kFrameContextOffset);
}

void RawFunction::set_context(RawValue context) {
  set_field_at(kFrameContextOffset, context);
}

RawValue RawFunction::saved_self() const {
  return field_at(kSavedSelfOffset);
}

void RawFunction::set_saved_self(RawValue value) {
  set_field_at(kSavedSelfOffset, value);
}

RawValue RawFunction::host_class() const {
  return field_at(kHostClassOffset);
}

void RawFunction::set_host_class(RawValue host_class) {
  set_field_at(kHostClassOffset, host_class);
}

SharedFunctionInfo* RawFunction::shared_info() const {
  return bitcast<SharedFunctionInfo*>(pointer_at(kSharedInfoOffset));
}

void RawFunction::set_shared_info(SharedFunctionInfo* function) {
  set_pointer_at(kSharedInfoOffset, function);
}

BuiltinFunctionType RawBuiltinFunction::function() const {
  return (BuiltinFunctionType)(intptr_t)int_at(kFunctionPtrOffset);
}

void RawBuiltinFunction::set_function(BuiltinFunctionType function) {
  set_int_at(kFunctionPtrOffset, (intptr_t)function);
}

RawSymbol RawBuiltinFunction::name() const {
  return field_at<RawSymbol>(kNameOffset);
}

void RawBuiltinFunction::set_name(RawSymbol symbol) {
  return set_field_at(kNameOffset, symbol);
}

uint8_t RawBuiltinFunction::argc() const {
  return int_at(kArgcOffset);
}

void RawBuiltinFunction::set_argc(uint8_t argc) {
  return set_int_at(kArgcOffset, argc);
}

Thread* RawFiber::thread() const {
  return bitcast<Thread*>(pointer_at(kThreadPointerOffset));
}

void RawFiber::set_thread(Thread* thread) {
  set_pointer_at(kThreadPointerOffset, thread);
}

RawFunction RawFiber::function() const {
  return field_at<RawFunction>(kFunctionOffset);
}

void RawFiber::set_function(RawFunction function) {
  set_field_at(kFunctionOffset, function);
}

RawValue RawFiber::context() const {
  return field_at(kSelfOffset);
}

void RawFiber::set_context(RawValue context) {
  set_field_at(kSelfOffset, context);
}

RawValue RawFiber::arguments() const {
  return field_at(kArgumentsOffset);
}

void RawFiber::set_arguments(RawValue arguments) {
  set_field_at(kArgumentsOffset, arguments);
}

RawValue RawFiber::result() const {
  return field_at(kResultOffset);
}

void RawFiber::set_result(RawValue result) {
  set_field_at(kResultOffset, result);
}

bool RawFiber::has_finished() const {
  return thread() == nullptr;
}

RawValue RawException::message() const {
  return field_at(kMessageOffset);
}

void RawException::set_message(RawValue value) {
  set_field_at(kMessageOffset, value);
}

RawTuple RawException::stack_trace() const {
  return field_at<RawTuple>(kStackTraceOffset);
}

void RawException::set_stack_trace(RawTuple stack_trace) {
  set_field_at(kStackTraceOffset, stack_trace);
}

}  // namespace charly::core::runtime
