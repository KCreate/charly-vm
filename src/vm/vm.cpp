/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <random>

#include <utf8/utf8.h>

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
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeFrame;
  cell->frame.parent = this->frames;
  cell->frame.parent_environment_frame = function->context;
  cell->frame.last_active_catchtable = this->catchstack;
  cell->frame.caller_value = charly_create_pointer(function);
  cell->frame.self = self;
  cell->frame.origin_address = this->ip;
  cell->frame.return_address = return_address;
  cell->frame.halt_after_return = halt_after_return;

  // Calculate the number of local variables this frame has to support
  uint32_t lvarcount = function->lvarcount;

  // Allocate and prefill local variable space
  cell->frame.locals = new std::vector<VALUE>(lvarcount, kNull);

  // Append the frame
  this->frames = cell->as<Frame>();

  // Print the frame if the corresponding flag was set
  if (this->context.trace_frames) {
    this->context.err_stream << "Entering frame: ";
    this->pretty_print(this->context.err_stream, cell->as_value());
    this->context.err_stream << '\n';
  }

  return cell->as<Frame>();
}

Frame* VM::create_frame(VALUE self,
                        Frame* parent_environment_frame,
                        uint32_t lvarcount,
                        uint8_t* return_address,
                        bool halt_after_return) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeFrame;
  cell->frame.parent = this->frames;
  cell->frame.parent_environment_frame = parent_environment_frame;
  cell->frame.last_active_catchtable = this->catchstack;
  cell->frame.caller_value = kNull;
  cell->frame.self = self;
  cell->frame.origin_address = this->ip;
  cell->frame.return_address = return_address;
  cell->frame.halt_after_return = halt_after_return;
  cell->frame.locals = new std::vector<VALUE>(lvarcount, kNull);

  // Append the frame
  this->frames = cell->as<Frame>();

  // Print the frame if the corresponding flag was set
  if (this->context.trace_frames) {
    this->context.err_stream << "Entering frame: ";
    this->pretty_print(this->context.err_stream, cell->as_value());
    this->context.err_stream << '\n';
  }

  return cell->as<Frame>();
}

VALUE VM::pop_stack() {
  VALUE val = kNull;

  if (this->stack.size() == 0) {
    this->panic(Status::CorruptedStack);
  }
  val = this->stack.back();
  this->stack.pop_back();

  pop_queue.push(val);
  if (pop_queue.size() >= 10) pop_queue.pop();

  return val;
}

void VM::push_stack(VALUE value) {
  this->stack.push_back(value);
}

CatchTable* VM::create_catchtable(uint8_t* address) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeCatchTable;
  cell->catchtable.stacksize = this->stack.size();
  cell->catchtable.frame = this->frames;
  cell->catchtable.parent = this->catchstack;
  cell->catchtable.address = address;
  this->catchstack = cell->as<CatchTable>();

  // Print the catchtable if the corresponding flag was set
  if (this->context.trace_catchtables) {
    this->context.err_stream << "Entering catchtable: ";
    this->pretty_print(this->context.err_stream, cell->as_value());
    this->context.err_stream << '\n';
  }

  return cell->as<CatchTable>();
}

CatchTable* VM::pop_catchtable() {
  if (!this->catchstack) {
    this->context.err_stream << "No catchtable registered" << '\n';
    this->panic(Status::CatchStackEmpty);
  }
  CatchTable* current = this->catchstack;
  this->catchstack = current->parent;

  return current;
}

void VM::unwind_catchstack(std::optional<VALUE> payload = std::nullopt) {

  // Check if there is a catchtable
  if (!this->catchstack && charly_is_function(this->uncaught_exception_handler)) {
    Function* exception_handler = charly_as_function(this->uncaught_exception_handler);
    VALUE global_self = this->get_global_self();
    VALUE payload_value = payload.value_or(charly_create_number(25));
    this->call_function(exception_handler, 1, &payload_value, global_self, true);
    return;
  }

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
    this->context.err_stream << "Restored CatchTable: ";
    this->pretty_print(this->context.err_stream, charly_create_pointer(table));
    this->context.err_stream << '\n';
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
    this->pop_stack();
  }

  if (payload.has_value()) {
    this->push_stack(payload.value());
  }
}

VALUE VM::create_object(uint32_t initial_capacity) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeObject;
  cell->object.klass = this->primitive_object;
  cell->object.container = new std::unordered_map<VALUE, VALUE>();
  cell->object.container->reserve(initial_capacity);
  return cell->as_value();
}

VALUE VM::create_array(uint32_t initial_capacity) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeArray;
  cell->array.data = new std::vector<VALUE>();
  cell->array.data->reserve(initial_capacity);
  return cell->as_value();
}

VALUE VM::create_string(const char* data, uint32_t length) {
  if (length <= 6)
    return charly_create_istring(data, length);

  // Check if we can create a short string
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeString;

  // Copy the string onto the heap
  char* copied_string = static_cast<char*>(calloc(sizeof(char), length));
  std::memcpy(copied_string, data, length);

  // Setup long string
  cell->string.data = copied_string;
  cell->string.length = length;

  return cell->as_value();
}

VALUE VM::create_string(const std::string& str) {
  if (str.size() <= 6)
    return charly_create_istring(str.c_str(), str.size());

  // Check if we can create a short string
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeString;

  // Copy the string onto the heap
  char* copied_string = static_cast<char*>(calloc(sizeof(char), str.size()));
  std::memcpy(copied_string, str.c_str(), str.size());

  // Setup long string
  cell->string.data = copied_string;
  cell->string.length = str.size();

  return cell->as_value();
}

VALUE VM::create_weak_string(char* data, uint32_t length) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeString;
  cell->string.data = data;
  cell->string.length = length;
  return cell->as_value();
}

VALUE VM::create_function(VALUE name,
                          uint8_t* body_address,
                          uint32_t argc,
                          uint32_t minimum_argc,
                          uint32_t lvarcount,
                          bool anonymous,
                          bool needs_arguments) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeFunction;
  cell->function.name = name;
  cell->function.argc = argc;
  cell->function.minimum_argc = minimum_argc;
  cell->function.lvarcount = lvarcount;
  cell->function.context = this->frames;
  cell->function.body_address = body_address;
  cell->function.anonymous = anonymous;
  cell->function.needs_arguments = needs_arguments;
  cell->function.bound_self_set = false;
  cell->function.bound_self = kNull;
  cell->function.host_class = kNull;
  cell->function.container = new std::unordered_map<VALUE, VALUE>();
  return cell->as_value();
}

VALUE VM::create_cfunction(VALUE name, uint32_t argc, void* pointer, uint8_t thread_policy) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeCFunction;
  cell->cfunction.name = name;
  cell->cfunction.pointer = pointer;
  cell->cfunction.argc = argc;
  cell->cfunction.container = new std::unordered_map<VALUE, VALUE>();
  cell->cfunction.thread_policy = thread_policy;
  cell->cfunction.push_return_value = true;
  cell->cfunction.halt_after_return = false;
  return cell->as_value();
}

VALUE VM::create_class(VALUE name) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeClass;
  cell->klass.name = name;
  cell->klass.constructor = kNull;
  cell->klass.member_properties = new std::vector<VALUE>();
  cell->klass.prototype = kNull;
  cell->klass.parent_class = kNull;
  cell->klass.container = new std::unordered_map<VALUE, VALUE>();
  return cell->as_value();
}

VALUE VM::create_cpointer(void* data, void* destructor) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeCPointer;
  cell->cpointer.data = data;
  cell->cpointer.destructor = destructor;
  return cell->as_value();
}

VALUE VM::copy_value(VALUE value) {
  switch (charly_get_type(value)) {
    case kTypeString: return this->copy_string(value);
    case kTypeObject: return this->copy_object(value);
    case kTypeArray: return this->copy_array(value);
    case kTypeFunction: return this->copy_function(value);
    case kTypeCFunction: return this->copy_cfunction(value);
  }

  return value;
}

VALUE VM::deep_copy_value(VALUE value) {
  switch (charly_get_type(value)) {
    case kTypeString: return this->copy_string(value);
    case kTypeObject: return this->deep_copy_object(value);
    case kTypeArray: return this->deep_copy_array(value);
    case kTypeFunction: return this->copy_function(value);
    case kTypeCFunction: return this->copy_cfunction(value);
  }

  return value;
}

VALUE VM::copy_object(VALUE object) {
  Object* source = charly_as_object(object);
  Object* target = charly_as_object(this->create_object(source->container->size()));
  target->container->insert(source->container->begin(), source->container->end());
  return charly_create_pointer(target);
}

VALUE VM::deep_copy_object(VALUE object) {
  ManagedContext lalloc(this);

  Object* source = charly_as_object(object);
  Object* target = charly_as_object(lalloc.create_object(source->container->size()));

  // Copy each member from the source object into the target
  for (auto & [ key, value ] : *source->container) {
    (*target->container)[key] = this->deep_copy_value(value);
  }

  return charly_create_pointer(target);
}

VALUE VM::copy_array(VALUE array) {
  Array* source = charly_as_array(array);
  Array* target = charly_as_array(this->create_array(source->data->size()));

  for (auto value : *source->data) {
    target->data->push_back(value);
  }

  return charly_create_pointer(target);
}

VALUE VM::deep_copy_array(VALUE array) {
  ManagedContext lalloc(this);

  Array* source = charly_as_array(array);
  Array* target = charly_as_array(lalloc.create_array(source->data->size()));

  // Copy each member from the source object into the target
  for (auto& value : *source->data) {
    target->data->push_back(this->deep_copy_value(value));
  }

  return charly_create_pointer(target);
}

VALUE VM::copy_string(VALUE string) {
  char* str_data = charly_string_data(string);
  uint32_t str_length = charly_string_length(string);
  return this->create_string(str_data, str_length);
}

VALUE VM::copy_function(VALUE function) {
  Function* source = charly_as_function(function);
  Function* target = charly_as_function(this->create_function(
    source->name,
    source->body_address,
    source->argc,
    source->minimum_argc,
    source->lvarcount,
    source->anonymous,
    source->needs_arguments
  ));

  target->context = source->context;
  target->bound_self_set = source->bound_self_set;
  target->bound_self = source->bound_self;
  target->host_class = source->host_class;
  *(target->container) = *(source->container);

  return charly_create_pointer(target);
}

VALUE VM::copy_cfunction(VALUE function) {
  CFunction* source = charly_as_cfunction(function);
  CFunction* target = charly_as_cfunction(this->create_cfunction(
    source->name,
    source->argc,
    source->pointer,
    source->thread_policy
  ));
  *(target->container) = *(source->container);

  return charly_create_pointer(target);
}

