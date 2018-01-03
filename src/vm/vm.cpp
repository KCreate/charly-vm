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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>

#include "gc.h"
#include "operators.h"
#include "status.h"
#include "vm.h"

namespace Charly {

Frame* VM::pop_frame() {
  Frame* frame = this->frames;
  if (frame)
    this->frames = this->frames->parent;
  return frame;
}

Frame* VM::create_frame(VALUE self, Function* function, uint8_t* return_address) {
  MemoryCell* cell = this->context.gc.allocate();
  cell->basic.set_type(kTypeFrame);
  cell->frame.parent = this->frames;
  cell->frame.parent_environment_frame = function->context;
  cell->frame.last_active_catchtable = this->catchstack;
  cell->frame.function = function;
  cell->frame.self = self;

  cell->frame.return_address = return_address;

  // Calculate the number of local variables this frame has to support
  // Add 1 at the beginning to reserve space for the arguments magic variable
  uint32_t lvar_count = 1 + function->argc + function->lvarcount;

  // Allocate and prefill local variable space
  cell->frame.environment.reserve(lvar_count);
  while (lvar_count--)
    cell->frame.environment.push_back(kNull);

  // Append the frame
  this->frames = cell->as<Frame>();

  // Print the frame if the corresponding flag was set
  if (this->context.trace_frames) {
    this->context.out_stream << "Entering frame: ";
    this->pretty_print(this->context.out_stream, cell->value);
    this->context.out_stream << '\n';
  }

  return cell->as<Frame>();
}

std::optional<VALUE> VM::pop_stack() {
  std::optional<VALUE> value;
  if (this->stack.size() > 0) {
    value = this->stack.back();
    this->stack.pop_back();
  }

  return value;
}

void VM::push_stack(VALUE value) {
  this->stack.push_back(value);
}

CatchTable* VM::create_catchtable(uint8_t* address) {
  MemoryCell* cell = this->context.gc.allocate();
  cell->basic.set_type(kTypeCatchTable);
  cell->catchtable.stacksize = this->stack.size();
  cell->catchtable.frame = this->frames;
  cell->catchtable.parent = this->catchstack;
  cell->catchtable.address = address;
  this->catchstack = cell->as<CatchTable>();

  // Print the catchtable if the corresponding flag was set
  if (this->context.trace_catchtables) {
    this->context.out_stream << "Entering catchtable: ";
    this->pretty_print(this->context.out_stream, cell->value);
    this->context.out_stream << '\n';
  }

  return cell->as<CatchTable>();
}

CatchTable* VM::pop_catchtable() {
  if (!this->catchstack)
    this->panic(Status::CatchStackEmpty);
  CatchTable* parent = this->catchstack->parent;
  this->catchstack = parent;
  return parent;
}

void VM::restore_catchtable(CatchTable* table) {
  // Unwind and immediately free all catchtables which were pushed
  // after the one we're restoring.
  while (table != this->catchstack) {
    this->pop_catchtable();
  }

  this->frames = table->frame;
  this->ip = table->address;

  // Show the catchtable we just restored if the corresponding flag was set
  if (this->context.trace_catchtables) {
    // Show the table we've restored
    this->context.out_stream << "Restored CatchTable: ";
    this->pretty_print(this->context.out_stream, table);
    this->context.out_stream << '\n';
  }

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
  MemoryCell* cell = this->context.gc.allocate();
  cell->basic.set_type(kTypeObject);
  cell->object.klass = kNull;
  cell->object.container = new std::unordered_map<VALUE, VALUE>();
  cell->object.container->reserve(initial_capacity);
  return cell->value;
}

VALUE VM::create_array(uint32_t initial_capacity) {
  MemoryCell* cell = this->context.gc.allocate();
  cell->basic.set_type(kTypeArray);
  cell->array.data = new std::vector<VALUE>();
  cell->array.data->reserve(initial_capacity);
  return cell->value;
}

VALUE VM::create_float(double value) {
  // Check if this float would fit into an immediate encoded integer
  if (ceilf(value) == value)
    return VALUE_ENCODE_INTEGER(value);

  union {
    double d;
    VALUE v;
  } t;

  t.d = value;

  int bits = static_cast<int>(static_cast<VALUE>(t.v >> 60) & kPointerMask);

  // I have no idee what's going on here, this was taken from ruby source code
  if (t.v != 0x3000000000000000 && !((bits - 3) & ~0x01)) {
    return (BIT_ROTL(t.v, 3) & ~static_cast<VALUE>(0x01)) | kFloatFlag;
  } else if (t.v == 0) {
    // +0.0
    return 0x8000000000000002;
  }

  // Allocate from the GC
  MemoryCell* cell = this->context.gc.allocate();
  cell->basic.set_type(kTypeFloat);
  cell->flonum.float_value = value;
  return cell->value;
}

VALUE VM::create_string(const char* data, uint32_t length) {
  // Calculate the initial capacity of the string
  size_t string_capacity = kStringInitialCapacity;
  while (string_capacity <= length)
    string_capacity *= kStringCapacityGrowthFactor;

  // Create a copy of the data
  char* copied_string = static_cast<char*>(malloc(string_capacity));
  memcpy(copied_string, data, length);

  // Allocate the memory cell and initialize the values
  MemoryCell* cell = this->context.gc.allocate();
  cell->basic.set_type(kTypeString);
  cell->string.data = copied_string;
  cell->string.length = length;
  cell->string.capacity = string_capacity;
  return cell->value;
}

VALUE VM::create_function(VALUE name, uint8_t* body_address, uint32_t argc, uint32_t lvarcount, bool anonymous) {
  MemoryCell* cell = this->context.gc.allocate();
  cell->basic.set_type(kTypeFunction);
  cell->function.name = name;
  cell->function.argc = argc;
  cell->function.lvarcount = lvarcount;
  cell->function.context = this->frames;
  cell->function.body_address = body_address;
  cell->function.anonymous = anonymous;
  cell->function.bound_self_set = false;
  cell->function.bound_self = kNull;
  cell->function.container = new std::unordered_map<VALUE, VALUE>();
  return cell->value;
}

VALUE VM::create_cfunction(VALUE name, uint32_t argc, uintptr_t pointer) {
  MemoryCell* cell = this->context.gc.allocate();
  cell->basic.set_type(kTypeCFunction);
  cell->cfunction.name = name;
  cell->cfunction.pointer = pointer;
  cell->cfunction.argc = argc;
  cell->cfunction.bound_self_set = false;
  cell->cfunction.bound_self = kNull;
  cell->cfunction.container = new std::unordered_map<VALUE, VALUE>();
  return cell->value;
}

double VM::cast_to_double(VALUE value) {
  VALUE type = this->real_type(value);

  switch (type) {
    case kTypeInteger: return static_cast<double>(VALUE_DECODE_INTEGER(value));
    case kTypeFloat: return this->float_value(value);
    case kTypeString: {
      String* string = (String*)value;
      char* end_it = string->data + string->capacity;
      double interpreted = strtod(string->data, &end_it);

      // HUGE_VAL gets returned when the converted value
      // doesn't fit inside a double value
      // In this case we just return NAN
      if (interpreted == HUGE_VAL)
        return this->create_float(NAN);

      // If strtod could not perform a conversion it returns 0
      // and sets *str_end to str
      if (end_it == string->data)
        return this->create_float(NAN);

      // The value could be converted
      return interpreted;
    }
    default: return this->create_float(NAN);
  }
}

double VM::float_value(VALUE value) {
  if (!is_special(value))
    return reinterpret_cast<Float*>(value)->float_value;

  union {
    double d;
    VALUE v;
  } t;

  VALUE b63 = (value >> 63);
  t.v = BIT_ROTR((kFloatFlag - b63) | (value & ~static_cast<VALUE>(kFloatMask)), 3);
  return t.d;
}

double VM::numeric_value(VALUE value) {
  if (this->real_type(value) == kTypeFloat) {
    return this->float_value(value);
  } else {
    return static_cast<double>(VALUE_DECODE_INTEGER(value));
  }
}

bool VM::boolean_value(VALUE value) {
  if (value == kFalse || value == kNull)
    return false;
  return true;
}

VALUE VM::real_type(VALUE value) {
  // Because a False value or null can't be distinguished
  // from a pointer value, we check for them before we check
  // for a pointer value
  if (is_boolean(value))
    return kTypeBoolean;
  if (is_null(value))
    return kTypeNull;
  if (!is_special(value))
    return basics(value)->type();
  if (is_integer(value))
    return kTypeInteger;
  if (is_ifloat(value))
    return kTypeFloat;
  if (is_symbol(value))
    return kTypeSymbol;
  return kTypeDead;
}

VALUE VM::type(VALUE value) {
  VALUE type = this->real_type(value);

  if (type == kTypeFloat)
    return kTypeNumeric;
  if (type == kTypeInteger)
    return kTypeNumeric;
  if (type == kTypeCFunction)
    return kTypeFunction;
  return type;
}

Opcode VM::fetch_instruction() {
  if (this->ip == nullptr)
    return Opcode::Nop;
  return *reinterpret_cast<Opcode*>(this->ip);
}

void VM::op_readlocal(uint32_t index, uint32_t level) {
  // Travel to the correct frame
  Frame* frame = this->frames;
  while (level--) {
    if (!frame->parent) {
      return this->panic(Status::ReadFailedTooDeep);
    }

    frame = frame->parent;
  }

  // Check if the index isn't out-of-bounds
  if (index >= frame->environment.size()) {
    return this->panic(Status::ReadFailedOutOfBounds);
  }

  this->push_stack(frame->environment[index]);
}

void VM::op_readmembersymbol(VALUE symbol) {
  VALUE target = this->pop_stack().value_or(kNull);

  // Handle datatypes that have their own data members
  switch (this->type(target)) {
    case kTypeObject: {
      Object* obj = reinterpret_cast<Object*>(target);

      if (obj->container->count(symbol) == 1) {
        this->push_stack((*obj->container)[symbol]);
        return;
      }

      break;
    }

    case kTypeFunction: {
      Function* func = reinterpret_cast<Function*>(target);

      if (func->container->count(symbol) == 1) {
        this->push_stack((*func->container)[symbol]);
        return;
      }

      break;
    }

    case kTypeCFunction: {
      CFunction* cfunc = reinterpret_cast<CFunction*>(target);

      if (cfunc->container->count(symbol) == 1) {
        this->push_stack((*cfunc->container)[symbol]);
        return;
      }

      break;
    }
  }

  // Travel up the class chain and search for the right field
  // TODO: Implement this lol
  this->push_stack(kNull);
}

void VM::op_readarrayindex(uint32_t index) {
  VALUE stackval = this->pop_stack().value_or(kNull);

  // Check if we popped an array, if not push it back onto the stack
  if (this->real_type(stackval) != kTypeArray) {
    this->push_stack(stackval);
    return;
  }

  Array* arr = reinterpret_cast<Array*>(stackval);

  // Out-of-bounds checking
  if (index >= arr->data->size()) {
    this->push_stack(kNull);
  }

  this->push_stack((*arr->data)[index]);
}

void VM::op_setlocal(uint32_t index, uint32_t level) {
  VALUE value = this->pop_stack().value_or(kNull);

  // Travel to the correct frame
  Frame* frame = this->frames;
  while (level--) {
    if (!frame->parent) {
      return this->panic(Status::WriteFailedTooDeep);
    }

    frame = frame->parent;
  }

  // Check if the index isn't out-of-bounds
  if (index < frame->environment.size()) {
    frame->environment[index] = value;
  } else {
    this->panic(Status::WriteFailedOutOfBounds);
  }

  this->push_stack(value);
}

void VM::op_setmembersymbol(VALUE symbol) {
  VALUE value = this->pop_stack().value_or(kNull);
  VALUE target = this->pop_stack().value_or(kNull);

  // Check if we can write to the value
  switch (this->real_type(target)) {
    case kTypeObject: {
      Object* obj = reinterpret_cast<Object*>(target);
      (*obj->container)[symbol] = value;
      break;
    }

    case kTypeFunction: {
      Function* func = reinterpret_cast<Function*>(target);
      (*func->container)[symbol] = value;
      break;
    }

    case kTypeCFunction: {
      CFunction* cfunc = reinterpret_cast<CFunction*>(target);
      (*cfunc->container)[symbol] = value;
      break;
    }
  }

  this->push_stack(value);
}

void VM::op_setarrayindex(uint32_t index) {
  VALUE expression = this->pop_stack().value_or(kNull);
  VALUE stackval = this->pop_stack().value_or(kNull);

  // Check if we popped an array, if not push it back onto the stack
  if (this->real_type(stackval) != kTypeArray) {
    this->push_stack(stackval);
    return;
  }

  Array* arr = reinterpret_cast<Array*>(stackval);

  // Out-of-bounds checking
  if (index >= arr->data->size()) {
    this->push_stack(kNull);
  }

  (*arr->data)[index] = expression;

  this->push_stack(stackval);
}

void VM::op_putself(uint32_t level) {
  if (this->frames == nullptr) {
    this->push_stack(kNull);
    return;
  }

  VALUE self_val = kNull;

  Frame* frm = this->frames;
  while (frm && level--) {
    frm = frm->parent_environment_frame;
  }

  if (frm) {
    self_val = frm->self;
  }

  this->push_stack(self_val);
}

void VM::op_putvalue(VALUE value) {
  this->push_stack(value);
}

void VM::op_putfloat(double value) {
  this->push_stack(this->create_float(value));
}

void VM::op_putstring(char* data, uint32_t length) {
  this->push_stack(this->create_string(data, length));
}

void VM::op_putfunction(VALUE symbol, uint8_t* body_address, bool anonymous, uint32_t argc, uint32_t lvarcount) {
  VALUE function = this->create_function(symbol, body_address, argc, lvarcount, anonymous);
  this->push_stack(function);
}

void VM::op_putcfunction(VALUE symbol, uintptr_t pointer, uint32_t argc) {
  VALUE function = this->create_cfunction(symbol, argc, pointer);
  this->push_stack(function);
}

void VM::op_putarray(uint32_t count) {
  Array* array = reinterpret_cast<Array*>(this->create_array(count));

  while (count--) {
    array->data->insert(array->data->begin(), this->pop_stack().value_or(kNull));
  }

  this->push_stack(reinterpret_cast<VALUE>(array));
}

void VM::op_puthash(uint32_t count) {
  Object* object = reinterpret_cast<Object*>(this->create_object(count));

  VALUE key;
  VALUE value;
  while (count--) {
    key = this->pop_stack().value_or(kNull);
    value = this->pop_stack().value_or(kNull);
    (*object->container)[key] = value;
  }

  this->push_stack(reinterpret_cast<VALUE>(object));
}

void VM::op_pop() {
  this->pop_stack();
}

void VM::op_dup() {
  VALUE value = this->pop_stack().value_or(kNull);
  this->push_stack(value);
  this->push_stack(value);
}

void VM::op_swap() {
  VALUE value1 = this->pop_stack().value_or(kNull);
  VALUE value2 = this->pop_stack().value_or(kNull);
  this->push_stack(value1);
  this->push_stack(value2);
}

void VM::op_topn(uint32_t offset) {
  // Check if there are enough items on the stack
  if (this->stack.size() <= offset) {
    this->push_stack(kNull);
  } else {
    VALUE value = *(this->stack.end() - (offset + 1));
    this->push_stack(value);
  }
}

void VM::op_setn(uint32_t offset) {
  // Check if there are enough items on the stack
  if (this->stack.size() <= offset) {
    this->pop_stack();
  } else {
    VALUE& ref = *(this->stack.end() - (offset + 1));
    ref = this->stack.back();
  }
}

void VM::op_call(uint32_t argc) {
  this->call(argc, false);
}

void VM::op_callmember(uint32_t argc) {
  this->call(argc, true);
}

void VM::call(uint32_t argc, bool with_target) {
  // Stack allocate enough space to copy all arguments into
  VALUE arguments[argc];

  // Basically what this loop does is it counts argc_copy down to 0
  // We do this because arguments are constructed on the stack
  // in the reverse order than we are popping them off
  for (int argc_copy = argc - 1; argc_copy >= 0; argc_copy--) {
    VALUE value = this->pop_stack().value_or(kNull);
    arguments[argc_copy] = value;
  }

  // Pop the function off of the stack
  VALUE function = this->pop_stack().value_or(kNull);

  // The target of the function is either supplied explicitly via the call_member instruction
  // or implicitly via the functions frame
  // If it's supplied via the frame hierarchy, we need to resolve it here
  // If not we simply pop it off the stack
  VALUE target = kNull;
  if (with_target)
    target = this->pop_stack().value_or(kNull);

  // Redirect to the correct handler
  switch (this->real_type(function)) {
    // Normal functions as defined via the user
    case kTypeFunction: {
      Function* tfunc = reinterpret_cast<Function*>(function);

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

      this->call_function(tfunc, argc, arguments, target);
      return;
    }

    // Functions which wrap around a C function pointer
    case kTypeCFunction: {
      this->call_cfunction(reinterpret_cast<CFunction*>(function), argc, arguments);
      return;
    }

    default: {
      this->context.out_stream << "cant call a " << kValueTypeString[this->real_type(function)] << '\n';

      // TODO: Handle as runtime error
      this->panic(Status::UnspecifiedError);
    }
  }
}

void VM::call_function(Function* function, uint32_t argc, VALUE* argv, VALUE self) {
  // Check if the function was called with enough arguments
  if (argc < function->argc) {
    this->panic(Status::NotEnoughArguments);
  }

  // The return address is simply the instruction after the one we've been called from
  // If the ip is nullptr (non-existent instructions that are run at the beginning of the VM) we
  // don't compute a return address
  uint8_t* return_address = nullptr;
  if (this->ip != nullptr)
    return_address = this->ip + kInstructionLengths[Opcode::Call];
  Frame* frame = this->create_frame(self, function, return_address);

  Array* arguments_array = reinterpret_cast<Array*>(this->create_array(argc));

  // Copy the arguments from the temporary arguments buffer into
  // the frames environment
  //
  // We add 1 to the index as that's the index of the arguments array
  for (int i = 0; i < static_cast<int>(function->argc); i++) {
    frame->environment[i + 1] = argv[i];
    arguments_array->data->push_back(argv[i]);
  }

  // Insert the argument value and make it a constant
  // This is so that we can be sure that noone is going to overwrite it afterwards
  frame->environment[0] = reinterpret_cast<VALUE>(arguments_array);

  this->ip = function->body_address;
}

void VM::call_cfunction(CFunction* function, uint32_t argc, VALUE* argv) {
  // Check if enough arguments have been passed
  if (argc < function->argc) {
    this->panic(Status::NotEnoughArguments);
  }

  VALUE rv = kNull;

  // TODO: Expand this to 15 arguments
  switch (argc) {
    case 0: rv = reinterpret_cast<VALUE (*)(VM&)>(function->pointer)(*this); break;
    case 1: rv = reinterpret_cast<VALUE (*)(VM&, VALUE)>(function->pointer)(*this, argv[0]); break;
    case 2: rv = reinterpret_cast<VALUE (*)(VM&, VALUE, VALUE)>(function->pointer)(*this, argv[0], argv[1]); break;
    case 3:
      rv = reinterpret_cast<VALUE (*)(VM&, VALUE, VALUE, VALUE)>(function->pointer)(*this, argv[0], argv[1], argv[2]);
      break;
    default: { this->panic(Status::TooManyArgumentsForCFunction); }
  }

  this->push_stack(rv);
}

void VM::op_return() {
  Frame* frame = this->frames;
  if (!frame)
    this->panic(Status::CantReturnFromTopLevel);

  // Unwind and immediately free all catchtables which were pushed
  // after the one we're restoring.
  while (frame->last_active_catchtable != this->catchstack) {
    this->pop_catchtable();
  }

  this->frames = frame->parent;
  this->ip = frame->return_address;

  // Print the frame if the correponding flag was set
  if (this->context.trace_frames) {
    this->context.out_stream << "Left frame: ";
    this->pretty_print(this->context.out_stream, reinterpret_cast<VALUE>(frame));
    this->context.out_stream << '\n';
  }
}

void VM::op_throw() {
  return this->throw_exception(this->pop_stack().value_or(kNull));
}

void VM::throw_exception(VALUE payload) {
  CatchTable* table = this->catchstack;

  if (!table) {
    this->panic(Status::CatchStackEmpty);
  }

  this->restore_catchtable(table);
  this->push_stack(payload);
}

void VM::op_registercatchtable(int32_t offset) {
  this->create_catchtable(this->ip + offset);
}

void VM::op_popcatchtable() {
  this->pop_catchtable();

  // Show the catchtable we just restored if the corresponding flag was set
  if (this->context.trace_catchtables) {
    CatchTable* table = this->catchstack;

    if (table != nullptr) {
      // Show the table we've restored
      this->context.out_stream << "Restored CatchTable: ";
      this->pretty_print(this->context.out_stream, reinterpret_cast<VALUE>(table));
      this->context.out_stream << '\n';
    }
  }
}

void VM::op_branch(int32_t offset) {
  this->ip += offset;
}

void VM::op_branchif(int32_t offset) {
  VALUE test = this->pop_stack().value_or(kNull);
  if (Operators::truthyness(test))
    this->ip += offset;
}

void VM::op_branchunless(int32_t offset) {
  VALUE test = this->pop_stack().value_or(kNull);
  if (!Operators::truthyness(test))
    this->ip += offset;
}

void VM::op_typeof() {
  VALUE value = this->pop_stack().value_or(kNull);
  std::string& stringrep = kValueTypeString[this->real_type(value)];
  this->push_stack(this->create_string(stringrep.c_str(), stringrep.size()));
}

void VM::stacktrace(std::ostream& io) {
  Frame* frame = this->frames;

  int i = 0;
  io << "IP: " << static_cast<void*>(this->ip) << '\n';
  while (frame) {
    io << i++ << "# ";
    io << "<Frame:" << frame << " ";
    io << "name=";
    this->pretty_print(io, frame->function->name);
    io << " ";
    io << "return_address=" << static_cast<void*>(frame->return_address) << std::dec;
    io << ">";
    io << '\n';
    frame = frame->parent;
  }
}

void VM::catchstacktrace(std::ostream& io) {
  CatchTable* table = this->catchstack;

  int i = 0;
  while (table) {
    io << i++ << "# ";
    this->pretty_print(io, table);
    io << '\n';
    table = table->parent;
  }
}

void VM::stackdump(std::ostream& io) {
  for (VALUE& stackitem : this->stack) {
    this->pretty_print(io, stackitem);
    io << '\n';
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
      io << "<!!DEAD_CELL!!: " << reinterpret_cast<void*>(value) << ">";
      break;
    }

    case kTypeInteger: {
      io << VALUE_DECODE_INTEGER(value);
      break;
    }

    case kTypeFloat: {
      io << this->float_value(value);
      break;
    }

    case kTypeBoolean: {
      io << std::boolalpha << (value == kTrue) << std::noboolalpha;
      break;
    }

    case kTypeNull: {
      io << "null";
      break;
    }

    case kTypeString: {
      String* string = reinterpret_cast<String*>(value);
      io << "<String:" << string << " ";
      io << "\"";
      io.write(string->data, string->length);
      io << "\"";
      io << ">";
      break;
    }

    case kTypeObject: {
      Object* object = reinterpret_cast<Object*>(value);
      io << "<Object:" << object;

      // If this object was already printed, we avoid printing it again
      if (printed_before) {
        io << " ...>";
        break;
      }

      for (auto& entry : *object->container) {
        io << " ";
        std::string key = this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
        io << key << "=";
        this->pretty_print(io, entry.second);
      }

      io << ">";
      break;
    }

    case kTypeArray: {
      Array* array = reinterpret_cast<Array*>(value);
      io << "<Array:" << array << " ";

      // If this array was already printed, we avoid printing it again
      if (printed_before) {
        io << "[...]>";
        break;
      }

      io << "[";

      bool first = false;
      for (VALUE& entry : *array->data) {
        if (first) {
          io << ", ";
        } else {
          first = true;
        }

        this->pretty_print(io, entry);
      }

      io << "]>";
      break;
    }

    case kTypeFunction: {
      Function* func = reinterpret_cast<Function*>(value);

      if (printed_before) {
        io << "<Function:" << func << " ...>";
        break;
      }

      io << "<Function:" << func << " ";
      io << "name=";
      this->pretty_print(io, func->name);
      io << " ";
      io << "argc=" << func->argc;
      io << " ";
      io << "lvarcount=" << func->lvarcount;
      io << " ";
      io << "context=" << func->context << " ";
      io << "body_address=" << reinterpret_cast<void*>(func->body_address) << " ";
      io << "bound_self_set=" << (func->bound_self_set ? "true" : "false") << " ";
      io << "bound_self=";
      this->pretty_print(io, func->bound_self);

      for (auto& entry : *func->container) {
        io << " ";
        std::string key = this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
        io << key << "=";
        this->pretty_print(io, entry.second);
      }

      io << ">";
      break;
    }

    case kTypeCFunction: {
      CFunction* func = reinterpret_cast<CFunction*>(value);

      if (printed_before) {
        io << "<CFunction:" << func << " ...>";
        break;
      }

      io << "<CFunction:" << func << " ";
      io << "name=";
      this->pretty_print(io, func->name);
      io << " ";
      io << "argc=" << func->argc;
      io << " ";
      io << "pointer=" << func->pointer << " ";
      io << "bound_self_set=" << (func->bound_self_set ? "true" : "false") << " ";
      io << "bound_self=";
      this->pretty_print(io, func->bound_self);

      for (auto& entry : *func->container) {
        io << " ";
        std::string key = this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
        io << key << "=";
        this->pretty_print(io, entry.second);
      }

      io << ">";
      break;
    }

    case kTypeSymbol: {
      io << this->context.symtable(value).value_or(kUndefinedSymbolString);
      break;
    }

    case kTypeFrame: {
      Frame* frame = reinterpret_cast<Frame*>(value);

      if (printed_before) {
        io << "<Frame:" << frame << " ...>";
        break;
      }

      io << "<Frame:" << frame << " ";
      io << "parent=" << frame->parent << " ";
      io << "parent_environment_frame=" << frame->parent_environment_frame << " ";
      io << "function=";
      this->pretty_print(io, frame->function);
      io << " ";
      io << "self=";
      this->pretty_print(io, frame->self);
      io << " ";
      io << "return_address=" << reinterpret_cast<void*>(frame->return_address);
      io << ">";
      break;
    }

    case kTypeCatchTable: {
      CatchTable* table = reinterpret_cast<CatchTable*>(value);
      io << "<CatchTable:" << table << " ";
      io << "address=" << reinterpret_cast<void*>(table->address) << " ";
      io << "stacksize=" << table->stacksize << " ";
      io << "frame=" << table->frame << " ";
      io << "parent=" << table->parent;
      io << ">";
      break;
    }
  }

