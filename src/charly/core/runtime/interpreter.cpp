/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

#include <alloca.h>
#include <unordered_set>

#include "charly/core/compiler/compiler.h"
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"

namespace charly::core::runtime {

using namespace charly::core::compiler;
using namespace charly::core::compiler::ir;

#define THROW_NOT_IMPLEMENTED()                                                                                   \
  thread->throw_value(                                                                                            \
    thread->runtime()->create_string_from_template(thread, "Opcode '%' has not been implemented yet", op->name)); \
  return ContinueMode::Exception

Frame::Frame(Thread* thread, RawFunction function) :
  thread(thread),
  parent(thread->frame()),
  function(function),
  shared_function_info(nullptr),
  locals(nullptr),
  stack(nullptr),
  oldip(0),
  ip(0),
  sp(0),
  argc(0) {
  thread->push_frame(this);
}

Frame::~Frame() {
  thread->pop_frame(this);
}

RawValue Frame::pop(uint8_t count) {
  DCHECK(count >= 1);
  DCHECK(this->sp >= count);
  RawValue top = kNull;
  while (count--) {
    top = this->stack[this->sp - 1];
    this->sp--;
  }
  return top;
}

RawValue Frame::peek(uint8_t depth) const {
  DCHECK(this->sp > depth);
  return this->stack[this->sp - 1 - depth];
}

RawValue* Frame::top_n(uint8_t count) const {
  DCHECK(count <= shared_function_info->ir_info.stacksize);
  return &this->stack[this->sp - count];
}

void Frame::push(RawValue value) {
  DCHECK(this->sp < shared_function_info->ir_info.stacksize);
  this->stack[this->sp] = value;
  this->sp++;
}

const ExceptionTableEntry* Frame::find_active_exception_table_entry(uintptr_t thread_ip) const {
  for (const ExceptionTableEntry& entry : shared_function_info->exception_table) {
    if (thread_ip >= entry.begin_ptr && thread_ip < entry.end_ptr) {
      return &entry;
    }
  }

  return nullptr;
}

const StringTableEntry& Frame::get_string_table_entry(uint16_t index) const {
  const auto& info = *shared_function_info;
  CHECK(index < info.string_table.size());
  return info.string_table[index];
}

RawValue Interpreter::call_value(Thread* thread, RawValue self, RawValue target, RawValue* arguments, uint32_t argc) {
  Runtime* runtime = thread->runtime();

  if (target.isFunction()) {
    auto function = RawFunction::cast(target);
    return Interpreter::call_function(thread, self, function, arguments, argc);
  } else if (target.isBuiltinFunction()) {
    auto function = RawBuiltinFunction::cast(target);
    return function.function()(thread, arguments, argc);
  } else if (target.isClass()) {
    auto klass = RawClass::cast(target);

    if (klass.flags() & RawClass::kFlagNonConstructable) {
      thread->throw_value(runtime->create_string_from_template(thread, "cannot instantiate class '%'", klass.name()));
      return kErrorException;
    }

    auto instance = thread->runtime()->create_instance(thread, klass);
    RawFunction constructor = RawFunction::cast(klass.constructor());
    return Interpreter::call_function(thread, instance, constructor, arguments, argc);
  }

  thread->throw_value(runtime->create_string_from_template(thread, "called value is not a function"));
  return kErrorException;
}

RawValue Interpreter::call_function(
  Thread* thread, RawValue self, RawFunction function, RawValue* arguments, uint32_t argc) {
  Runtime* runtime = thread->runtime();

  // find the correct overload to call
  if (function.overload_table().isTuple()) {
    auto overload_table = RawTuple::cast(function.overload_table());
    DCHECK(overload_table.size() > 0);
    function = overload_table.field_at<RawFunction>(std::min(argc, overload_table.size() - 1));
  }

  Frame frame(thread, function);
  frame.self = self;
  frame.locals = nullptr;
  frame.stack = nullptr;
  frame.return_value = kNull;
  frame.oldip = 0;
  frame.ip = 0;
  frame.sp = 0;
  frame.argc = argc;

  //  if (frame.parent) {
  //    uintptr_t parent_base = bitcast<uintptr_t>(frame.parent);
  //    uintptr_t current_base = bitcast<uintptr_t>(&frame);
  //    size_t frame_size = parent_base - current_base;
  //    debuglnf("frame size: %", frame_size);
  //  }

  SharedFunctionInfo* shared_info = function.shared_info();
  frame.shared_function_info = shared_info;
  frame.ip = shared_info->bytecode_base_ptr;
  frame.oldip = frame.ip;

  // stack overflow check
  const Stack* stack = thread->stack();
  auto frame_address = (uintptr_t)__builtin_frame_address(0);
  auto stack_bottom_address = (uintptr_t)stack->lo();
  size_t remaining_bytes_on_stack = frame_address - stack_bottom_address;
  if (remaining_bytes_on_stack <= kStackOverflowLimit) {
    debuglnf("thread % stack overflow", thread->id());
    thread->throw_value(runtime->create_string_from_template(thread, "thread % stack overflow", thread->id()));
    return kErrorException;
  }

  // allocate stack space for local variables and the stack
  uint8_t localcount = shared_info->ir_info.local_variables;
  uint8_t stacksize = shared_info->ir_info.stacksize;
  uint16_t local_stack_buffer_slot_count = localcount + stacksize;
  auto* local_stack_buffer = static_cast<RawValue*>(alloca(sizeof(RawValue) * local_stack_buffer_slot_count));
  RawValue* local_ptr = local_stack_buffer;
  RawValue* stack_ptr = local_stack_buffer + localcount;
  frame.locals = local_ptr;
  frame.stack = stack_ptr;
  for (uint16_t i = 0; i < local_stack_buffer_slot_count; i++) {
    local_stack_buffer[i] = kNull;
  }

  // setup frame context
  uint8_t has_frame_context = shared_info->ir_info.has_frame_context;
  uint8_t heap_variables = shared_info->ir_info.heap_variables;
  if (has_frame_context) {
    auto context = runtime->create_tuple(thread, RawFunction::kContextHeapVariablesOffset + heap_variables);
    context.set_field_at(RawFunction::kContextParentOffset, function.context());
    context.set_field_at(RawFunction::kContextSelfOffset, self);
    frame.context = context;
  } else {
    frame.context = function.context();
  }

  if (argc < shared_info->ir_info.minargc) {
    thread->throw_value(
      runtime->create_string_from_template(thread, "not enough arguments for function call, expected % but got %",
                                           (uint32_t)shared_info->ir_info.minargc, (uint32_t)argc));
    return kErrorException;
  }

  // regular functions may not be called with more arguments than they declare
  // the exception to this rule are arrow functions and functions that declare a spread argument
  if (argc > shared_info->ir_info.argc && !shared_info->ir_info.spread_argument &&
      !shared_info->ir_info.arrow_function) {
    thread->throw_value(runtime->create_string_from_template(
      thread, "too many arguments for non-spread function '%', expected at most % but got %", function.name(),
      (uint32_t)shared_info->ir_info.argc, (uint32_t)argc));
    return kErrorException;
  }

  // copy function arguments into local variables
  uint8_t func_argc = shared_info->ir_info.argc;
  bool func_has_spread = shared_info->ir_info.spread_argument;
  DCHECK(localcount >= func_argc);
  if (func_has_spread) {
    for (uint8_t i = 0; i < argc && i < func_argc; i++) {
      DCHECK(arguments);
      frame.locals[i] = arguments[i];
    }

    if (argc < func_argc) {
      // copy arguments into local slots
      for (uint8_t i = 0; i < argc; i++) {
        DCHECK(arguments);
        frame.locals[i] = arguments[i];
      }

      // initialize spread argument with empty tuple
      frame.locals[func_argc] = runtime->create_tuple(thread, 0);
    } else {
      // copy arguments into local slots
      for (uint8_t i = 0; i < func_argc; i++) {
        DCHECK(arguments);
        frame.locals[i] = arguments[i];
      }

      // initialize spread argument with remaining arguments
      uint32_t remaining_arguments = argc - func_argc;
      auto spread_args = runtime->create_tuple(thread, remaining_arguments);
      for (uint8_t i = 0; i < remaining_arguments; i++) {
        DCHECK(arguments);
        spread_args.set_field_at(i, arguments[func_argc + i]);
      }
      frame.locals[func_argc] = spread_args;
    }
  } else {
    for (uint8_t i = 0; i < argc && i < func_argc; i++) {
      DCHECK(arguments);
      frame.locals[i] = arguments[i];
    }
  }

  // copy self from context if this is an arrow function
  if (shared_info->ir_info.arrow_function) {
    frame.self = function.saved_self();
  }

  thread->checkpoint();

  return Interpreter::execute(thread);
}

RawValue Interpreter::add_string_string(Thread* thread, RawString left, RawString right) {
  size_t left_size = left.length();
  size_t right_size = right.length();
  size_t total_size = left_size + right_size;

  utils::Buffer buf(total_size);
  buf.write(RawString::data(&left), left_size);
  buf.write(RawString::data(&right), right_size);

  if (total_size <= RawSmallString::kMaxLength) {
    return RawSmallString::make_from_memory(buf.data(), total_size);
  } else if (total_size <= RawLargeString::kMaxLength) {
    return thread->runtime()->create_string(thread, buf.data(), total_size, buf.hash());
  } else {
    SYMBOL buf_hash = buf.hash();
    char* buffer = buf.release_buffer();
    CHECK(buffer, "could not release buffer");
    return thread->runtime()->acquire_string(thread, buffer, total_size, buf_hash);
  }
}

RawValue Interpreter::execute(Thread* thread) {
  Frame* frame = thread->frame();

  // dispatch table to opcode handlers
  static void* OPCODE_DISPATCH_TABLE[] = {
#define OP(N, ...) &&execute_handler_##N,
    FOREACH_OPCODE(OP)
#undef OP
  };

  ContinueMode continue_mode;
  Instruction* op;

  auto next_handler = [&]() __attribute__((always_inline))->void* {
    op = bitcast<Instruction*>(frame->ip);
    Opcode opcode = op->opcode();
    frame->oldip = frame->ip;
    frame->ip += kInstructionLength;
    return OPCODE_DISPATCH_TABLE[opcode];
  };

  goto* next_handler();

#define OP(N, ...)                                                   \
  execute_handler_##N : {                                            \
    continue_mode = Interpreter::opcode_##N(thread, frame, op->N()); \
    if (continue_mode == ContinueMode::Next)                         \
      goto* next_handler();                                          \
    goto handle_return_or_exception;                                 \
  }
  FOREACH_OPCODE(OP)
#undef OP

handle_return_or_exception:
  switch (continue_mode) {
    case ContinueMode::Return: {
      return frame->return_value;
    }

    case ContinueMode::Exception: {
      // check if the current frame can handle this exception
      if (const ExceptionTableEntry* entry = frame->find_active_exception_table_entry(op->ip())) {
        frame->ip = entry->handler_ptr;
        frame->sp = 0;  // clear stack
        goto* next_handler();
      }

      return kErrorException;
    }

    default: {
      FAIL("unexpected continue mode");
    }
  }
}

