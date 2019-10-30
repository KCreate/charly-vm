/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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
  cell->frame.stacksize_at_entry = 0;  // set by call_generator
  cell->frame.self = self;
  cell->frame.return_address = return_address;
  cell->frame.set_halt_after_return(halt_after_return);

  // Calculate the number of local variables this frame has to support
  uint32_t lvarcount = function->lvarcount;

  // Allocate and prefill local variable space
  if (lvarcount <= kSmallFrameLocalCount) {
    cell->frame.senv.lvarcount = lvarcount;
    cell->frame.set_smallframe(true);

    while (lvarcount--)
      cell->frame.senv.data[lvarcount] = kNull;
  } else {
    cell->frame.lenv = new std::vector<VALUE>(lvarcount, kNull);
  }

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
  cell->frame.stacksize_at_entry = 0;  // set by call_generator
  cell->frame.self = self;
  cell->frame.return_address = return_address;
  cell->frame.set_halt_after_return(halt_after_return);

  if (lvarcount <= kSmallFrameLocalCount) {
    cell->frame.senv.lvarcount = lvarcount;
    cell->frame.set_smallframe(true);

    while (lvarcount--)
      cell->frame.senv.data[lvarcount] = kNull;
  } else {
    cell->frame.lenv = new std::vector<VALUE>(lvarcount, kNull);
  }

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

  if (this->stack.size() > 0) {
    val = this->stack.back();
    this->stack.pop_back();
  }

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
    this->context.err_stream << "Last exception thrown: ";
    this->to_s(this->context.err_stream, this->last_exception_thrown);
    this->context.err_stream << '\n';
    this->panic(Status::CatchStackEmpty);
  }
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
      if (this->frames->halt_after_return()) {
        this->halted = true;
      }

      // If this frame was created by a generator, we need to switch the catchtable to the one
      // stored inside the frame. The catchtables stored inside the generator are only valid inside the
      // generator itself
      if (charly_is_generator(this->frames->caller_value)) {
        Generator* generator = charly_as_generator(this->frames->caller_value);
        table = generator->context_frame->last_active_catchtable;
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
    this->stack.pop_back();
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

  if (length <= kShortStringMaxSize) {
    // Copy the string into the cell itself
    cell->string.set_shortstring(true);
    std::memcpy(cell->string.sbuf.data, data, length);
    cell->string.sbuf.length = length;
  } else {
    // Copy the string onto the heap
    char* copied_string = static_cast<char*>(calloc(sizeof(char), length));
    std::memcpy(copied_string, data, length);

    // Setup long string
    cell->string.set_shortstring(false);
    cell->string.lbuf.data = copied_string;
    cell->string.lbuf.length = length;
  }

  return cell->as_value();
}

VALUE VM::create_string(const std::string& str) {
  if (str.size() <= 6)
    return charly_create_istring(str.c_str(), str.size());

  // Check if we can create a short string
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeString;

  if (str.size() <= kShortStringMaxSize) {
    // Copy the string into the cell itself
    cell->string.set_shortstring(true);
    std::memcpy(cell->string.sbuf.data, str.c_str(), str.size());
    cell->string.sbuf.length = str.size();
  } else {
    // Copy the string onto the heap
    char* copied_string = static_cast<char*>(calloc(sizeof(char), str.size()));
    std::memcpy(copied_string, str.c_str(), str.size());

    // Setup long string
    cell->string.set_shortstring(false);
    cell->string.lbuf.data = copied_string;
    cell->string.lbuf.length = str.size();
  }

  return cell->as_value();
}

VALUE VM::create_weak_string(char* data, uint32_t length) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeString;
  cell->string.set_shortstring(false);
  cell->string.lbuf.data = data;
  cell->string.lbuf.length = length;
  return cell->as_value();
}

VALUE VM::create_empty_short_string() {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeString;
  cell->string.set_shortstring(true);
  cell->string.sbuf.length = 0;
  return cell->as_value();
}

VALUE VM::create_function(VALUE name,
                          uint8_t* body_address,
                          uint32_t argc,
                          uint32_t lvarcount,
                          bool anonymous,
                          bool needs_arguments) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeFunction;
  cell->function.name = name;
  cell->function.argc = argc;
  cell->function.lvarcount = lvarcount;
  cell->function.context = this->frames;
  cell->function.body_address = body_address;
  cell->function.set_anonymous(anonymous);
  cell->function.set_needs_arguments(needs_arguments);
  cell->function.bound_self_set = false;
  cell->function.bound_self = kNull;
  cell->function.container = new std::unordered_map<VALUE, VALUE>();
  return cell->as_value();
}

VALUE VM::create_cfunction(VALUE name, uint32_t argc, void* pointer) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeCFunction;
  cell->cfunction.name = name;
  cell->cfunction.pointer = pointer;
  cell->cfunction.argc = argc;
  cell->cfunction.container = new std::unordered_map<VALUE, VALUE>();
  return cell->as_value();
}

