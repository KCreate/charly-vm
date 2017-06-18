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

#include "value.h"

namespace Charly {
  namespace Primitive {
    namespace Value {

      /* Returns the type of a given VALUE */
      const VALUE type(VALUE value) {

        /* Constants */
        if (is_false(value) || is_true(value)) return Type::Boolean;
        if (is_null(value)) return Type::Null;

        /* More logic required */
        if (!is_special(value)) return basics(value)->type();
        if (is_integer(value)) return Type::Integer;
        if (is_ifloat(value)) return Type::Float;
        return Type::Undefined;
      }

      const VALUE Object(uint32_t initial_capacity, VALUE klass) {
        return (VALUE)(new class Object(initial_capacity, klass));
      }

      const VALUE Integer(int64_t val) {
        return ((VALUE) val << 1) | IIntegerFlag;
      }

      const VALUE Float(double val) {
        union {
          double d;
          VALUE v;
        } t;

        t.d = val;

        int bits = (int)((VALUE)(t.v >> 60) & SpecialMask);

        /* I have no idea what's going on here, this was taken from ruby source code */
        if (t.v != 0x3000000000000000 && !((bits - 3) & ~0x01)) {
          return (BIT_ROTL(t.v, 3) & ~(VALUE)0x01) | IFloatFlag;
        } else if (t.v == 0) {

          /* +0.0 */
          return 0x8000000000000002;
        }

        // TODO: Replace with call to GC malloc
        return (VALUE)(new class Float(val));
      }

      const int64_t IntegerValue(VALUE val) {
        return ((SIGNED_VALUE)val) >> 1;
      }

      const double FloatValue(VALUE val) {
        if (!is_special(val)) return ((class Float *)val)->float_value;

        union {
          double d;
          VALUE v;
        } t;

        VALUE b63 = (val >> 63);
        t.v = BIT_ROTR((IFloatFlag - b63) | (val & ~(VALUE)IFloatMask), 3);
        return t.d;
      }
    }

  }
}
