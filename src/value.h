/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include "defines.h"
#include "scope.h"
#include "block.h"

#pragma once

namespace Charly {
  namespace Value {
    using namespace Scope;
    using namespace Machine;

    // Different masks for the flags field in the Basic struct
    const VALUE fType = 0x1f;
    const VALUE fMark = 0x20;

    // Values used to represent some types
    namespace Type {
      const VALUE Undefined        = 0x00;
      const VALUE Integer          = 0x01;
      const VALUE Float            = 0x02;
      const VALUE Numeric          = 0x03;
      const VALUE Boolean          = 0x04;
      const VALUE Null             = 0x05;
      const VALUE Object           = 0x06;
      const VALUE Function         = 0x07;
      const VALUE CFunction        = 0x08;
      const VALUE Symbol           = 0x09;
      const VALUE Frame            = 0x0a;
      const VALUE CatchTable       = 0x0b;
      const VALUE InstructionBlock = 0x0c;

      const std::string str[] = {
        "Undefined",
        "Integer",
        "Float",
        "Numeric",
        "Boolean",
        "Null",
        "Object",
        "Function",
        "CFunction",
        "Symbol",
        "Frame",
        "CatchTable",
        "InstructionBlock"
      };
    }

    // Basic fields every data type in Charly has
    // This is inspired by the way Ruby stores it's values
    struct Basic {
        VALUE flags;

        Basic(VALUE type = 0) : flags(type) {}

        // Getters for different flag fields
        inline VALUE type() { return this->flags & fType; }
        inline VALUE mark() { return (this->flags & fMark) != 0; }

        // Setters for different flag fields
        inline void set_type(VALUE val) {
          this->flags = ((this->flags & ~fType) | (fType & val));
        }

        inline void set_mark(bool val) {
          this->flags ^= (-val ^ this->flags) & fMark;
        }
    };

    // Memory that is allocated via the GC will be aligned to 8 bytes
    // This means that if VALUE is a pointer, the last 3 bits will be set to 0.
    // We can use this to our advantage to store some additional information
    // in there.
    const VALUE IPointerMask  = 0b00111;
    const VALUE IPointerFlag  = 0b00000;
    const VALUE IIntegerMask  = 0b00001;
    const VALUE IIntegerFlag  = 0b00001;
    const VALUE IFloatMask    = 0b00011;
    const VALUE IFloatFlag    = 0b00010;
    const VALUE ISymbolMask   = 0b01111;
    const VALUE ISymbolFlag   = 0b01100;
    const VALUE False         = 0b00000;
    const VALUE True          = 0b10100;
    const VALUE Null          = 0b01000;

    inline bool is_boolean(VALUE value) { return value == False || value == True; }
    inline bool is_integer(VALUE value) { return (value & IIntegerMask) == IIntegerFlag; }
    inline bool is_ifloat(VALUE value)  { return (value & IFloatMask) == IFloatFlag; }
    inline bool is_symbol(VALUE value)  { return (value & ISymbolMask) == ISymbolFlag; }
    inline bool is_false(VALUE value)   { return value == False; }
    inline bool is_true(VALUE value)    { return value == True; }
    inline bool is_null(VALUE value)    { return value == Null; }
    inline bool is_special(VALUE value) {
      return (
          is_boolean(value) ||
          is_null(value) ||
          is_symbol(value) ||
          (value & IPointerMask) != IPointerFlag);
    }

    // Returns this value as a pointer to a Basic structure
    inline Basic* basics(VALUE value) { return (Basic *)value; }

    // Describes an object type
    //
    // The basic field contains a pointer to the parent class that was
    // used to construct this object. Empty objects will have the Object class as their parent
    // Objects have their own data container in which they can store arbitary values
    struct Object {
        Basic basic;
        Container* container;
    };

    // Heap-allocated float type
    //
    // Used when a floating-point value won't fit into the immediate-encoded format
    struct Float {
        Basic basic;
        double float_value;
    };

    // Normal functions defined inside the virtual machine.
    struct Function {
      Basic basic;
      VALUE name;
      uint32_t argc;
      Frame* context;
      InstructionBlock* block;
      bool bound_self_set;
      VALUE bound_self;

      // TODO: Argumentlist and bound argumentlist
    };

    // Function type used for including external functions from C-Land into the virtual machine
    // These are basically just a function pointer with some metadata associated to them
    struct CFunction {
      Basic basic;
      VALUE name;
      void* pointer;
      uint32_t argc;
      bool bound_self_set;
      VALUE bound_self;

      // TODO: Argumentlist and bound argumentlist
    };

    // Rotate a given value to the left n times
    constexpr VALUE BIT_ROTL(VALUE v, VALUE n) {
      return (((v) << (n)) | ((v) >> ((sizeof(v) * 8) - n)));
    }

    // Rotate a given value to the right n times
    constexpr VALUE BIT_ROTR(VALUE v, VALUE n) {
      return (((v) >> (n)) | ((v) << ((sizeof(v) * 8) - n)));
    }
  }
}
