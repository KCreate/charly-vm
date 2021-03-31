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

// tagged-union representing the locations of values inside the runtime
//
// LocalFrame: value is stored inside the current frame at a fixed offset
// FarFrame: value is stored inside a parent frame at known depth and offset
// Global: value is a global variable and must be looked up by symbol
struct ValueLocation {
  enum class Type : uint8_t {
    Invalid,
    LocalFrame,
    FarFrame,
    Global
  };

  Type type;
  std::string name;

  union {

    // variables declared in the local frame
    struct {
      uint8_t offset;
    } local_frame;

    // variables declared in a parent function
    struct {
      uint8_t offset;
      uint8_t depth;
    } far_frame;

    // global variables
    struct {
      SYMBOL symbol;
    } global;
  } as;

  ValueLocation() : type(Type::Invalid) {}
  ValueLocation(const std::string& name, uint8_t offset) :
    type(Type::LocalFrame), name(name), as({ .local_frame = { offset } }) {}
  ValueLocation(const std::string& name, uint8_t offset, uint8_t depth) :
    type(Type::FarFrame), name(name), as({ .far_frame = { offset, depth } }) {}
  ValueLocation(const std::string& symbol) : type(Type::Global), name(symbol), as({ .global = { SYM(symbol) } }) {}

  inline bool valid() const {
    return this->type != Type::Invalid;
  }

  static ValueLocation LocalFrame(const std::string& name, uint8_t offset) {
    return ValueLocation(name, offset);
  }

  static ValueLocation FarFrame(const std::string& name, uint8_t depth, uint8_t offset) {
    return ValueLocation(name, offset, depth);
  }

  static ValueLocation Global(const std::string& symbol) {
    return ValueLocation(symbol);
  }

  // write a formatted version to the stream:
  //
  // <type>(depth=3, offset=5)
  friend std::ostream& operator<<(std::ostream& out, const ValueLocation& location) {
    switch (location.type) {
      case ValueLocation::Type::Invalid: {
        out << "invalid";
        break;
      }
      case ValueLocation::Type::LocalFrame: {
        out << "local(";
        out << "offset=" << static_cast<int>(location.as.local_frame.offset);
        out << ")";
        break;
      }
      case ValueLocation::Type::FarFrame: {
        out << "far(";
        out << "depth=" << static_cast<int>(location.as.far_frame.depth) << ", ";
        out << "offset=" << static_cast<int>(location.as.far_frame.offset);
        out << ")";
        break;
      }
      case ValueLocation::Type::Global: {
        out << "global(";
        out << "symbol=" << std::hex << location.as.global.symbol << std::dec;
        out << ")";
        break;
      }
    }

    return out;
  }
};

}  // namespace charly::core::compiler
