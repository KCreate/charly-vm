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

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <string>

#pragma once

namespace Charly {

  typedef uintptr_t VALUE;
  typedef intptr_t SIGNED_VALUE;
  typedef VALUE STATUS;
  typedef VALUE ID;

  /* Status codes returned by some Container methods */
  namespace Status {
    const STATUS Success                        = 0x00;
    const STATUS WriteFailedVariableIsConstant  = 0x01;
    const STATUS WriteFailedVariableUndefined   = 0x02;
    const STATUS WriteFailedOutOfBounds         = 0x03;
    const STATUS WriteFailedTooDeep             = 0x04;
    const STATUS RegisterFailedAlreadyDefined   = 0x05;
    const STATUS ReadFailedVariableUndefined    = 0x06;
    const STATUS ReadFailedOutOfBounds          = 0x07;
    const STATUS ReadFailedTooDeep              = 0x08;

    /* Human-readable error messages */
    const std::string str[] = {
      "Success",
      "Write failed: Field is a constant",
      "Write failed: Field doesn't exist",
      "Write failed: Index out of bounds",
      "Write failed: Environment doesn't exist",
      "Register failed: Key already defined",
      "Read failed: Field doesn't exist",
      "Read failed: Index out of bounds",
      "Read failed: Environment doesn't exist"
    };
  }

  namespace Scope {
    class Entry;
    class Container;
  }

  namespace Primitive {
    class Basic;
    class Object;
    class Float;

    // Data types for the Basic struct
    namespace Type {
      const VALUE Undefined = 0x00;
      const VALUE Integer   = 0x01;
      const VALUE Float     = 0x02;
      const VALUE Boolean   = 0x03;
      const VALUE Null      = 0x04;
      const VALUE Object    = 0x05;
      const VALUE Function  = 0x06;
      const VALUE Frame     = 0x07;

      const std::string str[] = {
        "Undefined",
        "Integer",
        "Float",
        "Boolean",
        "Null",
        "Object",
        "Function",
        "Frame"
      };
    }

    // Different masks for the flags field in the Basic struct
    namespace Flag {
      const VALUE Type = 0x1f; // 0..4
      const VALUE Mark = 0x20; // 5
    }

    /* Helper methods to operator on the VALUE type */
    namespace Value {

      /*
       * Memory that is allocated via the GC will be aligned to 8 bytes
       * This means that if VALUE is a pointer, the last 3 bits will be set to 0.
       * We can use this to our advantage to store some additional information
       * in there.
       * */

      const VALUE SpecialMask   = 0b00111;
      const VALUE IPointerFlag  = 0b00000;
      const VALUE IIntegerFlag  = 0b00001;
      const VALUE IFloatMask    = 0b00011;
      const VALUE IFloatFlag    = 0b00010;
      const VALUE False         = 0b00000;
      const VALUE True          = 0b10100;
      const VALUE Null          = 0b01000;

      const inline bool is_special(VALUE value) { return (value == False) || (value == Null) || (value & SpecialMask) != IPointerFlag; }
      const inline bool is_integer(VALUE value) { return (value & IIntegerFlag) == IIntegerFlag; }
      const inline bool is_ifloat(VALUE value)  { return (value & IFloatMask) == IFloatFlag; }
      const inline bool is_false(VALUE value)   { return value == False; }
      const inline bool is_true(VALUE value)    { return value == True; }
      const inline bool is_null(VALUE value)    { return value == Null; }

      /* Returns this value as a pointer to a Basic structure */
      inline Basic* basics(VALUE value) { return (Basic *)value; }
    }
  }

  namespace Machine {
    struct Frame;
    struct VM;

    const uint32_t InitialHeapCount = 8;
    const uint32_t HeapCellCount = 512;
  }
}
