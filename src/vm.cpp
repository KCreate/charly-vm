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

    Frame* VM::push_frame(VALUE self,
                          Function* function,
                          uint8_t* return_address) {

      GC::Cell* cell = this->gc->allocate();
      cell->as.frame.flags = Type::Frame;
      cell->as.frame.parent = this->frames;
      cell->as.frame.parent_environment_frame = function->context;
      cell->as.frame.function = function;
      cell->as.frame.last_block = this->frames ? this->frames->function->block : NULL;
      cell->as.frame.environment = new Container(4);
      cell->as.frame.self = self;
      cell->as.frame.return_address = return_address;
      this->frames = (Frame *)cell;
      return (Frame *)cell;
    }

    STATUS VM::read(VALUE* result, std::string key) {
      Frame* frame = this->frames;
      if (!frame) return Status::ReadFailedVariableUndefined;

      while (frame) {
        STATUS read_stat = frame->environment->read(key, result);
        if (read_stat == Status::Success) return Status::Success;
        frame = frame->parent_environment_frame;
      }

      return Status::ReadFailedVariableUndefined;
    }

    STATUS VM::read(VALUE* result, uint32_t index, uint32_t level) {
      Frame* frame = this->frames;

      // Move to the correct frame
      while (level--) {
        if (!frame) return Status::ReadFailedTooDeep;
        frame = frame->parent_environment_frame;
      }

      if (!frame) return Status::ReadFailedTooDeep;
      return frame->environment->read(index, result);
    }

    STATUS VM::write(std::string key, VALUE value) {
      Frame* frame = this->frames;
      if (!frame) return Status::ReadFailedVariableUndefined;

      while (frame) {
        STATUS write_stat = frame->environment->write(key, value, false);
        if (write_stat == Status::Success) return Status::Success;
        frame = frame->parent_environment_frame;
      }

      return Status::WriteFailedVariableUndefined;
    }

    STATUS VM::write(uint32_t index, uint32_t level, VALUE value) {
      Frame* frame = this->frames;

      // Move to the correct frame
      while (level--) {
        if (!frame) return Status::WriteFailedTooDeep;
        frame = frame->parent_environment_frame;
      }

      if (!frame) return Status::ReadFailedTooDeep;
      return frame->environment->write(index, value);
    }

    STATUS VM::pop_stack(VALUE* result) {
      if (this->stack.size() == 0) return Status::PopFailed;
      VALUE& top = this->stack.top();
      this->stack.pop();
      *result = top;
      return Status::Success;
    }

    STATUS VM::peek_stack(VALUE* result) {
      if (this->stack.size() == 0) return Status::PeekFailed;
      *result = this->stack.top();
      return Status::Success;
    }

    void VM::push_stack(VALUE value) {
      this->stack.push(value);
    }

    VALUE VM::create_object(uint32_t initial_capacity, VALUE klass) {
      GC::Cell* cell = this->gc->allocate();
      cell->as.basic.flags = Type::Object;
      cell->as.basic.klass = klass;
      cell->as.object.container = new Container(initial_capacity);
      return (VALUE)cell;
    }

    VALUE VM::create_integer(int64_t value) {
      return ((VALUE) value << 1) | Value::IIntegerFlag;
    }

    VALUE VM::create_float(double value) {
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

    VALUE VM::create_function(std::string name,
                              uint32_t required_arguments,
                              Frame* context,
                              InstructionBlock* block) {

      GC::Cell* cell = this->gc->allocate();
      cell->as.basic.flags = Type::Function;
      cell->as.basic.klass = Value::Null; // TODO: Replace with actual class
      cell->as.function.name = name;
      cell->as.function.required_arguments = required_arguments;
      cell->as.function.context = context;
      cell->as.function.block = block;
      cell->as.function.bound_self_set = false;
      cell->as.function.bound_self = Value::Null;
      return (VALUE)cell;
    }

    int64_t VM::integer_value(VALUE value) {
      return ((SIGNED_VALUE)value) >> 1;
    }

    double VM::float_value(VALUE value) {
      if (!Value::is_special(value)) return ((Float *)value)->float_value;

      union {
        double d;
        VALUE v;
      } t;

      VALUE b63 = (value >> 63);
      t.v = BIT_ROTR((Value::IFloatFlag - b63) | (value & ~(VALUE)Value::IFloatMask), 3);
      return t.d;
    }

    bool VM::boolean_value(VALUE value) {
      if (value == Value::False || value == Value::Null) return false;
      return true;
    }

    VALUE VM::type(VALUE value) {

      /* Constants */
      if (Value::is_false(value) || Value::is_true(value)) return Type::Boolean;
      if (Value::is_null(value)) return Type::Null;

      /* More logic required */
      if (!Value::is_special(value)) return Value::basics(value)->type();
      if (Value::is_integer(value)) return Type::Integer;
      if (Value::is_ifloat(value)) return Type::Float;
      return Type::Undefined;
    }

    Opcode VM::fetch_instruction() {
      if (this->ip == NULL) return Opcode::Nop;
      return *(Opcode *)this->ip;
    }

    uint32_t VM::decode_instruction_length() {
      switch (this->fetch_instruction()) {
          case Opcode::Nop:              return 1;
          case Opcode::ReadSymbol:       return 1 + sizeof(ID);
          case Opcode::ReadMemberSymbol: return 1 + sizeof(ID);
          case Opcode::ReadMemberValue:  return 1;
          case Opcode::SetSymbol:        return 1 + sizeof(ID);
          case Opcode::SetMemberSymbol:  return 1 + sizeof(ID);
          case Opcode::SetMemberValue:   return 1;
          case Opcode::PutSelf:          return 1;
          case Opcode::PutString:        return 1 + sizeof(char*) + (sizeof(uint32_t) * 2);
          case Opcode::PutFloat:         return 1 + sizeof(double);
          case Opcode::PutFunction:      return 1 + sizeof(ID) + sizeof(void*) + sizeof(bool) sizeof(uint32_t);
          case Opcode::PutCFunction:     return 1 + sizeof(ID) + sizeof(void *) + sizeof(uint32_t);
          case Opcode::PutArray:         return 1 + sizeof(uint32_t);
          case Opcode::PutHash:          return 1 + sizeof(uint32_t);
          case Opcode::PutClass:         return 1 + sizeof(ID) + sizeof(uint32_t);
          case Opcode::RegisterLocal:    return 1 + sizeof(ID) + sizeof(uint32_t);
          case Opcode::MakeConstant:     return 1 + sizeof(uint32_t);
          case Opcode::Pop:              return 1 + sizeof(uint32_t);
          case Opcode::Dup:              return 1;
          case Opcode::Swap:             return 1;
          case Opcode::Topn:             return 1 + sizeof(uint32_t);
          case Opcode::Setn:             return 1 + sizeof(uint32_t);
          case Opcode::Call:             return 1 + sizeof(uint32_t);
          case Opcode::CallMember:       return 1 + sizeof(uint32_t);
          case Opcode::Throw:            return 1 + sizeof(uint8_t);
          case Opcode::Branch:           return 1 + sizeof(uint32_t);
          case Opcode::BranchIf:         return 1 + sizeof(uint32_t);
          case Opcode::BranchUnless:     return 1 + sizeof(uint32_t);

          // Catch all for binary and unary operators
          default:                       return 1;
      }
    }

    void VM::call(uint32_t argc) {
      if (this->stack.size() < 1) panic("Not enough items on the stack for call");

      Function* function; this->pop_stack((VALUE *)&function);

      // TODO: Handle as runtime error
      if (this->type((VALUE)function) != Type::Function) panic("Popped value isn't a function");

      // Push a control frame for the function
      // TODO: Correct self value
      this->push_frame(Value::Null, function, this->ip + 1);
      this->ip = function->block->data;
    }

    void VM::stacktrace() {
      Frame* frame = this->frames;

      int i = 0;
      cout << i++ << " : (" << (void*)this->ip << ")" << endl;
      while (frame) {
        Function* func = frame->function;

        cout << i++ << " : (";
        cout << frame->function->name << " : ";
        cout << (void*)frame->function->block->data << "[" << frame->function->block->data_size << "] : ";
        cout << (void*)frame->return_address;
        cout << ")" << endl;
        frame = frame->parent;
      }
    }

    void VM::init() {
      this->frames = NULL; // Initialize top-level here
      this->ip = NULL;

      InstructionBlock* main_block = new InstructionBlock{ 0x00, 0, (new uint8_t[32] { 0 }), 32 };
      VALUE main_function = this->create_function("__charly_init", 0, NULL, main_block);

      InstructionBlock* foo_block = new InstructionBlock{ 0x00, 0, (new uint8_t[32] { 0 }), 32 };
      VALUE foo_function = this->create_function("foo", 0, NULL, foo_block);

      this->push_stack(foo_function);
      this->push_stack(main_function);
    }

    void VM::run() {
      this->call(0); // Call the main function that was pushed in the init method
      this->call(0);
      this->stacktrace();
    }

  }
}
