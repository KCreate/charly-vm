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

bool is_shape_with_external_heap_pointers(ShapeId shape_id) {
  switch (shape_id) {
    case ShapeId::kHugeString:
    case ShapeId::kHugeBytes:
    case ShapeId::kFuture: return true;
    default: return false;
  }
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
    auto address = bitcast<uintptr_t>(this);
    uint32_t offset_in_heap = address % kHeapSize;

    if (cas_hashcode(0, offset_in_heap)) {
      set_has_cached_hashcode();
    }

    return m_hashcode;
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
  uintptr_t object_address = bitcast<uintptr_t>(this) + sizeof(ObjectHeader);
  return RawObject::make_from_ptr(object_address, is_young_generation());
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

bool RawValue::isNumber() const {
  return isInt() || isFloat();
}

int64_t RawValue::int_value() const {
  if (isInt()) {
    return RawInt::cast(*this).value();
  } else if (isFloat()) {
    return RawFloat::cast(*this).value();
  }

  UNREACHABLE();
}

double RawValue::double_value() const {
  if (isInt()) {
    return RawInt::cast(*this).value();
  } else if (isFloat()) {
    return RawFloat::cast(*this).value();
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
      bool arrow = ir_info.arrow_function;
      const std::string& name = shared_info.name;

      if (arrow) {
        writer.fg(Color::Magenta, "->()");
      } else {
        if (function.host_class().isClass()) {
          auto klass = RawClass::cast(function.host_class());
          if (klass.flags() & RawClass::kFlagStatic) {
            writer.fg(Color::Yellow, "static ", klass.name(), "::", name, "()");
          } else {
            writer.fg(Color::Yellow, klass.name(), "::", name, "()");
          }
        } else {
          writer.fg(Color::Yellow, name, "()");
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
      for (uint32_t i = 0; i < keys.size(); i++) {
        auto encoded = keys.field_at<RawInt>(i);
        SYMBOL prop_name;
        uint8_t prop_flags;
        RawShape::decode_shape_key(encoded, prop_name, prop_flags);
        writer.fg(Color::Cyan, RawSymbol::make(prop_name));

        if (i < keys.size() - 1) {
          writer.fg(Color::Cyan, ", ");
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
        auto import_exception = RawImportException::cast(this);
        auto errors = import_exception.errors();
        auto message = RawString::cast(import_exception.message());

        writer << message.view() << "\n";

        for (uint32_t i = 0; i < errors.size(); i++) {
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
        RawTuple stack_trace = exception.stack_trace();
        writer.fg(Color::Green, "<", klass.name(), " ", message, ", ", stack_trace, ">");
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
      HandleScope scope(thread);
      Value sym_value(scope, thread->lookup_symbol(symbol.value()));

      if (sym_value.isString()) {
        RawString string = RawString::cast(sym_value);
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
  value.dump(out);
  return out;
}

int64_t RawInt::value() const {
  auto tmp = static_cast<int64_t>(m_raw);
  return tmp >> kShiftInt;
}

uintptr_t RawInt::external_pointer_value() const {
  return static_cast<uintptr_t>(value());
}

RawInt RawInt::make(int64_t value) {
  CHECK(is_valid(value));
  return make_truncate(value);
}

RawInt RawInt::make_truncate(int64_t value) {
  return RawInt::cast((static_cast<uintptr_t>(value) << kShiftInt) | kTagInt);
}

RawInt RawInt::make_from_external_pointer(uintptr_t address) {
  DCHECK((address & kExternalPointerValidationMask) == 0);
  return make(static_cast<int64_t>(address));
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
  return static_cast<ErrorId>(raw() >> kShiftError);
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

std::string_view RawString::view() const & {
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

  if (value.is_young_pointer() && this->is_old_pointer()) {
    HeapRegion* region = header()->heap_region();
    DCHECK(region->type == HeapRegion::Type::Old);
    size_t span_index = region->span_get_index_for_pointer(base_address());
    region->span_set_dirty_flag(span_index);
  }
}

RawObject RawObject::make_from_ptr(uintptr_t address, bool is_young) {
  CHECK((address % kObjectAlignment) == 0, "invalid pointer alignment");
  auto object = RawObject::cast(address | (is_young ? kTagYoungObject : kTagOldObject));
  return object;
}

size_t RawData::length() const {
  return count();
}

const uint8_t* RawData::data() const {
  return bitcast<const uint8_t*>(address());
}

SYMBOL RawData::hashcode() const {
  if (header()->has_cached_hashcode()) {
    return header()->hashcode();
  }
  SYMBOL hash = crc32::hash_block(bitcast<const char*>(data()), length());
  header()->cas_hashcode(0, hash);
  return hash;
}

const char* RawLargeString::data() const {
  return bitcast<const char*>(address());
}

const RawValue* RawTuple::data() const {
  return bitcast<const RawValue*>(address());
}

uint32_t RawTuple::size() const {
  return count();
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
  set_field_at(index, RawInt::make_from_external_pointer(raw));
}

RawValue RawInstance::klass_field() const {
  return field_at(kKlassOffset);
}

void RawInstance::set_klass_field(RawValue klass) const {
  set_field_at(kKlassOffset, klass);
}

SYMBOL RawHugeBytes::hashcode() const {
  DCHECK(header()->has_cached_hashcode());
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
  set_field_at(kDataLengthOffset, RawInt::make(length));
}

SYMBOL RawHugeString::hashcode() const {
  DCHECK(header()->has_cached_hashcode());
  return header()->hashcode();
}

const char* RawHugeString::data() const {
  return bitcast<const char*>(pointer_at(kDataPointerOffset));
}

void RawHugeString::set_data(const char* data) const {
  set_pointer_at(kDataPointerOffset, data);
}

size_t RawHugeString::length() const {
  return field_at<RawInt>(kDataLengthOffset).value();
}

void RawHugeString::set_length(size_t length) const {
  auto length_int = bitcast<int64_t>(length);
  DCHECK(length_int >= 0);
  set_field_at(kDataLengthOffset, RawInt::make(length_int));
}

uint8_t RawClass::flags() const {
  return field_at<RawInt>(kFlagsOffset).value();
}

void RawClass::set_flags(uint8_t flags) const {
  set_field_at(kFlagsOffset, RawInt::make(flags));
}

RawTuple RawClass::ancestor_table() const {
  return field_at<RawTuple>(kAncestorTableOffset);
}

void RawClass::set_ancestor_table(RawTuple ancestor_table) const {
  set_field_at(kAncestorTableOffset, ancestor_table);
}

RawSymbol RawClass::name() const {
  return field_at<RawSymbol>(kNameOffset);
}

void RawClass::set_name(RawSymbol name) const {
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
  for (uint32_t i = 0; i < functions.size(); i++) {
    auto function = functions.field_at<RawFunction>(i);
    if (function.shared_info()->name_symbol == name) {
      return function;
    }
  }

  // search in parent class
  // TODO: can this be removed? all classes should contain copies of their parents functions inside their own
  //       function tables anyway
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

ShapeId RawShape::own_shape_id() const {
  return static_cast<ShapeId>(field_at<RawInt>(kOwnShapeIdOffset).value());
}

void RawShape::set_own_shape_id(ShapeId id) const {
  set_field_at(kOwnShapeIdOffset, RawInt::make(static_cast<uint32_t>(id)));
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

void RawFunction::set_name(RawSymbol name) const {
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

BuiltinFunctionType RawBuiltinFunction::function() const {
  return (BuiltinFunctionType)pointer_at(kFunctionPtrOffset);
}

void RawBuiltinFunction::set_function(BuiltinFunctionType function) const {
  set_pointer_at(kFunctionPtrOffset, (const void*)(function));
}

RawSymbol RawBuiltinFunction::name() const {
  return field_at<RawSymbol>(kNameOffset);
}

void RawBuiltinFunction::set_name(RawSymbol symbol) const {
  return set_field_at(kNameOffset, symbol);
}

uint8_t RawBuiltinFunction::argc() const {
  return field_at<RawInt>(kArgcOffset).value();
}

void RawBuiltinFunction::set_argc(uint8_t argc) const {
  return set_field_at(kArgcOffset, RawInt::make(argc));
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

  {
    std::unique_lock lock(future);

    // wait for the future to finish
    if (!future.has_finished()) {
      RawFuture::WaitQueue* wait_queue = future.wait_queue();
      future.set_wait_queue(wait_queue->append_thread(wait_queue, thread));
      thread->acas_state(Thread::State::Running, Thread::State::Waiting);

      lock.unlock();
      thread->enter_scheduler();
      lock.lock();
    }
  }

  if (!future.exception().isNull()) {
    thread->throw_value(future.exception());
    return kErrorException;
  }
  return future.result();
}

RawValue RawFuture::resolve(Thread* thread, RawValue value) const {
  auto runtime = thread->runtime();

  {
    std::lock_guard lock(*this);
    if (wait_queue() == nullptr) {
      thread->throw_value(runtime->create_string(thread, "Future has already completed"));
      return kErrorException;
    }

    set_result(value);
    set_exception(kNull);
    wake_waiting_threads(thread);
  }

  return *this;
}

RawValue RawFuture::reject(Thread* thread, RawException exception) const {
  auto runtime = thread->runtime();

  {
    std::lock_guard lock(*this);
    if (wait_queue() == nullptr) {
      thread->throw_value(runtime->create_string(thread, "Future has already completed"));
      return kErrorException;
    }

    set_result(kNull);
    set_exception(exception);
    wake_waiting_threads(thread);
  }

  return *this;
}

void RawFuture::wake_waiting_threads(Thread* thread) const {
  DCHECK(is_locked());
  auto* scheduler = thread->runtime()->scheduler();
  auto* processor = thread->worker()->processor();
  auto* queue = wait_queue();

  for (size_t i = 0; i < queue->used; i++) {
    Thread* waiting_thread = queue->buffer[i];
    DCHECK(waiting_thread->state() == Thread::State::Waiting);
    waiting_thread->ready();
    scheduler->schedule_thread(waiting_thread, processor);
  }

  utils::Allocator::free(queue);
  set_wait_queue(nullptr);
}

RawString RawException::message() const {
  return field_at<RawString>(kMessageOffset);
}

void RawException::set_message(RawString value) const {
  set_field_at(kMessageOffset, value);
}

RawTuple RawException::stack_trace() const {
  return field_at<RawTuple>(kStackTraceOffset);
}

void RawException::set_stack_trace(RawTuple stack_trace) const {
  set_field_at(kStackTraceOffset, stack_trace);
}

RawValue RawException::cause() const {
  return field_at(kCauseOffset);
}

void RawException::set_cause(RawValue cause) const {
  set_field_at(kCauseOffset, cause);
}

RawTuple RawImportException::errors() const {
  return field_at<RawTuple>(kErrorsOffset);
}

void RawImportException::set_errors(RawTuple errors) const {
  set_field_at(kErrorsOffset, errors);
}

}  // namespace charly::core::runtime