VALUE VM::add(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_add_number(left, right);
  }

  if (charly_is_array(left)) {
    Array* new_array = charly_as_array(this->copy_array(left));
    if (charly_is_array(right)) {
      Array* aright = charly_as_array(right);

      for (auto& value : *aright->data) {
        new_array->data->push_back(value);
      }

      return charly_create_pointer(new_array);
    }

    new_array->data->push_back(right);
    return charly_create_pointer(new_array);
  }

  if (charly_is_string(left) && charly_is_string(right)) {
    uint32_t left_length = charly_string_length(left);
    uint32_t right_length = charly_string_length(right);
    uint64_t new_length = left_length + right_length;

    // Check if we have to do any work at all
    if (new_length == 0) {
      return charly_create_empty_string();
    }

    // If one of the strings is empty, we can return the other one
    if (left_length == 0)
      return right;
    if (right_length == 0)
      return left;
    if (new_length >= kMaxStringLength)
      return kNull;

    // If both strings fit into the immediate encoded format (nan-boxed inside the VALUE type)
    // we call a more optimized version of string concatenation
    // This allows us to not allocate an additional buffer for this
    if (new_length == kMaxPStringLength) {
      return charly_string_concat_into_packed(left, right);
    }
    if (new_length <= kMaxIStringLength) {
      return charly_string_concat_into_immediate(left, right);
    }

    char* left_data = charly_string_data(left);
    char* right_data = charly_string_data(right);

    // Allocate buffer and copy in the data
    char* new_data = reinterpret_cast<char*>(std::malloc(new_length));
    std::memcpy(new_data, left_data, left_length);
    std::memcpy(new_data + left_length, right_data, right_length);
    return this->create_weak_string(new_data, new_length);
  }

  // String concatenation for different types
  if (charly_is_string(left) || charly_is_string(right)) {
    std::stringstream buf;
    this->to_s(buf, left);
    this->to_s(buf, right);
    return this->create_string(buf.str());
  }

  return kNaN;
}

VALUE VM::sub(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_sub_number(left, right);
  }

  return kNaN;
}

VALUE VM::mul(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_mul_number(left, right);
  }

  // String multiplication should work bidirectionally
  if (charly_is_number(left) && charly_is_string(right))
    std::swap(left, right);
  if (charly_is_string(left) && charly_is_number(right)) {
    char* str_data = charly_string_data(left);
    uint32_t str_length = charly_string_length(left);
    int64_t amount = charly_number_to_int64(right);
    uint64_t new_length = str_length * amount;

    // Check if we have to do any work at all
    if (amount <= 0)
      return charly_create_empty_string();
    if (amount == 1)
      return left;
    if (amount >= kMaxStringLength)
      return kNull;

    // If the result fits into the immediate encoded format (nan-boxed inside the VALUE type)
    // we call a more optimized version of string multiplication
    // This allows us to not allocate an additional buffer for this
    if (new_length == kMaxPStringLength) {
      return charly_string_mul_into_packed(left, amount);
    }
    if (new_length <= kMaxIStringLength) {
      return charly_string_mul_into_immediate(left, amount);
    }

    // Allocate the buffer for the string
    char* new_data = reinterpret_cast<char*>(std::malloc(new_length));
    uint32_t offset = 0;
    while (amount--) {
      std::memcpy(new_data + offset, str_data, str_length);
      offset += str_length;
    }
    return this->create_weak_string(new_data, new_length);
  }

  return kNaN;
}

VALUE VM::div(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_div_number(left, right);
  }

  return kNaN;
}

VALUE VM::mod(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_mod_number(left, right);
  }

  return kNaN;
}

VALUE VM::pow(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_pow_number(left, right);
  }

  return kNaN;
}

VALUE VM::uadd(VALUE value) {
  return value;
}

VALUE VM::usub(VALUE value) {
  if (charly_is_number(value)) {
    return charly_usub_number(value);
  }

  return kNaN;
}

VALUE VM::eq(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_eq_number(left, right);
  }

  if (charly_is_string(left) && charly_is_string(right)) {
    char* str_left_data = charly_string_data(left);
    char* str_right_data = charly_string_data(right);

    if (str_left_data == str_right_data) {
      return kTrue;
    }

    uint32_t str_left_length = charly_string_length(left);
    uint32_t str_right_length = charly_string_length(right);

    if (str_left_length != str_right_length) {
      return kFalse;
    }

    // std::strncmp returns 0 if the two strings are equal
    return std::strncmp(str_left_data, str_right_data, str_left_length) ? kFalse : kTrue;
  }

  if (charly_is_array(left) && charly_is_array(right)) {
    Array* a_left = charly_as_array(left);
    Array* a_right = charly_as_array(right);

    // Arrays of different sizes are not equal
    if (a_left->data->size() != a_right->data->size()) return kFalse;

    // Empty arrays are always equal
    if (a_left->data->size() == 0 && a_right->data->size() == 0) return kTrue;

    // Compare each item, return false if a value doesn't match
    for (uint32_t i = 0; i < a_left->data->size(); i++) {
      VALUE vl = a_left->data->at(i);
      VALUE vr = a_right->data->at(i);
      if (this->eq(vl, vr) == kFalse) return kFalse;
    }

    return kTrue;
  }

  return left == right ? kTrue : kFalse;
}

VALUE VM::neq(VALUE left, VALUE right) {
  return this->eq(left, right) == kTrue ? kFalse : kTrue;
}

VALUE VM::lt(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_lt_number(left, right);
  }

  if (charly_is_string(left) && charly_is_string(right)) {
    return charly_string_length(left) < charly_string_length(right) ? kTrue : kFalse;
  }

  return kFalse;
}

VALUE VM::gt(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_gt_number(left, right);
  }

  if (charly_is_string(left) && charly_is_string(right)) {
    return charly_string_length(left) > charly_string_length(right) ? kTrue : kFalse;
  }

  return kFalse;
}

VALUE VM::le(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_le_number(left, right);
  }

  if (charly_is_string(left) && charly_is_string(right)) {
    return charly_string_length(left) <= charly_string_length(right) ? kTrue : kFalse;
  }

  return kFalse;
}

VALUE VM::ge(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_ge_number(left, right);
  }

  if (charly_is_string(left) && charly_is_string(right)) {
    return charly_string_length(left) >= charly_string_length(right) ? kTrue : kFalse;
  }

  return kFalse;
}

VALUE VM::unot(VALUE value) {
  return charly_truthyness(value) ? kFalse : kTrue;
}

VALUE VM::shl(VALUE left, VALUE right) {
  if (charly_is_array(left)) {
    Array* arr = charly_as_array(left);
    (*arr->data).push_back(right);
    return left;
  }

  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_shl_number(left, right);
  }

  return kNaN;
}

VALUE VM::shr(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_shr_number(left, right);
  }

  return kNaN;
}

VALUE VM::band(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_and_number(left, right);
  }

  return kNaN;
}

VALUE VM::bor(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_or_number(left, right);
  }

  return kNaN;
}

VALUE VM::bxor(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_xor_number(left, right);
  }

  return kNaN;
}

VALUE VM::ubnot(VALUE value) {
  if (charly_is_number(value)) {
    return charly_ubnot_number(value);
  }

  return kNaN;
}

VALUE VM::readmembersymbol(VALUE source, VALUE symbol) {
  switch (charly_get_type(source)) {
    case kTypeObject: {
      Object* obj = charly_as_object(source);

      if (symbol == SYM("klass")) {
        return obj->klass;
      }

      if (obj->container->count(symbol) == 1) {
        return (*obj->container)[symbol];
      }

      break;
    }

    case kTypeFunction: {
      Function* func = charly_as_function(source);

      if (symbol == SYM("name")) {
        return this->create_string(SymbolTable::decode(func->name));
      }

      if (symbol == SYM("host_class")) {
        return func->host_class;
      }

      if (symbol == SYM("argc")) {
        return charly_create_number(func->minimum_argc);
      }

      if (func->container->count(symbol) == 1) {
        return (*func->container)[symbol];
      }

      break;
    }

    case kTypeCFunction: {
      CFunction* cfunc = charly_as_cfunction(source);

      if (symbol == SYM("name")) {
        return this->create_string(SymbolTable::decode(cfunc->name));
      }

      if (symbol == SYM("push_return_value")) {
        return cfunc->push_return_value ? kTrue : kFalse;
      }

      if (symbol == SYM("halt_after_return")) {
        return cfunc->halt_after_return ? kTrue : kFalse;
      }

      if (symbol == SYM("argc")) {
        return charly_create_number(cfunc->argc);
      }

      if (symbol == SYM("thread_policy")) {
        return charly_create_number(cfunc->thread_policy);
      }

      if (cfunc->container->count(symbol) == 1) {
        return (*cfunc->container)[symbol];
      }

      break;
    }
    case kTypeClass: {
      Class* klass = charly_as_class(source);

      if (symbol == SYM("prototype")) {
        return klass->prototype;
      }

      if (symbol == SYM("constructor")) {
        return klass->constructor;
      }

      if (symbol == SYM("name")) {
        return this->create_string(SymbolTable::decode(klass->name));
      }

      if (symbol == SYM("parent_class")) {
        return klass->parent_class;
      }

      if (klass->container->count(symbol) == 1) {
        return (*klass->container)[symbol];
      }

      break;
    }
    case kTypeArray: {
      Array* arr = charly_as_array(source);

      if (symbol == SYM("length")) {
        return charly_create_integer(arr->data->size());
      }

      break;
    }
    case kTypeString: {
      if (symbol == SYM("length")) {
        return charly_create_integer(charly_string_utf8_length(source));
      }

      break;
    }
  }

  // At this point, the symbol was not found in the container of the source
  // or it didn't have a container
  //
  // If the value was an object, we walk the class hierarchy and search for a method
  // If the value was any other object, we check it's primitive class
  // If the primitive class didn't contain a method, the primitive class for Object
  // is checked
  //
  // If no result was found, null is returned
  if (charly_is_object(source)) {
    VALUE val_klass = charly_as_object(source)->klass;

    // Make sure the klass field is a Class value
    if (!charly_is_class(val_klass)) {
      val_klass = this->primitive_object;
    }

    if (charly_is_class(val_klass)) {
      Class* klass = charly_as_class(val_klass);
      auto result = charly_find_prototype_value(klass, symbol);

      if (result.has_value()) {
        return result.value();
      }
    }
  }

  auto primitive_lookup = this->findprimitivevalue(source, symbol);
  if (primitive_lookup.has_value()) {
    return primitive_lookup.value();
  } else {
    return kNull;
  }
}

VALUE VM::readmembervalue(VALUE source, VALUE value) {
  switch (charly_get_type(source)) {

    case kTypeArray: {
      Array* arr = charly_as_array(source);
      if (charly_is_number(value)) {
        int32_t index = charly_number_to_int32(value);

        // Negative indices read from the end of the array
        if (index < 0) {
          index += arr->data->size();
        }

        // Out of bounds check
        if (index < 0 || index >= static_cast<int>(arr->data->size())) {
          return kNull;
        }

        return (*arr->data)[index];
      }
    }

    case kTypeString: {
      if (charly_is_number(value)) {
        int32_t index = charly_number_to_int32(value);
        return charly_string_cp_at_index(source, index);
      }

      break;
    }
  }

  return this->readmembersymbol(source, SymbolTable::encode(value));
}

