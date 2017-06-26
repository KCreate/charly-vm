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

#include <stack>

#include "defines.h"
#include "gc.h"
#include "value.h"
#include "scope.h"
#include "frame.h"
#include "instruction.h"

#pragma once

namespace Charly {
  namespace Machine {
    using namespace Primitive;

    class VM {
      private:
        GC::Collector* gc;
        std::stack<VALUE> stack;
        Frame* frames;

      public:

        // Methods that operate on the VM's frames
        Frame* pop_frame();
        inline Frame* top_frame() { return this->frames; }
        Frame* push_frame(VALUE self, Frame* parent_environment_frame, Function* calling_function);

        // Read and write from/to the frame hierarchy
        const STATUS read(VALUE* result, std::string key);
        const STATUS read(VALUE* result, uint32_t index, uint32_t level);
        const STATUS write(std::string key, VALUE value);
        const STATUS write(uint32_t index, uint32_t level, VALUE value);

        // Stack manipulation
        const STATUS pop_stack(VALUE* result);
        const STATUS peek_stack(VALUE* result);
        const void push_stack(VALUE value);

        // Methods to create new data types
        const VALUE create_object(uint32_t initial_capacity, VALUE klass);
        const VALUE create_integer(int64_t value);
        const VALUE create_float(double value);
        const VALUE create_function(std::string name, size_t required_arguments, Frame* context);

        // Methods that operate on the VALUE type
        const int64_t integer_value(VALUE value);
        const double float_value(VALUE value);
        const bool boolean_value(VALUE value);
        const VALUE type(VALUE value);

      private:
        void pretty_print(VALUE value);

      public:
        VM() : gc(new GC::Collector(InitialHeapCount, HeapCellCount)), frames(NULL) {
          this->init();
        }
        void init();
        void run();
    };
  }
}
