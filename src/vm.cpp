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
#include <algorithm>
#include <cmath>

#include "vm.h"
#include "gc.h"
#include "status.h"
#include "operators.h"

namespace Charly {

  VM::~VM() {
    this->frames = nullptr;
    this->catchstack = nullptr;
    this->stack.clear();
    this->gc.collect(this);
  }

  Frame* VM::pop_frame() {
    Frame* frame = this->frames;
    if (frame) this->frames = this->frames->parent;
    return frame;
  }

  Frame* VM::push_frame(VALUE self, Function* function, uint8_t* return_address) {
    MemoryCell* cell = this->gc.allocate(this);
    cell->as.basic.set_type(kTypeFrame);
    cell->as.frame.parent = this->frames;
    cell->as.frame.parent_environment_frame = function->context;
    cell->as.frame.function = function;
    cell->as.frame.self = self;
    cell->as.frame.return_address = return_address;

    // Calculate the number of local variables this frame has to support
    uint32_t lvar_count = function->argc + function->block->lvarcount;

    // Allocate and prefill local variable space
    cell->as.frame.environment = new std::vector<FrameEnvironmentEntry>;
    cell->as.frame.environment->reserve(lvar_count);
    while (lvar_count--) cell->as.frame.environment->push_back({false, kNull});

    // Append the frame
    this->frames = (Frame *)cell;
    return (Frame *)cell;
  }

  InstructionBlock& VM::request_instruction_block(uint32_t lvarcount) {
    MemoryCell* cell = this->gc.allocate(this);
    new (&cell->as.instructionblock) InstructionBlock(lvarcount);
    cell->as.basic.set_type(kTypeInstructionBlock);
    return *(InstructionBlock *)cell;
  }

  VALUE VM::pop_stack() {
    VALUE value = kNull;
    if (this->stack.size() > 0) {
      value = this->stack.back();
      this->stack.pop_back();
    }

    return value;
  }

  void VM::push_stack(VALUE value) {
    this->stack.push_back(value);
  }

  CatchTable* VM::push_catchtable(ThrowType type, uint8_t* address) {
    MemoryCell* cell = this->gc.allocate(this);
    cell->as.basic.set_type(kTypeCatchTable);
    cell->as.catchtable.stacksize = this->stack.size();
    cell->as.catchtable.frame = this->frames;
    cell->as.catchtable.parent = this->catchstack;
    cell->as.catchtable.type = type;
    cell->as.catchtable.address = address;
    this->catchstack = (CatchTable *)cell;
    return (CatchTable *)cell;
  }

  CatchTable* VM::pop_catchtable() {
    if (!this->catchstack) this->panic(kStatusCatchStackEmpty);
    CatchTable* parent = this->catchstack->parent;
    this->gc.free((VALUE)this->catchstack);
    this->catchstack = parent;
    return parent;
  }

  CatchTable* VM::find_catchtable(ThrowType type) {
    CatchTable* table = this->catchstack;

    while (table) {
      if (table->type == type) {
        break;
      }

      table = table->parent;
    }

    return table;
  }

  void VM::restore_catchtable(CatchTable* table) {

    // Unwind and immediately free all catchtables which were pushed
    // after the one we're restoring.
    while (table != this->catchstack) {
      this->pop_catchtable();
    }

    this->catchstack = table->parent;
    this->frames = table->frame;
    this->ip = table->address;

    // Unwind the stack to be the size it was when this catchtable
    // was pushed. Because the stack could be smaller, we need to
    // calculate the amount of values we can pop
    if (this->stack.size() <= table->stacksize) {
      return;
    }

    size_t popcount = this->stack.size() - table->stacksize;
    while (popcount--) {
      this->stack.pop_back();
    }
  }

  VALUE VM::create_object(uint32_t initial_capacity) {
    MemoryCell* cell = this->gc.allocate(this);
    cell->as.basic.set_type(kTypeObject);
    cell->as.object.klass = kNull;
    cell->as.object.container = new std::unordered_map<VALUE, VALUE>();
    cell->as.object.container->reserve(initial_capacity);
    return (VALUE)cell;
  }

  VALUE VM::create_array(uint32_t initial_capacity) {
    MemoryCell* cell = this->gc.allocate(this);
    cell->as.basic.set_type(kTypeArray);
    cell->as.array.data = new std::vector<VALUE>();
    cell->as.array.data->reserve(initial_capacity);
    return (VALUE)cell;
  }