VALUE VM::setmembersymbol(VALUE target, VALUE symbol, VALUE value) {
  switch (charly_get_type(target)) {
    case kTypeObject: {
      Object* obj = charly_as_object(target);

      if (symbol == SYM("klass")) break;

      (*obj->container)[symbol] = value;
      break;
    }

    case kTypeFunction: {
      Function* func = charly_as_function(target);

      if (symbol == SYM("name")) break;
      if (symbol == SYM("host_class")) break;

      (*func->container)[symbol] = value;
      break;
    }

    case kTypeCFunction: {
      CFunction* cfunc = charly_as_cfunction(target);

      if (symbol == SYM("push_return_value")) {
        cfunc->push_return_value = charly_truthyness(value);
        break;
      }

      if (symbol == SYM("halt_after_return")) {
        cfunc->halt_after_return = charly_truthyness(value);
        break;
      }

      if (symbol == SYM("name")) break;

      (*cfunc->container)[symbol] = value;
      break;
    }

    case kTypeClass: {
      Class* klass = charly_as_class(target);

      if (symbol == SYM("prototype")) break;
      if (symbol == SYM("constructor")) break;
      if (symbol == SYM("name")) break;
      if (symbol == SYM("parent_class")) break;

      (*klass->container)[symbol] = value;
      break;
    }
  }

  return value;
}

VALUE VM::setmembervalue(VALUE target, VALUE member_value, VALUE value) {
  if (charly_get_type(target) == kTypeArray) {
    Array* arr = charly_as_array(target);

    if (charly_is_number(member_value)) {
      int32_t index = charly_number_to_int32(member_value);

      // Negative indices read from the end of the array
      if (index < 0) {
        index += arr->data->size();
      }

      // Out of bounds check
      if (index < 0 || index >= static_cast<int>(arr->data->size())) {
        return kNull;
      }

      // Update the value
      (*arr->data)[index] = value;
      return value;
    }
  }

  // Turn member_value into a symbol
  return this->setmembersymbol(target, SymbolTable::encode(member_value), value);
}

std::optional<VALUE> VM::findprimitivevalue(VALUE value, VALUE symbol) {

  // Get the corresponding primitive class
  VALUE found_primitive_class = kNull;
  switch (charly_get_type(value)) {
    case kTypeNumber: found_primitive_class = this->primitive_number; break;
    case kTypeString: found_primitive_class = this->primitive_string; break;
    case kTypeBoolean: found_primitive_class = this->primitive_boolean; break;
    case kTypeNull: found_primitive_class = this->primitive_null; break;
    case kTypeArray: found_primitive_class = this->primitive_array; break;
    case kTypeFunction: found_primitive_class = this->primitive_function; break;
    case kTypeCFunction: found_primitive_class = this->primitive_function; break;
    case kTypeClass: found_primitive_class = this->primitive_class; break;
    default: found_primitive_class = this->primitive_value; break;
  }

  if (symbol == SYM("klass")) {
    return found_primitive_class;
  }

  if (!charly_is_class(found_primitive_class)) {
    return std::nullopt;
  }

  Class* pclass = charly_as_class(found_primitive_class);
  return charly_find_prototype_value(pclass, symbol);
}

void VM::call(uint32_t argc, bool with_target, bool halt_after_return) {
  // Stack allocate enough space to copy all arguments into
  VALUE arguments[argc];

  // Basically what this loop does is it counts argc_copy down to 0
  // We do this because arguments are constructed on the stack
  // in the reverse order than we are popping them off
  for (int argc_copy = argc - 1; argc_copy >= 0; argc_copy--) {
    VALUE value = this->pop_stack();
    arguments[argc_copy] = value;
  }

  // Pop the function off of the stack
  VALUE function = this->pop_stack();

  // The target of the function is either supplied explicitly via the call_member instruction
  // or implicitly via the functions frame
  // If it's supplied via the frame hierarchy, we need to resolve it here
  // If not we simply pop it off the stack
  VALUE target = kNull;
  if (with_target)
    target = this->pop_stack();

  // Redirect to the correct handler
  switch (charly_get_type(function)) {
    // Normal functions as defined via the user
    case kTypeFunction: {
      Function* tfunc = charly_as_function(function);
      target = this->get_self_for_function(tfunc, with_target ? &target : nullptr);
      this->call_function(tfunc, argc, arguments, target, halt_after_return);
      return;
    }

    // Functions which wrap around a C function pointer
    case kTypeCFunction: {
      this->call_cfunction(charly_as_cfunction(function), argc, arguments);
      if (halt_after_return) {
        this->halted = true;
      }
      return;
    }

    default: { this->throw_exception("Attempted to call a non-callable type: " + charly_get_typestring(function)); }
  }
}

void VM::call_function(Function* function, uint32_t argc, VALUE* argv, VALUE self, bool halt_after_return) {
  // Check if the function was called with enough arguments
  if (argc < function->minimum_argc) {
    this->throw_exception("Not enough arguments for function call");
    return;
  }

  // The return address is simply the instruction after the one we've been called from
  // If the ip is nullptr (non-existent instructions that are run at the beginning of the VM) we
  // don't compute a return address
  uint8_t* return_address = nullptr;
  if (this->ip != nullptr) {
    if (halt_after_return) {
      return_address = this->ip;
    } else {
      return_address = this->ip + kInstructionLengths[this->fetch_instruction()];
    }
  }

  ManagedContext ctx(*this);
  ctx.mark_in_gc(function);
  ctx.mark_in_gc(self);
  for (uint32_t i = 0; i < argc; i++) {
    ctx.mark_in_gc(argv[i]);
  }

  Frame* frame = ctx.create_frame(self, function, return_address, halt_after_return);

  // Copy the arguments into the function frame
  //
  // If the function requires an arguments array, we create one and push it onto
  // offset 0 of the frame
  if (function->needs_arguments) {
    Array* arguments_array = charly_as_array(ctx.create_array(argc));
    frame->write_local(0, charly_create_pointer(arguments_array));

    for (size_t i = 0; i < argc; i++) {
      if (i < function->argc) {
        frame->write_local(i + 1, argv[i]);
      }
      arguments_array->data->push_back(argv[i]);
    }
  } else {
    for (size_t i = 0; i < argc && i < function->lvarcount; i++)
      frame->write_local(i, argv[i]);
  }

  this->ip = function->body_address;
}

void VM::call_cfunction(CFunction* function, uint32_t argc, VALUE* argv) {
  if (!function->allowed_on_main_thread()) {
    this->throw_exception("Calling this CFunction in the main thread is prohibited");
    return;
  }

  // Check if enough arguments have been passed
  if (argc < function->argc) {
    this->throw_exception("Not enough arguments for CFunction call");
    return;
  }

  // Mark the function and function arguments as temporaries
  this->gc.mark_persistent(charly_create_pointer(function));
  for (uint i = 0; i < argc; i++) {
    this->gc.mark_persistent(argv[i]);
  }

  // We keep a reference to the current catchtable around in case the constructor throws an exception
  // After the constructor call we check if the table changed
  CatchTable* original_catchtable = this->catchstack;

  // Invoke the C function
  VALUE rv = charly_call_cfunction(this, function, argc, argv);

  this->gc.unmark_persistent(charly_create_pointer(function));
  for (uint i = 0; i < argc; i++) {
    this->gc.unmark_persistent(argv[i]);
  }

  this->halted = function->halt_after_return;

  if (function->push_return_value && this->catchstack == original_catchtable) {
    this->push_stack(rv);
  }
}

void VM::initialize_member_properties(Class* klass, Object* object) {
  if (charly_is_class(klass->parent_class)) {
    this->initialize_member_properties(charly_as_class(klass->parent_class), object);
  }

  for (auto field : *klass->member_properties) {
    (*object->container)[field] = kNull;
  }
}

Opcode VM::fetch_instruction() {
  return *reinterpret_cast<Opcode*>(this->ip);
}

void VM::op_readlocal(uint32_t index, uint32_t level) {
  // Travel to the correct frame
  Frame* frame = this->frames;
  while (level != 0 && level--) {
    if (!frame->parent_environment_frame) {
      return this->panic(Status::ReadFailedTooDeep);
    }

    frame = frame->parent_environment_frame;
  }

  // Check if the index isn't out-of-bounds
  if (index >= frame->lvarcount()) {
    return this->panic(Status::ReadFailedOutOfBounds);
  }

  this->push_stack(frame->read_local(index));
}

void VM::op_readmembersymbol(VALUE symbol) {
  VALUE source = this->pop_stack();
  this->push_stack(this->readmembersymbol(source, symbol));
}

void VM::op_readmembervalue() {
  VALUE value = this->pop_stack();
  VALUE source = this->pop_stack();
  this->push_stack(this->readmembervalue(source, value));
}

void VM::op_readarrayindex(uint32_t index) {
  VALUE stackval = this->pop_stack();

  // Check if we popped an array, if not push it back onto the stack
  if (!charly_is_array(stackval)) {
    this->push_stack(stackval);
    return;
  }

  Array* arr = charly_as_array(stackval);

  // Out-of-bounds checking
  if (index >= arr->data->size()) {
    this->push_stack(kNull);
    return;
  }

  this->push_stack((*arr->data)[index]);
}

void VM::op_readglobal(VALUE symbol) {

  // Check internal methods table
  if (Internals::Index::methods.count(symbol)) {
    Internals::MethodSignature& sig = Internals::Index::methods.at(symbol);
    VALUE cfunc = this->create_cfunction(symbol, sig.argc, sig.func_pointer, sig.thread_policy);
    this->push_stack(cfunc);
    return;
  }

  // Check vm specific symbols
  switch (symbol) {
    case SYM("charly.vm.primitive.value"):            this->push_stack(this->primitive_value); return;
    case SYM("charly.vm.primitive.object"):           this->push_stack(this->primitive_object); return;
    case SYM("charly.vm.primitive.class"):            this->push_stack(this->primitive_class); return;
    case SYM("charly.vm.primitive.array"):            this->push_stack(this->primitive_array); return;
    case SYM("charly.vm.primitive.string"):           this->push_stack(this->primitive_string); return;
    case SYM("charly.vm.primitive.number"):           this->push_stack(this->primitive_number); return;
    case SYM("charly.vm.primitive.function"):         this->push_stack(this->primitive_function); return;
    case SYM("charly.vm.primitive.boolean"):          this->push_stack(this->primitive_boolean); return;
    case SYM("charly.vm.primitive.null"):             this->push_stack(this->primitive_null); return;
    case SYM("charly.vm.uncaught_exception_handler"): this->push_stack(this->uncaught_exception_handler); return;
    case SYM("charly.vm.internal_error_class"):       this->push_stack(this->internal_error_class); return;
    case SYM("charly.vm.globals"):                    this->push_stack(this->globals); return;
  }

  // Check globals table
  if (!charly_is_object(this->globals)) this->panic(Status::GlobalsNotAnObject);
  Object* globals_obj = charly_as_object(this->globals);
  if (globals_obj->container->count(symbol)) {
    this->push_stack(globals_obj->container->at(symbol));
    return;
  }

  this->throw_exception("Unidentified global symbol '" + SymbolTable::decode(symbol) + "'");
}

