/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2018 Leonard Schütz
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
#include <iomanip>
#include <iostream>
#include <sstream>

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
  cell->frame.function = function;
  cell->frame.self = self;
  cell->frame.return_address = return_address;
  cell->frame.halt_after_return = halt_after_return;

  // Calculate the number of local variables this frame has to support
  // Add 1 at the beginning to reserve space for the arguments magic variable
  uint32_t lvarcount = 1 + function->argc + function->lvarcount;

  // Allocate and prefill local variable space
  cell->frame.environment = new std::vector<VALUE>();
  cell->frame.environment->reserve(lvarcount);
  while (lvarcount--)
    cell->frame.environment->push_back(kNull);

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
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeFrame;
  cell->frame.parent = this->frames;
  cell->frame.parent_environment_frame = parent_environment_frame;
  cell->frame.last_active_catchtable = this->catchstack;
  cell->frame.function = nullptr;
  cell->frame.self = self;
  cell->frame.return_address = return_address;
  cell->frame.halt_after_return = halt_after_return;
  cell->frame.environment = new std::vector<VALUE>();
  cell->frame.environment->reserve(lvarcount);
  while (lvarcount--)
    cell->frame.environment->push_back(kNull);

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
    this->pretty_print(this->context.out_stream, charly_create_pointer(table));
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
  if (length <= 6) return charly_create_istring(data, length);

  // Check if we can create a short string
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeString;

  if (length <= kShortStringMaxSize) {

    // Copy the string into the cell itself
    cell->basic.shortstring = true;
    std::memcpy(cell->string.sbuf.data, data, length);
    cell->string.sbuf.length = length;
  } else {

    // Copy the string onto the heap
    char* copied_string = static_cast<char*>(calloc(sizeof(char), length));
    std::memcpy(copied_string, data, length);

    // Setup long string
    cell->basic.shortstring = false;
    cell->string.lbuf.data = copied_string;
    cell->string.lbuf.length = length;
  }

  return cell->as_value();
}

VALUE VM::create_function(VALUE name, uint8_t* body_address, uint32_t argc, uint32_t lvarcount, bool anonymous) {
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeFunction;
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
  MemoryCell* cell = this->gc.allocate();
  cell->basic.type = kTypeCFunction;
  cell->cfunction.name = name;
  cell->cfunction.pointer = pointer;
  cell->cfunction.argc = argc;
  cell->cfunction.bound_self_set = false;
  cell->cfunction.bound_self = kNull;
  cell->cfunction.container = new std::unordered_map<VALUE, VALUE>();
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
  Function* target = charly_as_function(
      this->create_function(source->name, source->body_address, source->argc, source->lvarcount, source->anonymous));

  target->context = source->context;
  target->bound_self_set = source->bound_self_set;
  target->bound_self = source->bound_self;
  *(target->container) = *(source->container);

  return charly_create_pointer(target);
}

VALUE VM::copy_cfunction(VALUE function) {
  CFunction* source = charly_as_cfunction(function);
  CFunction* target = charly_as_cfunction(this->create_cfunction(source->name, source->argc, source->pointer));

  target->bound_self_set = source->bound_self_set;
  target->bound_self = source->bound_self;
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

  // TODO: String concatenation

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

  // TODO: String multiplication

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
    uint32_t str_left_length = charly_string_length(left);
    char* str_right_data = charly_string_data(right);
    uint32_t str_right_length = charly_string_length(right);

    if (str_left_length != str_right_length) {
      return kFalse;
    }

    return std::strncmp(str_left_data, str_right_data, str_left_length) ? kTrue : kFalse;
  }

  return left == right ? kTrue : kFalse;
}

VALUE VM::neq(VALUE left, VALUE right) {
  return this->eq(left, right) ? kFalse : kTrue;
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
      this->panic(Status::ObjectClassNotAClass);
    }

    Class* klass = charly_as_class(val_klass);
    auto result = this->findprototypevalue(klass, symbol);

    if (result.has_value()) {
      return *result;
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
    default: {
      return this->readmembersymbol(source, charly_create_symbol(value));
    }
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
    default: {
      return this->setmembersymbol(target, charly_create_symbol(member_value), value);
    }
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
          result = presult;
        }
      }
    }
  }

  return result;
}

