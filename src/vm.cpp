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

    Frame* VM::push_frame(VALUE self, Function* function, uint8_t* return_address) {
      GC::Cell* cell = this->gc->allocate();
      cell->as.frame.flags = Type::Frame;
      cell->as.frame.parent = this->frames;
      cell->as.frame.parent_environment_frame = function->context;
      cell->as.frame.function = function;
      cell->as.frame.last_block = this->frames ? this->frames->function->block : NULL;

      uint32_t lvar_count = function->required_arguments + function->block->lvarcount;
      cell->as.frame.environment = new Container(lvar_count);

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
      VALUE& top = this->stack.back();
      this->stack.pop_back();
      *result = top;
      return Status::Success;
    }

    STATUS VM::peek_stack(VALUE* result) {
      if (this->stack.size() == 0) return Status::PeekFailed;
      *result = this->stack.back();
      return Status::Success;
    }

    void VM::push_stack(VALUE value) {
      this->stack.push_back(value);
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
                              InstructionBlock* block) {

      GC::Cell* cell = this->gc->allocate();
      cell->as.basic.flags = Type::Function;
      cell->as.basic.klass = Value::Null; // TODO: Replace with actual class
      cell->as.function.name = name;
      cell->as.function.required_arguments = required_arguments;
      cell->as.function.context = this->frames;
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

    uint32_t VM::decode_instruction_length(Opcode opcode) {
      switch (opcode) {
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
        case Opcode::PutFunction:      return 1 + sizeof(ID) + sizeof(void*) + sizeof(bool) + sizeof(uint32_t);
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
        default:                       return 1;
      }
    }

    void VM::op_putvalue(VALUE value) {
      this->push_stack(value);
    }

    void VM::op_call(uint32_t argc) {
      cout << "calling function with " << argc << " arguments" << endl;

      // Check if there are enough items on the stack
      //
      // +- VALUE of the function
      // |
      // |   +- Amount of VALUEs that are on the
      // |   |  stack which are arguments
      // v   v
      // 1 + argc
      if (this->stack.size() < (1 + argc)) panic("Not enough items on the stack for call");

      // Allocate enough space to copy the arguments into a temporary buffer
      // We need to keep them around until we have access to the new frames environment
      VALUE* arguments = NULL;
      if (argc > 0) {
        arguments = (VALUE*)alloca(argc * sizeof(VALUE));
        cout << "allocated " << (argc * sizeof(VALUE)) << " bytes for arguments at " << arguments << endl;
      }

      uint32_t argc_backup = argc;
      while (argc--) {
        cout << "popping stack into " << (arguments + argc) << endl;
        this->pop_stack(arguments + (argc));
      }

      // Pop the function from the stack
      Function* function; this->pop_stack((VALUE *)&function);

      // TODO: Handle as runtime error
      if (this->type((VALUE)function) != Type::Function) panic("Popped value isn't a function");

      // Push a control frame for the function
      VALUE self = function->context ? function->context->self : Value::Null;
      uint8_t* return_address = this->ip + this->decode_instruction_length(Opcode::Call);
      Frame* frame = this->push_frame(self, function, return_address);

      // Copy the arguments from the temporary arguments buffer into
      // the frames environment
      //
      // The environment will contain enough space for argcount + function.lvar_count
      // values so there's no need to allocate new space in it
      //
      // TODO: Copy the _arguments_ field into the function
      uint32_t arg_copy_count = function->required_arguments;
      while (arg_copy_count--) {
        frame->environment->write(arg_copy_count, arguments[arg_copy_count]);
      }

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
      VALUE main_function = this->create_function("__charly_init", 0, main_block);

      this->push_stack(main_function);
      this->op_call(0);

      // Set the self value
      this->frames->self = this->create_integer(25);
    }

    void VM::run() {

      // Create the foo function on the stack
      InstructionBlock* foo_block = new InstructionBlock{ 0x00, 4, (new uint8_t[32] { 0 }), 32 };
      VALUE foo_function = this->create_function("foo", 4, foo_block);
      this->push_stack(foo_function);

      // Call the foo function
      this->op_putvalue(this->create_integer(25));
      this->op_putvalue(this->create_integer(75));
      this->op_putvalue(this->create_integer(75));
      this->op_putvalue(this->create_integer(75));
      this->op_call(4);

      this->stacktrace();
    }

  }
}
