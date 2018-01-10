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
#include "managedcontext.h"
#include "status.h"
#include "vm.h"

namespace Charly {

Frame* VM::pop_frame() {
  Frame* frame = this->frames;
  if (frame)
    this->frames = this->frames->parent;
  return frame;
}

Frame* VM::create_frame(VALUE self, Function* function, uint8_t* return_address, bool halt_after_return) {
  MemoryCell* cell = this->context.gc.allocate();
  cell->basic.set_type(kTypeFrame);
  cell->frame.parent = this->frames;
  cell->frame.parent_environment_frame = function->context;
  cell->frame.last_active_catchtable = this->catchstack;
  cell->frame.function = function;
  cell->frame.self = self;
  cell->frame.return_address = return_address;
  cell->frame.halt_after_return = halt_after_return;

  // Calculate the number of local variables this frame has to support
  // Add 1 at the beginning to reserve space for the arguments magic variable
  uint32_t lvarcount = 1 + function->argc + function->lvarcount;

  // Allocate and prefill local variable space
  cell->frame.environment.reserve(lvarcount);
  while (lvarcount--)
    cell->frame.environment.push_back(kNull);

  // Append the frame
  this->frames = cell->as<Frame>();

  // Print the frame if the corresponding flag was set
  if (this->context.trace_frames) {
    this->context.out_stream << "Entering frame: ";
    this->pretty_print(this->context.out_stream, cell->as_value());
    this->context.out_stream << '\n';
  }

  return cell->as<Frame>();
}

Frame* VM::create_frame(VALUE self,
                        Frame* parent_environment_frame,
                        uint32_t lvarcount,
                        uint8_t* return_address,
                        bool halt_after_return) {
  MemoryCell* cell = this->context.gc.allocate();
  cell->basic.set_type(kTypeFrame);
  cell->frame.parent = this->frames;
  cell->frame.parent_environment_frame = parent_environment_frame;
  cell->frame.last_active_catchtable = this->catchstack;
  cell->frame.function = nullptr;
  cell->frame.self = self;
  cell->frame.return_address = return_address;
  cell->frame.halt_after_return = halt_after_return;
  cell->frame.environment.reserve(lvarcount);
  while (lvarcount--)
    cell->frame.environment.push_back(kNull);

  // Append the frame
  this->frames = cell->as<Frame>();

  // Print the frame if the corresponding flag was set
  if (this->context.trace_frames) {
    this->context.out_stream << "Entering frame: ";
    this->pretty_print(this->context.out_stream, cell->as_value());
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
    this->pretty_print(this->context.out_stream, cell->as_value());
    this->context.out_stream << '\n';
  }

  return cell->as<CatchTable>();
}

CatchTable* VM::pop_catchtable() {
  if (!this->catchstack)
    this->panic(Status::CatchStackEmpty);
  CatchTable* current = this->catchstack;
  this->catchstack = current->parent;

  return current;
}

void VM::unwind_catchstack() {
  CatchTable* table = this->pop_catchtable();

  // Walk the frame tree until we reach the frame stored in the catchtable
  while (this->frames) {
    if (this->frames == table->frame) {
      break;
    } else {
      if (this->frames->halt_after_return) {
        this->halted = true;
      }
    }

    this->frames = this->frames->parent;
  }

  // Jump to the handler block of the catchtable
  this->ip = table->address;

  // Show the catchtable we just restored if the corresponding flag was set
  if (this->context.trace_catchtables) {
    // Show the table we've restored
    this->context.out_stream << "Restored CatchTable: ";
    this->pretty_print(this->context.out_stream, table);
    this->context.out_stream << '\n';
  }

  // If there are less elements on the stack than there were when the table was pushed
  // that means that the stack is not in a predictable state anymore
  // There is nothing we can do, but crash
  if (this->stack.size() < table->stacksize) {
    this->panic(Status::CorruptedStack);
  }

  // Unwind the stack to be the size it was when this catchtable
  // was pushed. Because the stack could be smaller, we need to
  // calculate the amount of values we can pop
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
  return cell->as_value();
}

VALUE VM::create_array(uint32_t initial_capacity) {
  MemoryCell* cell = this->context.gc.allocate();
  cell->basic.set_type(kTypeArray);
  cell->array.data = new std::vector<VALUE>();
  cell->array.data->reserve(initial_capacity);
  return cell->as_value();
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
  return cell->as_value();
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
  return cell->as_value();
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
  return cell->as_value();
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
  return cell->as_value();
}

VALUE VM::create_class(VALUE name) {
  MemoryCell* cell = this->context.gc.allocate();
  cell->basic.set_type(kTypeClass);
  cell->klass.name = name;
  cell->klass.constructor = kNull;
  cell->klass.member_properties = new std::vector<VALUE>();
  cell->klass.member_functions = new std::unordered_map<VALUE, VALUE>();
  cell->klass.parent_classes = new std::vector<VALUE>();
  cell->klass.container = new std::unordered_map<VALUE, VALUE>();
  return cell->as_value();
}

VALUE VM::copy_value(VALUE value) {
  switch (VM::real_type(value)) {
    case kTypeFloat: {
      if (is_special(value)) {
        return this->create_float(this->float_value(value));
      }
      break;
    }
    case kTypeString: return this->copy_string(value);
    case kTypeObject: return this->copy_object(value);
    case kTypeArray: return this->copy_array(value);
    case kTypeFunction: return this->copy_function(value);
    case kTypeCFunction: return this->copy_cfunction(value);
  }

  return value;
}

VALUE VM::deep_copy_value(VALUE value) {
  switch (VM::real_type(value)) {
    case kTypeFloat: {
      if (is_special(value)) {
        return this->create_float(this->float_value(value));
      }
      break;
    }
    case kTypeString: return this->copy_string(value);
    case kTypeObject: return this->deep_copy_object(value);
    case kTypeArray: return this->deep_copy_array(value);
    case kTypeFunction: return this->copy_function(value);
    case kTypeCFunction: return this->copy_cfunction(value);
  }

  return value;
}

VALUE VM::copy_object(VALUE object) {
  Object* source = reinterpret_cast<Object*>(object);
  Object* target = reinterpret_cast<Object*>(this->create_object(source->container->size()));
  target->container->insert(source->container->begin(), source->container->end());
  return reinterpret_cast<VALUE>(target);
}

VALUE VM::deep_copy_object(VALUE object) {
  ManagedContext lalloc(*this);

  Object* source = reinterpret_cast<Object*>(object);
  Object* target = reinterpret_cast<Object*>(lalloc.create_object(source->container->size()));

  // Copy each member from the source object into the target
  for (auto & [ key, value ] : *source->container) {
    (*target->container)[key] = this->deep_copy_value(value);
  }

  return reinterpret_cast<VALUE>(target);
}

VALUE VM::copy_array(VALUE array) {
  Array* source = reinterpret_cast<Array*>(array);
  Array* target = reinterpret_cast<Array*>(this->create_array(source->data->size()));

  for (auto value : *source->data) {
    target->data->push_back(value);
  }

  return reinterpret_cast<VALUE>(target);
}

VALUE VM::deep_copy_array(VALUE array) {
  ManagedContext lalloc(*this);

  Array* source = reinterpret_cast<Array*>(array);
  Array* target = reinterpret_cast<Array*>(lalloc.create_array(source->data->size()));

  // Copy each member from the source object into the target
  for (auto& value : *source->data) {
    target->data->push_back(this->deep_copy_value(value));
  }

  return reinterpret_cast<VALUE>(target);
}

VALUE VM::copy_string(VALUE string) {
  String* source = reinterpret_cast<String*>(string);
  return this->create_string(source->data, source->length);
}

VALUE VM::copy_function(VALUE function) {
  Function* source = reinterpret_cast<Function*>(function);
  Function* target = reinterpret_cast<Function*>(
      this->create_function(source->name, source->body_address, source->argc, source->lvarcount, source->anonymous));

  target->context = source->context;
  target->bound_self_set = source->bound_self_set;
  target->bound_self = source->bound_self;
  *(target->container) = *(source->container);

  return reinterpret_cast<VALUE>(target);
}

VALUE VM::copy_cfunction(VALUE function) {
  CFunction* source = reinterpret_cast<CFunction*>(function);
  CFunction* target = reinterpret_cast<CFunction*>(this->create_cfunction(source->name, source->argc, source->pointer));

  target->bound_self_set = source->bound_self_set;
  target->bound_self = source->bound_self;
  *(target->container) = *(source->container);

  return reinterpret_cast<VALUE>(target);
}

VALUE VM::cast_to_numeric(VALUE value) {
  return this->create_float(this->cast_to_double(value));
}

int64_t VM::cast_to_integer(VALUE value) {
  VALUE type = VM::real_type(value);

  switch (type) {
    case kTypeInteger: return VALUE_DECODE_INTEGER(value);
    case kTypeFloat: return VM::float_value(value);
    case kTypeString: {
      String* string = reinterpret_cast<String*>(value);
      char* end_it = string->data + string->length;
      int64_t interpreted = strtol(string->data, &end_it, 0);

      // strol sets errno to ERANGE if the result value doesn't fit
      // into the return type
      if (errno == ERANGE) {
        errno = 0;
        return NAN;
      }

      if (end_it == string->data) {
        return NAN;
      }

      // The value could be converted
      return interpreted;
    }
    default: return NAN;
  }
}

double VM::cast_to_double(VALUE value) {
  VALUE type = VM::real_type(value);

  switch (type) {
    case kTypeInteger: return static_cast<double>(VALUE_DECODE_INTEGER(value));
    case kTypeFloat: return VM::float_value(value);
    case kTypeString: {
      String* string = reinterpret_cast<String*>(value);
      char* end_it = string->data + string->length;
      double interpreted = strtod(string->data, &end_it);

      // HUGE_VAL gets returned when the converted value
      // doesn't fit inside a double value
      // In this case we just return NAN
      if (interpreted == HUGE_VAL)
        return NAN;

      // If strtod could not perform a conversion it returns 0
      // and sets *str_end to str
      if (end_it == string->data)
        return NAN;

      // The value could be converted
      return interpreted;
    }
    default: return NAN;
  }
}

VALUE VM::create_number(double value) {
  return this->create_float(value);
}

VALUE VM::create_number(int64_t value) {
  return VALUE_ENCODE_INTEGER(value);
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

double VM::double_value(VALUE value) {
  if (VM::real_type(value) == kTypeFloat) {
    return VM::float_value(value);
  } else {
    return static_cast<double>(VALUE_DECODE_INTEGER(value));
  }
}

int64_t VM::int_value(VALUE value) {
  if (VM::real_type(value) == kTypeFloat) {
    return static_cast<int64_t>(VM::float_value(value));
  } else {
    return VALUE_DECODE_INTEGER(value);
  }
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
  VALUE type = VM::real_type(value);

  if (type == kTypeFloat)
    return kTypeNumeric;
  if (type == kTypeInteger)
    return kTypeNumeric;
  if (type == kTypeCFunction)
    return kTypeFunction;
  return type;
}

bool VM::truthyness(VALUE value) {
  if (is_numeric(value)) {
    return VM::double_value(value) != 0;
  }

  if (is_null(value)) {
    return false;
  }

  if (is_false(value)) {
    return false;
  }

  return true;
}

VALUE VM::add(VALUE left, VALUE right) {
  if (VM::type(left) == kTypeNumeric && VM::type(right) == kTypeNumeric) {
    double nleft = VM::double_value(left);
    double nright = VM::double_value(right);
    return this->create_number(nleft + nright);
  }

  if (VM::real_type(left) == kTypeArray) {
    Array* new_array = reinterpret_cast<Array*>(this->copy_array(left));
    if (VM::real_type(right) == kTypeArray) {
      Array* aright = reinterpret_cast<Array*>(right);

      for (auto& value : *aright->data) {
        new_array->data->push_back(value);
      }

      return reinterpret_cast<VALUE>(new_array);
    }

    new_array->data->push_back(right);
    return reinterpret_cast<VALUE>(new_array);
  }

  // TODO: String concatenation

  return this->create_float(NAN);
}

VALUE VM::sub(VALUE left, VALUE right) {
  if (VM::type(left) == kTypeNumeric && VM::type(right) == kTypeNumeric) {
    double nleft = VM::double_value(left);
    double nright = VM::double_value(right);
    return this->create_number(nleft - nright);
  }

  return this->create_float(NAN);
}

VALUE VM::mul(VALUE left, VALUE right) {
  if (VM::type(left) == kTypeNumeric && VM::type(right) == kTypeNumeric) {
    double nleft = VM::double_value(left);
    double nright = VM::double_value(right);
    return this->create_number(nleft * nright);
  }

  // TODO: String multiplication

  return this->create_float(NAN);
}

VALUE VM::div(VALUE left, VALUE right) {
  if (VM::type(left) == kTypeNumeric && VM::type(right) == kTypeNumeric) {
    double nleft = VM::double_value(left);
    double nright = VM::double_value(right);
    return this->create_number(nleft / nright);
  }

  return this->create_float(NAN);
}

VALUE VM::mod(VALUE left, VALUE right) {
  if (VM::type(left) == kTypeNumeric && VM::type(right) == kTypeNumeric) {
    int64_t nleft = VM::int_value(left);
    int64_t nright = VM::int_value(right);
    return this->create_number(nleft % nright);
  }

  return this->create_float(NAN);
}

VALUE VM::pow(VALUE left, VALUE right) {
  if (VM::type(left) == kTypeNumeric && VM::type(right) == kTypeNumeric) {
    double nleft = VM::double_value(left);
    double nright = VM::double_value(right);
    double result = std::pow(nleft, nright);
    return this->create_number(result);
  }

  return this->create_float(NAN);
}

VALUE VM::uadd(VALUE value) {
  return value;
}

VALUE VM::usub(VALUE value) {
  if (VM::type(value) == kTypeNumeric) {
    double num = VM::double_value(value);
    return this->create_number(-num);
  }

  return this->create_float(NAN);
}

VALUE VM::eq(VALUE left, VALUE right) {
  if (VM::type(left) == kTypeNumeric && VM::type(right) == kTypeNumeric) {
    double nleft = VM::double_value(left);
    double nright = VM::double_value(right);
    return nleft == nright ? kTrue : kFalse;
  }

  if (VM::real_type(left) == kTypeString && VM::real_type(right) == kTypeString) {
    String* sleft = reinterpret_cast<String*>(left);
    String* sright = reinterpret_cast<String*>(right);

    if (sleft->length != sright->length) {
      return kFalse;
    }

    return strcmp(sleft->data, sright->data) ? kTrue : kFalse;
  }

  return left == right ? kTrue : kFalse;
}

VALUE VM::neq(VALUE left, VALUE right) {
  return !this->eq(left, right);
}

VALUE VM::lt(VALUE left, VALUE right) {
  if (VM::type(left) == kTypeNumeric && VM::type(right) == kTypeNumeric) {
    double nleft = VM::double_value(left);
    double nright = VM::double_value(right);
    return nleft < nright ? kTrue : kFalse;
  }

  if (VM::real_type(left) == kTypeString && VM::real_type(right) == kTypeString) {
    String* sleft = reinterpret_cast<String*>(left);
    String* sright = reinterpret_cast<String*>(right);
    return sleft->length < sright->length ? kTrue : kFalse;
  }

  return kFalse;
}

VALUE VM::gt(VALUE left, VALUE right) {
  if (VM::type(left) == kTypeNumeric && VM::type(right) == kTypeNumeric) {
    double nleft = VM::double_value(left);
    double nright = VM::double_value(right);
    return nleft > nright ? kTrue : kFalse;
  }

  if (VM::real_type(left) == kTypeString && VM::real_type(right) == kTypeString) {
    String* sleft = reinterpret_cast<String*>(left);
    String* sright = reinterpret_cast<String*>(right);
    return sleft->length > sright->length ? kTrue : kFalse;
  }

  return kFalse;
}

VALUE VM::le(VALUE left, VALUE right) {
  if (VM::type(left) == kTypeNumeric && VM::type(right) == kTypeNumeric) {
    double nleft = VM::double_value(left);
    double nright = VM::double_value(right);
    return nleft <= nright ? kTrue : kFalse;
  }

  if (VM::real_type(left) == kTypeString && VM::real_type(right) == kTypeString) {
    String* sleft = reinterpret_cast<String*>(left);
    String* sright = reinterpret_cast<String*>(right);
    return sleft->length <= sright->length ? kTrue : kFalse;
  }

  return kFalse;
}

VALUE VM::ge(VALUE left, VALUE right) {
  if (VM::type(left) == kTypeNumeric && VM::type(right) == kTypeNumeric) {
    double nleft = VM::double_value(left);
    double nright = VM::double_value(right);
    return nleft >= nright ? kTrue : kFalse;
  }

  if (VM::real_type(left) == kTypeString && VM::real_type(right) == kTypeString) {
    String* sleft = reinterpret_cast<String*>(left);
    String* sright = reinterpret_cast<String*>(right);
    return sleft->length >= sright->length ? kTrue : kFalse;
  }

  return kFalse;
}

VALUE VM::unot(VALUE value) {
  return !VM::truthyness(value) ? kTrue : kFalse;
}

VALUE VM::shl(VALUE left, VALUE right) {
  if (VM::type(left) != kTypeNumeric) {
    if (VM::real_type(left) == kTypeArray) {
      Array* larr = reinterpret_cast<Array*>(left);
      (*larr->data).push_back(right);
      return left;
    }

    return this->create_float(NAN);
  }

  if (VM::type(right) != kTypeNumeric) {
    return this->create_float(NAN);
  }

  int64_t ileft = VM::int_value(left);
  int64_t iright = VM::int_value(right);
  return this->create_float(ileft << iright);
}

VALUE VM::shr(VALUE left, VALUE right) {
  if (VM::type(left) != kTypeNumeric || VM::type(right) != kTypeNumeric) {
    return this->create_float(NAN);
  }

  int64_t ileft = VM::int_value(left);
  int64_t iright = VM::int_value(right);
  return this->create_float(ileft << iright);
}

VALUE VM::band(VALUE left, VALUE right) {
  if (VM::type(left) != kTypeNumeric || VM::type(right) != kTypeNumeric) {
    return this->create_float(NAN);
  }

  int64_t ileft = VM::int_value(left);
  int64_t iright = VM::int_value(right);
  return this->create_float(ileft & iright);
}

VALUE VM::bor(VALUE left, VALUE right) {
  if (VM::type(left) != kTypeNumeric || VM::type(right) != kTypeNumeric) {
    return this->create_float(NAN);
  }

  int64_t ileft = VM::int_value(left);
  int64_t iright = VM::int_value(right);
  return this->create_float(ileft | iright);
}

VALUE VM::bxor(VALUE left, VALUE right) {
  if (VM::type(left) != kTypeNumeric || VM::type(right) != kTypeNumeric) {
    return this->create_float(NAN);
  }

  int64_t ileft = VM::int_value(left);
  int64_t iright = VM::int_value(right);
  return this->create_float(ileft ^ iright);
}

VALUE VM::ubnot(VALUE value) {
  if (VM::type(value) != kTypeNumeric) {
    return this->create_float(NAN);
  }

  int64_t ivalue = VM::int_value(value);
  return this->create_float(~ivalue);
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
    if (!frame->parent_environment_frame) {
      return this->panic(Status::ReadFailedTooDeep);
    }

    frame = frame->parent_environment_frame;
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
  switch (VM::type(target)) {
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
    case kTypeClass: {
      Class* klass = reinterpret_cast<Class*>(target);

      if (klass->container->count(symbol) == 1) {
        this->push_stack((*klass->container)[symbol]);
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
  if (VM::real_type(stackval) != kTypeArray) {
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
    if (!frame->parent_environment_frame) {
      return this->panic(Status::WriteFailedTooDeep);
    }

    frame = frame->parent_environment_frame;
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
  switch (VM::real_type(target)) {
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

    case kTypeClass: {
      Class* klass = reinterpret_cast<Class*>(target);
      (*klass->container)[symbol] = value;
      break;
    }
  }

  this->push_stack(value);
}

void VM::op_setarrayindex(uint32_t index) {
  VALUE expression = this->pop_stack().value_or(kNull);
  VALUE stackval = this->pop_stack().value_or(kNull);

  // Check if we popped an array, if not push it back onto the stack
  if (VM::real_type(stackval) != kTypeArray) {
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

void VM::op_putclass(VALUE name,
                     uint32_t propertycount,
                     uint32_t staticpropertycount,
                     uint32_t methodcount,
                     uint32_t staticmethodcount,
                     uint32_t parentclasscount,
                     bool has_constructor) {
  Class* klass = reinterpret_cast<Class*>(this->create_class(name));
  klass->member_properties->reserve(propertycount);
  klass->member_functions->reserve(methodcount);
  klass->parent_classes->reserve(parentclasscount);
  klass->container->reserve(staticpropertycount + staticmethodcount);

  if (has_constructor) {
    klass->constructor = this->pop_stack().value_or(kNull);
  }

  while (parentclasscount--) {
    VALUE pclass = this->pop_stack().value_or(kNull);
    if (VM::real_type(pclass) != kTypeClass) {
      // TODO: throw an exception here
      this->panic(Status::ParentClassNotAClass);
    }
    klass->parent_classes->push_back(pclass);
  }

  while (staticmethodcount--) {
    VALUE smethod = this->pop_stack().value_or(kNull);
    if (VM::real_type(smethod) != kTypeFunction) {
      // TODO: throw an exception here
      this->panic(Status::InvalidArgumentType);
    }
    Function* func_smethod = reinterpret_cast<Function*>(smethod);
    (*klass->container)[func_smethod->name] = smethod;
  }

  while (methodcount--) {
    VALUE method = this->pop_stack().value_or(kNull);
    if (VM::real_type(method) != kTypeFunction) {
      // TODO: throw an exception here
      this->panic(Status::InvalidArgumentType);
    }
    Function* func_method = reinterpret_cast<Function*>(method);
    (*klass->member_functions)[func_method->name] = method;
  }

  while (staticpropertycount--) {
    VALUE sprop = this->pop_stack().value_or(sprop);
    if (VM::real_type(sprop) != kTypeSymbol) {
      // TODO: throw an exception here
      this->panic(Status::InvalidArgumentType);
    }
    (*klass->container)[sprop] = kNull;
  }

  while (propertycount--) {
    VALUE prop = this->pop_stack().value_or(kNull);
    if (VM::real_type(prop) != kTypeSymbol) {
      // TODO: throw an exception here
      this->panic(Status::InvalidArgumentType);
    }
    klass->member_properties->push_back(prop);
  }

  this->push_stack(reinterpret_cast<VALUE>(klass));
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

void VM::call(uint32_t argc, bool with_target, bool halt_after_return) {
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
  switch (VM::real_type(function)) {
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

      this->call_function(tfunc, argc, arguments, target, halt_after_return);
      return;
    }

    // Functions which wrap around a C function pointer
    case kTypeCFunction: {
      this->call_cfunction(reinterpret_cast<CFunction*>(function), argc, arguments);
      if (halt_after_return) {
        this->halted = true;
      }
      return;
    }

    // Class construction
    case kTypeClass: {
      this->call_class(reinterpret_cast<Class*>(function), argc, arguments);
      if (halt_after_return) {
        this->halted = true;
      }
      return;
    }

    default: {
      this->context.out_stream << "cant call a " << kValueTypeString[VM::real_type(function)] << '\n';

      // TODO: Handle as runtime error
      this->panic(Status::UnspecifiedError);
    }
  }
}

void VM::call_function(Function* function, uint32_t argc, VALUE* argv, VALUE self, bool halt_after_return) {
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
  Frame* frame = this->create_frame(self, function, return_address, halt_after_return);

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

void VM::call_class(Class* klass, uint32_t argc, VALUE* argv) {

  // We keep a reference to the current catchtable around in case the constructor throws an exception
  // After the constructor call we check if the table changed
  CatchTable* original_catchtable = this->catchstack;

  ManagedContext lalloc(*this);
  Object* object = reinterpret_cast<Object*>(lalloc.create_object(klass->member_properties->size()));

  // Initialize object
  object->klass = reinterpret_cast<VALUE>(klass);
  for (auto field : *klass->member_properties) {
    (* object->container)[field] = kNull;
  }

  // If we have a constructor, call it with the object
  if (klass->constructor != kNull) {
    this->call_function(reinterpret_cast<Function*>(klass->constructor), argc, argv, reinterpret_cast<VALUE>(object), true);
    this->run();
    this->halted = false;
  }

  if (this->catchstack == original_catchtable) {
    this->push_stack(reinterpret_cast<VALUE>(object));
  }
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

  if (frame->halt_after_return) {
    this->halted = true;
  }

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
  this->unwind_catchstack();
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
  if (VM::truthyness(test))
    this->ip += offset;
}

void VM::op_branchunless(int32_t offset) {
  VALUE test = this->pop_stack().value_or(kNull);
  if (!VM::truthyness(test))
    this->ip += offset;
}

void VM::op_typeof() {
  VALUE value = this->pop_stack().value_or(kNull);
  std::string& stringrep = kValueTypeString[VM::real_type(value)];
  this->push_stack(this->create_string(stringrep.c_str(), stringrep.size()));
}

void VM::stacktrace(std::ostream& io) {
  Frame* frame = this->frames;

  int i = 0;
  io << "IP: " << static_cast<void*>(this->ip) << '\n';
  while (frame && VM::real_type(reinterpret_cast<VALUE>(frame)) == kTypeFrame) {
    io << i++ << "# ";
    this->pretty_print(io, reinterpret_cast<VALUE>(frame));
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
  for (VALUE stackitem : this->stack) {
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

  switch (VM::real_type(value)) {
    case kTypeDead: {
      io << "<!!DEAD_CELL!!: " << reinterpret_cast<void*>(value) << ">";
      break;
    }

    case kTypeInteger: {
      io << VALUE_DECODE_INTEGER(value);
      break;
    }

    case kTypeFloat: {
      io << VM::float_value(value);
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

    case kTypeClass: {
      Class* klass = reinterpret_cast<Class*>(value);

      if (printed_before) {
        io << "<Class:" << klass << " ...>";
        break;
      }

      io << "<Class:" << klass << " ";
      io << "name=";
      this->pretty_print(io, klass->name);
      io << " ";
      io << "constructor=";
      this->pretty_print(io, klass->constructor);
      io << " ";

      io << "member_properties=[";
      for (auto entry : *klass->member_properties) {
        io << " " << this->context.symtable(entry).value_or(kUndefinedSymbolString);
      }
      io << "] ";

      io << "member_functions=[";
      for (auto entry : *klass->member_functions) {
        io << " " << this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
        io << "=";
        this->pretty_print(io, entry.second);
      }
      io << "] ";

      io << "parent_classes=[";
      for (auto entry : *klass->parent_classes) {
        io << " ";
        this->pretty_print(io, entry);
      }
      io << "]";

      for (auto entry : *klass->container) {
        io << " " << this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
        io << "=";
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

      if (frame->function != nullptr) {
        io << "function=";
        this->pretty_print(io, frame->function);
      } else {
        io << "<no address info>";
      }

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
  this->halted = false;

  // Execute instructions as long as we have a valid ip
  // and the machine wasn't halted
  while (this->ip && !this->halted) {
    std::chrono::time_point<std::chrono::high_resolution_clock> exec_start;
    if (this->context.trace_opcodes) {
      exec_start = std::chrono::high_resolution_clock::now();
    }

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
        char* str_start = reinterpret_cast<char*>(this->context.stringpool.get_data() + offset);
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

      case Opcode::PutClass: {
        VALUE name = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
        uint32_t propertycount = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode) + sizeof(VALUE));
        uint32_t staticpropertycount =
            *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(uint32_t));
        uint32_t methodcount = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) +
                                                            sizeof(uint32_t) + sizeof(uint32_t));
        uint32_t staticmethodcount = *reinterpret_cast<uint32_t*>(
            this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t));
        uint32_t parentclasscount =
            *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(uint32_t) +
                                         sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t));
        bool has_constructor =
            *reinterpret_cast<bool*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(uint32_t) + sizeof(uint32_t) +
                                     sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t));
        this->op_putclass(name, propertycount, staticpropertycount, methodcount, staticmethodcount, parentclasscount,
                          has_constructor);
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
        this->push_stack(this->add(left, right));
        break;
      }

      case Opcode::Sub: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->sub(left, right));
        break;
      }

      case Opcode::Mul: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->mul(left, right));
        break;
      }

      case Opcode::Div: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->div(left, right));
        break;
      }