  VALUE VM::create_integer(int64_t value) {
    return ((VALUE) value << 1) | kIntegerFlag;
  }

  VALUE VM::create_float(double value) {

    // Check if this float would fit into an immediate encoded integer
    if (ceilf(value) == value) return this->create_integer(value);

    union {
      double d;
      VALUE v;
    } t;

    t.d = value;

    int bits = (int)((VALUE)(t.v >> 60) & kPointerMask);

    // I have no idee what's going on here, this was taken from ruby source code
    if (t.v != 0x3000000000000000 && !((bits - 3) & ~0x01)) {
      return (BIT_ROTL(t.v, 3) & ~(VALUE)0x01) | kFloatFlag;
    } else if (t.v == 0) {

      // +0.0
      return 0x8000000000000002;
    }

    // Allocate from the GC
    MemoryCell* cell = this->gc.allocate(this);
    cell->as.basic.set_type(kTypeFloat);
    cell->as.flonum.float_value = value;
    return (VALUE)cell;
  }

  VALUE VM::create_function(VALUE name, uint32_t argc, bool anonymous, InstructionBlock* block) {
    MemoryCell* cell = this->gc.allocate(this);
    cell->as.basic.set_type(kTypeFunction);
    cell->as.function.name = name;
    cell->as.function.argc = argc;
    cell->as.function.context = this->frames;
    cell->as.function.block = block;
    cell->as.function.anonymous = anonymous;
    cell->as.function.bound_self_set = false;
    cell->as.function.bound_self = kNull;
    return (VALUE)cell;
  }

  VALUE VM::create_cfunction(VALUE name, uint32_t argc, void* pointer) {
    MemoryCell* cell = this->gc.allocate(this);
    cell->as.basic.set_type(kTypeCFunction);
    cell->as.cfunction.name = name;
    cell->as.cfunction.pointer = pointer;
    cell->as.cfunction.argc = argc;
    cell->as.cfunction.bound_self_set = false;
    cell->as.cfunction.bound_self = kNull;
    return (VALUE)cell;
  }

  int64_t VM::integer_value(VALUE value) {
    return ((SIGNED_VALUE)value) >> 1;
  }

  double VM::float_value(VALUE value) {
    if (!is_special(value)) return ((Float *)value)->float_value;

    union {
      double d;
      VALUE v;
    } t;

    VALUE b63 = (value >> 63);
    t.v = BIT_ROTR((kFloatFlag - b63) | (value & ~(VALUE)kFloatMask), 3);
    return t.d;
  }

  double VM::numeric_value(VALUE value) {
    if (this->real_type(value) == kTypeFloat) {
      return this->float_value(value);
    } else {
      return (double)this->integer_value(value);
    }
  }

  bool VM::boolean_value(VALUE value) {
    if (value == kFalse || value == kNull) return false;
    return true;
  }

  VALUE VM::real_type(VALUE value) {
    if (is_boolean(value)) return kTypeBoolean;
    if (is_null(value)) return kNull;
    if (!is_special(value)) return basics(value)->type();
    if (is_integer(value)) return kTypeInteger;
    if (is_ifloat(value)) return kTypeFloat;
    if (is_symbol(value)) return kTypeSymbol;
    return kTypeDead;
  }

  VALUE VM::type(VALUE value) {
    VALUE type = this->real_type(value);

    if (type == kTypeFloat) return kTypeNumeric;
    if (type == kTypeInteger) return kTypeNumeric;
    if (type == kTypeCFunction) return kTypeFunction;
    return type;
  }

  Opcode VM::fetch_instruction() {
    if (this->ip == nullptr) return Opcode::Nop;
    return *(Opcode *)this->ip;
  }

