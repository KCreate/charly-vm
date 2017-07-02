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
#include <functional>

#include "vm.h"

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
      cell->as.basic.set_type(Type::Frame);
      cell->as.frame.parent = this->frames;
      cell->as.frame.parent_environment_frame = function->context;
      cell->as.frame.function = function;

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

    InstructionBlock* VM::request_instruction_block(ID id, uint32_t lvarcount) {
      InstructionBlock* block = new InstructionBlock(id, lvarcount);
      this->register_instruction_block(block);
      return block;
    }

    void VM::register_instruction_block(InstructionBlock* block) {
      this->unassigned_blocks.push_back(block);
    }

    void VM::claim_instruction_block(InstructionBlock* block) {
      size_t index = this->unassigned_blocks.size();

      while (index--) {
        if (this->unassigned_blocks[index] == block) {
          this->unassigned_blocks.erase(this->unassigned_blocks.begin() + index);
          return;
        }
      }
    }

    VALUE VM::pop_stack() {
      if (this->stack.size() == 0) return Value::Null;
      VALUE val = this->stack.back();
      this->stack.pop_back();
      return val;
    }

    VALUE VM::peek_stack() {
      if (this->stack.size() == 0) return Value::Null;
      return this->stack.back();
    }

    void VM::push_stack(VALUE value) {
      this->stack.push_back(value);
    }

    STATUS VM::lookup_symbol(VALUE symbol, std::string* result) {
      auto found_string = this->id_table.find(symbol);

      if (found_string == this->id_table.end()) {
        return Status::UnknownSymbol;
      }

      *result = found_string->second; // second refers to the value
      return Status::Success;
    }

    VALUE VM::create_symbol(std::string value) {
      size_t hashvalue = std::hash<std::string>{}(value);
      VALUE symbol = (VALUE)((hashvalue & ~Value::ISymbolMask) | Value::ISymbolFlag);

      // Add this hash to the id_table if it doesn't exist
      if (this->id_table.find(symbol) == this->id_table.end()) {
        this->id_table.insert({symbol, value});
      }

      return symbol;
    }

    VALUE VM::create_object(uint32_t initial_capacity, VALUE klass) {
      GC::Cell* cell = this->gc->allocate();
      cell->as.basic.set_type(Type::Object);
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

      int bits = (int)((VALUE)(t.v >> 60) & Value::IPointerMask);

      /* I have no idea what's going on here, this was taken from ruby source code */
      if (t.v != 0x3000000000000000 && !((bits - 3) & ~0x01)) {
        return (BIT_ROTL(t.v, 3) & ~(VALUE)0x01) | Value::IFloatFlag;
      } else if (t.v == 0) {

        /* +0.0 */
        return 0x8000000000000002;
      }

      // Allocate from the GC
      GC::Cell* cell = this->gc->allocate();
      cell->as.basic.set_type(Type::Float);
      cell->as.basic.klass = Value::Null; // TODO: Replace with actual class
      cell->as.flonum.float_value = value;
      return (VALUE)cell;
    }

    VALUE VM::create_function(std::string name, uint32_t required_arguments, InstructionBlock* block) {

      GC::Cell* cell = this->gc->allocate();
      cell->as.basic.set_type(Type::Function);
      cell->as.basic.klass = Value::Null; // TODO: Replace with actual class
      cell->as.function.name = name;
      cell->as.function.required_arguments = required_arguments;
      cell->as.function.context = this->frames;
      cell->as.function.block = block;
      cell->as.function.bound_self_set = false;
      cell->as.function.bound_self = Value::Null;

      this->claim_instruction_block(block);

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
        case Opcode::Nop:               return 1;
        case Opcode::ReadLocal:         return 1 + sizeof(uint32_t);
        case Opcode::ReadSymbol:        return 1 + sizeof(ID);
        case Opcode::ReadMemberSymbol:  return 1 + sizeof(ID);
        case Opcode::ReadMemberValue:   return 1;
        case Opcode::SetLocal:          return 1 + sizeof(uint32_t);
        case Opcode::SetSymbol:         return 1 + sizeof(ID);
        case Opcode::SetMemberSymbol:   return 1 + sizeof(ID);
        case Opcode::SetMemberValue:    return 1;
        case Opcode::PutSelf:           return 1;
        case Opcode::PutValue:          return 1 + sizeof(VALUE);
        case Opcode::PutString:         return 1 + sizeof(char*) + (sizeof(uint32_t) * 2);
        case Opcode::PutFloat:          return 1 + sizeof(double);
        case Opcode::PutFunction:       return 1 + sizeof(ID) + sizeof(void*) + sizeof(bool) + sizeof(uint32_t);
        case Opcode::PutCFunction:      return 1 + sizeof(ID) + sizeof(void *) + sizeof(uint32_t);
        case Opcode::PutArray:          return 1 + sizeof(uint32_t);
        case Opcode::PutHash:           return 1 + sizeof(uint32_t);
        case Opcode::PutClass:          return 1 + sizeof(ID) + sizeof(uint32_t);
        case Opcode::RegisterLocal:     return 1 + sizeof(ID) + sizeof(uint32_t);
        case Opcode::MakeConstant:      return 1 + sizeof(uint32_t);
        case Opcode::Pop:               return 1 + sizeof(uint32_t);
        case Opcode::Dup:               return 1;
        case Opcode::Swap:              return 1;
        case Opcode::Topn:              return 1 + sizeof(uint32_t);
        case Opcode::Setn:              return 1 + sizeof(uint32_t);
        case Opcode::Call:              return 1 + sizeof(uint32_t);
        case Opcode::CallMember:        return 1 + sizeof(uint32_t);
        case Opcode::Throw:             return 1 + sizeof(uint8_t);
        case Opcode::Branch:            return 1 + sizeof(uint32_t);
        case Opcode::BranchIf:          return 1 + sizeof(uint32_t);
        case Opcode::BranchUnless:      return 1 + sizeof(uint32_t);
        default:                        return 1;
      }
    }

    void VM::op_readlocal(uint32_t index) {

      // Check if the index points to a valid entry
      if (index >= this->frames->environment->entries.size()) {
        this->push_stack(Value::Null);
        return;
      }

      VALUE value = Value::Null;
      this->frames->environment->read(index, &value);
      this->push_stack(value);
    }

    void VM::op_setlocal(uint32_t index) {
      VALUE value = this->pop_stack();

      if (index < this->frames->environment->entries.size()) {
        STATUS write_status = this->frames->environment->write(index, value);

        if (write_status != Status::Success) {

          // TODO: Handle this in a better way
          this->panic("Write failed");
        }
      }
    }

    void VM::op_putself() {
      if (this->frames == NULL) {
        this->push_stack(Value::Null);
        return;
      }

      this->push_stack(this->frames->self);
    }

    void VM::op_putvalue(VALUE value) {
      this->push_stack(value);
    }

    void VM::op_call(uint32_t argc) {

      // Check if there are enough items on the stack
      //
      // +- VALUE of the function
      // |
      // |   +- Amount of VALUEs that are on the
      // |   |  stack which are arguments
      // v   v
      // 1 + argc
      if (this->stack.size() < (1 + argc)) this->panic("Not enough items on the stack for call");

      // Allocate enough space to copy the arguments into a temporary buffer
      // We need to keep them around until we have access to the new frames environment
      VALUE* arguments = NULL;
      if (argc > 0) {
        arguments = (VALUE*)alloca(argc * sizeof(VALUE));
      }

      uint32_t argc_backup = argc;
      while (argc--) {
        *(arguments + argc) = this->pop_stack();
      }

      // Pop the function from the stack
      Function* function = (Function *)this->pop_stack();

      // TODO: Handle as runtime error
      if (this->type((VALUE)function) != Type::Function) this->panic("Popped value isn't a function");

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

    void VM::op_throw(ThrowType type) {

      switch (type) {
        case ThrowType::Return: {

          // Unlink the current frame
          Frame* frame = this->frames;
          if (frame == NULL) this->panic("Can't return from top-level");
          this->frames = frame->parent;

          // Restore the return address
          this->ip = frame->return_address;

          break;
        }

        default: {
          this->panic("Unknown throw type");
        }
      }
    }

    void VM::op_add() {
      VALUE right = this->pop_stack();
      VALUE left = this->pop_stack();

      if (this->type(left) == Type::Integer && this->type(right) == Type::Integer) {
        int64_t i_left = this->integer_value(left);
        int64_t i_right = this->integer_value(right);
        this->push_stack(this->create_integer(i_left + i_right));
        return;
      }

      if (this->type(left) == Type::Float && this->type(right) == Type::Float) {
        double f_left = this->float_value(left);
        double f_right = this->float_value(right);
        this->push_stack(this->create_float(f_left + f_right));
        return;
      }

      if (this->type(left) == Type::Integer && this->type(right) == Type::Float) {
        double f_left = (double)this->integer_value(left);
        double f_right = this->float_value(right);
        this->push_stack(this->create_float(f_left + f_right));
        return;
      }

      if (this->type(left) == Type::Float && this->type(right) == Type::Integer) {
        double f_left = this->float_value(left);
        double f_right = (double)this->integer_value(right);
        this->push_stack(this->create_float(f_left + f_right));
        return;
      }

      this->panic("Unknown types in ADD operation");
    }

    void VM::stacktrace(std::ostream& io) {
      Frame* frame = this->frames;

      int i = 0;
      io << i++ << " : (" << (void*)this->ip << ")" << std::endl;
      while (frame) {
        Function* func = frame->function;

        io << i++ << " : (";
        io << frame->function->name << " : ";
        io << (void*)frame->function->block->data << "[" << frame->function->block->write_offset << "] : ";
        io << (void*)frame->return_address;
        io << ")" << std::endl;
        frame = frame->parent;
      }
    }

    void VM::stackdump(std::ostream& io) {
      for (int i = 0; i < this->stack.size(); i++) {
        VALUE entry = this->stack[i];

        io << "<" << Type::str[this->type(entry)] << "@" << (void*)entry << ">" << std::endl;
      }
    }

    void VM::init() {
      this->frames = NULL; // Initialize top-level here
      this->ip = NULL;

      // Setup top-level-block
      VALUE __charly_init_block_id = this->create_symbol("__charly_init_block");
      auto __charly_init_block = this->request_instruction_block(__charly_init_block_id, 0);
      VALUE __charly_init = this->create_function("__charly_init", 0, __charly_init_block);

      this->push_stack(__charly_init);
      this->op_call(0);

      // Set the self value
      this->frames->self = this->create_integer(25);
    }

    void VM::run() {

      // Execute instructions as long as we have a valid ip
      // and the machine wasn't halted
      bool machine_halted = false;
      while (this->ip && !machine_halted) {

        // Backup the current ip
        // After the instruction was executed we check if
        // ip stayed the same. If it's still the same value
        // we increment it to the next instruction (via decode_instruction_length)
        uint8_t* old_ip = this->ip;

        // Check if we're out-of-bounds relative to the current block
        uint8_t* block_data = this->frames->function->block->data;
        uint32_t block_write_offset = this->frames->function->block->write_offset;


        if (this->ip + sizeof(Opcode) - 1 >= block_data + block_write_offset) {
          this->panic("IP is out of bounds!");
        }

        // Retrieve the current opcode
        Opcode opcode = *(Opcode *)this->ip;

        // Check if there is enough space for instruction arguments
        uint32_t instruction_length = this->decode_instruction_length(opcode);
        if (this->ip + instruction_length >= (block_data + block_write_offset + sizeof(Opcode))) {
          this->panic("Not enough space for instruction arguments");
        }

        // Redirect to specific instruction handler
        switch (opcode) {
          case Opcode::Nop: {
            break;
          }

          case Opcode::ReadLocal: {
            uint32_t index = *(uint32_t *)(this->ip + sizeof(Opcode));
            this->op_readlocal(index);
            break;
          }

          case Opcode::SetLocal: {
            uint32_t index = *(uint32_t *)(this->ip + sizeof(Opcode));
            this->op_setlocal(index);
            break;
          }

          case Opcode::PutSelf: {
            this->op_putself();
          }

          case Opcode::PutValue: {
            VALUE value = *(VALUE *)(this->ip + sizeof(Opcode));
            this->op_putvalue(value);
            break;
          }

          case Opcode::Call: {
            uint32_t argc = *(VALUE *)(this->ip + sizeof(Opcode));
            this->op_call(argc);
            break;
          }

          case Opcode::Throw: {
            ThrowType throw_type = *(ThrowType *)(this->ip + sizeof(Opcode));
            this->op_throw(throw_type);
            break;
          }

          case Opcode::Add: {
            this->op_add();
            break;
          }

          case 0xff: {
            machine_halted = true;
            break;
          }

          default: {
            std::cout << "Opcode: " << (void *)opcode << std::endl;
            this->panic("Unrecognized opcode");
          }
        }

        // Increment ip if the instruction didn't change it
        if (this->ip == old_ip) {
          this->ip += instruction_length;
        }
      }
    }

  }
}
