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

#include "charly.h"

#pragma once

namespace Charly {

  namespace Value {

    namespace Type {
      enum : TYPE {
        Numeric = 0x00,
        Mask = 0x1f
      };
    }

    /*
     * Basic fields every data type in Charly has
     * This is inspired by the way Ruby stores it's values
     * */
    struct Basic {
      VALUE flags;
      VALUE klass;

      /* Returns the type field in the flags value */
      TYPE type() {
        return this->flags & Type::Mask;
      }
    };

    /*
     * Represents numeric values as a double-precision floating-point number
     * */
    struct Numeric : public Basic {
      double value;
      Numeric(double arg) : value(arg) {}

      /* Returns value casted to an integer */
      int64_t intval() {
        return (int64_t)value;
      }
    };

  }

}