  uint32_t VM::decode_instruction_length(Opcode opcode) {
    switch (opcode) {
      case Opcode::Nop:                 return 1;
      case Opcode::ReadLocal:           return 1 + sizeof(uint32_t);
      case Opcode::ReadMemberSymbol:    return 1 + sizeof(VALUE);
      case Opcode::ReadMemberValue:     return 1;
      case Opcode::SetLocal:            return 1 + sizeof(uint32_t);
      case Opcode::SetMemberSymbol:     return 1 + sizeof(VALUE);
      case Opcode::SetMemberValue:      return 1;
      case Opcode::PutSelf:             return 1;
      case Opcode::PutValue:            return 1 + sizeof(VALUE);
      case Opcode::PutFloat:            return 1 + sizeof(double);
      case Opcode::PutString:           return 1 + sizeof(char*) + sizeof(uint32_t) * 2;
      case Opcode::PutFunction:         return 1 + sizeof(VALUE) + sizeof(void*) + sizeof(bool) + sizeof(uint32_t);
      case Opcode::PutCFunction:        return 1 + sizeof(VALUE) + sizeof(void *) + sizeof(uint32_t);
      case Opcode::PutArray:            return 1 + sizeof(uint32_t);
      case Opcode::PutHash:             return 1 + sizeof(uint32_t);
      case Opcode::PutClass:            return 1 + sizeof(VALUE) + sizeof(uint32_t) * 5;
      case Opcode::MakeConstant:        return 1 + sizeof(uint32_t);
      case Opcode::Pop:                 return 1 + sizeof(uint32_t);
      case Opcode::Dup:                 return 1;
      case Opcode::Swap:                return 1;
      case Opcode::Topn:                return 1 + sizeof(uint32_t);
      case Opcode::Setn:                return 1 + sizeof(uint32_t);
      case Opcode::Call:                return 1 + sizeof(uint32_t);
      case Opcode::CallMember:          return 1 + sizeof(uint32_t);
      case Opcode::Return:              return 1;
      case Opcode::Throw:               return 1 + sizeof(ThrowType);
      case Opcode::RegisterCatchTable:  return 1 + sizeof(ThrowType) + sizeof(int32_t);
      case Opcode::PopCatchTable:       return 1;
      case Opcode::Branch:              return 1 + sizeof(uint32_t);
      case Opcode::BranchIf:            return 1 + sizeof(uint32_t);
      case Opcode::BranchUnless:        return 1 + sizeof(uint32_t);
      default:                          return 1;
    }
  }

  void VM::op_readlocal(uint32_t index, uint32_t level) {

    // Travel to the correct frame
    Frame* frame = this->frames;
    while (level--) {
      if (!frame->parent) {
        return this->panic(kStatusReadFailedTooDeep);
      }

      frame = frame->parent;
    }

    // Check if the index isn't out-of-bounds
    if (index >= frame->environment->size()) {
      return this->panic(kStatusReadFailedOutOfBounds);
    }

    this->push_stack((* frame->environment)[index].value);
  }

  void VM::op_readmembersymbol(VALUE symbol) {
    VALUE target = this->pop_stack();

    // Handle datatypes that have their own data members
    switch (this->type(target)) {
      case kTypeObject: {
        Object* obj = (Object *)target;

        if (obj->container->count(symbol) == 1) {
          this->push_stack((*obj->container)[symbol]);
          return;
        }

        break;
      }
    }

    // Travel up the class chain and search for the right field
    // TODO: Implement this lol
    this->push_stack(kNull);
  }

  void VM::op_setlocal(uint32_t index, uint32_t level) {
    VALUE value = this->pop_stack();

    // Travel to the correct frame
    Frame* frame = this->frames;
    while (level--) {
      if (!frame->parent) {
        return this->panic(kStatusWriteFailedTooDeep);
      }

      frame = frame->parent;
    }

    // Check if the index isn't out-of-bounds
    if (index < frame->environment->size()) {
      FrameEnvironmentEntry& entry = (* frame->environment)[index];
      if (entry.is_constant) {
        this->panic(kStatusFieldIsConstant);
      } else {
        entry.value = value;
      }
    } else {
      this->panic(kStatusWriteFailedOutOfBounds);
    }
  }

  void VM::op_setmembersymbol(VALUE symbol) {
    VALUE value = this->pop_stack();
    VALUE target = this->pop_stack();

    // Check if we can write to the value
    switch (this->type(target)) {
      case kTypeObject: {
        Object* obj = (Object *)target;
        (*obj->container)[symbol] = value;
        break;
      }

      default: {
        this->panic(kStatusUnspecifiedError);
      }
    }
  }

  void VM::op_putself() {
    if (this->frames == nullptr) {
      this->push_stack(kNull);
      return;
    }

    this->push_stack(this->frames->self);
  }

  void VM::op_putvalue(VALUE value) {
    this->push_stack(value);
  }