#define OP(name)                                                                       \
  Interpreter::ContinueMode __attribute__((always_inline)) Interpreter::opcode_##name( \
    [[maybe_unused]] Thread* thread, [[maybe_unused]] Frame* frame, [[maybe_unused]] Instruction_##name* op)

OP(nop) {
  return ContinueMode::Next;
}

OP(panic) {
  debuglnf("panic in thread % in % at %", thread->id(), frame->function, (void*)frame->ip);
  thread->abort(1);
}

OP(import) {
  Runtime* runtime = thread->runtime();
  auto file_path_string = RawString::cast(frame->pop()).str();
  auto module_path_string = RawString::cast(frame->pop()).str();
  auto file_path = fs::path(file_path_string);
  auto module_path = fs::path(module_path_string);

  // TODO: perform filesystem lookup and compilation in native mode

  // attempt to resolve the module path to a real file path
  std::optional<fs::path> resolve_result = runtime->resolve_module(module_path, file_path);
  if (!resolve_result.has_value()) {
    thread->throw_value(
      runtime->create_string_from_template(thread, "could not resolve '%' to a valid path", module_path_string));
    return ContinueMode::Exception;
  }
  fs::path import_path = resolve_result.value();

  RawValue import_result = runtime->import_module(thread, import_path);
  if (import_result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->push(import_result);
  return ContinueMode::Next;
}

OP(stringconcat) {
  uint8_t count = op->arg();

  utils::Buffer buffer;
  for (int64_t depth = count - 1; depth >= 0; depth--) {
    frame->peek(depth).to_string(buffer);
  }

  SYMBOL buf_hash = buffer.hash();
  size_t buf_size = buffer.size();
  char* buf_ptr = buffer.release_buffer();
  CHECK(buf_ptr, "could not release buffer");

  frame->pop(count);
  frame->push(thread->runtime()->acquire_string(thread, buf_ptr, buf_size, buf_hash));

  return ContinueMode::Next;
}

OP(declareglobal) {
  uint8_t string_index = op->arg();
  SYMBOL name = frame->get_string_table_entry(string_index).hash;
  RawValue result = thread->runtime()->declare_global_variable(thread, name, false);

  if (result.is_error_exception()) {
    Runtime* runtime = thread->runtime();
    thread->throw_value(runtime->create_string_from_template(thread, "duplicate declaration of global variable %",
                                                             RawSymbol::make(name)));
    return ContinueMode::Exception;
  }
  DCHECK(result.is_error_ok());

  return ContinueMode::Next;
}

OP(declareglobalconst) {
  uint8_t string_index = op->arg();
  SYMBOL name = frame->get_string_table_entry(string_index).hash;
  RawValue result = thread->runtime()->declare_global_variable(thread, name, true);

  if (result.is_error_exception()) {
    Runtime* runtime = thread->runtime();
    thread->throw_value(runtime->create_string_from_template(thread, "duplicate declaration of global variable %",
                                                             RawSymbol::make(name)));
    return ContinueMode::Exception;
  }
  DCHECK(result.is_error_ok());

  return ContinueMode::Next;
}

OP(type) {
  RawValue value = frame->pop();

  Runtime* runtime = thread->runtime();
  frame->push(runtime->lookup_class(thread, value));

  return ContinueMode::Next;
}

OP(swap) {
  RawValue v1 = frame->pop();
  RawValue v2 = frame->pop();
  frame->push(v1);
  frame->push(v2);
  return ContinueMode::Next;
}

OP(pop) {
  frame->pop();
  return ContinueMode::Next;
}

OP(dup) {
  frame->push(frame->peek());
  return ContinueMode::Next;
}

OP(dup2) {
  RawValue top1 = frame->peek(1);
  RawValue top2 = frame->peek(0);
  frame->push(top1);
  frame->push(top2);
  return ContinueMode::Next;
}

OP(jmp) {
  int16_t offset = op->arg();
  frame->ip = op->ip() + offset;
  return ContinueMode::Next;
}

OP(jmpf) {
  RawValue condition = frame->pop();
  if (!condition.truthyness()) {
    int16_t offset = op->arg();
    frame->ip = op->ip() + offset;
  }
  return ContinueMode::Next;
}

OP(jmpt) {
  RawValue condition = frame->pop();
  if (condition.truthyness()) {
    int16_t offset = op->arg();
    frame->ip = op->ip() + offset;
  }
  return ContinueMode::Next;
}

OP(argcjmp) {
  uint8_t argc = frame->argc;
  uint8_t check = op->arg1();

  if (argc == check) {
    int16_t offset = op->arg2();
    frame->ip = op->ip() + offset;
  }

  return ContinueMode::Next;
}

OP(throwex) {
  RawValue value = frame->pop();
  thread->throw_value(value);
  return ContinueMode::Exception;
}

OP(rethrowex) {
  RawValue value = frame->pop();
  thread->rethrow_value(value);
  return ContinueMode::Exception;
}

OP(getpendingexception) {
  frame->push(thread->pending_exception());
  return ContinueMode::Next;
}

OP(setpendingexception) {
  RawValue value = frame->pop();
  thread->set_pending_exception(value);
  return ContinueMode::Next;
}

OP(call) {
  // stack layout
  //
  // +-----------+
  // | Arg n     | <- top of stack
  // +-----------+
  // | Arg 2     |
  // +-----------+
  // | Arg 1     |
  // +-----------+
  // | Function  |
  // +-----------+
  // | Self      |
  // +-----------+
  uint32_t argc = op->arg();
  RawValue* args = frame->top_n(argc);
  RawValue callee = frame->peek(argc);
  RawValue self = frame->peek(argc + 1);

  RawValue rval = Interpreter::call_value(thread, self, callee, args, argc);

  if (rval.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->pop(argc + 2);
  frame->push(rval);
  return ContinueMode::Next;
}

OP(callspread) {
  uint32_t segment_count = op->arg();

  // pop segments from stack
  uint32_t total_arg_count = 0;
  RawTuple* segments = static_cast<RawTuple*>(frame->top_n(segment_count));
  for (uint32_t i = 0; i < segment_count; i++) {
    auto segment = RawTuple::cast(segments[i]);
    total_arg_count += segment.size();
    CHECK(total_arg_count <= kUInt32Max);
  }

  // unpack segments and copy arguments into local array
  RawValue arguments[total_arg_count];
  uint32_t last_written_index = 0;
  for (uint32_t i = 0; i < segment_count; i++) {
    auto segment = segments[i];
    for (uint32_t j = 0; j < segment.size(); j++) {
      arguments[last_written_index++] = segment.field_at(j);
    }
  }

  RawValue callee = frame->peek(segment_count);
  RawValue self = frame->peek(segment_count + 1);

  RawValue rval = Interpreter::call_value(thread, self, callee, arguments, total_arg_count);

  if (rval.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->pop(segment_count + 2);
  frame->push(rval);
  return ContinueMode::Next;
}

OP(ret) {
  return ContinueMode::Return;
}

OP(loadconst) {
  uint16_t index = op->arg();
  RawValue value = frame->shared_function_info->constant_table[index];
  frame->push(value);
  return ContinueMode::Next;
}

OP(loadsmi) {
  auto value = bitcast<uintptr_t>(static_cast<size_t>(op->arg()));
  frame->push(RawValue(value));
  return ContinueMode::Next;
}

OP(loadself) {
  frame->push(frame->self);
  return ContinueMode::Next;
}

OP(loadfarself) {
  uint8_t depth = op->arg();

  RawTuple context = RawTuple::cast(frame->context);
  while (depth) {
    context = context.field_at<RawTuple>(RawFunction::kContextParentOffset);
    depth--;
  }

  frame->push(context.field_at(RawFunction::kContextSelfOffset));
  return ContinueMode::Next;
}

OP(loadglobal) {
  uint8_t string_index = op->arg();
  SYMBOL name = frame->get_string_table_entry(string_index).hash;
  RawValue result = thread->runtime()->read_global_variable(thread, name);

  if (result.is_error_not_found()) {
    Runtime* runtime = thread->runtime();
    thread->throw_value(
      runtime->create_string_from_template(thread, "unknown global variable %", RawSymbol::make(name)));
    return ContinueMode::Exception;
  }
  DCHECK(!result.is_error());

  frame->push(result);
  return ContinueMode::Next;
}

OP(loadlocal) {
  uint8_t index = op->arg();
  DCHECK(index < frame->shared_function_info->ir_info.local_variables);
  frame->push(frame->locals[index]);
  return ContinueMode::Next;
}

OP(loadfar) {
  uint8_t depth = op->arg1();
  uint8_t index = op->arg2();

  RawTuple context = RawTuple::cast(frame->context);
  while (depth) {
    context = context.field_at<RawTuple>(RawFunction::kContextParentOffset);
    depth--;
  }

  frame->push(context.field_at(RawFunction::kContextHeapVariablesOffset + index));
  return ContinueMode::Next;
}

OP(loadattr) {
  RawValue index = frame->pop();
  RawValue value = frame->pop();

  // tuple[int]
  if (value.isTuple() && index.isInt()) {
    RawTuple tuple = RawTuple::cast(value);
    int64_t tuple_size = tuple.size();
    int64_t index_int = RawInt::cast(index).value();

    // wrap negative indices
    if (index_int < 0) {
      index_int += tuple_size;
      if (index_int < 0) {
        frame->push(kNull);
        return ContinueMode::Next;
      }
    }

    if (index_int >= tuple_size) {
      frame->push(kNull);
      return ContinueMode::Next;
    }

    frame->push(tuple.field_at(index_int));
    return ContinueMode::Next;
  }

  frame->push(kNull);
  return ContinueMode::Next;
}

OP(loadattrsym) {
  Runtime* runtime = thread->runtime();

  RawValue value = frame->pop();
  uint16_t symbol_offset = op->arg();
  SYMBOL attr = frame->get_string_table_entry(symbol_offset).hash;

  // builtin attributes
  switch (attr) {
    case SYM("klass"): {
      frame->push(runtime->lookup_class(thread, value));
      return ContinueMode::Next;
    }
    case SYM("length"): {
      if (value.isTuple()) {
        auto tuple = RawTuple::cast(value);
        frame->push(RawInt::make(tuple.size()));
        return ContinueMode::Next;
      } else if (value.isString()) {
        auto string = RawString::cast(value);
        frame->push(RawInt::make(string.length()));
        return ContinueMode::Next;
      } else if (value.isBytes()) {
        auto bytes = RawBytes::cast(value);
        frame->push(RawInt::make(bytes.length()));
        return ContinueMode::Next;
      }
      break;
    }
  }

  auto klass = runtime->lookup_class(thread, value);
  if (value.isInstance()) {
    auto instance = RawInstance::cast(value);

    // lookup attribute in shape key table
    // TODO: cache result via inline cache
    auto shape = runtime->lookup_shape(thread, instance.shape_id());
    auto result = shape.lookup_symbol(attr);
    if (result.found) {
      // TODO: allow accessing private member of same class
      if (result.is_private() && runtime->check_private_access_permitted(thread, instance) <= result.offset) {
        thread->throw_value(runtime->create_string_from_template(
          thread, "cannot read private property '%' of class '%'", RawSymbol::make(attr), klass.name()));
        return ContinueMode::Exception;
      }

      frame->push(instance.field_at(result.offset));
      return ContinueMode::Next;
    }
  }

  // lookup attribute in class hierarchy
  // TODO: cache via inline cache
  RawValue lookup = klass.lookup_function(attr);
  if (lookup.isFunction()) {
    auto function = RawFunction::cast(lookup);
    // TODO: allow accessing private member of same class
    if (function.shared_info()->ir_info.private_function && (value != frame->self)) {
      thread->throw_value(runtime->create_string_from_template(thread, "cannot call private function '%' of class '%'",
                                                               RawSymbol::make(attr), klass.name()));
      return ContinueMode::Exception;
    }

    frame->push(lookup);
    return ContinueMode::Next;
  }

  if (value.isNull()) {
    thread->throw_value(
      runtime->create_string_from_template(thread, "null has no field called '%'", RawSymbol::make(attr)));
    return ContinueMode::Exception;
  } else {
    frame->push(kNull);
    return ContinueMode::Next;
  }
}

OP(loadsuperconstructor) {
  auto host_class = RawClass::cast(frame->function.host_class());
  auto parent_klass = RawClass::cast(host_class.parent());
  auto parent_constructor = parent_klass.constructor();
  frame->push(parent_constructor);
  return ContinueMode::Next;
}

OP(loadsuperattr) {
  uint8_t string_index = op->arg();
  SYMBOL name = frame->get_string_table_entry(string_index).hash;

  auto klass = RawClass::cast(frame->function.host_class());
  auto parent = RawClass::cast(klass.parent());
  auto func = parent.lookup_function(name);

  if (func.is_error_not_found()) {
    auto runtime = thread->runtime();
    thread->throw_value(runtime->create_string_from_template(
      thread, "super class '%' has no member function called '%'", parent.name(), RawSymbol::make(name)));
    return ContinueMode::Exception;
  }

  frame->push(func);
  return ContinueMode::Next;
}

OP(setglobal) {
  uint8_t string_index = op->arg();
  SYMBOL name = frame->get_string_table_entry(string_index).hash;
  RawValue value = frame->peek();
  RawValue result = thread->runtime()->set_global_variable(thread, name, value);
  Runtime* runtime = thread->runtime();

  if (result.is_error_not_found()) {
    thread->throw_value(
      runtime->create_string_from_template(thread, "unknown global variable %", RawSymbol::make(name)));
    return ContinueMode::Exception;
  } else if (result.is_error_read_only()) {
    thread->throw_value(
      runtime->create_string_from_template(thread, "write to const global variable %", RawSymbol::make(name)));
    return ContinueMode::Exception;
  }
  DCHECK(result.is_error_ok());

  return ContinueMode::Next;
}

OP(setlocal) {
  RawValue top = frame->peek();
  uint8_t index = op->arg();
  DCHECK(index < frame->shared_function_info->ir_info.local_variables);
  frame->locals[index] = top;
  return ContinueMode::Next;
}

OP(setreturn) {
  frame->return_value = frame->pop();
  return ContinueMode::Next;
}

OP(setfar) {
  uint8_t depth = op->arg1();
  uint8_t index = op->arg2();

  RawTuple context = RawTuple::cast(frame->context);
  while (depth) {
    context = context.field_at<RawTuple>(RawFunction::kContextParentOffset);
    depth--;
  }

  context.set_field_at(RawFunction::kContextHeapVariablesOffset + index, frame->peek());
  return ContinueMode::Next;
}

OP(setattr) {
  RawValue value = frame->pop();
  RawValue index = frame->pop();
  RawValue target = frame->pop();

  // tuple[int] = value
  if (target.isTuple() && index.isInt()) {
    RawTuple tuple = RawTuple::cast(target);
    uint32_t tuple_size = tuple.size();
    int64_t index_int = RawInt::cast(index).value();

    // wrap negative indices
    if (index_int < 0) {
      index_int += tuple_size;
      if (index_int < 0) {
        frame->push(kNull);
        return ContinueMode::Next;
      }
    }

    if (index_int >= tuple_size) {
      frame->push(kNull);
      return ContinueMode::Next;
    }

    tuple.set_field_at(index_int, value);
    frame->push(value);
    return ContinueMode::Next;
  }

  frame->push(kNull);
  return ContinueMode::Next;
}

OP(setattrsym) {
  Runtime* runtime = thread->runtime();

  RawValue value = frame->pop();
  RawValue target = frame->pop();
  uint16_t symbol_offset = op->arg();
  SYMBOL attr = frame->get_string_table_entry(symbol_offset).hash;

  // lookup attribute in shape key table
  // TODO: cache result via inline cache
  auto klass = runtime->lookup_class(thread, target);
  if (target.isInstance()) {
    auto instance = RawInstance::cast(target);
    auto shape = runtime->lookup_shape(thread, instance.shape_id());
    auto result = shape.lookup_symbol(attr);
    if (result.found) {
      if (result.is_read_only()) {
        thread->throw_value(runtime->create_string_from_template(thread, "property '%' of type '%' is read-only",
                                                                 RawSymbol::make(attr), klass.name()));
        return ContinueMode::Exception;
      }

      if (result.is_private() && runtime->check_private_access_permitted(thread, instance) <= result.offset) {
        thread->throw_value(runtime->create_string_from_template(
          thread, "cannot assign to private property '%' of class '%'", RawSymbol::make(attr), klass.name()));
        return ContinueMode::Exception;
      }

      instance.set_field_at(result.offset, value);
      frame->push(target);
      return ContinueMode::Next;
    }
  }

  thread->throw_value(runtime->create_string_from_template(thread, "value of type '%' has no property called '%'",
                                                           klass.name(), RawSymbol::make(attr)));
  return ContinueMode::Exception;
}

OP(unpacksequence) {
  uint8_t count = op->arg();
  Runtime* runtime = thread->runtime();

  RawValue value = frame->pop();

  if (value.isTuple()) {
    auto tuple = RawTuple::cast(value);
    uint32_t tuple_size = tuple.size();

    if (tuple_size != count) {
      thread->throw_value(runtime->create_string_from_template(thread, "expected tuple to be of size %, not %",
                                                               (size_t)count, tuple_size));
      return ContinueMode::Exception;
    }

    // push values in reverse so that values can be assigned to their
    // target fields in source order
    for (int64_t i = tuple_size - 1; i >= 0; i--) {
      frame->push(tuple.field_at(i));
    }

    return ContinueMode::Next;
  } else {
    thread->throw_value(runtime->create_string_from_template(thread, "value is not a sequence"));
    return ContinueMode::Exception;
  }
}

OP(unpacksequencespread) {
  Runtime* runtime = thread->runtime();
  uint8_t before_count = op->arg1();
  uint8_t after_count = op->arg2();
  uint16_t total_count = before_count + after_count;

  RawValue value = frame->pop();

  if (value.isTuple()) {
    auto tuple = RawTuple::cast(value);
    uint32_t tuple_size = tuple.size();
    if (tuple_size < total_count) {
      thread->throw_value(
        runtime->create_string_from_template(thread, "touple does not contain enough values to unpack"));
      return ContinueMode::Exception;
    }

    // push the values after the spread
    for (uint8_t i = 0; i < after_count; i++) {
      frame->push(tuple.field_at(tuple_size - i - 1));
    }

    // put spread arguments in a tuple
    uint32_t spread_count = tuple_size - total_count;
    auto spread_tuple = runtime->create_tuple(thread, spread_count);
    for (uint32_t i = 0; i < spread_count; i++) {
      spread_tuple.set_field_at(i, tuple.field_at(before_count + i));
    }
    frame->push(spread_tuple);

    // push the values before the spread
    for (uint8_t i = 0; i < before_count; i++) {
      frame->push(tuple.field_at(before_count - i - 1));
    }

    return ContinueMode::Next;
  } else {
    thread->throw_value(runtime->create_string_from_template(thread, "value is not a sequence"));
    return ContinueMode::Exception;
  }
}

OP(unpackobject) {
  THROW_NOT_IMPLEMENTED();
}

OP(unpackobjectspread) {
  THROW_NOT_IMPLEMENTED();
}

OP(makefunc) {
  int16_t offset = op->arg();
  SharedFunctionInfo* shared_data = *bitcast<SharedFunctionInfo**>(op->ip() + offset);
  RawFunction func =
    RawFunction::cast(thread->runtime()->create_function(thread, frame->context, shared_data, frame->self));
  frame->push(func);
  return ContinueMode::Next;
}

OP(makeclass) {
  auto static_prop_values = RawTuple::cast(frame->pop());
  auto static_prop_keys = RawTuple::cast(frame->pop());
  auto static_functions = RawTuple::cast(frame->pop());
  auto member_props = RawTuple::cast(frame->pop());
  auto member_functions = RawTuple::cast(frame->pop());
  auto constructor = RawFunction::cast(frame->pop());
  auto parent_value = frame->pop();
  auto name = RawSymbol::cast(frame->pop());
  auto flags = RawInt::cast(frame->pop());

  // attempt to create the new class
  RawValue result =
    thread->runtime()->create_class(thread, name, parent_value, constructor, member_props, member_functions,
                                    static_prop_keys, static_prop_values, static_functions, flags.value());

  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->push(RawClass::cast(result));
  return ContinueMode::Next;
}

OP(makestr) {
  uint16_t index = op->arg();

  const SharedFunctionInfo& shared_info = *frame->shared_function_info;
  DCHECK(index < shared_info.string_table.size());
  const StringTableEntry& entry = shared_info.string_table[index];
  frame->push(thread->runtime()->create_string(thread, entry.value.data(), entry.value.size(), entry.hash));

  return ContinueMode::Next;
}

OP(makelist) {
  THROW_NOT_IMPLEMENTED();
}

OP(makelistspread) {
  THROW_NOT_IMPLEMENTED();
}

OP(makedict) {
  THROW_NOT_IMPLEMENTED();
}

OP(makedictspread) {
  THROW_NOT_IMPLEMENTED();
}

OP(maketuple) {
  Runtime* runtime = thread->runtime();
  int16_t count = op->arg();
  RawTuple tuple = RawTuple::cast(runtime->create_tuple(thread, count));

  while (count--) {
    tuple.set_field_at(count, frame->pop());
  }

  frame->push(tuple);

  return ContinueMode::Next;
}

OP(maketuplespread) {
  Runtime* runtime = thread->runtime();
  uint32_t segment_count = op->arg();

  // pop segments from stack
  uint32_t total_arg_count = 0;
  RawTuple* segments = static_cast<RawTuple*>(frame->top_n(segment_count));
  for (uint32_t i = 0; i < segment_count; i++) {
    auto segment = RawTuple::cast(segments[i]);
    total_arg_count += segment.size();
    CHECK(total_arg_count <= kUInt32Max);
  }

  // unpack segments and copy arguments into new tuple
  auto tuple = runtime->create_tuple(thread, total_arg_count);
  uint32_t last_written_index = 0;
  for (uint32_t i = 0; i < segment_count; i++) {
    auto segment = segments[i];
    for (uint32_t j = 0; j < segment.size(); j++) {
      tuple.set_field_at(last_written_index++, segment.field_at(j));
    }
  }

  frame->pop(segment_count);
  frame->push(tuple);
  return ContinueMode::Next;
}

OP(makefiber) {
  Runtime* runtime = thread->runtime();
  RawValue arg_argstuple = frame->pop();
  RawValue arg_function = frame->pop();
  RawValue arg_context = frame->pop();

  if (!arg_function.isFunction()) {
    thread->throw_value(runtime->create_string_from_template(thread, "argument is not a function"));
    return ContinueMode::Exception;
  }

  frame->push(runtime->create_fiber(thread, RawFunction::cast(arg_function), arg_context, arg_argstuple));

  return ContinueMode::Next;
}

OP(await) {
  Runtime* runtime = thread->runtime();
  RawValue value = frame->pop();
  RawValue result = kNull;

  if (value.isFiber()) {
    result = runtime->join_future(thread, RawFiber::cast(value).result_future());
  } else if (value.isFuture()) {
    result = runtime->join_future(thread, RawFuture::cast(value));
  } else {
    auto name = runtime->lookup_class(thread, value).name();
    thread->throw_value(runtime->create_string_from_template(thread, "value of type '%' cannot be awaited", name));
    return ContinueMode::Exception;
  }

  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  } else {
    frame->push(result);
    return ContinueMode::Next;
  }
}

OP(castbool) {
  RawValue value = frame->pop();
  frame->push(RawBool::make(value.truthyness()));
  return ContinueMode::Next;
}

OP(caststring) {
  RawValue value = frame->pop();

  utils::Buffer buffer;
  value.to_string(buffer);
  frame->push(thread->runtime()->create_string(thread, buffer.data(), buffer.size(), buffer.hash()));

  return ContinueMode::Next;
}

OP(casttuple) {
  RawValue value = frame->peek();

  if (value.isTuple()) {
    return ContinueMode::Next;
  }

  Runtime* runtime = thread->runtime();
  auto name = runtime->lookup_class(thread, value).name();
  thread->throw_value(
    runtime->create_string_from_template(thread, "could not cast value of type '%' to a tuple", name));
  return ContinueMode::Exception;
}

OP(castsymbol) {
  THROW_NOT_IMPLEMENTED();
}

OP(castiterator) {
  THROW_NOT_IMPLEMENTED();
}

OP(iteratornext) {
  THROW_NOT_IMPLEMENTED();
}

OP(add) {
  RawValue right = frame->pop();
  RawValue left = frame->pop();

  if (left.isInt() && right.isInt()) {
    RawInt result = RawInt::make(RawInt::cast(left).value() + RawInt::cast(right).value());
    frame->push(result);
    return ContinueMode::Next;
  }

  if (left.isFloat() && right.isFloat()) {
    RawFloat result = RawFloat::make(RawFloat::cast(left).value() + RawFloat::cast(right).value());
    frame->push(result);
    return ContinueMode::Next;
  }

  if ((left.isInt() || left.isFloat()) && (right.isInt() || right.isFloat())) {
    double lf = left.isInt() ? (double)RawInt::cast(left).value() : RawFloat::cast(left).value();
    double rf = right.isInt() ? (double)RawInt::cast(right).value() : RawFloat::cast(right).value();
    RawFloat result = RawFloat::make(lf + rf);
    frame->push(result);
    return ContinueMode::Next;
  }

  if (left.isString() && right.isString()) {
    RawValue result = add_string_string(thread, RawString::cast(left), RawString::cast(right));
    frame->push(result);
    return ContinueMode::Next;
  }

  frame->push(kNaN);
  return ContinueMode::Next;
}

OP(sub) {
  RawValue right = frame->pop();
  RawValue left = frame->pop();

  if (left.isInt() && right.isInt()) {
    RawInt result = RawInt::make(RawInt::cast(left).value() - RawInt::cast(right).value());
    frame->push(result);
    return ContinueMode::Next;
  }

  if (left.isFloat() && right.isFloat()) {
    RawFloat result = RawFloat::make(RawFloat::cast(left).value() - RawFloat::cast(right).value());
    frame->push(result);
    return ContinueMode::Next;
  }

  if ((left.isInt() || left.isFloat()) && (right.isInt() || right.isFloat())) {
    double lf = left.isInt() ? (double)RawInt::cast(left).value() : RawFloat::cast(left).value();
    double rf = right.isInt() ? (double)RawInt::cast(right).value() : RawFloat::cast(right).value();
    RawFloat result = RawFloat::make(lf - rf);
    frame->push(result);
    return ContinueMode::Next;
  }

  frame->push(kNaN);
  return ContinueMode::Next;
}

OP(mul) {
  THROW_NOT_IMPLEMENTED();
}

OP(div) {
  THROW_NOT_IMPLEMENTED();
}

OP(mod) {
  THROW_NOT_IMPLEMENTED();
}

OP(pow) {
  THROW_NOT_IMPLEMENTED();
}

OP(eq) {
  RawValue right = frame->pop();
  RawValue left = frame->pop();
  frame->push(RawBool::make(left.raw() == right.raw()));
  return ContinueMode::Next;
}

OP(neq) {
  RawValue right = frame->pop();
  RawValue left = frame->pop();
  frame->push(RawBool::make(left.raw() != right.raw()));
  return ContinueMode::Next;
}

OP(lt) {
  THROW_NOT_IMPLEMENTED();
}

OP(gt) {
  THROW_NOT_IMPLEMENTED();
}

OP(le) {
  THROW_NOT_IMPLEMENTED();
}

OP(ge) {
  THROW_NOT_IMPLEMENTED();
}

OP(shl) {
  THROW_NOT_IMPLEMENTED();
}

OP(shr) {
  THROW_NOT_IMPLEMENTED();
}

OP(shru) {
  THROW_NOT_IMPLEMENTED();
}

OP(band) {
  THROW_NOT_IMPLEMENTED();
}

OP(bor) {
  THROW_NOT_IMPLEMENTED();
}

OP(bxor) {
  THROW_NOT_IMPLEMENTED();
}

OP(usub) {
  RawValue value = frame->pop();

  if (value.isInt()) {
    frame->push(RawInt::make(-RawInt::cast(value).value()));
  } else if (value.isFloat()) {
    frame->push(RawFloat::make(-RawFloat::cast(value).value()));
  } else if (value.isBool()) {
    frame->push(RawBool::make(!RawBool::cast(value).value()));
  }

  return ContinueMode::Next;
}

OP(unot) {
  RawValue value = frame->pop();
  frame->push(value.truthyness() ? kFalse : kTrue);
  return ContinueMode::Next;
}

OP(ubnot) {
  THROW_NOT_IMPLEMENTED();
}

#undef OP

}  // namespace charly::core::runtime