      case Opcode::Mod: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->mod(left, right));
        break;
      }

      case Opcode::Pow: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->pow(left, right));
        break;
      }

      case Opcode::UAdd: {
        VALUE value = this->pop_stack().value_or(kNull);
        this->push_stack(this->uadd(value));
        break;
      }

      case Opcode::USub: {
        VALUE value = this->pop_stack().value_or(kNull);
        this->push_stack(this->usub(value));
        break;
      }

      case Opcode::Eq: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->eq(left, right));
        break;
      }

      case Opcode::Neq: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->neq(left, right));
        break;
      }

      case Opcode::Lt: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->lt(left, right));
        break;
      }

      case Opcode::Gt: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->gt(left, right));
        break;
      }

      case Opcode::Le: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->le(left, right));
        break;
      }

      case Opcode::Ge: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->ge(left, right));
        break;
      }

      case Opcode::UNot: {
        VALUE value = this->pop_stack().value_or(kNull);
        this->push_stack(this->unot(value));
        break;
      }

      case Opcode::Shr: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->shr(left, right));
        break;
      }

      case Opcode::Shl: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->shl(left, right));
        break;
      }

      case Opcode::And: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->band(left, right));
        break;
      }

      case Opcode::Or: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->bor(left, right));
        break;
      }

      case Opcode::Xor: {
        VALUE right = this->pop_stack().value_or(kNull);
        VALUE left = this->pop_stack().value_or(kNull);
        this->push_stack(this->bxor(left, right));
        break;
      }

      case Opcode::UBNot: {
        VALUE value = this->pop_stack().value_or(kNull);
        this->push_stack(this->ubnot(value));
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
}

void VM::exec_prelude() {
  this->create_frame(kNull, this->frames, 20, nullptr);
  this->op_putcfunction(this->context.symtable("get_method"), reinterpret_cast<uintptr_t>(Internals::get_method), 1);
  this->op_putvalue(this->context.symtable("get_method"));
  this->op_puthash(1);
  this->op_putvalue(this->context.symtable("internals"));
  this->op_puthash(1);
  this->op_setlocal(19, 0);
  this->op_pop();
}

void VM::exec_block(InstructionBlock* block) {
  // Calculate the return address
  uint8_t* return_address = nullptr;
  if (this->ip != nullptr) {
    Opcode opcode = this->fetch_instruction();
    uint32_t instruction_length = kInstructionLengths[opcode];
    return_address = this->ip + instruction_length;
  }

  // Execute the block and reset the instruction pointer afterwards
  this->ip = block->get_data();
  this->run();
  this->ip = return_address;
}

void VM::exec_module(InstructionBlock* block) {
  ManagedContext lalloc(*this);

  // Execute the module inclusion function
  this->exec_block(block);
  this->push_stack(lalloc.create_object(0));
  this->op_call(1);
  this->run();
}
}  // namespace Charly
