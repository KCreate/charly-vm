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
#include "value.h"
#include "scope.h"

#pragma once

namespace Charly {
  namespace Machine {
    /*
     * Frames introduce new environments and branch logic
     * */
    struct Frame {
      VALUE flags;
      Frame* parent;
      Frame* parent_environment_frame;
      Scope::Container* environment;
      VALUE self;

      void init(Frame* parent, Frame* parent_environment_frame, VALUE self) {
        this->flags = Primitive::Type::Frame;
        this->parent = parent;
        this->parent_environment_frame = parent_environment_frame;
        if (this->environment) delete this->environment;
        this->environment = new Scope::Container();
        this->self = self;
      }

      // TODO: Add VM specific control values here
    };
  }
}