VALUE VM::create_generator(VALUE name, uint8_t* resume_address) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeGenerator;
  cell->generator.name = name;
  cell->generator.context_frame = this->frames;
  cell->generator.context_catchtable = this->catchstack;
  cell->generator.context_stack = new std::vector<VALUE>();
  cell->generator.resume_address = resume_address;
  cell->generator.owns_catchtable = false;
  cell->generator.running = false;
  cell->generator.set_finished(false);
  cell->generator.set_started(false);
  cell->generator.bound_self_set = false;
  cell->generator.bound_self = kNull;
  cell->generator.container = new std::unordered_map<VALUE, VALUE>();
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
    case kTypeGenerator: return this->copy_generator(value);
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
    case kTypeGenerator: return this->copy_generator(value);
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
  ManagedContext lalloc(*this);

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
  ManagedContext lalloc(*this);

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
  Function* target =
      charly_as_function(this->create_function(source->name, source->body_address, source->argc, source->lvarcount,
                                               source->anonymous(), source->needs_arguments()));

  target->context = source->context;
  target->bound_self_set = source->bound_self_set;
  target->bound_self = source->bound_self;
  *(target->container) = *(source->container);

  return charly_create_pointer(target);
}

VALUE VM::copy_cfunction(VALUE function) {
  CFunction* source = charly_as_cfunction(function);
  CFunction* target = charly_as_cfunction(this->create_cfunction(source->name, source->argc, source->pointer));
  *(target->container) = *(source->container);

  return charly_create_pointer(target);
}

VALUE VM::copy_generator(VALUE generator) {
  Generator* source = charly_as_generator(generator);
  Generator* target = charly_as_generator(this->create_generator(source->name, source->resume_address));

  target->bound_self_set = source->bound_self_set;
  target->bound_self = source->bound_self;
  target->set_finished(source->finished());
  *(target->container) = *(source->container);
  *(target->context_stack) = *(source->context_stack);
  *(target->context_frame) = *(source->context_frame);

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
    if (left_length == 0 && right_length == 0) {
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

    // Check if both strings would fit into the short encoding
    if (new_length <= kShortStringMaxSize) {
      String* new_string = charly_as_hstring(this->create_empty_short_string());
      std::memcpy(new_string->sbuf.data, left_data, left_length);
      std::memcpy(new_string->sbuf.data + left_length, right_data, right_length);
      new_string->sbuf.length = new_length;
      return charly_create_pointer(new_string);
    }

    // Allocate the buffer for the string
    char* new_data = reinterpret_cast<char*>(std::malloc(new_length));
    std::memcpy(new_data, left_data, left_length);
    std::memcpy(new_data + left_length, right_data, right_length);
    return this->create_weak_string(new_data, new_length);
  }

  // TODO: String concatenation for different types
  // Will require a charly_value_to_string method
  if (charly_is_string(left) || charly_is_string(right)) {
    std::stringstream buf;
    this->to_s(buf, left);
    this->to_s(buf, right);
    return this->create_string(buf.str());
  }

  return kBitsNaN;
}

VALUE VM::sub(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_sub_number(left, right);
  }

  return kBitsNaN;
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

    // Check if the result would fit into the short encoding
    if (new_length <= kShortStringMaxSize) {
      String* new_string = charly_as_hstring(this->create_empty_short_string());

      // Copy the string amount times
      uint32_t offset = 0;
      while (amount--) {
        std::memcpy(new_string->sbuf.data + offset, str_data, str_length);
        offset += str_length;
      }

      new_string->sbuf.length = new_length;
      return charly_create_pointer(new_string);
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

  return kBitsNaN;
}

VALUE VM::div(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_div_number(left, right);
  }

  return kBitsNaN;
}

VALUE VM::mod(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_mod_number(left, right);
  }

  return kBitsNaN;
}

VALUE VM::pow(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_pow_number(left, right);
  }

  return kBitsNaN;
}

VALUE VM::uadd(VALUE value) {
  return value;
}

VALUE VM::usub(VALUE value) {
  if (charly_is_number(value)) {
    return charly_usub_number(value);
  }

  return kBitsNaN;
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

  return kBitsNaN;
}

VALUE VM::shr(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_shr_number(left, right);
  }

  return kBitsNaN;
}

VALUE VM::band(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_and_number(left, right);
  }

  return kBitsNaN;
}

VALUE VM::bor(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_or_number(left, right);
  }

  return kBitsNaN;
}

VALUE VM::bxor(VALUE left, VALUE right) {
  if (charly_is_number(left) && charly_is_number(right)) {
    return charly_xor_number(left, right);
  }

  return kBitsNaN;
}

VALUE VM::ubnot(VALUE value) {
  if (charly_is_number(value)) {
    return charly_ubnot_number(value);
  }

  return kBitsNaN;
}

