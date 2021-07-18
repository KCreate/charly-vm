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

#include "charly/symbol.h"

#pragma once

namespace charly::core::compiler::ir {

using VariableId = uint32_t;

// tagged-union representing the locations of values inside the runtime
struct ValueLocation {
  enum class Type : uint8_t {
    Invalid,
    Id,           // temporary variable id set by local analyzer pass
    LocalFrame,   // value stored in current frames locals
    FarFrame,     // value stored in the heap allocated locals of some frame
    Global        // value stored in global context
  };

  Type type;
  std::string name; // cannot be in union because of non-trivial destructor

  union {

    // temporary variable id used by local analyzer pass
    struct {
      VariableId id;
    } id;

    // variables declared in the local frame
    struct {
      uint8_t index;
    } local_frame;

    // variables declared in a parent function
    struct {
      uint8_t depth;
      uint8_t index;
    } far_frame;

    // global variables
    struct {
      SYMBOL symbol;
    } global;
  } as;

  ValueLocation() : type(Type::Invalid) {}

  bool valid() const {
    return this->type != Type::Invalid;
  }

  static ValueLocation Id(VariableId id) {
    ValueLocation loc;
    loc.type = Type::Id;
    loc.as.id.id = id;
    return loc;
  }

  static ValueLocation LocalFrame(uint8_t index) {
    ValueLocation loc;
    loc.type = Type::LocalFrame;
    loc.as.local_frame.index = index;
    return loc;
  }

  static ValueLocation FarFrame(uint8_t depth, uint8_t index) {
    ValueLocation loc;
    loc.type = Type::FarFrame;
    loc.as.far_frame.depth = depth;
    loc.as.far_frame.index = index;
    return loc;
  }

  static ValueLocation Global(const std::string& name) {
    ValueLocation loc;
    loc.type = Type::Global;
    loc.name = name;
    loc.as.global.symbol = SYM(name);
    return loc;
  }

  // write a formatted version to the stream:
  //
  // <type>(depth=3, index=5)
  friend std::ostream& operator<<(std::ostream& out, const ValueLocation& location) {
    switch (location.type) {
      case ValueLocation::Type::Invalid: {
        out << "invalid";
        break;
      }
      case ValueLocation::Type::Id: {
        out << "id(";
        out << "id=" << static_cast<int>(location.as.id.id);
        out << ")";
        break;
      }
      case ValueLocation::Type::LocalFrame: {
        out << "local(";
        out << "index=" << static_cast<int>(location.as.local_frame.index);
        out << ")";
        break;
      }
      case ValueLocation::Type::FarFrame: {
        out << "far(";
        out << "depth=" << static_cast<int>(location.as.far_frame.depth) << ", ";
        out << "index=" << static_cast<int>(location.as.far_frame.index);
        out << ")";
        break;
      }
      case ValueLocation::Type::Global: {
        out << "global(" << location.name << ")";
        break;
      }
    }

    return out;
  }
};

}  // namespace charly::core::compiler