  void VM::op_putfloat(double value) {
    this->push_stack(this->create_float(value));
  }

  void VM::op_putfunction(VALUE symbol, InstructionBlock* block, bool anonymous, uint32_t argc) {
    VALUE function = this->create_function(symbol, argc, anonymous, block);
    this->push_stack(function);
  }

  void VM::op_putcfunction(VALUE symbol, void* pointer, uint32_t argc) {
    VALUE function = this->create_cfunction(symbol, argc, pointer);
    this->push_stack(function);
  }

  void VM::op_putarray(uint32_t count) {
    Array* array = (Array *)this->create_array(count);

    while (count--) {
      array->data->insert(array->data->begin(), this->pop_stack());
    }

    this->push_stack((VALUE)array);
  }

  void VM::op_puthash(uint32_t count) {
    Object* object = (Object *)this->create_object(count);

    VALUE key;
    VALUE value;
    while (count--) {
      key = this->pop_stack();
      value = this->pop_stack();
      (*object->container)[key] = value;
    }

    this->push_stack((VALUE)object);
  }

  void VM::op_makeconstant(uint32_t offset) {
    if (offset < this->frames->environment->size()) {
      (* this->frames->environment)[offset].is_constant = true;
    }
  }

  void VM::op_pop(uint32_t count) {
    while (count--) {
      this->pop_stack();
    }
  }

  void VM::op_dup() {
    VALUE value = this->pop_stack();
    this->push_stack(value);
    this->push_stack(value);
  }

  void VM::op_swap() {
    VALUE value1 = this->pop_stack();
    VALUE value2 = this->pop_stack();
    this->push_stack(value1);
    this->push_stack(value2);
  }

  void VM::op_call(uint32_t argc) {
    this->call(argc, false);
  }

  void VM::op_callmember(uint32_t argc) {
    this->call(argc, true);
  }

  void VM::call(uint32_t argc, bool with_target) {

    // Check if there are enough items on the stack
    //
    // +- VALUE of the function
    // |
    // |   +- Amount of VALUEs that are on the
    // |   |  stack which are arguments
    // v   v
    // 1 + argc
    if (this->stack.size() < (1 + argc)) this->panic(kStatusStackEmpty);

    // Allocate enough space to copy the arguments into a temporary buffer
    // We need to keep them around until we have access to the new frames environment
    VALUE* arguments = nullptr;
    if (argc > 0) {
      arguments = (VALUE*)alloca(argc * sizeof(VALUE));
    }

    uint32_t argc_backup = argc;
    while (argc--) {
      *(arguments + argc) = this->pop_stack();
    }
    argc = argc_backup;

    // Pop the function off of the stack
    VALUE function = this->pop_stack();

    // The target of the function is either supplied explicitly via the call_member instruction
    // or implicitly via the functions frame
    // If it's supplied via the frame hierarchy, we need to resolve it here
    // If not we simply pop it off the stack
    VALUE target = kNull;
    if (with_target) target = this->pop_stack();

    // Redirect to the correct handler
    switch (this->real_type(function)) {

      // Normal functions as defined via the user
      case kTypeFunction: {
        Function* tfunc = (Function *)function;

        // Where to source the self value from
        //
        // bound_self_set   anonymous   with_target  has_context   self
        // |                |           |            |             |
        // true             -           -            -             bound_self
        //
        // false            true        -            true          from context
        // false            true        -            false         Null
        //
        // false            false       true         -             target
        // false            -           false        true          from context
        // false            false       false        false         Null
        if (tfunc->bound_self_set) {
          target = tfunc->bound_self;
        } else {
          if (tfunc->anonymous) {
            if (tfunc->context) {
              target = tfunc->context->self;
            } else {
              target = kNull;
            }
          } else {
            if (with_target) {
              // do nothing as target already contains the one supplied
              // via the stack
            } else {
              if (tfunc->context) {
                target = tfunc->context->self;
              } else {
                target = kNull;
              }
            }
          }
        }

        return this->call_function(tfunc, argc, arguments, target);
      }

      // Functions which wrap around a C function pointer
      case kTypeCFunction: {
        return this->call_cfunction((CFunction *)function, argc, arguments);
      }

      default: {
        // TODO: Handle as runtime error
        this->panic(kStatusUnspecifiedError);
      }
    }
  }

