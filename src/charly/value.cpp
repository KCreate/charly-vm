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

#include "charly/utils/colorwriter.h"
#include "charly/utils/buffer.h"

#include "charly/value.h"

#include "charly/core/runtime/allocator.h"

namespace charly {

using Color = utils::Color;

using namespace charly::core::runtime;

void VALUE::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);

  if (this->is_pointer()) {
    writer.fg(Color::Cyan, this->to_pointer<void>());
    return;
  }

  if (this->is_int()) {
    writer.fg(Color::Cyan, this->to_int());
    return;
  }

  if (this->is_float()) {
    writer.fg(Color::Red, this->to_float());
    return;
  }

  if (this->is_char()) {
    utils::Buffer buf(4);
    buf.emit_utf8_cp(this->to_char());

    writer.fg(Color::Red, "'", buf.buffer_string(), "'");

    writer.fg(Color::Grey, " [");
    for (size_t i = 0; i < buf.size(); i++) {
      uint32_t character = *(buf.data() + i) & 0xFF;

      if (i) {
        out << " ";
      }

      writer.fg(Color::Red, "0x", std::hex, character, std::dec);
    }
    writer.fg(Color::Grey, "]");

    return;
  }

  if (this->is_symbol()) {
    writer.fg(Color::Red, std::hex, this->to_symbol(), std::dec);
    return;
  }

  if (this->is_bool()) {
    if (this->to_bool()) {
      writer.fg(Color::Green, "true");
    } else {
      writer.fg(Color::Red, "false");
    }
    return;
  }

  if (this->is_null()) {
    writer.fg(Color::Grey, "null");
    return;
  }

  writer.fg(Color::Grey, "???");
}

bool VALUE::truthyness() const {
  if (this->raw == taggedvalue::kNaN) return false;
  if (this->raw == taggedvalue::kNull) return false;
  if (this->raw == taggedvalue::kFalse) return false;
  if (this->is_int()) return this->to_int() != 0;
  if (this->is_float()) return this->to_float() != 0.0f;
  return true;
}

bool VALUE::is_pointer_to(HeapType type) const {
  if (is_pointer()) {
    void* ptr = to_pointer<void>();
    HeapHeader* header = MemoryAllocator::object_header(ptr);
    return header->type == type;
  }
  return false;
}

bool VALUE::compare(VALUE other) const {
  if (compare_strict(other)) {
    return true;
  }

  if (is_bool() || other.is_bool()) {
    return truthyness() == other.truthyness();
  }

  return false;
}

bool VALUE::compare_strict(VALUE other) const {
  return this->raw == other.raw;
}

}  // namespace charly::taggedvalue
