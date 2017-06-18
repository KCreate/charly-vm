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

#include "defines.h"
#include "scope.h"

#pragma once

namespace Charly {

  namespace Primitive {

    /*
     * Basic fields every data type in Charly has
     * This is inspired by the way Ruby stores it's values
     * */
    class Basic {
      public:
        VALUE flags;
        VALUE klass;

        Basic(VALUE type, VALUE kl) : flags((VALUE)type), klass(kl) {}

        /* Getters for different flag fields */
        const inline VALUE type() { return this->flags & Flag::Type; }
        const inline VALUE mark() { return this->flags & Flag::Mark; }

        /* Setters for different flag fields */
        inline void set_type(VALUE val) { this->flags &= (~Flag::Type & val); }
        inline void set_mark(bool val) { this->flags ^= (-val ^ this->flags) & Flag::Mark; }
    };

    class Object {
      public:
        Basic basic;
        Scope::Container* container;

        Object(uint32_t initial_capacity, VALUE klass)
          : basic(Basic(Type::Object, klass)),
          container(new Scope::Container(initial_capacity)) {}
    };

    class Float {
      public:
        Basic basic;
        double float_value;

        Float(double val) : basic(Basic(Type::Float, 0)), float_value(val) {}
    };

    /* Rotate a given value to the left n times */
    const constexpr VALUE BIT_ROTL(VALUE v, VALUE n) {
      return (((v) << (n)) | ((v) >> ((sizeof(v) * 8) - n)));
    }

    /* Rotate a given value to the right n times */
    const constexpr VALUE BIT_ROTR(VALUE v, VALUE n) {
      return (((v) >> (n)) | ((v) << ((sizeof(v) * 8) - n)));
    }
  }

}