void VM::op_setlocalpush(uint32_t index, uint32_t level) {
  VALUE value = this->pop_stack();

  // Travel to the correct frame
  Frame* frame = this->frames;
  while (level--) {
    if (!frame->parent_environment_frame) {
      return this->panic(Status::WriteFailedTooDeep);
    }

    frame = frame->parent_environment_frame;
  }

  // Check if the index isn't out-of-bounds
  if (index < frame->lvarcount()) {
    frame->write_local(index, value);
  } else {
    this->panic(Status::WriteFailedOutOfBounds);
  }

  this->push_stack(value);
}

void VM::op_setmembersymbolpush(VALUE symbol) {
  VALUE value = this->pop_stack();
  VALUE target = this->pop_stack();
  this->push_stack(this->setmembersymbol(target, symbol, value));
}

void VM::op_setmembervaluepush() {
  VALUE value = this->pop_stack();
  VALUE member_value = this->pop_stack();
  VALUE target = this->pop_stack();
  this->push_stack(this->setmembervalue(target, member_value, value));
}

void VM::op_setarrayindexpush(uint32_t index) {
  VALUE expression = this->pop_stack();
  VALUE stackval = this->pop_stack();

  // Check if we popped an array, if not push it back onto the stack
  if (!charly_is_array(stackval)) {
    this->push_stack(stackval);
    return;
  }

  Array* arr = charly_as_array(stackval);

  // Out-of-bounds checking
  if (index >= arr->data->size()) {
    this->push_stack(kNull);
  }

  (*arr->data)[index] = expression;

  this->push_stack(stackval);
}

void VM::op_setlocal(uint32_t index, uint32_t level) {
  VALUE value = this->pop_stack();

  // Travel to the correct frame
  Frame* frame = this->frames;
  while (level--) {
    if (!frame->parent_environment_frame) {
      return this->panic(Status::WriteFailedTooDeep);
    }

    frame = frame->parent_environment_frame;
  }

  // Check if the index isn't out-of-bounds
  if (index < frame->lvarcount()) {
    frame->write_local(index, value);
  } else {
    this->panic(Status::WriteFailedOutOfBounds);
  }
}

void VM::op_setmembersymbol(VALUE symbol) {
  VALUE value = this->pop_stack();
  VALUE target = this->pop_stack();
  this->setmembersymbol(target, symbol, value);
}

void VM::op_setmembervalue() {
  VALUE value = this->pop_stack();
  VALUE member_value = this->pop_stack();
  VALUE target = this->pop_stack();
  this->setmembervalue(target, member_value, value);
}

void VM::op_setarrayindex(uint32_t index) {
  VALUE expression = this->pop_stack();
  VALUE stackval = this->pop_stack();

  // Check if we popped an array, if not push it back onto the stack
  if (!charly_is_array(stackval)) {
    this->push_stack(stackval);
    return;
  }

  Array* arr = charly_as_array(stackval);

  // Out-of-bounds checking
  if (index >= arr->data->size()) {
    this->push_stack(kNull);
  }

  (*arr->data)[index] = expression;
}

void VM::op_setglobal(VALUE symbol) {
  VALUE value = this->pop_stack();

  // Check internal methods table
  if (Internals::Index::methods.count(symbol)) {
    this->throw_exception("Cannot overwrite internal vm methods");
    return;
  }


  // Check vm specific symbols
  switch (symbol) {
    case SYM("charly.vm.primitive.value"):            this->primitive_value            = value; return;
    case SYM("charly.vm.primitive.object"):           this->primitive_object           = value; return;
    case SYM("charly.vm.primitive.class"):            this->primitive_class            = value; return;
    case SYM("charly.vm.primitive.array"):            this->primitive_array            = value; return;
    case SYM("charly.vm.primitive.string"):           this->primitive_string           = value; return;
    case SYM("charly.vm.primitive.number"):           this->primitive_number           = value; return;
    case SYM("charly.vm.primitive.function"):         this->primitive_function         = value; return;
    case SYM("charly.vm.primitive.boolean"):          this->primitive_boolean          = value; return;
    case SYM("charly.vm.primitive.null"):             this->primitive_null             = value; return;
    case SYM("charly.vm.uncaught_exception_handler"): this->uncaught_exception_handler = value; return;
    case SYM("charly.vm.internal_error_class"):       this->internal_error_class       = value; return;
    case SYM("charly.vm.globals"):                    this->globals                    = value; return;
  }

  // Check globals table
  if (!charly_is_object(this->globals)) this->panic(Status::GlobalsNotAnObject);
  Object* globals_obj = charly_as_object(this->globals);
  if (globals_obj->container->count(symbol)) {
    globals_obj->container->insert_or_assign(symbol, value);
    return;
  }

  this->throw_exception("Unidentified global symbol '" + SymbolTable::decode(symbol) + "'");
}

void VM::op_setglobalpush(VALUE symbol) {
  VALUE value = this->pop_stack();
  this->push_stack(value);

  // Check internal methods table
  if (Internals::Index::methods.count(symbol)) {
    this->throw_exception("Cannot overwrite internal vm methods");
    return;
  }

  // Check vm specific symbols
  switch (symbol) {
    case SYM("charly.vm.primitive.value"):            this->primitive_value            = value; return;
    case SYM("charly.vm.primitive.object"):           this->primitive_object           = value; return;
    case SYM("charly.vm.primitive.class"):            this->primitive_class            = value; return;
    case SYM("charly.vm.primitive.array"):            this->primitive_array            = value; return;
    case SYM("charly.vm.primitive.string"):           this->primitive_string           = value; return;
    case SYM("charly.vm.primitive.number"):           this->primitive_number           = value; return;
    case SYM("charly.vm.primitive.function"):         this->primitive_function         = value; return;
    case SYM("charly.vm.primitive.boolean"):          this->primitive_boolean          = value; return;
    case SYM("charly.vm.primitive.null"):             this->primitive_null             = value; return;
    case SYM("charly.vm.uncaught_exception_handler"): this->uncaught_exception_handler = value; return;
    case SYM("charly.vm.internal_error_class"):       this->internal_error_class       = value; return;
    case SYM("charly.vm.globals"):                    this->globals                    = value; return;
  }

  // Check globals table
  if (!charly_is_object(this->globals))
    this->panic(Status::GlobalsNotAnObject);
  Object* globals_obj = charly_as_object(this->globals);
  if (globals_obj->container->count(symbol)) {
    globals_obj->container->insert_or_assign(symbol, value);
    return;
  }

  this->throw_exception("Unidentified global symbol '" + SymbolTable::decode(symbol) + "'");
}
#undef WORDGLOB

void VM::op_putself() {
  if (this->frames == nullptr) {
    this->push_stack(kNull);
    return;
  }

  this->push_stack(this->frames->self);
}

void VM::op_putsuper() {
  Function* func = this->get_active_function();
  if (!func) this->panic(Status::InvalidArgumentType);
  if (!charly_is_class(func->host_class)) this->panic(Status::InvalidArgumentType);

  Class* host_class = charly_as_class(func->host_class);

  // A super call inside a constructor will return the super constructor
  // A super call inside a member function will return the super function of
  // the currently active function
  VALUE super_value = kNull;
  if (func->name == SYM("constructor")) {
    super_value = charly_find_super_constructor(host_class);
  } else {
    super_value = charly_find_super_method(host_class, func->name);
  }

  // If we've got a function, we need to make a copy of it and bind the currently
  // active self value to it
  if (charly_is_function(super_value)) {
    Function* copied_super_value = charly_as_function(this->copy_function(super_value));
    copied_super_value->bound_self_set = true;
    copied_super_value->bound_self = this->frames->self;
    super_value = charly_create_pointer(copied_super_value);
  }

  this->push_stack(super_value);
}

void VM::op_putsupermember(VALUE symbol) {
  Function* func = this->get_active_function();
  if (!func) this->panic(Status::InvalidArgumentType);
  if (!charly_is_class(func->host_class)) this->panic(Status::InvalidArgumentType);

  Class* host_class = charly_as_class(func->host_class);
  VALUE super_value = charly_find_super_method(host_class, symbol);

  // If we've got a function, we need to make a copy of it and bind the currently
  // active self value to it
  if (charly_is_function(super_value)) {
    Function* copied_super_value = charly_as_function(this->copy_function(super_value));
    copied_super_value->bound_self_set = true;
    copied_super_value->bound_self = this->frames->self;
    super_value = charly_create_pointer(copied_super_value);
  }

  this->push_stack(super_value);
}

void VM::op_putvalue(VALUE value) {
  this->push_stack(value);
}

void VM::op_putstring(char* data, uint32_t length) {
  this->push_stack(this->create_string(data, length));
}

void VM::op_putfunction(VALUE symbol,
                        uint8_t* body_address,
                        bool anonymous,
                        bool needs_arguments,
                        uint32_t argc,
                        uint32_t minimum_argc,
                        uint32_t lvarcount) {
  VALUE function = this->create_function(symbol, body_address, argc, minimum_argc, lvarcount, anonymous, needs_arguments);
  this->push_stack(function);
}

void VM::op_putarray(uint32_t count) {
  Array* array = charly_as_array(this->create_array(count));

  while (count--) {
    array->data->insert(array->data->begin(), this->pop_stack());
  }

  this->push_stack(charly_create_pointer(array));
}

void VM::op_puthash(uint32_t count) {
  Object* object = charly_as_object(this->create_object(count));

  VALUE key;
  VALUE value;
  while (count--) {
    key = this->pop_stack();
    value = this->pop_stack();
    (*object->container)[key] = value;
  }

  this->push_stack(charly_create_pointer(object));
}

void VM::op_putclass(VALUE name,
                     uint32_t propertycount,
                     uint32_t staticpropertycount,
                     uint32_t methodcount,
                     uint32_t staticmethodcount,
                     bool has_parent_class,
                     bool has_constructor) {
  ManagedContext lalloc(this);

  Class* klass = charly_as_class(lalloc.create_class(name));
  klass->member_properties->reserve(propertycount);
  klass->prototype = lalloc.create_object(methodcount);
  klass->container->reserve(staticpropertycount + staticmethodcount);

  bool parent_class_invalid_type = false;

  if (has_constructor) {
    klass->constructor = this->pop_stack();
    if (!charly_is_function(klass->constructor)) {
      this->panic(Status::InvalidArgumentType);
    }
    Function* constructor = charly_as_function(klass->constructor);
    constructor->host_class = charly_create_pointer(klass);
  }

  if (has_parent_class) {
    VALUE parent_class = this->pop_stack();
    parent_class_invalid_type = !charly_is_class(parent_class);
    klass->parent_class = parent_class;
  } else {
    klass->parent_class = this->primitive_object;
  }

  while (staticmethodcount--) {
    VALUE smethod = this->pop_stack();
    if (!charly_is_function(smethod)) {
      this->panic(Status::InvalidArgumentType);
    }
    Function* func_smethod = charly_as_function(smethod);
    func_smethod->host_class = charly_create_pointer(klass);
    (*klass->container)[func_smethod->name] = smethod;
  }

  while (methodcount--) {
    VALUE method = this->pop_stack();
    if (!charly_is_function(method)) {
      this->panic(Status::InvalidArgumentType);
    }
    Function* func_method = charly_as_function(method);
    func_method->host_class = charly_create_pointer(klass);
    Object* obj_methods = charly_as_object(klass->prototype);
    (*obj_methods->container)[func_method->name] = method;
  }

  while (staticpropertycount--) {
    VALUE sprop = this->pop_stack();
    if (!charly_is_symbol(sprop)) {
      this->panic(Status::InvalidArgumentType);
    }
    (*klass->container)[sprop] = kNull;
  }

  while (propertycount--) {
    VALUE prop = this->pop_stack();
    if (!charly_is_symbol(prop)) {
      this->panic(Status::InvalidArgumentType);
    }
    klass->member_properties->push_back(prop);
  }

  // Make sure the parent class is a class
  if (parent_class_invalid_type) {
    this->throw_exception("Can't extend from non class value");
  } else {
    this->push_stack(charly_create_pointer(klass));
  }
}

