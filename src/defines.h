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

#pragma once

namespace Charly {

  typedef uintptr_t VALUE;
  typedef VALUE STATUS;
  typedef intptr_t SIGNED_VALUE;

  namespace Scope {
    class Entry;
    class Container;

    /* Status codes returned by some Container methods */
    namespace Status {
      const STATUS Success                       = 0x00;
      const STATUS WriteFailedVariableIsConstant = 0x01;
      const STATUS WriteFailedVariableUndefined  = 0x02;
      const STATUS WriteFailedAlreadyDefined     = 0x03;
      const STATUS WriteFailedOutOfBounds        = 0x04;
      const STATUS RegisterFailedAlreadyDefined  = 0x05;
    }
  }

  namespace Primitive {
    class Basic;
    class Object;

    // Data types for the Basic struct
    namespace Type {
      const VALUE Undefined   = 0b00000; // 0x0
      const VALUE Integer     = 0b00001; // 0x1
      const VALUE Object      = 0b00010; // 0x2
    }

    // Different masks for the flags field in the Basic struct
    namespace Flag {
      const VALUE Type = 0x1f; // 0..4
      const VALUE Mark = 0x20; // 5
    }

    /* Helper methods to operator on the VALUE type */
    namespace Value {
      /* Masks for the special bits */
      const uint64_t SpecialPointerFlag = 0b000;
      const uint64_t SpecialIntegerFlag = 0b001;
      const uint64_t SpecialNullFlag    = 0b010;
      const uint64_t SpecialMask        = 0b111;

      /* Pre-defined constants for a VALUE */
      const VALUE Null = SpecialNullFlag;

      /* Read several flag fields */
      inline uint64_t specials(VALUE value) { return value & SpecialMask; }
      inline uint64_t is_pointer(VALUE value) { return specials(value) == SpecialPointerFlag; }
      inline uint64_t is_integer(VALUE value) { return specials(value) == SpecialIntegerFlag; }

      /* Returns this value as a pointer to a Basic structure */
      inline Basic* basics(VALUE value) { return (Basic *)value; }

      /* Conversion methods between integers and VALUE */
      const constexpr inline VALUE int_to_value(int64_t val) { return ((VALUE) val << 1) | SpecialIntegerFlag; }
      const constexpr inline int64_t value_to_int(VALUE val) { return ((SIGNED_VALUE)val) >> 1; }
    }
  }

}