  void VM::call_function(Function* function, uint32_t argc, VALUE* argv, VALUE self) {

    // Check if the function was called with enough arguments
    if (argc < function->argc) {
      this->panic(kStatusNotEnoughArguments);
    }

    // The return address is simply the instruction after the one we've been called from
    // If the ip is nullptr (non-existent instructions that are run at the beginning of the VM) we don't
    // compute a return address
    uint8_t* return_address = nullptr;
    if (this->ip != nullptr) return_address = this->ip + this->decode_instruction_length(Opcode::Call);
    Frame* frame = this->push_frame(self, function, return_address);

    // Copy the arguments from the temporary arguments buffer into
    // the frames environment
    //
    // The environment will contain enough space for argcount + function.lvar_count
    // values so there's no need to allocate new space in it
    //
    // TODO: Copy the *arguments* field into the function (this is an array containing all arguments)
    uint32_t arg_copy_count = function->argc;
    while (arg_copy_count--) {
      (* frame->environment)[arg_copy_count].value = argv[arg_copy_count];
    }

    this->ip = function->block->data;
  }

  void VM::call_cfunction(CFunction* function, uint32_t argc, VALUE* argv) {

    // Check if enough arguments have been passed
    if (argc < function->argc) {
      this->panic(kStatusNotEnoughArguments);
    }

    VALUE rv = kNull;

    switch (argc) {

      // TODO: Also pass the *arguments* field (this is an array containing all arguments)
      case 0: rv = ((VALUE (*)(VM*))function->pointer)(this); break;
      case 1: rv = ((VALUE (*)(VM*, VALUE))function->pointer)(this, argv[0]); break;
      case 2: rv = ((VALUE (*)(VM*, VALUE, VALUE))function->pointer)(this, argv[0], argv[1]); break;
      case 3: rv = ((VALUE (*)(VM*, VALUE, VALUE, VALUE))function->pointer)(this, argv[0], argv[1], argv[2]); break;

      // TODO: Expand to 15??
      default: {
        this->panic(kStatusTooManyArgumentsForCFunction);
      }
    }

    this->push_stack(rv);
  }

  void VM::op_return() {
    Frame* frame = this->frames;
    if (!frame) this->panic(kStatusCantReturnFromTopLevel);
    this->frames = frame->parent;
    this->ip = frame->return_address;
  }

  void VM::op_throw(ThrowType type) {
    if (type == ThrowType::Exception) {
      return this->throw_exception(this->pop_stack());
    }

    CatchTable* table = this->find_catchtable(type);
    if (!table) {
      this->panic(kStatusNoSuitableCatchTableFound);
    }

    this->restore_catchtable(table);
  }

  void VM::throw_exception(VALUE payload) {
    CatchTable* table = this->find_catchtable(ThrowType::Exception);

    if (!table) {
      this->panic(kStatusNoSuitableCatchTableFound);
    }

    this->restore_catchtable(table);
    this->push_stack(payload);
    this->gc.collect(this);
  }

  void VM::op_registercatchtable(ThrowType type, int32_t offset) {
    this->push_catchtable(type, this->ip + offset);
  }

  void VM::op_popcatchtable() {
    this->pop_catchtable();
  }

  void VM::op_branch(int32_t offset) {
    this->ip += offset;
  }

  void VM::op_branchif(int32_t offset) {
    VALUE test = this->pop_stack();
    if (Operators::truthyness(test)) this->ip += offset;
  }

  void VM::op_branchunless(int32_t offset) {
    VALUE test = this->pop_stack();
    if (!Operators::truthyness(test)) this->ip += offset;
  }

  void VM::stacktrace(std::ostream& io) {
    Frame* frame = this->frames;

    int i = 0;
    io << "IP: " << (void*)this->ip << std::endl;
    while (frame) {
      io << i++ << "# ";
      io << "<Frame:" << frame << " ";
      io << "name="; this->pretty_print(io, frame->function->name); io << " ";
      io << "return_address=" << (void *)frame->return_address;
      io << ">";
      io << std::endl;
      frame = frame->parent;
    }
  }

  void VM::catchstacktrace(std::ostream& io) {
    CatchTable* table = this->catchstack;

    int i = 0;
    while (table) {
      io << i++ << "# ";
      this->pretty_print(io, table);
      io << std::endl;
      table = table->parent;
    }
  }