VALUE VM::readmembersymbol(VALUE source, VALUE symbol) {
  switch (charly_get_type(source)) {
    case kTypeObject: {
      Object* obj = charly_as_object(source);

      if (symbol == charly_create_symbol("klass")) {
        return obj->klass;
      }

      if (obj->container->count(symbol) == 1) {
        return (*obj->container)[symbol];
      }

      break;
    }

    case kTypeFunction: {
      Function* func = charly_as_function(source);

      if (func->container->count(symbol) == 1) {
        return (*func->container)[symbol];
      }

      if (symbol == charly_create_symbol("name")) {
        return func->name;
      }

      break;
    }

    case kTypeCFunction: {
      CFunction* cfunc = charly_as_cfunction(source);

      if (cfunc->container->count(symbol) == 1) {
        return (*cfunc->container)[symbol];
      }

      if (symbol == charly_create_symbol("name")) {
        return cfunc->name;
      }

      break;
    }
    case kTypeClass: {
      Class* klass = charly_as_class(source);

      if (klass->container->count(symbol) == 1) {
        return (*klass->container)[symbol];
      }

      if (symbol == charly_create_symbol("prototype")) {
        return klass->prototype;
      }

      if (symbol == charly_create_symbol("name")) {
        return klass->name;
      }

      break;
    }
    case kTypeArray: {
      Array* arr = charly_as_array(source);

      if (symbol == charly_create_symbol("length")) {
        return charly_create_integer(arr->data->size());
      }

      break;
    }
    case kTypeString: {
      if (symbol == charly_create_symbol("length")) {
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

    Class* klass = charly_as_class(val_klass);
    auto result = this->findprototypevalue(klass, symbol);

    if (result.has_value()) {
      return result.value();
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
      } else {
        return this->readmembersymbol(source, charly_create_symbol(value));
      }
    }
    case kTypeString: {
      if (charly_is_number(value)) {
        int32_t index = charly_number_to_int32(value);
        return charly_string_cp_at_index(source, index);
      } else {
        return this->readmembersymbol(source, charly_create_symbol(value));
      }
    }
    default: { return this->readmembersymbol(source, charly_create_symbol(value)); }
  }
}

VALUE VM::setmembersymbol(VALUE target, VALUE symbol, VALUE value) {
  switch (charly_get_type(target)) {
    case kTypeObject: {
      Object* obj = charly_as_object(target);
      (*obj->container)[symbol] = value;
      break;
    }

    case kTypeFunction: {
      Function* func = charly_as_function(target);
      (*func->container)[symbol] = value;
      break;
    }

    case kTypeCFunction: {
      CFunction* cfunc = charly_as_cfunction(target);
      (*cfunc->container)[symbol] = value;
      break;
    }

    case kTypeClass: {
      Class* klass = charly_as_class(target);

      if (symbol == charly_create_symbol("prototype")) {
        klass->prototype = value;
        break;
      }

      (*klass->container)[symbol] = value;
      break;
    }
  }

  return value;
}

VALUE VM::setmembervalue(VALUE target, VALUE member_value, VALUE value) {
  switch (charly_get_type(target)) {
    case kTypeArray: {
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
      } else {
        return this->setmembersymbol(target, charly_create_symbol(member_value), value);
      }
    }
    default: { return this->setmembersymbol(target, charly_create_symbol(member_value), value); }
  }
}