void VM::op_pop() {
  this->pop_stack();
}

void VM::op_dup() {
  this->push_stack(this->stack.back());
}

void VM::op_dupn(uint32_t count) {
  VALUE buffer[count];
  size_t off = 0;

  // Pop count elements off the stack into our local buffer
  while (count--) {
    buffer[off++] = this->pop_stack();
  }

  // Push them back onto the stack twice
  size_t backup_off = off;
  while (off--) {
    this->push_stack(buffer[off]);
  }
  off = backup_off;
  while (off--) {
    this->push_stack(buffer[off]);
  }
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

void VM::op_new(uint32_t argc) {

  // Load arguments from stack
  VALUE tmp_args[argc];
  for (uint32_t i = 0; i < argc; i++) {
    tmp_args[i] = this->pop_stack();
  }

  // Load klass from stack
  VALUE klass = this->pop_stack();
  if (!charly_is_class(klass)) {
    this->throw_exception("Value is not a class");
    return;
  }

  // Setup object
  ManagedContext lalloc(this);
  uint32_t member_property_count = charly_as_class(klass)->member_properties->size();
  Object* obj = charly_as_object(lalloc.create_object(member_property_count));
  obj->klass = klass;
  this->initialize_member_properties(charly_as_class(klass), obj);

  // Call initial constructor if there is one
  // else just return the object
  VALUE initial_constructor = charly_find_constructor(charly_as_class(klass));
  if (charly_is_function(initial_constructor)) {

    // Check if there are enough arguments
    if (charly_as_function(initial_constructor)->minimum_argc > argc) {
      std::string class_name = SymbolTable::decode(charly_as_class(klass)->name);
      this->throw_exception("Not enough arguments for class constructor: " + class_name);
      return;
    }

    this->push_stack(charly_create_pointer(obj));
    this->push_stack(initial_constructor);
    for (int i = argc - 1; i >= 0; i--) {
      this->push_stack(tmp_args[i]);
    }
    this->call(argc, true);
  } else {
    this->push_stack(charly_create_pointer(obj));
  }
}

void VM::op_return() {
  Frame* frame = this->frames;
  if (!frame)
    this->panic(Status::CantReturnFromTopLevel);

  this->catchstack = frame->last_active_catchtable;
  this->frames = frame->parent;
  this->ip = frame->return_address;

  if (frame->halt_after_return) {
    this->halted = true;
  }

  // Print the frame if the correponding flag was set
  if (this->context.trace_frames) {
    this->context.err_stream << "Left frame: ";
    this->pretty_print(this->context.err_stream, charly_create_pointer(frame));
    this->context.err_stream << '\n';
  }
}

void VM::op_yield() {
  this->throw_exception("Unused opcode: Yield");
}

void VM::op_throw() {
  return this->throw_exception(this->pop_stack());
}

void VM::throw_exception(const std::string& message) {

  // Special exception logic when coming from a worker thread
  if (this->is_worker_thread()) {
    this->handle_worker_thread_exception(message);
    return;
  }

  Frame* old_frame = this->frames;

  // Load the error class
  VALUE error_class_val = this->internal_error_class;
  if (charly_is_class(error_class_val)) {

    // Instantiate error object
    this->push_stack(error_class_val);
    this->push_stack(this->create_string(message.c_str(), message.size()));
    this->push_stack(charly_create_integer(reinterpret_cast<size_t>(old_frame)));
    this->op_new(2);
  } else {
    this->unwind_catchstack(this->create_string(message.c_str(), message.size()));
  }
}

void VM::throw_exception(VALUE payload) {
  this->unwind_catchstack(payload);
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
      this->context.err_stream << "Restored CatchTable: ";
      this->pretty_print(this->context.err_stream, charly_create_pointer(table));
      this->context.err_stream << '\n';
    }
  }
}

void VM::op_branch(int32_t offset) {
  this->ip += offset;
}

void VM::op_branchif(int32_t offset) {
  VALUE test = this->pop_stack();
  if (charly_truthyness(test))
    this->ip += offset;
}

void VM::op_branchunless(int32_t offset) {
  VALUE test = this->pop_stack();
  if (!charly_truthyness(test))
    this->ip += offset;
}

void VM::op_branchlt(int32_t offset) {
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  if (this->lt(left, right) == kTrue)
    this->ip += offset;
}

void VM::op_branchgt(int32_t offset) {
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  if (this->gt(left, right) == kTrue)
    this->ip += offset;
}

void VM::op_branchle(int32_t offset) {
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  if (this->le(left, right) == kTrue)
    this->ip += offset;
}

void VM::op_branchge(int32_t offset) {
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  if (this->ge(left, right) == kTrue)
    this->ip += offset;
}

void VM::op_brancheq(int32_t offset) {
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  if (this->eq(left, right) == kTrue)
    this->ip += offset;
}

void VM::op_branchneq(int32_t offset) {
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  if (this->neq(left, right) == kTrue)
    this->ip += offset;
}

void VM::op_typeof() {
  VALUE value = this->pop_stack();
  const std::string& stringrep = charly_get_typestring(value);
  this->push_stack(this->create_string(stringrep.data(), stringrep.size()));
}

void VM::stackdump(std::ostream& io) {
  for (VALUE stackitem : this->stack) {
    this->pretty_print(io, stackitem);
    io << '\n';
  }
}