  void VM::stackdump(std::ostream& io) {
    for (size_t i = 0; i < this->stack.size(); i++) {
      VALUE entry = this->stack[i];
      this->pretty_print(io, entry);
      io << std::endl;
    }
  }

  void VM::pretty_print(std::ostream& io, VALUE value) {

    // Check if this value was already printed before
    auto begin = this->pretty_print_stack.begin();
    auto end = this->pretty_print_stack.end();
    bool printed_before = std::find(begin, end, value) != end;

    this->pretty_print_stack.push_back(value);

    switch (this->real_type(value)) {

      case kTypeDead: {
        io << "<!!DEAD_CELL!!: " << (void *)value << ">";
        break;
      }

      case kTypeInteger: {
        io << this->integer_value(value);
        break;
      }

      case kTypeFloat: {
        io << this->float_value(value);
        break;
      }

      case kTypeBoolean: {
        io << (value == kTrue ? "true" : "false");
        break;
      }

      case kTypeNull: {
        io << "null";
        break;
      }

      case kTypeObject: {
        Object* object = (Object *)value;
        io << "<Object:" << object;

        // If this object was already printed, we avoid printing it again
        if (printed_before) {
          io << " ...>";
          break;
        }

        for (auto& entry : *object->container) {
          io << " ";
          std::string key = this->symbol_table(entry.first);
          io << key << "="; this->pretty_print(io, entry.second);
        }

        io << ">";
        break;
      }

      case kTypeArray: {
        Array* array = (Array *)value;
        io << "<Array:" << array << " ";

        // If this array was already printed, we avoid printing it again
        if (printed_before) {
          io << "[...]>";
          break;
        }

        io << "[";

        bool first = true;
        for (auto& entry : *array->data) {
          if (!first) io << ", ";
          if (first) first = false;
          this->pretty_print(io, entry);
        }

        io << "]>";
        break;
      }

      case kTypeFunction: {
        Function* func = (Function *)value;

        if (printed_before) {
          io << "<Function:" << func << " ...>";
          break;
        }

        io << "<Function:" << func << " ";
        io << "name="; this->pretty_print(io, func->name); io << " ";
        io << "argc=" << func->argc; io << " ";
        io << "context=" << func->context << " ";
        io << "block="; this->pretty_print(io, func->block); io << " ";
        io << "bound_self_set=" << (func->bound_self_set ? "true" : "false") << " ";
        io << "bound_self="; this->pretty_print(io, func->bound_self);
        io << ">";
        break;
      }

      case kTypeCFunction: {
        CFunction* func = (CFunction *)value;

        if (printed_before) {
          io << "<CFunction:" << func << " ...>";
          break;
        }

        io << "<CFunction:" << func << " ";
        io << "name="; this->pretty_print(io, func->name); io << " ";
        io << "argc=" << func->argc; io << " ";
        io << "pointer=" << func->pointer << " ";
        io << "bound_self_set=" << (func->bound_self_set ? "true" : "false") << " ";
        io << "bound_self="; this->pretty_print(io, func->bound_self);
        io << ">";
        break;
      }

      case kTypeSymbol: {
        io << this->symbol_table(value);
        break;
      }

      case kTypeFrame: {
        Frame* frame = (Frame *)value;

        if (printed_before) {
          io << "<Frame:" << frame << " ...>";
          break;
        }

        io << "<Frame:" << frame << " ";
        io << "parent=" << frame->parent << " ";
        io << "parent_environment_frame=" << frame->parent_environment_frame << " ";
        io << "function="; this->pretty_print(io, frame->function); io << " ";
        io << "self="; this->pretty_print(io, frame->self); io << " ";
        io << "return_address=" << (void *)frame->return_address;
        io << ">";
        break;
      }

      case kTypeCatchTable: {
        CatchTable* table = (CatchTable *)value;
        io << "<CatchTable:" << table << " ";
        io << "type=" << table->type << " ";
        io << "address=" << (void *)table->address << " ";
        io << "stacksize=" << table->stacksize << " ";
        io << "frame=" << table->frame << " ";
        io << "parent=" << table->parent << " ";
        io << ">";
        break;
      }

      case kTypeInstructionBlock: {
        InstructionBlock* block = (InstructionBlock *)value;
        io << "<InstructionBlock:" << block << " ";
        io << "lvarcount=" << block->lvarcount << " ";
        io << "data=" << (void *)block->data << " ";
        io << "data_size=" << block->data_size << " ";
        io << "write_offset=" << block->write_offset;
        io << ">";
        break;
      }

    }

    this->pretty_print_stack.pop_back();
  }