std::optional<VALUE> VM::findprimitivevalue(VALUE value, VALUE symbol) {
  std::optional<VALUE> result;

  // Get the corresponding primitive class
  VALUE found_primitive_class = kNull;
  switch (charly_get_type(value)) {
    case kTypeNumber: found_primitive_class = this->primitive_number; break;
    case kTypeString: found_primitive_class = this->primitive_string; break;
    case kTypeBoolean: found_primitive_class = this->primitive_boolean; break;
    case kTypeNull: found_primitive_class = this->primitive_null; break;
    case kTypeArray: found_primitive_class = this->primitive_array; break;
    case kTypeFunction: found_primitive_class = this->primitive_function; break;
    case kTypeClass: found_primitive_class = this->primitive_class; break;
  }

  if (!charly_is_class(found_primitive_class)) {
    return std::nullopt;
  }

  Class* pclass = charly_as_class(found_primitive_class);
  result = this->findprototypevalue(pclass, symbol);

  if (!result.has_value() && !charly_is_object(value)) {
    found_primitive_class = this->primitive_object;
    if (!charly_is_class(found_primitive_class)) {
      return std::nullopt;
    }

    pclass = charly_as_class(found_primitive_class);
    result = this->findprototypevalue(pclass, symbol);
  }

  return result;
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
      this->call_cfunction(charly_as_cfunction(function), argc, arguments);
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

    default: {
      this->throw_exception("Attempted to call a non-callable type: " + charly_get_typestring(function));
    }
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
  Frame* frame = this->create_frame(self, function, return_address, halt_after_return);

  Array* arguments_array = charly_as_array(this->create_array(argc));

  // Copy the arguments from the temporary arguments buffer into
  // the frames environment
  //
  // We add 1 to the index as that's the index of the arguments array
  for (size_t i = 0; i < argc; i++) {

    // The function only knows about function->argc arguments,
    // the rest is supplied via the first slot in the frames environment
    //
    // We can't inject more argument than the function expects because the
    // readlocal and setlocal instructions contain fixed offsets
    if (i < function->argc) {
      (*frame->environment)[i + 1] = argv[i];
    }
    arguments_array->data->push_back(argv[i]);
  }

  // Insert the argument value and make it a constant
  // This is so that we can be sure that noone is going to overwrite it afterwards
  (*frame->environment)[0] = charly_create_pointer(arguments_array);

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
  switch (argc) {
    case 0: rv = reinterpret_cast<VALUE (*)(VM&)>(function->pointer)(*this); break;
    case 1: rv = reinterpret_cast<VALUE (*)(VM&, VALUE)>(function->pointer)(*this, argv[0]); break;
    case 2: rv = reinterpret_cast<VALUE (*)(VM&, VALUE, VALUE)>(function->pointer)(*this, argv[0], argv[1]); break;
    case 3:
      rv = reinterpret_cast<VALUE (*)(VM&, VALUE, VALUE, VALUE)>(function->pointer)(*this, argv[0], argv[1], argv[2]);
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
  if (index >= frame->environment->size()) {
    return this->panic(Status::ReadFailedOutOfBounds);
  }

  this->push_stack((*frame->environment)[index]);
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
  if (index < frame->environment->size()) {
    (*frame->environment)[index] = value;
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
  if (index < frame->environment->size()) {
    (*frame->environment)[index] = value;
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

void VM::op_putfunction(VALUE symbol, uint8_t* body_address, bool anonymous, uint32_t argc, uint32_t lvarcount) {
  VALUE function = this->create_function(symbol, body_address, argc, lvarcount, anonymous);
  this->push_stack(function);
}

void VM::op_putcfunction(VALUE symbol, uintptr_t pointer, uint32_t argc) {
  VALUE function = this->create_cfunction(symbol, argc, pointer);
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
    this->pretty_print(this->context.out_stream, charly_create_pointer(frame));
    this->context.out_stream << '\n';
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

  // Unwind stack and push exception object
  this->unwind_catchstack();
  this->push_stack(charly_create_pointer(ex_obj));
}

void VM::throw_exception(VALUE payload) {
  this->unwind_catchstack();
  this->push_stack(payload);
}

VALUE VM::stacktrace_array() {
  ManagedContext lalloc(*this);
  Array* arr = charly_as_array(lalloc.create_array(1));

  std::stringstream io;

  Frame* frame = this->frames;
  while (frame && charly_is_frame(charly_create_pointer(frame))) {
    this->pretty_print(io, charly_create_pointer(frame));
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
      this->context.out_stream << "Restored CatchTable: ";
      this->pretty_print(this->context.out_stream, charly_create_pointer(table));
      this->context.out_stream << '\n';
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

  this->pretty_print_stack.push_back(value);

  switch (charly_get_type(value)) {
    case kTypeDead: {
      io << "<dead>";
      break;
    }

    case kTypeNumber: {
      if (charly_is_int(value)) {
        io << charly_int_to_int64(value);
      } else {
        io << charly_double_to_double(value);
      }

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
      io << "<String ";
      io << "\"";
      io.write(charly_string_data(value), charly_string_length(value));
      io << "\"";
      io << " ";
      io << charly_string_length(value);

      if (charly_is_on_heap(value)) {
        if (charly_as_basic(value)->shortstring) {
          io << " s";
        } else {
          io << "";
        }
      } else {
        if (charly_is_istring(value)) {
          io << " i";
        } else {
          io << " p";
        }
      }
      io << ">";
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
      Array* array = charly_as_array(value);
      io << "<Array ";

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
      Function* func = charly_as_function(value);

      if (printed_before) {
        io << "<Function ...>";
        break;
      }

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
      break;
    }

    case kTypeCFunction: {
      CFunction* func = charly_as_cfunction(value);

      if (printed_before) {
        io << "<CFunction ...>";
        break;
      }

      io << "<CFunction ";
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
      Class* klass = charly_as_class(value);

      if (printed_before) {
        io << "<Class ...>";
        break;
      }

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

      io << "<Frame ";
      io << "parent=" << frame->parent << " ";
      io << "parent_environment_frame=" << frame->parent_environment_frame << " ";

      if (frame->function != nullptr) {
        io << "function=";
        this->pretty_print(io, charly_create_pointer(frame->function));
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
      CatchTable* table = charly_as_catchtable(value);

      if (printed_before) {
        io << "<CatchTable ...>";
        break;
      }

      io << "<CatchTable ";
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
  this->context.out_stream << '\n' << "Stacktrace:" << '\n';
  this->stacktrace(this->context.out_stream);
  this->context.out_stream << '\n' << "Stackdump:" << '\n';
  this->stackdump(this->context.out_stream);

  exit(1);
}

void VM::run() {
  this->halted = false;

  // Execute instructions as long as we have a valid ip
  // and the machine wasn't halted
  while (this->ip && !this->halted) {
    std::chrono::time_point<std::chrono::high_resolution_clock> exec_start;
    if (this->context.instruction_profile) {
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

    // Show opcodes as they are executed if the corresponding flag was set
    if (this->context.trace_opcodes) {
      this->context.out_stream << std::setw(12);
      this->context.out_stream.fill('0');
      this->context.out_stream << reinterpret_cast<void*>(old_ip);
      this->context.out_stream << std::setw(1);
      this->context.out_stream.fill(' ');
      this->context.out_stream << ": " << kOpcodeMnemonics[opcode] << '\n';
    }

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

      case Opcode::ReadMemberValue: {
        this->op_readmembervalue();
        break;
      }

      case Opcode::ReadArrayIndex: {
        uint32_t index = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        this->op_readarrayindex(index);
        break;
      }

      case Opcode::SetLocalPush: {
        uint32_t index = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        uint32_t level = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode) + sizeof(uint32_t));
        this->op_setlocalpush(index, level);
        break;
      }

      case Opcode::SetMemberSymbolPush: {
        VALUE symbol = *reinterpret_cast<VALUE*>(this->ip + sizeof(Opcode));
        this->op_setmembersymbolpush(symbol);
        break;
      }

      case Opcode::SetMemberValuePush: {
        this->op_setmembervaluepush();
        break;
      }

      case Opcode::SetArrayIndexPush: {
        uint32_t index = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        this->op_setarrayindexpush(index);
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

      case Opcode::SetMemberValue: {
        this->op_setmembervalue();
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
        bool has_parent_class = *reinterpret_cast<bool*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(uint32_t) +
                                                         sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t));
        bool has_constructor =
            *reinterpret_cast<bool*>(this->ip + sizeof(Opcode) + sizeof(VALUE) + sizeof(uint32_t) + sizeof(uint32_t) +
                                     sizeof(uint32_t) + sizeof(uint32_t) + sizeof(bool));
        this->op_putclass(name, propertycount, staticpropertycount, methodcount, staticmethodcount, has_parent_class,
                          has_constructor);
        break;
      }

      case Opcode::Pop: {
        this->op_pop();
        break;
      }

      case Opcode::Dup: {
        this->op_dup();
        break;
      }

      case Opcode::Dupn: {
        uint32_t count = *reinterpret_cast<uint32_t*>(this->ip + sizeof(Opcode));
        this->op_dupn(count);
        break;
      }

      case Opcode::Swap: {
        this->op_swap();
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
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->add(left, right));
        break;
      }

      case Opcode::Sub: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->sub(left, right));
        break;
      }

      case Opcode::Mul: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->mul(left, right));
        break;
      }

      case Opcode::Div: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->div(left, right));
        break;
      }

      case Opcode::Mod: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->mod(left, right));
        break;
      }

      case Opcode::Pow: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->pow(left, right));
        break;
      }

      case Opcode::UAdd: {
        VALUE value = this->pop_stack();
        this->push_stack(this->uadd(value));
        break;
      }

      case Opcode::USub: {
        VALUE value = this->pop_stack();
        this->push_stack(this->usub(value));
        break;
      }

      case Opcode::Eq: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->eq(left, right));
        break;
      }

      case Opcode::Neq: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->neq(left, right));
        break;
      }

      case Opcode::Lt: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->lt(left, right));
        break;
      }

      case Opcode::Gt: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->gt(left, right));
        break;
      }

      case Opcode::Le: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->le(left, right));
        break;
      }

      case Opcode::Ge: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->ge(left, right));
        break;
      }

      case Opcode::UNot: {
        VALUE value = this->pop_stack();
        this->push_stack(this->unot(value));
        break;
      }

      case Opcode::Shr: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->shr(left, right));
        break;
      }

      case Opcode::Shl: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->shl(left, right));
        break;
      }

      case Opcode::And: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->band(left, right));
        break;
      }

      case Opcode::Or: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->bor(left, right));
        break;
      }

      case Opcode::Xor: {
        VALUE right = this->pop_stack();
        VALUE left = this->pop_stack();
        this->push_stack(this->bxor(left, right));
        break;
      }

      case Opcode::UBNot: {
        VALUE value = this->pop_stack();
        this->push_stack(this->ubnot(value));
        break;
      }

      case Opcode::Halt: {
        this->halted = true;
        break;
      }

      case Opcode::GCCollect: {
        this->gc.collect();
        break;
      }

      case Opcode::Typeof: {
        this->op_typeof();
        break;
      }

      default: {
        this->throw_exception("Unknown opcode: " + kOpcodeMnemonics[opcode]);
        break;
      }
    }

    // Increment ip if the instruction didn't change it
    if (this->ip == old_ip) {
      this->ip += instruction_length;
    }

    // Add an entry to the instruction profile with the time it took to execute this instruction
    if (this->context.instruction_profile) {
      std::chrono::duration<double> exec_duration = std::chrono::high_resolution_clock::now() - exec_start;
      uint64_t duration_in_nanoseconds = static_cast<uint32_t>(exec_duration.count() * 1000000000);
      if (this->context.instruction_profile) {
        this->instruction_profile.add_entry(opcode, duration_in_nanoseconds);
      }
    }
  }
}

void VM::exec_prelude() {
  // Charly = {
  //   internals: {
  //     get_method: <Internals::get_method>
  //   }
  // }
  this->top_frame = this->create_frame(kNull, this->frames, 19, nullptr);
  this->op_putcfunction(this->context.symtable("get_method"), reinterpret_cast<uintptr_t>(Internals::get_method), 1);
  this->op_putvalue(this->context.symtable("get_method"));
  this->op_puthash(1);
  this->op_putvalue(this->context.symtable("internals"));
  this->op_puthash(1);
  this->op_setlocal(18, 0);
  this->op_pop();
}

void VM::exec_block(InstructionBlock* block) {
  uint8_t* old_ip = this->ip;
  this->ip = block->get_data();
  this->run();
  this->ip = old_ip;
}

void VM::exec_module(InstructionBlock* block) {
  ManagedContext lalloc(*this);
  Object* export_obj = charly_as_object(lalloc.create_object(0));

  // Execute the module inclusion function
  this->exec_block(block);
  this->push_stack(charly_create_pointer(export_obj));
  this->op_call(1);
  this->frames->parent_environment_frame = this->top_frame;
  this->run();
}
}  // namespace Charly