  this->pretty_print_stack.pop_back();
}

void VM::panic(STATUS reason) {
  this->context.out_stream << "Panic: " << kStatusHumanReadable[reason] << '\n';
  this->context.out_stream << "Stacktrace:" << '\n';
  this->stacktrace(this->context.out_stream);

  exit(1);
}

void VM::run() {
  // Execute instructions as long as we have a valid ip
  // and the machine wasn't halted
  while (this->ip && !this->halted) {
    auto exec_start = std::chrono::high_resolution_clock::now();

    // Backup the current ip
    // After the instruction was executed we check if
    // ip stayed the same. If it's still the same value
    // we increment it to the next instruction
    uint8_t* old_ip = this->ip;

    // Fetch the current opcode
    Opcode opcode = this->fetch_instruction();
    uint32_t instruction_length = kInstructionLengths[opcode];

    // Redirect to specific instruction handler
    switch (opcode) {
      case Opcode::Nop: {
        break;
      }

      case Opcode::ReadLocal: {
        uint32_t index = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        uint32_t level = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(uint32_t));
        this->op_readlocal(index, level);
        break;
      }

      case Opcode::ReadMemberSymbol: {
        VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
        this->op_readmembersymbol(symbol);
        break;
      }

      case Opcode::ReadArrayIndex: {
        uint32_t index = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        this->op_readarrayindex(index);
        break;
      }

      case Opcode::SetLocal: {
        uint32_t index = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        uint32_t level = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(uint32_t));
        this->op_setlocal(index, level);
        break;
      }

      case Opcode::SetMemberSymbol: {
        VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
        this->op_setmembersymbol(symbol);
        break;
      }

      case Opcode::SetArrayIndex: {
        uint32_t index = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        this->op_setarrayindex(index);
        break;
      }

      case Opcode::PutSelf: {
        uint32_t level = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        this->op_putself(level);
        break;
      }

      case Opcode::PutValue: {
        VALUE value = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
        this->op_putvalue(value);
        break;
      }

      case Opcode::PutFloat: {
        double value = *reinterpret_cast<double*>(this->ip + sizeof(Opcode));
        this->op_putfloat(value);
        break;
      }

      case Opcode::PutString: {
        uint32_t offset = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        uint32_t length = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(uint32_t));

        // We assume the compiler generated valid offsets and lengths, so we don't do
        // any out-of-bounds checking here
        // TODO: Should we do out-of-bounds checking here?
        char* str_start = reinterpret_cast<char*>(this->context.stringpool.getData() + offset);
        this->op_putstring(str_start, length);

        break;
      }

      case Opcode::PutFunction: {
        VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
        int32_t body_offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE));
        bool anonymous = *reinterpret_cast<bool*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(int32_t));
        uint32_t argc =
            *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(int32_t) + sizeof(bool));
        uint32_t lvarcount = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(int32_t) +
                                                          sizeof(bool) + sizeof(uint32_t));

        this->op_putfunction(symbol, this->ip + body_offset, anonymous, argc, lvarcount);
        break;
      }

      case Opcode::PutCFunction: {
        VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
        uintptr_t pointer = *reinterpret_cast<uintptr_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE));
        uint32_t argc = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(void*));
        this->op_putcfunction(symbol, pointer, argc);
        break;
      }

      case Opcode::PutArray: {
        uint32_t count = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        this->op_putarray(count);
        break;
      }

      case Opcode::PutHash: {
        uint32_t size = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        this->op_puthash(size);
        break;
      }

      case Opcode::Pop: {
        this->op_pop();
        break;
      }

      case Opcode::Dup: {
        this->op_dup();
      }

      case Opcode::Swap: {
        this->op_swap();
        break;
      }

      case Opcode::Topn: {
        uint32_t offset = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        this->op_topn(offset);
        break;
      }

      case Opcode::Setn: {
        uint32_t offset = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        this->op_setn(offset);
        break;
      }

      case Opcode::Call: {
        uint32_t argc = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        this->op_call(argc);
        break;
      }

      case Opcode::CallMember: {
        uint32_t argc = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        this->op_callmember(argc);
        break;
      }

      case Opcode::Return: {
        this->op_return();
        break;
      }

      case Opcode::Throw: {
        this->op_throw();
        break;
      }

      case Opcode::RegisterCatchTable: {
        int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
        this->op_registercatchtable(offset);
        break;
      }

      case Opcode::PopCatchTable: {
        this->op_popcatchtable();
        break;
      }

      case Opcode::Branch: {
        int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
        this->op_branch(offset);
        break;
      }

      case Opcode::BranchIf: {
        int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
        this->op_branchif(offset);
        break;
      }

      case Opcode::BranchUnless: {
        int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
        this->op_branchunless(offset);
        break;
      }

      case Opcode::Add: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(Operators::add(*this, left, right));
        break;
      }

      case Opcode::Halt: {
        this->halted = true;
        break;
      }

      case Opcode::GCCollect: {
        this->context.gc.mark(this->stack);
        this->context.gc.mark(this->frames);
        this->context.gc.mark(this->catchstack);
        this->context.gc.collect();
        break;
      }

      case Opcode::Typeof: {
        this->op_typeof();
        break;
      }

      default: {
        this->context.out_stream << "Unknown opcode: " << kOpcodeMnemonics[opcode] << " at "
                                 << reinterpret_cast<void*>(this->ip) << '\n';
        this->panic(Status::UnknownOpcode);
      }
    }

    // Increment ip if the instruction didn't change it
    if (this->ip == old_ip) {
      this->ip += instruction_length;
    }

    // Show opcodes as they are executed if the corresponding flag was set
    if (this->context.trace_opcodes) {
      std::chrono::duration<double> exec_duration = std::chrono::high_resolution_clock::now() - exec_start;
      this->context.out_stream << reinterpret_cast<void*>(old_ip) << ": " << kOpcodeMnemonics[opcode] << " ";
      this->context.out_stream << " (";
      this->context.out_stream << static_cast<uint32_t>(exec_duration.count() * 1000000000);
      this->context.out_stream << " nanoseconds)" << '\n';
    }
  }

  this->context.gc.mark(this->stack);
  this->context.gc.mark(this->frames);
  this->context.gc.mark(this->catchstack);
  this->context.gc.collect();
}
}  // namespace Charly