  void VM::init_frames() {

    // On startup, we create a stack frame that serves as our global scope
    // This is the scope in which global values such as `Charly` live.
    //
    // When a file is being included, either as requested by the machine,
    // or via the user, a new stack frame is created for it, effectively
    // separating it from the global scope
    //
    // This way it can still access the global scope, but not register any
    // new variables or constants into it.
    uint32_t global_var_count = 1;
    InstructionBlock& block = this->request_instruction_block(global_var_count);

    // let Charly = {
    //   internals = {
    //     get_method = (CFunction *)Internals::get_method
    //   }
    // };
    block.write_putcfunction(this->symbol_table("get_method"), (void *)Internals::get_method, 1);
    block.write_putvalue(this->symbol_table("get_method"));
    block.write_puthash(1);
    block.write_putvalue(this->symbol_table("internals"));
    block.write_puthash(1);
    block.write_setlocal(0, 0);

    // [[{}, { boye = 250 }], 25, 25, 25, 25]
    block.write_puthash(0);
    block.write_puthash(0);
    block.write_dup();
    block.write_putvalue(this->create_integer(250));
    block.write_setmembersymbol(this->symbol_table("boye"));
    block.write_putarray(2);
    block.write_putfloat(25);
    block.write_putfloat(25);
    block.write_putfloat(25);
    block.write_putfloat(25);
    block.write_putarray(5);

    // Charly.internals.get_method(:"hello_world")
    block.write_readlocal(0, 0);
    block.write_readmembersymbol(this->symbol_table("internals"));
    block.write_readmembersymbol(this->symbol_table("get_method"));
    block.write_putvalue(this->symbol_table("hello world"));
    block.write_call(1);

    block.write_byte(Opcode::Halt);

    // Push a function onto the stack containing this block and call it
    this->op_putfunction(this->symbol_table("__charly_init"), &block, false, 0);
    this->op_call(0);
    this->frames->self = kNull;
  }

