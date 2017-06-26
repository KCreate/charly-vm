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

#include "vm.h"

using namespace std;

namespace Charly {
  namespace Machine {
    using namespace Primitive;
    using namespace Scope;

    Frame* VM::pop_frame() {
      Frame* frame = this->frames;
      if (frame) this->frames = this->frames->parent;
      return frame;
    }

    Frame* VM::push_frame(VALUE self, Frame* parent_environment_frame, Function* calling_function) {
      GC::Cell* cell = this->gc->allocate();
      cell->as.frame.flags = Type::Frame;
      cell->as.frame.parent = this->frames;
      cell->as.frame.parent_environment_frame = parent_environment_frame;
      cell->as.frame.calling_function = calling_function;
      cell->as.frame.environment = new Container(4);
      cell->as.frame.self = self;
      this->frames = (Frame *)cell;
      return (Frame *)cell;
    }

    const STATUS VM::read(VALUE* result, std::string key) {
      Frame* frame = this->frames;
      if (!frame) return Status::ReadFailedVariableUndefined;

      while (frame) {
        STATUS read_stat = frame->environment->read(key, result);
        if (read_stat == Status::Success) return Status::Success;
        frame = frame->parent_environment_frame;
      }

      return Status::ReadFailedVariableUndefined;
    }

    const STATUS VM::read(VALUE* result, uint32_t index, uint32_t level) {
      Frame* frame = this->frames;

      // Move to the correct frame
      while (level--) {
        if (!frame) return Status::ReadFailedTooDeep;
        frame = frame->parent_environment_frame;
      }

      if (!frame) return Status::ReadFailedTooDeep;
      return frame->environment->read(index, result);
    }

    const STATUS VM::write(std::string key, VALUE value) {
      Frame* frame = this->frames;
      if (!frame) return Status::ReadFailedVariableUndefined;

      while (frame) {
        STATUS write_stat = frame->environment->write(key, value, false);
        if (write_stat == Status::Success) return Status::Success;
        frame = frame->parent_environment_frame;
      }

      return Status::WriteFailedVariableUndefined;
    }

    const STATUS VM::write(uint32_t index, uint32_t level, VALUE value) {
      Frame* frame = this->frames;

      // Move to the correct frame
      while (level--) {
        if (!frame) return Status::WriteFailedTooDeep;
        frame = frame->parent_environment_frame;
      }

      if (!frame) return Status::ReadFailedTooDeep;
      return frame->environment->write(index, value);
    }

    const STATUS VM::pop_stack(VALUE* result) {
      if (this->stack.size() == 0) return Status::PopFailed;
      VALUE& top = this->stack.top();
      this->stack.pop();
      *result = top;
      return Status::Success;
    }

    const STATUS VM::peek_stack(VALUE* result) {
      if (this->stack.size() == 0) return Status::PeekFailed;
      *result = this->stack.top();
      return Status::Success;
    }

    const void VM::push_stack(VALUE value) {
      this->stack.push(value);
    }

    const VALUE VM::create_object(uint32_t initial_capacity, VALUE klass) {
      GC::Cell* cell = this->gc->allocate();
      cell->as.basic.flags = Type::Object;
      cell->as.basic.klass = klass;
      cell->as.object.container = new Container(initial_capacity);
      return (VALUE)cell;
    }

    const VALUE VM::create_integer(int64_t value) {
      return ((VALUE) value << 1) | Value::IIntegerFlag;
    }

    const VALUE VM::create_float(double value) {
      union {
        double d;
        VALUE v;
      } t;

      t.d = value;

      int bits = (int)((VALUE)(t.v >> 60) & Value::SpecialMask);

      /* I have no idea what's going on here, this was taken from ruby source code */
      if (t.v != 0x3000000000000000 && !((bits - 3) & ~0x01)) {
        return (BIT_ROTL(t.v, 3) & ~(VALUE)0x01) | Value::IFloatFlag;
      } else if (t.v == 0) {

        /* +0.0 */
        return 0x8000000000000002;
      }

      // Allocate from the GC
      GC::Cell* cell = this->gc->allocate();
      cell->as.basic.flags = Type::Float;
      cell->as.basic.klass = Value::Null; // TODO: Replace with actual class
      cell->as.flonum.float_value = value;
      return (VALUE)cell;
    }

    const VALUE VM::create_function(std::string name, uint32_t required_arguments, Frame* context) {
      GC::Cell* cell = this->gc->allocate();
      cell->as.basic.flags = Type::Function;
      cell->as.basic.klass = Value::Null; // TODO: Replace with actual class
      cell->as.function.name = name;
      cell->as.function.required_arguments = required_arguments;
      cell->as.function.context = context;
      cell->as.function.bound_self_set = false;
      cell->as.function.bound_self = Value::Null;
      return (VALUE)cell;
    }

    const int64_t VM::integer_value(VALUE value) {
      return ((SIGNED_VALUE)value) >> 1;
    }

    const double VM::float_value(VALUE value) {
      if (!Value::is_special(value)) return ((Float *)value)->float_value;

      union {
        double d;
        VALUE v;
      } t;

      VALUE b63 = (value >> 63);
      t.v = BIT_ROTR((Value::IFloatFlag - b63) | (value & ~(VALUE)Value::IFloatMask), 3);
      return t.d;
    }

    const bool VM::boolean_value(VALUE value) {
      return value == Value::True;
    }

    const VALUE VM::type(VALUE value) {

      /* Constants */
      if (Value::is_false(value) || Value::is_true(value)) return Type::Boolean;
      if (Value::is_null(value)) return Type::Null;

      /* More logic required */
      if (!Value::is_special(value)) return Value::basics(value)->type();
      if (Value::is_integer(value)) return Type::Integer;
      if (Value::is_ifloat(value)) return Type::Float;
      return Type::Undefined;
    }

    void VM::init() {
      cout << "Initalizing vm at " << this << endl;
    }

    void VM::run() {
    }

  }
}
