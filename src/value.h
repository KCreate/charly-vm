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

#include <cstdint>

#include "util.h"

#pragma once

namespace Charly {

  namespace Primitive {

    // Basic datatype in the whole VM
    typedef uintptr_t VALUE;
    typedef intptr_t SIGNED_VALUE;

    // Data types for the Basic struct
    namespace Type {
      const VALUE Undefined   = 0b00000000;
      const VALUE Integer     = 0b00000001;
    };

    /*
     * Basic fields every data type in Charly has
     * This is inspired by the way Ruby stores it's values
     * */
    class Basic {
      public:
        static const VALUE TypeMask     = 0x05; // bits 0 -> 4
        static const VALUE RefCountMask = 0x1FFFFFFFE0; // bits 5 -> 36

      public:
        VALUE flags;
        VALUE klass;

        Basic(VALUE type, VALUE kl) : flags((VALUE)type), klass(kl) {};

        /* Returns the type field in the flags value */
        VALUE type() {
          return this->flags & TypeMask;
        }

        VALUE ref_count() {
          return this->flags & RefCountMask;
        }
    };

    /* Helper methods to operator on the VALUE type */
    namespace Value {
      const uint64_t SpecialPointerFlag = 0x00;
      const uint64_t SpecialIntegerFlag = 0x01;
      const uint64_t SpecialMask        = 0x7;

      /* Maximum integer that can be stored inside a VALUE */
      const uint64_t MaxInt  = ~(0xE000000000000000);

      /* Read several flag fields */
      inline uint64_t specials(VALUE value) { return value & SpecialMask; }
      inline uint64_t is_pointer(VALUE value) { return specials(value) == SpecialPointerFlag; }
      inline uint64_t is_integer(VALUE value) { return specials(value) == SpecialIntegerFlag; }

      /* Returns this value as a pointer to a Basic structure */
      inline Basic* basics(VALUE value) { return (Basic *)value; }

      /* Conversion methods between integers and VALUE */
      const constexpr inline VALUE int_to_value(int64_t val) { return ((VALUE) val << 1) | SpecialPointerFlag; }
      const constexpr inline int64_t value_to_int(VALUE val) { return ((SIGNED_VALUE)val) >> 1; }
    }
  }

}