  void VM::run() {

    // Execute instructions as long as we have a valid ip
    // and the machine wasn't halted
    while (this->ip && !this->halted) {

      // Backup the current ip
      // After the instruction was executed we check if
      // ip stayed the same. If it's still the same value
      // we increment it to the next instruction (via decode_instruction_length)
      uint8_t* old_ip = this->ip;

      // Check if we're out-of-bounds relative to the current block
      uint8_t* block_data = this->frames->function->block->data;
      uint32_t block_write_offset = this->frames->function->block->write_offset;
      if (this->ip + sizeof(Opcode) - 1 >= block_data + block_write_offset) {
        this->panic(kStatusIpOutOfBounds);
      }

      // Retrieve the current opcode
      Opcode opcode = *(Opcode *)this->ip;

      // Check if there is enough space for instruction arguments
      uint32_t instruction_length = this->decode_instruction_length(opcode);
      if (this->ip + instruction_length >= (block_data + block_write_offset + sizeof(Opcode))) {
        this->panic(kStatusNotEnoughSpaceForInstructionArguments);
      }

      // Redirect to specific instruction handler
      switch (opcode) {
        case Opcode::Nop: {
          break;
        }

        case Opcode::ReadLocal: {
          uint32_t index = *(uint32_t *)(this->ip + sizeof(Opcode));
          uint32_t level = *(uint32_t *)(this->ip + sizeof(Opcode) + sizeof(uint32_t));
          this->op_readlocal(index, level);
          break;
        }

        case Opcode::ReadMemberSymbol: {
          VALUE symbol = *(VALUE *)(this->ip + sizeof(Opcode));
          this->op_readmembersymbol(symbol);
          break;
        }

        case Opcode::SetLocal: {
          uint32_t index = *(uint32_t *)(this->ip + sizeof(Opcode));
          uint32_t level = *(uint32_t *)(this->ip + sizeof(Opcode) + sizeof(uint32_t));
          this->op_setlocal(index, level);
          break;
        }

        case Opcode::SetMemberSymbol: {
          VALUE symbol = *(VALUE *)(this->ip + sizeof(Opcode));
          this->op_setmembersymbol(symbol);
          break;
        }

        case Opcode::PutSelf: {
          this->op_putself();
          break;
        }

        case Opcode::PutValue: {
          VALUE value = *(VALUE *)(this->ip + sizeof(Opcode));
          this->op_putvalue(value);
          break;
        }

        case Opcode::PutFloat: {
          double value = *(double *)(this->ip + sizeof(Opcode));
          this->op_putfloat(value);
          break;
        }

        case Opcode::PutFunction: {
          auto symbol = *(VALUE *)(this->ip + sizeof(Opcode));
          auto block = *(InstructionBlock **)(this->ip + sizeof(Opcode) + sizeof(VALUE));
          auto anonymous = *(bool *)(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(VALUE));
          auto argc = *(uint32_t *)(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(VALUE) + sizeof(bool));
          this->op_putfunction(symbol, block, anonymous, argc);
          break;
        }

        case Opcode::PutCFunction: {
          auto symbol = *(VALUE *)(this->ip + sizeof(Opcode));
          auto pointer = *(void **)(this->ip + sizeof(Opcode) + sizeof(VALUE));
          auto argc = *(uint32_t *)(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(void *));
          this->op_putcfunction(symbol, pointer, argc);
          break;
        }

        case Opcode::PutArray: {
          uint32_t count = *(uint32_t *)(this->ip + sizeof(Opcode));
          this->op_putarray(count);
          break;
        }

        case Opcode::PutHash: {
          uint32_t size = *(uint32_t *)(this->ip + sizeof(Opcode));
          this->op_puthash(size);
          break;
        }

        case Opcode::MakeConstant: {
          uint32_t offset = *(uint32_t *)(this->ip + sizeof(Opcode));
          this->op_makeconstant(offset);
          break;
        }

        case Opcode::Pop: {
          uint32_t count = *(uint32_t *)(this->ip + sizeof(Opcode));
          this->op_pop(count);
          break;
        }

        case Opcode::Dup: {
          this->op_dup();
        }

        case Opcode::Swap: {
          this->op_swap();
          break;
        }

        case Opcode::Call: {
          uint32_t argc = *(uint32_t *)(this->ip + sizeof(Opcode));
          this->op_call(argc);
          break;
        }

        case Opcode::CallMember: {
          uint32_t argc = *(uint32_t *)(this->ip + sizeof(Opcode));
          this->op_callmember(argc);
          break;
        }

        case Opcode::Return: {
          this->op_return();
          break;
        }

        case Opcode::Throw: {
          ThrowType throw_type = *(ThrowType *)(this->ip + sizeof(Opcode));
          this->op_throw(throw_type);
          break;
        }

        case Opcode::RegisterCatchTable: {
          ThrowType throw_type = *(ThrowType *)(this->ip + sizeof(Opcode));
          int32_t offset = *(int32_t *)(this->ip + sizeof(Opcode) + sizeof(ThrowType));
          this->op_registercatchtable(throw_type, offset);
          break;
        }

        case Opcode::PopCatchTable: {
          this->op_popcatchtable();
          break;
        }

        case Opcode::Branch: {
          int32_t offset = *(int32_t *)(this->ip + sizeof(Opcode));
          this->op_branch(offset);
          break;
        }

        case Opcode::BranchIf: {
          int32_t offset = *(int32_t *)(this->ip + sizeof(Opcode));
          this->op_branchif(offset);
          break;
        }

        case Opcode::BranchUnless: {
          int32_t offset = *(int32_t *)(this->ip + sizeof(Opcode));
          this->op_branchunless(offset);
          break;
        }

        case Opcode::Add: {
          VALUE right = this->pop_stack();
          VALUE left = this->pop_stack();
          this->push_stack(Operators::add(this, left, right));
          break;
        }

        case Opcode::Halt: {
          this->halted = true;
          break;
        }

        default: {
          std::cout << "Opcode: " << (void *)opcode << std::endl;
          this->panic(kStatusUnknownOpcode);
        }
      }

      // Increment ip if the instruction didn't change it
      if (this->ip == old_ip) {
        this->ip += instruction_length;
      }
    }

    std::cout << "Stacktrace:" << std::endl;
    this->stacktrace(std::cout);

    std::cout << "CatchStacktrace:" << std::endl;
    this->catchstacktrace(std::cout);

    std::cout << "Stackdump:" << std::endl;
    this->stackdump(std::cout);

    this->gc.collect(this);
  }
}