// TODO: Move this method to value.h file
std::optional<VALUE> VM::findprototypevalue(Class* klass, VALUE symbol) {
  std::optional<VALUE> result;

  if (charly_is_object(klass->prototype)) {
    Object* prototype = charly_as_object(klass->prototype);

    // Check if this class container contains the symbol
    if (prototype->container->count(symbol) == 1) {
      result = (*prototype->container)[symbol];
    } else {
      if (charly_is_class(klass->parent_class)) {
        Class* pklass = charly_as_class(klass->parent_class);
        auto presult = this->findprototypevalue(pklass, symbol);

        if (presult.has_value()) {
          return presult;
        }
      }
    }
  }

  return result;
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
    case kTypeGenerator: found_primitive_class = this->primitive_generator; break;
    case kTypeClass: found_primitive_class = this->primitive_class; break;
  }

  if (!charly_is_class(found_primitive_class)) {
    return std::nullopt;
  }

  Class* pclass = charly_as_class(found_primitive_class);
  return this->findprototypevalue(pclass, symbol);
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
        if (tfunc->anonymous()) {
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
      this->call_cfunction(charly_as_cfunction(function), argc, arguments);
      if (halt_after_return) {
        this->halted = true;
      }
      return;
    }

    // Generators
    case kTypeGenerator: {
      this->call_generator(charly_as_generator(function), argc, arguments);
      if (halt_after_return) {
        this->halted = true;
      }
      return;
    }

    // Class construction
    case kTypeClass: {
      this->call_class(charly_as_class(function), argc, arguments);
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
  if (argc < function->argc) {
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

  Frame* frame = ctx.create_frame(self, function, return_address, halt_after_return);

  // Copy the arguments into the function frame
  //
  // If the function requires an arguments array, we create one and push it onto
  // offset 0 of the frame
  if (function->needs_arguments()) {
    Array* arguments_array = charly_as_array(ctx.create_array(argc));
    frame->write_local(0, charly_create_pointer(arguments_array));

    for (size_t i = 0; i < argc; i++) {
      if (i < function->argc) {
        frame->write_local(i + 1, argv[i]);
      }
      arguments_array->data->push_back(argv[i]);
    }
  } else {
    for (size_t i = 0; i < function->argc; i++)
      frame->write_local(i, argv[i]);
  }

  this->ip = function->body_address;
}

void VM::call_cfunction(CFunction* function, uint32_t argc, VALUE* argv) {
  // Check if enough arguments have been passed
  if (argc < function->argc) {
    this->throw_exception("Not enough arguments for CFunction call");
    return;
  }

  // We keep a reference to the current catchtable around in case the constructor throws an exception
  // After the constructor call we check if the table changed
  CatchTable* original_catchtable = this->catchstack;

  VALUE rv = kNull;

  // TODO: Expand this to 15 arguments
  switch (function->argc) {
    case 0: rv = reinterpret_cast<VALUE (*)(VM&)>(function->pointer)(*this); break;
    case 1: rv = reinterpret_cast<VALUE (*)(VM&, VALUE)>(function->pointer)(*this, argv[0]); break;
    case 2: rv = reinterpret_cast<VALUE (*)(VM&, VALUE, VALUE)>(function->pointer)(*this, argv[0], argv[1]); break;
    case 3:
      rv = reinterpret_cast<VALUE (*)(VM&, VALUE, VALUE, VALUE)>(function->pointer)(*this, argv[0], argv[1], argv[2]);
      break;
    case 4:
      rv = reinterpret_cast<VALUE (*)(VM&, VALUE, VALUE, VALUE, VALUE)>(function->pointer)(*this, argv[0], argv[1], argv[2], argv[3]);
      break;
    case 5:
      rv = reinterpret_cast<VALUE (*)(VM&, VALUE, VALUE, VALUE, VALUE, VALUE)>(function->pointer)(*this, argv[0], argv[1], argv[2], argv[3], argv[4]);
      break;
    default: {
      this->throw_exception("Too many arguments for CFunction call");
      return;
    }
  }

  // The cfunction call might have halted the machine by either executing a module
  // or calling a user defined function
  this->halted = false;

  if (this->catchstack == original_catchtable) {
    this->push_stack(rv);
  }
}

void VM::call_class(Class* klass, uint32_t argc, VALUE* argv) {
  ManagedContext lalloc(*this);
  Object* object = charly_as_object(lalloc.create_object(klass->member_properties->size()));
  object->klass = charly_create_pointer(klass);

  // Add the fields of parent classes
  this->initialize_member_properties(klass, object);

  bool success = this->invoke_class_constructors(klass, object, argc, argv);
  if (success) {
    this->push_stack(charly_create_pointer(object));
  }
}

void VM::call_generator(Generator* generator, uint32_t argc, VALUE* argv) {
  // You can only call a generator with a single argument
  if (argc > 1) {
    this->throw_exception("Can't call generator with more than 1 argument");
    return;
  }

  // If the generator is currently running, we throw an error
  if (generator->running) {
    this->throw_exception("Can't call already running generator");
    return;
  }

  // If the generator is already finished, we return null
  if (generator->finished()) {
    this->push_stack(kNull);
    return;
  }

  // Calculate the return address
  uint8_t* return_address = nullptr;
  if (this->ip != nullptr) {
    return_address = this->ip + kInstructionLengths[this->fetch_instruction()];
  }

  // Restore the frame that was active when the generator was created
  // We patch some fields of the frame, so a return or yield call can return to the
  // correct position
  Frame* frame = generator->context_frame;
  frame->parent = this->frames;
  frame->last_active_catchtable = this->catchstack;
  frame->caller_value = charly_create_pointer(generator);
  frame->stacksize_at_entry = this->stack.size();
  frame->return_address = return_address;

  this->frames = frame;
  if (generator->owns_catchtable) {
    this->catchstack = generator->context_catchtable;
  }
  this->ip = generator->resume_address;

  // Restore the values on the stack which ere active when the generator was paused
  while (generator->context_stack->size()) {
    this->push_stack(generator->context_stack->back());
    generator->context_stack->pop_back();
  }

  // Push the argument onto the stack
  if (generator->started()) {
    if (argc == 0) {
      this->push_stack(kNull);
    } else {
      this->push_stack(argv[0]);
    }
  } else {
    generator->set_started(true);
  }

  generator->running = true;
}

void VM::initialize_member_properties(Class* klass, Object* object) {
  if (charly_is_class(klass->parent_class)) {
    this->initialize_member_properties(charly_as_class(klass->parent_class), object);
  }

  for (auto field : *klass->member_properties) {
    (*object->container)[field] = kNull;
  }
}

bool VM::invoke_class_constructors(Class* klass, Object* object, uint32_t argc, VALUE* argv) {
  // We keep a reference to the current catchtable around in case the constructor throws an exception
  // After the constructor call we check if the table changed
  CatchTable* original_catchtable = this->catchstack;

  if (charly_is_class(klass->parent_class)) {
    bool success = this->invoke_class_constructors(charly_as_class(klass->parent_class), object, argc, argv);
    if (!success)
      return false;
  }

  if (charly_is_function(klass->constructor)) {
    Function* constructor = charly_as_function(klass->constructor);

    if (constructor->argc > argc) {
      this->throw_exception("Not enough arguments for class constructor");
      return false;
    }

    this->call_function(constructor, constructor->argc, argv, charly_create_pointer(object), true);
    this->run();
    this->halted = false;

    // Pop the return value generated by the class constructor off the stack
    // We don't need it anymore
    this->pop_stack();
  }

  return this->catchstack == original_catchtable;
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

void VM::op_putstring(char* data, uint32_t length) {
  this->push_stack(this->create_string(data, length));
}

void VM::op_putfunction(VALUE symbol,
                        uint8_t* body_address,
                        bool anonymous,
                        bool needs_arguments,
                        uint32_t argc,
                        uint32_t lvarcount) {
  VALUE function = this->create_function(symbol, body_address, argc, lvarcount, anonymous, needs_arguments);
  this->push_stack(function);
}

void VM::op_putcfunction(VALUE symbol, void* pointer, uint32_t argc) {
  VALUE function = this->create_cfunction(symbol, argc, pointer);
  this->push_stack(function);
}

void VM::op_putgenerator(VALUE symbol, uint8_t* resume_address) {
  VALUE generator = this->create_generator(symbol, resume_address);
  this->push_stack(generator);
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
  ManagedContext lalloc(*this);

  Class* klass = charly_as_class(lalloc.create_class(name));
  klass->member_properties->reserve(propertycount);
  klass->prototype = lalloc.create_object(methodcount);
  klass->container->reserve(staticpropertycount + staticmethodcount);

  if (has_constructor) {
    klass->constructor = this->pop_stack();
  }

  if (has_parent_class) {
    klass->parent_class = this->pop_stack();
  } else {
    klass->parent_class = this->primitive_object;
  }

  while (staticmethodcount--) {
    VALUE smethod = this->pop_stack();
    if (!charly_is_function(smethod)) {
      this->panic(Status::InvalidArgumentType);
    }
    Function* func_smethod = charly_as_function(smethod);
    (*klass->container)[func_smethod->name] = smethod;
  }

  while (methodcount--) {
    VALUE method = this->pop_stack();
    if (!charly_is_function(method)) {
      this->panic(Status::InvalidArgumentType);
    }
    Function* func_method = charly_as_function(method);
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

  this->push_stack(charly_create_pointer(klass));
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

void VM::op_return() {
  Frame* frame = this->frames;
  if (!frame)
    this->panic(Status::CantReturnFromTopLevel);

  // Returning from a generator causes the generator to terminate
  // Mark it as done and delete some items which are no longer needed
  if (charly_is_generator(frame->caller_value)) {
    Generator* generator = charly_as_generator(frame->caller_value);
    generator->set_finished(true);
    generator->set_started(false);
    generator->running = false;
  }

  this->catchstack = frame->last_active_catchtable;
  this->frames = frame->parent;
  this->ip = frame->return_address;

  if (frame->halt_after_return()) {
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
  Frame* frame = this->frames;
  if (!frame)
    this->panic(Status::CantReturnFromTopLevel);

  // Check if we are exiting from a generator
  if (!charly_is_generator(frame->caller_value)) {
    this->panic(Status::CantYieldFromNonGenerator);
  }

  // Store the yielded value
  VALUE yield_value = this->pop_stack();

  // Store context info inside the generator
  Generator* generator = charly_as_generator(frame->caller_value);
  generator->owns_catchtable = generator->context_catchtable != this->catchstack;
  generator->context_catchtable = this->catchstack;
  generator->resume_address = this->ip + kInstructionLengths[Opcode::Yield];
  generator->running = false;
  size_t stack_value_pop_count = this->stack.size() - frame->stacksize_at_entry;
  while (stack_value_pop_count--) {
    generator->context_stack->push_back(this->pop_stack());
  }

  this->push_stack(yield_value);

  // We can't restore catchtables by popping them, since the list of tables
  // in the generator might be different than of the outside world
  this->catchstack = frame->last_active_catchtable;
  this->frames = frame->parent;
  this->ip = frame->return_address;

  if (frame->halt_after_return()) {
    this->halted = true;
  }
}

void VM::op_throw() {
  return this->throw_exception(this->pop_stack());
}

void VM::throw_exception(const std::string& message) {
  ManagedContext lalloc(*this);

  // Create exception object
  Object* ex_obj = charly_as_object(lalloc.create_object(2));
  VALUE ex_msg = lalloc.create_string(message.c_str(), message.size());
  (*ex_obj->container)[this->context.symtable("message")] = ex_msg;
  (*ex_obj->container)[this->context.symtable("stacktrace")] = this->stacktrace_array();

  this->last_exception_thrown = charly_create_pointer(ex_obj);

  // Unwind stack and push exception object
  this->unwind_catchstack();
  this->push_stack(charly_create_pointer(ex_obj));
}

void VM::throw_exception(VALUE payload) {
  this->unwind_catchstack();
  this->last_exception_thrown = payload;
  this->push_stack(payload);
}

VALUE VM::stacktrace_array() {
  ManagedContext lalloc(*this);
  Array* arr = charly_as_array(lalloc.create_array(1));

  std::stringstream io;

  Frame* frame = this->frames;
  while (frame && charly_is_frame(charly_create_pointer(frame))) {
    this->to_s(io, charly_create_pointer(frame));
    frame = frame->parent;

    // Append the trace entry to the array and reset the stringstream
    std::string strcopy = io.str();
    arr->data->push_back(lalloc.create_string(strcopy.data(), strcopy.size()));
    io.str("");
  }

  return charly_create_pointer(arr);
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

void VM::stacktrace(std::ostream& io) {
  Frame* frame = this->frames;

  int i = 0;
  io << "IP: " << static_cast<void*>(this->ip) << '\n';
  while (frame && charly_is_frame(charly_create_pointer(frame))) {
    io << i++ << "# ";
    this->pretty_print(io, charly_create_pointer(frame));
    io << '\n';
    frame = frame->parent;
  }
}

void VM::catchstacktrace(std::ostream& io) {
  CatchTable* table = this->catchstack;

  int i = 0;
  while (table) {
    io << i++ << "# ";
    this->pretty_print(io, charly_create_pointer(table));
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
        std::string key = this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
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
        std::string key = this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
        io << key << "=";
        this->pretty_print(io, entry.second);
      }

      io << ">";

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeGenerator: {
      Generator* generator = charly_as_generator(value);

      if (printed_before) {
        io << "<Generator ...>";
        break;
      }

      this->pretty_print_stack.push_back(value);

      io << "<Generator ";
      io << "name=";
      this->pretty_print(io, generator->name);
      io << " ";
      io << "resume_address=" << reinterpret_cast<void*>(generator->resume_address) << " ";
      io << "finished=" << (generator->finished() ? "true" : "false") << " ";
      io << "started=" << (generator->started() ? "true" : "false") << " ";
      io << "running=" << (generator->running ? "true" : "false") << " ";
      io << "context_frame=" << reinterpret_cast<void*>(generator->context_frame) << " ";
      io << "context_catchtable=" << reinterpret_cast<void*>(generator->context_catchtable) << " ";
      io << "bound_self_set=" << (generator->bound_self_set ? "true" : "false") << " ";
      io << "bound_self=";
      this->pretty_print(io, generator->bound_self);

      for (auto& entry : *generator->container) {
        io << " ";
        std::string key = this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
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
        io << " " << this->context.symtable(entry).value_or(kUndefinedSymbolString);
      }
      io << "] ";

      io << "member_functions=";
      this->pretty_print(io, klass->prototype);
      io << " ";

      io << "parent_class=";
      this->pretty_print(io, klass->parent_class);
      io << " ";

      for (auto entry : *klass->container) {
        io << " " << this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
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
      io << this->context.symtable(value).value_or(kUndefinedSymbolString);
      break;
    }

    case kTypeFrame: {
      Frame* frame = charly_as_frame(value);

      if (printed_before) {
        io << "<Frame ...>";
        break;
      }

      this->pretty_print_stack.push_back(value);

      io << "<@";
      io << reinterpret_cast<void*>(frame);
      io << "Frame ";
      io << "parent=" << frame->parent << " ";
      io << "parent_environment_frame=" << frame->parent_environment_frame << " ";
      io << "caller_value=";
      this->pretty_print(io, frame->caller_value);
      io << " ";
      io << "self=";
      this->pretty_print(io, frame->self);
      io << " ";
      io << "return_address=" << reinterpret_cast<void*>(frame->return_address);
      io << ">";

      this->pretty_print_stack.pop_back();
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

void VM::to_s(std::ostream& io, VALUE value, uint32_t depth) {
  // Check if this value was already printed before
  auto begin = this->pretty_print_stack.begin();
  auto end = this->pretty_print_stack.end();
  bool printed_before = std::find(begin, end, value) != end;

  switch (charly_get_type(value)) {
    case kTypeDead: {
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
      io.write(charly_string_data(value), charly_string_length(value));
      break;
    }

    case kTypeObject: {
      Object* object = charly_as_object(value);

      // If this object was already printed, we avoid printing it again
      if (printed_before) {
        io << "{circular}";
        break;
      }

      this->pretty_print_stack.push_back(value);

      io << "{\n";

      for (auto& entry : *object->container) {
        io << std::string(depth + 2, ' ');
        std::string key = this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
        io << key << " = ";
        this->to_s(io, entry.second, depth + 2);
        io << '\n';
      }

      io << std::string(depth == 0 ? 0 : depth, ' ');
      io << "}";

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeArray: {
      Array* array = charly_as_array(value);

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

        this->to_s(io, entry, depth);
      }

      io << "]";

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
      io << "";
      this->to_s(io, func->name);
      io << "#" << func->argc;

      for (auto& entry : *func->container) {
        io << " ";
        std::string key = this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
        io << key << "=";
        this->to_s(io, entry.second, depth);
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
      this->to_s(io, func->name, depth);
      io << "#" << func->argc;

      for (auto& entry : *func->container) {
        io << " ";
        std::string key = this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
        io << key << "=";
        this->to_s(io, entry.second, depth);
      }

      io << ">";

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeGenerator: {
      Generator* generator = charly_as_generator(value);

      if (printed_before) {
        io << "<Generator ...>";
        break;
      }

      this->pretty_print_stack.push_back(value);

      io << "<Generator ";
      this->to_s(io, generator->name, depth);
      io << (generator->finished() ? " finished" : "");
      io << (generator->started() ? " started" : "");
      io << (generator->running ? " running" : "");

      for (auto& entry : *generator->container) {
        io << " ";
        std::string key = this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
        io << key << "=";
        this->to_s(io, entry.second, depth);
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
      this->to_s(io, klass->name, depth);

      for (auto entry : *klass->container) {
        io << " " << this->context.symtable(entry.first).value_or(kUndefinedSymbolString);
        io << "=";
        this->to_s(io, entry.second, depth);
      }

      io << ">";

      this->pretty_print_stack.pop_back();
      break;
    }

    case kTypeCPointer: {
      io << "<CPointer>";
      break;
    }

    case kTypeSymbol: {
      io << this->context.symtable(value).value_or(kUndefinedSymbolString);
      break;
    }

    case kTypeFrame: {
      Frame* frame = charly_as_frame(value);

      if (printed_before) {
        io << "<Frame ...>";
        break;
      }

      this->pretty_print_stack.push_back(value);

      io << "<Frame ";
      io << "caller_value=";
      this->pretty_print(io, frame->caller_value);
      io << " ";
      io << "self=";
      this->pretty_print(io, frame->self);
      io << " ";
      io << ">";

      this->pretty_print_stack.pop_back();
      break;
    }

    default: {
      io << "<?>";
      break;
    }
  }
}

void VM::panic(STATUS reason) {
  this->context.err_stream << "Panic: " << kStatusHumanReadable[reason] << '\n';
  this->context.err_stream << '\n' << "Stacktrace:" << '\n';
  this->stacktrace(this->context.err_stream);
  this->context.err_stream << '\n' << "Stackdump:" << '\n';
  this->stackdump(this->context.err_stream);

  this->exit(1);
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
                                          &&charly_main_switch_setlocalpush,
                                          &&charly_main_switch_setmembersymbolpush,
                                          &&charly_main_switch_setmembervaluepush,
                                          &&charly_main_switch_setarrayindexpush,
                                          &&charly_main_switch_setlocal,
                                          &&charly_main_switch_setmembersymbol,
                                          &&charly_main_switch_setmembervalue,
                                          &&charly_main_switch_setarrayindex,
                                          &&charly_main_switch_putself,
                                          &&charly_main_switch_putvalue,
                                          &&charly_main_switch_putstring,
                                          &&charly_main_switch_putfunction,
                                          &&charly_main_switch_putcfunction,
                                          &&charly_main_switch_putgenerator,
                                          &&charly_main_switch_putarray,
                                          &&charly_main_switch_puthash,
                                          &&charly_main_switch_putclass,
                                          &&charly_main_switch_pop,
                                          &&charly_main_switch_dup,
                                          &&charly_main_switch_dupn,
                                          &&charly_main_switch_swap,
                                          &&charly_main_switch_call,
                                          &&charly_main_switch_callmember,
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

charly_main_switch_putself : {
  OPCODE_PROLOGUE();
  uint32_t level = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
  this->op_putself(level);
  OPCODE_EPILOGUE();
  NEXTOP();
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
  char* str_start = reinterpret_cast<char*>(this->context.stringpool.get_data() + offset);
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
  uint32_t lvarcount = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(int32_t) +
                                                    sizeof(bool) + sizeof(bool) + sizeof(uint32_t));

  this->op_putfunction(symbol, this->ip + body_offset, anonymous, needs_arguments, argc, lvarcount);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_putcfunction : {
  OPCODE_PROLOGUE();
  VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
  void* pointer = this->ip + sizeof(Opcode) + sizeof(VALUE);
  uint32_t argc = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(void*));
  this->op_putcfunction(symbol, pointer, argc);
  OPCODE_EPILOGUE();
  NEXTOP();
}

charly_main_switch_putgenerator : {
  OPCODE_PROLOGUE();
  VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
  int32_t body_offset = *reinterpret_cast<int32_t*>(this->ip + sizeof(Opcode) + sizeof(VALUE));

  this->op_putgenerator(symbol, this->ip + body_offset);
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
  NEXTOP();
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

void VM::exec_prelude() {
  // Charly = {
  //   internals: {
  //     get_method: <Internals::get_method>
  //   }
  // }
  this->top_frame = this->create_frame(kNull, this->frames, 20, nullptr);
  this->op_putcfunction(this->context.symtable("get_method"), reinterpret_cast<void*>(Internals::get_method), 1);
  this->op_putvalue(this->context.symtable("get_method"));
  this->op_puthash(1);
  this->op_putvalue(this->context.symtable("internals"));
  this->op_puthash(1);
  this->op_setlocal(19, 0);
  this->op_pop();
}

uint8_t VM::start_runtime() {
  while (this->running) {
    Timestamp now = std::chrono::steady_clock::now();

    // Add all expired timers and intervals to the task_queue
    if (this->timers.size()) {
      auto it = this->timers.begin();
      while (it != this->timers.end() && it->first <= now) {
        now = std::chrono::steady_clock::now();
        this->register_task(it->second);
        this->timers.erase(it);
        it = this->timers.begin();
      }
    }
    if (this->intervals.size()) {
      auto it = this->intervals.begin();
      while (it != this->intervals.end() && it->first <= now) {
        now = std::chrono::steady_clock::now();
        this->register_task(std::get<0>(it->second));
        this->intervals.insert({now + std::chrono::milliseconds(std::get<1>(it->second)), it->second});
        this->intervals.erase(it);
        it = this->intervals.begin();
      }
    }

    // Add all worker thread results to the task queue
    {
      std::unique_lock<std::mutex> lk(this->worker_result_queue_m);
      while (this->worker_result_queue.size()) {
        AsyncTaskResult result = this->worker_result_queue.front();
        this->worker_result_queue.pop();
        this->register_task(VMTask(result.cb, result.result));
      }
    }

    // Execute a task from the task queue
    if (this->task_queue.size()) {
      ManagedContext lalloc(*this);

      VMTask task = this->task_queue.front();
      this->task_queue.pop();

      // Make sure we got a callable type as callback
      if (!charly_is_function(task.fn)) {
        this->panic(Status::RuntimeTaskNotCallable);
      }

      Function* fn = charly_as_function(task.fn);
      this->call_function(fn, 1, &task.argument, kNull, true);
      this->run();
      this->pop_stack();
    } else {

      // Wait for the next result from the worker result queue
      //
      // We calculate the wait timeout based on the next timers and / or intervals
      // This is so we don't stall the thread unneccesarily
      now = std::chrono::steady_clock::now();
      auto timer_wait = std::chrono::milliseconds(10 * 1000);
      auto interval_wait = std::chrono::milliseconds(10 * 1000);

      // Check the next timers and intervals
      if (this->timers.size())
        timer_wait = std::chrono::duration_cast<std::chrono::milliseconds>(this->timers.begin()->first - now);
      if (this->intervals.size())
        interval_wait = std::chrono::duration_cast<std::chrono::milliseconds>(this->intervals.begin()->first - now);

      // Wait for the result queue
      std::unique_lock<std::mutex> lk(this->worker_result_queue_m);
      this->worker_result_queue_cv.wait_for(lk, std::min(timer_wait, interval_wait));
    }

    // Check if we can exit the runtime
    if (this->task_queue.size() == 0 && this->timers.size() == 0 && this->intervals.size() == 0 &&
        this->worker_task_queue.size() == 0 && this->worker_result_queue.size() == 0) {

      // Check if there is at least one worker thread executing a task
      auto it_begin = this->worker_threads.begin();
      auto it_end = this->worker_threads.end();
      if (std::find_if(it_begin, it_end, [](WorkerThread* wt) {
        return wt->currently_executing_task;
        }) == it_end) {
        this->running = false;
      }
    }
  }

  return this->status_code;
}

VALUE VM::exec_module(Function* fn) {
  ManagedContext lalloc(*this);
  VALUE export_obj = lalloc.create_object(0);

  uint8_t* old_ip = this->ip;
  this->call_function(fn, 1, &export_obj, kNull, true);
  this->frames->parent_environment_frame = this->top_frame;
  this->frames->set_halt_after_return(true);
  this->run();
  this->ip = old_ip;
  return this->pop_stack();
}

VALUE VM::exec_function(Function* fn, VALUE argument) {
  uint8_t* old_ip = this->ip;
  this->call_function(fn, 1, &argument, kNull, true);
  this->run();
  this->ip = old_ip;
  return this->pop_stack();
}

void VM::exit(uint8_t status_code) {

  // Clear all timers and remaining tasks
  // and interrupt currently running task
  // Stop all the currently running worker threads
  this->timers.clear();
  while (this->task_queue.size()) {
    this->task_queue.pop();
  }

  {
    std::unique_lock<std::mutex> lk(this->worker_task_queue_m);
    while (this->worker_task_queue.size()) {
      this->worker_task_queue.pop();
    }
  }

  {
    std::unique_lock<std::mutex> lk(this->worker_result_queue_m);
    while (this->worker_result_queue.size()) {
      this->worker_result_queue.pop();
    }
  }

  this->halted = true;
  this->running = false;

  // Join all worker threads
  this->worker_threads_active = false;
  for (WorkerThread* t : this->worker_threads) {
    if (t->th.joinable()) t->th.join();
  }

  this->status_code = status_code;
}

VALUE VM::register_module(InstructionBlock* block) {
  uint8_t* old_ip = this->ip;
  this->ip = block->get_data();
  this->run();
  this->ip = old_ip;
  return this->pop_stack();
}

void VM::register_task(VMTask task) {
  this->gc.mark_persistent(task.fn);
  this->gc.mark_persistent(task.argument);
  this->task_queue.push(task);
}

uint64_t VM::register_timer(Timestamp ts, VMTask task) {
  this->gc.mark_persistent(task.fn);
  this->gc.mark_persistent(task.argument);
  task.uid = this->get_next_timer_id();
  this->timers.insert({ts, task});
  return task.uid;
}

uint64_t VM::register_interval(uint32_t period, VMTask task) {
  this->gc.mark_persistent(task.fn);
  this->gc.mark_persistent(task.argument);

  Timestamp now = std::chrono::steady_clock::now();
  Timestamp exec_at = now + std::chrono::milliseconds(period);

  task.uid = this->get_next_timer_id();
  this->intervals.insert({exec_at, {task, period}});
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

void VM::clear_interval(uint64_t uid) {
  for (auto it : this->intervals) {
    if (std::get<0>(it.second).uid == uid) {
      this->intervals.erase(it.first);
      break;
    }
  }
}

std::mutex db_mutex;

void VM::worker_thread_handler(void* vm_handle, uint16_t tid) {
  VM* vm = static_cast<VM*>(vm_handle);
  WorkerThread* wt = vm->worker_threads[tid];

  while (vm->worker_threads_active) {
    AsyncTask task;

    // If there are no tasks available right now, we wait
    {
      std::unique_lock<std::mutex> lk(vm->worker_task_queue_m);
      while (vm->worker_task_queue.size() == 0) {
        vm->worker_task_queue_cv.wait_for(lk, std::chrono::milliseconds(100));

        if (!vm->worker_threads_active) {
          return;
        }
      }

      wt->currently_executing_task = true;
      task = vm->worker_task_queue.front();

      // Mark the task payload as persistent in the gc
      vm->gc.mark_persistent(task.cb);
      for (VALUE item : task.arguments) {
        vm->gc.mark_persistent(item);
      }
      vm->worker_task_queue.pop();
    }

    // Simulate some cpu action
    //
    // We can't just sleep because that would pause the thread
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(1, 3);
    Timestamp work_till = std::chrono::steady_clock::now() + std::chrono::milliseconds(100 * dis(gen));
    while (std::chrono::steady_clock::now() < work_till) {
      continue;
    }

    // Push the result onto the vms result queue
    {
      std::unique_lock<std::mutex> lk(vm->worker_result_queue_m);
      vm->worker_result_queue.push({task.cb, task.arguments[0]});
      vm->worker_result_queue_cv.notify_one();
      wt->currently_executing_task = false;

      // Unmark the task payload as persistent in the gc
      vm->gc.unmark_persistent(task.cb);
      for (VALUE item : task.arguments) {
        vm->gc.unmark_persistent(item);
      }
    }
  }
}

void VM::register_worker_task(AsyncTask task) {
  std::unique_lock<std::mutex> lk(this->worker_task_queue_m);
  this->worker_task_queue.push(task);
  this->worker_task_queue_cv.notify_one();
}

}  // namespace Charly