// TODO: Move this message to value.h
// TODO: It will still require access to the symbol table, figure out how to remove this dependency
void VM::pretty_print(std::ostream& io, VALUE value) {
  // Check if this value was already printed before
  auto begin = this->pretty_print_stack.begin();
  auto end = this->pretty_print_stack.end();
  bool printed_before = std::find(begin, end, value) != end;

  switch (charly_get_type(value)) {
    case kTypeDead: {
      io << "<@";
      io << reinterpret_cast<void*>(value);
      io << " : Dead>";
      break;
    }

    case kTypeNumber: {
      const double d = io.precision();
      io.precision(16);
      io << std::fixed;
      if (charly_is_int(value)) {
        io << charly_int_to_int64(value);
      } else {
        io << charly_double_to_double(value);
      }
      io << std::scientific;
      io.precision(d);

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
      io << "\"";
      io.write(charly_string_data(value), charly_string_length(value));
      io << "\"";
      break;
    }

    case kTypeObject: {
      Object* object = charly_as_object(value);
      io << "<Object";

      // If this object was already printed, we avoid printing it again
      if (printed_before) {
        io << " ...>";
        break;
      }

      this->pretty_print_stack.push_back(value);

      for (auto& entry : *object->container) {
        io << " ";
        std::string key = SymbolTable::decode(entry.first);
        io << key << "=";
        this->pretty_print(io, entry.second);
      }

      io << ">";

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeArray: {
      Array* array = charly_as_array(value);
      io << "<Array ";

      // If this array was already printed, we avoid printing it again
      if (printed_before) {
        io << "[...]>";
        break;
      }

      this->pretty_print_stack.push_back(value);

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

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeFunction: {
      Function* func = charly_as_function(value);

      if (printed_before) {
        io << "<Function ...>";
        break;
      }

      this->pretty_print_stack.push_back(value);

      io << "<Function ";
      io << "name=";
      if (charly_is_class(func->host_class)) {
        Class* host_class = charly_as_class(func->host_class);
        this->pretty_print(io, host_class->name);
        io << ":";
      }
      this->pretty_print(io, func->name);
      io << " ";
      io << "argc=" << func->argc;
      io << " ";
      io << "minimum_argc=" << func->minimum_argc;
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
        std::string key = SymbolTable::decode(entry.first);
        io << key << "=";
        this->pretty_print(io, entry.second);
      }

      io << ">";

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeCFunction: {
      CFunction* func = charly_as_cfunction(value);

      if (printed_before) {
        io << "<CFunction ...>";
        break;
      }

      this->pretty_print_stack.push_back(value);

      io << "<CFunction ";
      io << "name=";
      this->pretty_print(io, func->name);
      io << " ";
      io << "argc=" << func->argc;
      io << " ";
      io << "pointer=" << reinterpret_cast<uint64_t*>(func->pointer) << "";

      for (auto& entry : *func->container) {
        io << " ";
        std::string key = SymbolTable::decode(entry.first);
        io << key << "=";
        this->pretty_print(io, entry.second);
      }

      io << ">";

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeClass: {
      Class* klass = charly_as_class(value);

      if (printed_before) {
        io << "<Class ...>";
        break;
      }

      this->pretty_print_stack.push_back(value);

      io << "<Class ";
      io << "name=";
      this->pretty_print(io, klass->name);
      io << " ";
      io << "constructor=";
      this->pretty_print(io, klass->constructor);
      io << " ";

      io << "member_properties=[";
      for (auto entry : *klass->member_properties) {
        io << " " << SymbolTable::decode(entry);
      }
      io << "] ";

      io << "member_functions=";
      this->pretty_print(io, klass->prototype);
      io << " ";

      io << "parent_class=";
      this->pretty_print(io, klass->parent_class);
      io << " ";

      for (auto entry : *klass->container) {
        io << " " << SymbolTable::decode(entry.first);
        io << "=";
        this->pretty_print(io, entry.second);
      }

      io << ">";

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeCPointer: {
      CPointer* cpointer = charly_as_cpointer(value);
      io << "<CPointer " << reinterpret_cast<void*>(cpointer->data);
      io << ":" << reinterpret_cast<void*>(cpointer->destructor);
      io << ">";
      break;
    }

    case kTypeSymbol: {
      io << SymbolTable::decode(value);
      break;
    }

    case kTypeFrame: {
      Frame* frame = charly_as_frame(value);
      VALUE callee = frame->caller_value;

      // Get the name of the calling value
      VALUE name = kNull;
      switch (charly_get_type(callee)) {
        case kTypeFunction: {
          Function* fn = charly_as_function(callee);
          if (fn->anonymous) {
            name = SymbolTable::encode("<anonymous>");
          } else {
            name = charly_as_function(callee)->name; break;
          }

          break;
        }
        case kTypeCFunction: name = charly_as_cfunction(callee)->name; break;
        default: name = charly_create_istring(kUndefinedSymbolString);
      }

      // Get the body address of the calling value
      void* body_address = nullptr;
      switch (charly_get_type(callee)) {
        case kTypeFunction:   body_address = charly_as_function(callee)->body_address; break;
        case kTypeCFunction:  body_address = charly_as_cfunction(callee)->pointer; break;
        default:              body_address = nullptr;
      }

      // Lookup the name of the file this address is from
      auto lookup_result =
          this->context.compiler_manager.address_mapping.resolve_address(reinterpret_cast<uint8_t*>(body_address));

      io << "(";
      io << std::setfill(' ') << std::setw(14);
      io << body_address;
      io << std::setw(1);
      io << ") ";
      io << "(";
      io << std::setw(10);
      io << charly_get_typestring(callee);
      io << std::setw(1);
      io << ") ";
      io << SymbolTable::decode(name);
      io << " ";
      io << lookup_result.value_or("??");

      break;
    }

    case kTypeCatchTable: {
      CatchTable* table = charly_as_catchtable(value);

      if (printed_before) {
        io << "<CatchTable ...>";
        break;
      }

      this->pretty_print_stack.push_back(value);

      io << "<CatchTable ";
      io << "address=" << reinterpret_cast<void*>(table->address) << " ";
      io << "stacksize=" << table->stacksize << " ";
      io << "frame=" << table->frame << " ";
      io << "parent=" << table->parent;
      io << ">";

      this->pretty_print_stack.pop_back();
      break;
    }
  }
}

void VM::to_s(std::ostream& io, VALUE value, uint32_t depth, bool inside_container) {
  // Check if this value was already printed before
  auto begin = this->pretty_print_stack.begin();
  auto end = this->pretty_print_stack.end();
  bool printed_before = std::find(begin, end, value) != end;

  switch (charly_get_type(value)) {
    case kTypeDead: {
      if (this->context.verbose_addresses) io << "@" << reinterpret_cast<void*>(value) << ":";
      io << "<dead>";
      break;
    }

    case kTypeNumber: {
      const double d = io.precision();
      io.precision(16);
      io << std::fixed;
      if (charly_is_int(value)) {
        io << charly_int_to_int64(value);
      } else {
        io << charly_double_to_double(value);
      }
      io << std::scientific;
      io.precision(d);

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
      if (this->context.verbose_addresses) io << "@" << reinterpret_cast<void*>(value) << ":";
      if (inside_container) io << "\"";
      io.write(charly_string_data(value), charly_string_length(value));
      if (inside_container) io << "\"";
      break;
    }

    case kTypeObject: {
      Object* object = charly_as_object(value);

      // If this object was already printed, we avoid printing it again
      if (printed_before) {
        io << "{...}";
        break;
      }

      this->pretty_print_stack.push_back(value);

      if (charly_is_class(object->klass)) {
        Class* klass = charly_as_class(object->klass);
        this->to_s(io, klass->name);
      }

      if (this->context.verbose_addresses) io << "@" << reinterpret_cast<void*>(value) << ":";
      io << "{\n";

      for (auto& entry : *object->container) {
        io << std::string(depth + 2, ' ');
        std::string key = SymbolTable::decode(entry.first);
        io << key << " = ";
        this->to_s(io, entry.second, depth + 2, true);
        io << '\n';
      }

      io << std::string(depth == 0 ? 0 : depth, ' ');
      io << "}";

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeArray: {
      Array* array = charly_as_array(value);
      if (this->context.verbose_addresses) io << "@" << reinterpret_cast<void*>(value) << ":";

      // If this array was already printed, we avoid printing it again
      if (printed_before) {
        io << "[...]";
        break;
      }

      this->pretty_print_stack.push_back(value);

      io << "[";

      bool first = false;
      for (VALUE& entry : *array->data) {
        if (first) {
          io << ", ";
        } else {
          first = true;
        }

        this->to_s(io, entry, depth, true);
      }

      io << "]";

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeFunction: {
      Function* func = charly_as_function(value);
      if (this->context.verbose_addresses) io << "@" << reinterpret_cast<void*>(value) << ":";

      if (printed_before) {
        io << "<Function ...>";
        break;
      }

      this->pretty_print_stack.push_back(value);

      io << "<Function ";
      io << "";
      if (charly_is_class(func->host_class)) {
        Class* host_class = charly_as_class(func->host_class);
        this->to_s(io, host_class->name);
        io << ":";
      }
      this->to_s(io, func->name);
      io << "#" << func->minimum_argc;

      for (auto& entry : *func->container) {
        io << " ";
        std::string key = SymbolTable::decode(entry.first);
        io << key << "=";
        this->to_s(io, entry.second, depth, true);
      }

      io << ">";

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeCFunction: {
      CFunction* func = charly_as_cfunction(value);
      if (this->context.verbose_addresses) io << "@" << reinterpret_cast<void*>(value) << ":";

      if (printed_before) {
        io << "<CFunction ...>";
        break;
      }

      this->pretty_print_stack.push_back(value);

      io << "<CFunction ";
      this->to_s(io, func->name, depth);
      io << "#" << func->argc;

      for (auto& entry : *func->container) {
        io << " ";
        std::string key = SymbolTable::decode(entry.first);
        io << key << "=";
        this->to_s(io, entry.second, depth, true);
      }

      io << ">";

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeClass: {
      Class* klass = charly_as_class(value);
      if (this->context.verbose_addresses) io << "@" << reinterpret_cast<void*>(value) << ":";

      if (printed_before) {
        io << "<Class ...>";
        break;
      }

      this->pretty_print_stack.push_back(value);

      io << "<Class ";
      this->to_s(io, klass->name, depth);

      for (auto entry : *klass->container) {
        io << " " << SymbolTable::decode(entry.first);
        io << "=";
        this->to_s(io, entry.second, depth, true);
      }

      io << ">";

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeCPointer: {
      if (this->context.verbose_addresses) io << "@" << reinterpret_cast<void*>(value) << ":";
      io << "<CPointer>";
      break;
    }

    case kTypeSymbol: {
      if (this->context.verbose_addresses) io << "@" << reinterpret_cast<void*>(value) << ":";
      io << SymbolTable::decode(value);
      break;
    }

    case kTypeFrame: {
      if (this->context.verbose_addresses) io << "@" << reinterpret_cast<void*>(value) << ":";
      io << "<Frame>";
      break;
    }

    default: {
      if (this->context.verbose_addresses) io << "@" << reinterpret_cast<void*>(value) << ":";
      io << "<?>";
      break;
    }
  }
}

VALUE VM::get_self_for_function(Function* function, const VALUE* fallback) {
  if (function->bound_self_set) return function->bound_self;
  if (function->anonymous) return function->context ? function->context->self : kNull;
  if (fallback != nullptr) return *fallback;
  return function->context ? function->context->self : kNull;
}

VALUE VM::get_global_self() {
  return this->get_global_symbol(SYM("Charly"));
}

VALUE VM::get_global_symbol(VALUE symbol) {
  if (charly_is_object(this->globals)) {
    Object* globals_obj = charly_as_object(this->globals);
    if (globals_obj->container->count(symbol))
      return globals_obj->container->at(symbol);
  }

  return kNull;
}

Function* VM::get_active_function() {
  VALUE caller = this->frames->caller_value;

  if (charly_is_function(caller)) {
    return charly_as_function(caller);
  } else {
    return nullptr;
  }
}

void VM::panic(STATUS reason) {
  this->context.err_stream << "Panic: " << kStatusHumanReadable[reason] << '\n';
  this->context.err_stream << "IP: ";
  this->context.err_stream.fill('0');                                                                \
  this->context.err_stream << "0x" << std::hex;                                                      \
  this->context.err_stream << std::setw(12) << reinterpret_cast<uint64_t>(this->ip) << std::setw(1); \
  this->context.err_stream << std::dec;                                                              \
  this->context.err_stream.fill(' ');                                                                \
  this->context.err_stream << '\n' << "Stackdump:" << '\n';
  this->stackdump(this->context.err_stream);

  this->exit(1);
  this->context.err_stream << "Aborting the charly runtime!" << '\n';
  std::abort();
}

void VM::run() {
  this->halted = false;

  // High resolution clock used to track how long instructions take
  std::chrono::time_point<std::chrono::high_resolution_clock> exec_start;
  Opcode opcode = Opcode::Halt;
  uint8_t* old_ip = this->ip;

// Runs at the beginning of every instruction
#define OPCODE_PROLOGUE()                                                                              \
  if (this->halted)                                                                                    \
    return;                                                                                            \
  if (this->context.instruction_profile) {                                                             \
    exec_start = std::chrono::high_resolution_clock::now();                                            \
  }                                                                                                    \
  if (this->ip == nullptr) {                                                                           \
    this->panic(Status::InvalidInstructionPointer);                                                    \
  }                                                                                                    \
  if (this->context.trace_opcodes) {                                                                   \
    this->context.err_stream.fill('0');                                                                \
    this->context.err_stream << "0x" << std::hex;                                                      \
    this->context.err_stream << std::setw(12) << reinterpret_cast<uint64_t>(this->ip) << std::setw(1); \
    this->context.err_stream << std::dec;                                                              \
    this->context.err_stream.fill(' ');                                                                \
    this->context.err_stream << ": " << kOpcodeMnemonics[opcode] << '\n';                              \
  }

// Runs at the end of each instruction
#define OPCODE_EPILOGUE()                                                                                 \
  if (this->context.instruction_profile) {                                                                \
    std::chrono::duration<double> exec_duration = std::chrono::high_resolution_clock::now() - exec_start; \
    uint64_t duration_in_nanoseconds = static_cast<uint32_t>(exec_duration.count() * 1000000000);         \
    if (this->context.instruction_profile) {                                                              \
      this->instruction_profile.add_entry(opcode, duration_in_nanoseconds);                               \
    }                                                                                                     \
  }

// Increment the instruction pointer
#define INCIP() this->ip += kInstructionLengths[opcode];
#define CONDINCIP()       \
  if (this->ip == old_ip) \
    this->ip += kInstructionLengths[opcode];

// Dispatch control to the next instruction handler
#define DISPATCH()                    \
  opcode = this->fetch_instruction(); \
  old_ip = this->ip;                  \
  goto* OPCODE_DISPATCH_TABLE[opcode];
#define DISPATCH_POSSIBLY_NULL()      \
  if (this->ip == nullptr)            \
    return;                           \
  opcode = this->fetch_instruction(); \
  old_ip = this->ip;                  \
  goto* OPCODE_DISPATCH_TABLE[opcode];
#define NEXTOP() \
  INCIP();       \
  DISPATCH();

  // Dispatch table used for the computed goto main interpreter switch
  static void* OPCODE_DISPATCH_TABLE[] = {&&charly_main_switch_nop,
                                          &&charly_main_switch_readlocal,
                                          &&charly_main_switch_readmembersymbol,
                                          &&charly_main_switch_readmembervalue,
                                          &&charly_main_switch_readarrayindex,
                                          &&charly_main_switch_readglobal,
                                          &&charly_main_switch_setlocalpush,
                                          &&charly_main_switch_setmembersymbolpush,
                                          &&charly_main_switch_setmembervaluepush,
                                          &&charly_main_switch_setarrayindexpush,
                                          &&charly_main_switch_setlocal,
                                          &&charly_main_switch_setmembersymbol,
                                          &&charly_main_switch_setmembervalue,
                                          &&charly_main_switch_setarrayindex,
                                          &&charly_main_switch_setglobal,
                                          &&charly_main_switch_setglobalpush,
                                          &&charly_main_switch_putself,
                                          &&charly_main_switch_putsuper,
                                          &&charly_main_switch_putsupermember,
                                          &&charly_main_switch_putvalue,
                                          &&charly_main_switch_putstring,
                                          &&charly_main_switch_putfunction,
                                          &&charly_main_switch_putarray,
                                          &&charly_main_switch_puthash,
                                          &&charly_main_switch_putclass,
                                          &&charly_main_switch_pop,
                                          &&charly_main_switch_dup,
                                          &&charly_main_switch_dupn,
                                          &&charly_main_switch_swap,
                                          &&charly_main_switch_call,
                                          &&charly_main_switch_callmember,
                                          &&charly_main_switch_new,
                                          &&charly_main_switch_return,
                                          &&charly_main_switch_yield,
                                          &&charly_main_switch_throw,
                                          &&charly_main_switch_registercatchtable,
                                          &&charly_main_switch_popcatchtable,
                                          &&charly_main_switch_branch,
                                          &&charly_main_switch_branchif,
                                          &&charly_main_switch_branchunless,
                                          &&charly_main_switch_branchlt,
                                          &&charly_main_switch_branchgt,
                                          &&charly_main_switch_branchle,
                                          &&charly_main_switch_branchge,
                                          &&charly_main_switch_brancheq,
                                          &&charly_main_switch_branchneq,
                                          &&charly_main_switch_add,
                                          &&charly_main_switch_sub,
                                          &&charly_main_switch_mul,
                                          &&charly_main_switch_div,
                                          &&charly_main_switch_mod,
                                          &&charly_main_switch_pow,
                                          &&charly_main_switch_eq,
                                          &&charly_main_switch_neq,
                                          &&charly_main_switch_lt,
                                          &&charly_main_switch_gt,
                                          &&charly_main_switch_le,
                                          &&charly_main_switch_ge,
                                          &&charly_main_switch_shr,
                                          &&charly_main_switch_shl,
                                          &&charly_main_switch_band,
                                          &&charly_main_switch_bor,
                                          &&charly_main_switch_bxor,
                                          &&charly_main_switch_uadd,
                                          &&charly_main_switch_usub,
                                          &&charly_main_switch_unot,
                                          &&charly_main_switch_ubnot,
                                          &&charly_main_switch_halt,
                                          &&charly_main_switch_gccollect,
                                          &&charly_main_switch_typeof};

  DISPATCH();
charly_main_switch_nop : {
  OPCODE_PROLOGUE();
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_readlocal : {
  OPCODE_PROLOGUE();
  uint32_t index = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  uint32_t level = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(uint32_t));
  this->op_readlocal(index, level);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_readmembersymbol : {
  OPCODE_PROLOGUE();
  VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
  this->op_readmembersymbol(symbol);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_readmembervalue : {
  OPCODE_PROLOGUE();
  this->op_readmembervalue();
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_readarrayindex : {
  OPCODE_PROLOGUE();
  uint32_t index = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  this->op_readarrayindex(index);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_readglobal : {
  OPCODE_PROLOGUE();
  VALUE symbol = *reinterpret_cast<uint64_t*>(this->ip + sizeof(Opcode));
  this->op_readglobal(symbol);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_setlocalpush : {
  OPCODE_PROLOGUE();
  uint32_t index = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  uint32_t level = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(uint32_t));
  this->op_setlocalpush(index, level);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_setmembersymbolpush : {
  OPCODE_PROLOGUE();
  VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
  this->op_setmembersymbolpush(symbol);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_setmembervaluepush : {
  OPCODE_PROLOGUE();
  this->op_setmembervaluepush();
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_setarrayindexpush : {
  OPCODE_PROLOGUE();
  uint32_t index = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  this->op_setarrayindexpush(index);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_setlocal : {
  OPCODE_PROLOGUE();
  uint32_t index = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  uint32_t level = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(uint32_t));
  this->op_setlocal(index, level);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_setmembersymbol : {
  OPCODE_PROLOGUE();
  VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
  this->op_setmembersymbol(symbol);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_setmembervalue : {
  OPCODE_PROLOGUE();
  this->op_setmembervalue();
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_setarrayindex : {
  OPCODE_PROLOGUE();
  uint32_t index = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  this->op_setarrayindex(index);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_setglobal : {
  OPCODE_PROLOGUE();
  VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
  this->op_setglobal(symbol);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_setglobalpush : {
  OPCODE_PROLOGUE();
  VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
  this->op_setglobalpush(symbol);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_putself : {
  OPCODE_PROLOGUE();
  this->op_putself();
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_putsuper : {
  OPCODE_PROLOGUE();
  this->op_putsuper();
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_putsupermember : {
  OPCODE_PROLOGUE();
  VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
  this->op_putsupermember(symbol);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_putvalue : {
  OPCODE_PROLOGUE();
  VALUE value = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
  this->op_putvalue(value);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_putstring : {
  OPCODE_PROLOGUE();
  uint32_t offset = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  uint32_t length = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(uint32_t));

  // We assume the compiler generated valid offsets and lengths, so we don't do
  // any out-of-bounds checking here
  // TODO: Should we do out-of-bounds checking here?
  char* str_start = StringPool::get_char_ptr(offset);
  this->op_putstring(str_start, length);

  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_putfunction : {
  OPCODE_PROLOGUE();
  VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
  int32_t body_offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE));
  bool anonymous = *reinterpret_cast<bool*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(int32_t));
  bool needs_arguments =
      *reinterpret_cast<bool*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(int32_t) + sizeof(bool));
  uint32_t argc = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(int32_t) +
                                               sizeof(bool) + sizeof(bool));
  uint32_t minimum_argc = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(int32_t) +
                                               sizeof(bool) + sizeof(bool) + sizeof(uint32_t));
  uint32_t lvarcount = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(int32_t) +
                                                    sizeof(bool) + sizeof(bool) + sizeof(uint32_t) + sizeof(uint32_t));

  this->op_putfunction(symbol, this->ip + body_offset, anonymous, needs_arguments, argc, minimum_argc, lvarcount);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_putarray : {
  OPCODE_PROLOGUE();
  uint32_t count = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  this->op_putarray(count);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_puthash : {
  OPCODE_PROLOGUE();
  uint32_t size = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  this->op_puthash(size);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_putclass : {
  OPCODE_PROLOGUE();
  VALUE name = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
  uint32_t propertycount = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode) + sizeof(VALUE));
  uint32_t staticpropertycount =
      *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(uint32_t));
  uint32_t methodcount =
      *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(uint32_t) + sizeof(uint32_t));
  uint32_t staticmethodcount = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) +
                                                            sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t));
  bool has_parent_class = *reinterpret_cast<bool*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(uint32_t) +
                                                   sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t));
  bool has_constructor =
      *reinterpret_cast<bool*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(uint32_t) + sizeof(uint32_t) +
                               sizeof(uint32_t) + sizeof(uint32_t) + sizeof(bool));
  this->op_putclass(name, propertycount, staticpropertycount, methodcount, staticmethodcount, has_parent_class,
                    has_constructor);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_pop : {
  OPCODE_PROLOGUE();
  this->op_pop();
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_dup : {
  OPCODE_PROLOGUE();
  this->op_dup();
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_dupn : {
  OPCODE_PROLOGUE();
  uint32_t count = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  this->op_dupn(count);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_swap : {
  OPCODE_PROLOGUE();
  this->op_swap();
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_call : {
  OPCODE_PROLOGUE();
  uint32_t argc = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  this->op_call(argc);
  OPCODE_EPILOGUE();
  CONDINCIP();
  if (this->ip == nullptr) {
    this->panic(Status::InvalidInstructionPointer);
  }
  DISPATCH();
}

charly_main_switch_callmember : {
  OPCODE_PROLOGUE();
  uint32_t argc = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  this->op_callmember(argc);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_new : {
  OPCODE_PROLOGUE();
  uint32_t argc = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  this->op_new(argc);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_return : {
  OPCODE_PROLOGUE();
  this->op_return();
  OPCODE_EPILOGUE();
  DISPATCH_POSSIBLY_NULL();
}

charly_main_switch_yield : {
  OPCODE_PROLOGUE();
  this->op_yield();
  OPCODE_EPILOGUE();
  DISPATCH_POSSIBLY_NULL();
}

charly_main_switch_throw : {
  OPCODE_PROLOGUE();
  this->op_throw();
  OPCODE_EPILOGUE();
  DISPATCH();
}

charly_main_switch_registercatchtable : {
  OPCODE_PROLOGUE();
  int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
  this->op_registercatchtable(offset);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_popcatchtable : {
  OPCODE_PROLOGUE();
  this->op_popcatchtable();
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_branch : {
  OPCODE_PROLOGUE();
  int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
  this->op_branch(offset);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_branchif : {
  OPCODE_PROLOGUE();
  int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
  this->op_branchif(offset);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_branchunless : {
  OPCODE_PROLOGUE();
  int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
  this->op_branchunless(offset);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_branchlt : {
  OPCODE_PROLOGUE();
  int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
  this->op_branchlt(offset);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_branchgt : {
  OPCODE_PROLOGUE();
  int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
  this->op_branchgt(offset);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_branchle : {
  OPCODE_PROLOGUE();
  int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
  this->op_branchle(offset);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_branchge : {
  OPCODE_PROLOGUE();
  int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
  this->op_branchge(offset);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_brancheq : {
  OPCODE_PROLOGUE();
  int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
  this->op_brancheq(offset);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_branchneq : {
  OPCODE_PROLOGUE();
  int32_t offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode));
  this->op_branchneq(offset);
  OPCODE_EPILOGUE();
  CONDINCIP();
  DISPATCH();
}

charly_main_switch_add : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->add(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_sub : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->sub(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_mul : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->mul(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_div : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->div(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_mod : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->mod(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_pow : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->pow(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_uadd : {
  OPCODE_PROLOGUE();
  VALUE value = this->pop_stack();
  this->push_stack(this->uadd(value));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_usub : {
  OPCODE_PROLOGUE();
  VALUE value = this->pop_stack();
  this->push_stack(this->usub(value));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_eq : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->eq(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_neq : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->neq(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_lt : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->lt(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_gt : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->gt(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_le : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->le(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_ge : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->ge(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_unot : {
  OPCODE_PROLOGUE();
  VALUE value = this->pop_stack();
  this->push_stack(this->unot(value));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_shr : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->shr(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_shl : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->shl(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_band : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->band(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_bor : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->bor(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_bxor : {
  OPCODE_PROLOGUE();
  VALUE right = this->pop_stack();
  VALUE left = this->pop_stack();
  this->push_stack(this->bxor(left, right));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_ubnot : {
  OPCODE_PROLOGUE();
  VALUE value = this->pop_stack();
  this->push_stack(this->ubnot(value));
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_halt : {
  OPCODE_PROLOGUE();
  this->halted = true;
  OPCODE_EPILOGUE();
  return;
}

charly_main_switch_gccollect : {
  OPCODE_PROLOGUE();
  this->gc.do_collect();
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_typeof : {
  OPCODE_PROLOGUE();
  this->op_typeof();
  OPCODE_EPILOGUE();
  NEXTOP();
}
}

uint8_t VM::start_runtime() {
  this->starttime = std::chrono::high_resolution_clock::now();

  while (this->running) {
    Timestamp now = std::chrono::steady_clock::now();

    // Add all expired timers and tickers to the task_queue
    if (this->timers.size()) {
      auto it = this->timers.begin();
      while (it != this->timers.end() && it->first <= now) {
        this->register_task(it->second);
        this->timers.erase(it);
        it = this->timers.begin();
      }
    }

    // Scheduling a ticker with period 0 will have
    // it be inserted over and over
    if (this->tickers.size()) {
      auto it = this->tickers.begin();

      std::map<Timestamp, std::tuple<VMTask, uint32_t>> newly_scheduled_tickers;
      while (it != this->tickers.end() && it->first <= now) {
        this->register_task(std::get<0>(it->second));

        Timestamp scheduled_time = now + std::chrono::milliseconds(std::get<1>(it->second));
        newly_scheduled_tickers.insert({scheduled_time, it->second});
        this->tickers.erase(it);

        it = this->tickers.begin();
      }

      this->tickers.insert(newly_scheduled_tickers.begin(), newly_scheduled_tickers.end());
    }

    // Check if there is a task to execute
    VMTask task;
    if (this->pop_task(&task)) {

      // A VMTask can either call a function with an argument or resume the execution
      // of a previously paused thread
      if (task.is_thread) {

        // Resume a suspended thread
        uint64_t thread_id = task.thread.id;
        if (this->paused_threads.count(thread_id) == 0) {
          this->panic(Status::InvalidThreadId);
        }

        VMThread& thread = this->paused_threads.at(thread_id);

        // Restore suspended thread
        this->uid = thread.uid;
        this->stack = std::move(thread.stack);
        this->frames = thread.frame;
        this->catchstack = thread.catchstack;
        this->ip = thread.resume_address;

        this->paused_threads.erase(thread_id);

        // Push null, to act as the return value from __internal_suspend_thread
        this->push_stack(task.thread.argument);
        this->run();
      } else {

        // Make sure we got a function as callback
        if (!charly_is_function(task.callback.func)) {
          this->panic(Status::RuntimeTaskNotCallable);
        }

        this->gc.mark_persistent(task.callback.func);
        this->gc.mark_persistent(task.callback.arguments[0]);
        this->gc.mark_persistent(task.callback.arguments[1]);
        this->gc.mark_persistent(task.callback.arguments[2]);
        this->gc.mark_persistent(task.callback.arguments[3]);

        // Prepare VM object
        this->catchstack = nullptr;

        // Get Charly object
        VALUE global_self = this->get_global_self();
        Function* fn = charly_as_function(task.callback.func);
        VALUE self = this->get_self_for_function(fn, &global_self);

        // Invoke function
        this->call_function(fn, 4, reinterpret_cast<VALUE*>(task.callback.arguments), self, true);
        this->uid = this->get_next_thread_uid();
        this->run();
        this->pop_stack();

        // Unmark this task in the GC
        this->gc.unmark_persistent(task.callback.func);
        this->gc.unmark_persistent(task.callback.arguments[0]);
        this->gc.unmark_persistent(task.callback.arguments[1]);
        this->gc.unmark_persistent(task.callback.arguments[2]);
        this->gc.unmark_persistent(task.callback.arguments[3]);
      }
    } else {

      // Since there are no tasks in the task queue, we can now simply wait until
      // there is one. In order to not miss out on any timers or tickers, we
      // calculate when the next timer/ticker is going to fire and use this time
      // as our maximum wait time.
      now = std::chrono::steady_clock::now();
      auto timer_wait = std::chrono::milliseconds(10 * 1000);
      auto ticker_wait = std::chrono::milliseconds(10 * 1000);

      // Check the next timers and tickers
      if (this->timers.size())
        timer_wait = std::chrono::duration_cast<std::chrono::milliseconds>(this->timers.begin()->first - now);
      if (this->tickers.size())
        ticker_wait = std::chrono::duration_cast<std::chrono::milliseconds>(this->tickers.begin()->first - now);

      // Wait for the result queue
      std::unique_lock<std::mutex> lk(this->task_queue_m);
      this->task_queue_cv.wait_for(lk, std::min(timer_wait, ticker_wait));
    }

    // Check if we can exit the runtime
    if (this->task_queue.size() == 0 &&
        this->timers.size() == 0 &&
        this->tickers.size() == 0 &&
        this->worker_threads.size() == 0) {

      if (this->paused_threads.size()) {
        for (auto& entry : this->paused_threads) {
          ManagedContext lalloc(this);
          VALUE exc_obj = lalloc.create_object(1);
          charly_as_object(exc_obj)->container->insert({ SYM("__charly_internal_stale_thread_exception"), kTrue });
          this->resume_thread(entry.first, exc_obj);
        }
      } else {
        this->running = false;
      }
    }
  }

  return this->status_code;
}

void VM::exit(uint8_t status_code) {
  this->timers.clear();
  this->clear_task_queue();

  this->halted = true;
  this->running = false;

  this->status_code = status_code;

  // Join the remaining worker threads
  {
    std::lock_guard<std::mutex> lock(this->worker_threads_m);

    if (this->worker_threads.size()) {
      this->context.err_stream << "Waiting for worker threads to terminate" << std::endl;
    }

    while (this->worker_threads.size()) {
      auto entry = this->worker_threads.begin();

      // Join worker thread if it is still running
      WorkerThread* thread = entry->second;
      if (thread->thread.joinable())
        thread->thread.join();

      this->worker_threads.erase(entry->first);
      delete thread;
    }
  }
}

uint64_t VM::get_thread_uid() {
  return this->uid;
}

uint64_t VM::get_next_thread_uid() {
  return this->next_thread_id++;
}

void VM::suspend_thread() {
  this->halted = true;
  this->paused_threads.emplace(this->uid, VMThread(
    this->uid,
    std::move(this->stack),
    this->frames,
    this->catchstack,
    this->ip + kInstructionLengths[this->fetch_instruction()]
  ));
}

void VM::resume_thread(uint64_t uid, VALUE argument) {
  this->register_task(VMTask::init_thread(uid, argument));
}

void VM::register_task(VMTask task) {
  std::unique_lock<std::mutex> lock(this->task_queue_m);

  if (!task.is_thread) {
    this->gc.mark_persistent(task.callback.func);
    this->gc.mark_persistent(task.callback.arguments[0]);
    this->gc.mark_persistent(task.callback.arguments[1]);
    this->gc.mark_persistent(task.callback.arguments[2]);
    this->gc.mark_persistent(task.callback.arguments[3]);
  }

  this->task_queue.push(task);

  lock.unlock();
  this->task_queue_cv.notify_one();
}

bool VM::pop_task(VMTask* target) {
  std::lock_guard<std::mutex> lock(this->task_queue_m);

  if (this->task_queue.size() == 0) return false;

  VMTask front = this->task_queue.front();
  this->task_queue.pop();

  if (target != nullptr) {
    *target = front;
  }

  return true;
}

void VM::clear_task_queue() {
  std::lock_guard<std::mutex> lock(this->task_queue_m);

  while (this->task_queue.size()) {
    this->task_queue.pop();
  }
}

VALUE VM::register_module(InstructionBlock* block) {
  uint8_t* old_ip = this->ip;
  this->ip = block->get_data();
  this->run();
  this->ip = old_ip;
  return this->pop_stack();
}

uint64_t VM::register_timer(Timestamp ts, VMTask task) {
  if (!task.is_thread) {
    this->gc.mark_persistent(task.callback.func);
    this->gc.mark_persistent(task.callback.arguments[0]);
    this->gc.mark_persistent(task.callback.arguments[1]);
    this->gc.mark_persistent(task.callback.arguments[2]);
    this->gc.mark_persistent(task.callback.arguments[3]);
  }

  task.uid = this->get_next_timer_id();
  this->timers.insert({ts, task});
  return task.uid;
}

uint64_t VM::register_ticker(uint32_t period, VMTask task) {
  if (!task.is_thread) {
    this->gc.mark_persistent(task.callback.func);
    this->gc.mark_persistent(task.callback.arguments[0]);
    this->gc.mark_persistent(task.callback.arguments[1]);
    this->gc.mark_persistent(task.callback.arguments[2]);
    this->gc.mark_persistent(task.callback.arguments[3]);
  }

  Timestamp now = std::chrono::steady_clock::now();
  Timestamp exec_at = now + std::chrono::milliseconds(period);

  task.uid = this->get_next_timer_id();
  this->tickers.insert({exec_at, {task, period}});
  return task.uid;
}

uint64_t VM::get_next_timer_id() {
  return this->next_timer_id++;
}

void VM::clear_timer(uint64_t uid) {
  for (auto it : this->timers) {
    if (it.second.uid == uid) {
      this->timers.erase(it.first);
      break;
    }
  }
}

void VM::clear_ticker(uint64_t uid) {
  for (auto it : this->tickers) {
    if (std::get<0>(it.second).uid == uid) {
      this->tickers.erase(it.first);
      break;
    }
  }
}

WorkerThread* VM::start_worker_thread(CFunction* cfunc, const std::vector<VALUE>& args, Function* callback) {
  WorkerThread* worker_thread = new WorkerThread(cfunc, args, callback);
  worker_thread->thread = std::thread([this, worker_thread]() {

    // Invoke c function
    uint32_t argc = worker_thread->arguments.size();
    VALUE* argv   = &worker_thread->arguments.at(0);
    VALUE return_value = charly_call_cfunction(this, worker_thread->cfunc, argc, argv);

    // Return our value to the VM
    this->close_worker_thread(worker_thread, return_value);
  });

  std::lock_guard<std::mutex> lock(this->worker_threads_m);
  this->worker_threads.insert({ worker_thread->thread.get_id(), worker_thread });

  return worker_thread;
}

void VM::close_worker_thread(WorkerThread* thread, VALUE return_value) {
  if (!this->running) return; // Do nothing if VM is dead
  std::lock_guard<std::mutex> lock(this->worker_threads_m);

  this->register_task(
      VMTask::init_callback(charly_create_pointer(thread->callback), return_value, thread->error_value));
  this->worker_threads.erase(thread->thread.get_id());

  delete thread;
}

void VM::handle_worker_thread_exception(const std::string& message) {
  ManagedContext lalloc(this);
  std::thread::id current_thread_id = std::this_thread::get_id();
  std::lock_guard<std::mutex> lock(this->worker_threads_m);
  WorkerThread* handle = this->worker_threads[current_thread_id];
  handle->error_value = lalloc.create_string(message);
}

bool VM::is_main_thread() {
  return std::this_thread::get_id() == this->main_thread_id;
}

bool VM::is_worker_thread() {
  return !this->is_main_thread();
}

}  // namespace Charly
