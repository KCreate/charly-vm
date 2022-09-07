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

#define THROW_NOT_IMPLEMENTED()                                               \
  thread->throw_message("Opcode '%' has not been implemented yet", op->name); \
  return ContinueMode::Exception

Frame::Frame(Thread* thread, Type type) :
  type(type), thread(thread), parent(thread->frame()), depth(parent ? parent->depth + 1 : 0) {
  thread->push_frame(this);
}

Frame::~Frame() {
  thread->pop_frame(this);
}

RawValue InterpreterFrame::pop(uint8_t count) {
  DCHECK(count >= 1);
  DCHECK(sp >= count);
  DCHECK(stack);
  RawValue top;
  while (count--) {
    top = stack[sp - 1];
    sp--;
  }
  return top;
}

RawValue InterpreterFrame::peek(uint8_t depth) const {
  DCHECK(sp > depth);
  DCHECK(stack);
  return stack[sp - 1 - depth];
}

RawValue* InterpreterFrame::top_n(uint8_t count) const {
  DCHECK(count <= shared_function_info->ir_info.stacksize);
  DCHECK(stack);
  return &stack[sp - count];
}

void InterpreterFrame::push(RawValue value) {
  DCHECK(sp < shared_function_info->ir_info.stacksize);
  DCHECK(stack);
  stack[sp] = value;
  sp++;
}

const ExceptionTableEntry* InterpreterFrame::find_active_exception_table_entry(uintptr_t ip) const {
  for (const ExceptionTableEntry& entry : shared_function_info->exception_table) {
    if (ip >= entry.begin_ptr && ip < entry.end_ptr) {
      return &entry;
    }
  }

  return nullptr;
}

const StringTableEntry& InterpreterFrame::get_string_table_entry(uint16_t index) const {
  const auto& info = *shared_function_info;
  CHECK(index < info.string_table.size());
  return info.string_table[index];
}

RawValue Interpreter::call_value(
  Thread* thread, RawValue self, RawValue target, RawValue* arguments, uint32_t argc, RawValue argument_tuple) {
  if (target.isFunction()) {
    auto function = RawFunction::cast(target);
    return Interpreter::call_function(thread, self, function, arguments, argc, false, argument_tuple);
  } else if (target.isBuiltinFunction()) {
    auto function = RawBuiltinFunction::cast(target);
    return Interpreter::call_builtin_function(thread, self, function, arguments, argc, argument_tuple);
  } else if (target.isClass()) {
    auto klass = RawClass::cast(target);

    if (klass.flags() & RawClass::kFlagNonConstructable) {
      return thread->throw_message("Cannot instantiate class '%'", klass.name());
    }

    auto constructor = RawFunction::cast(klass.constructor());
    return Interpreter::call_function(thread, klass, constructor, arguments, argc, true, argument_tuple);
  }

  return thread->throw_message("Cannot call value of type '%'", target.klass_name(thread));
}

RawValue Interpreter::call_function(Thread* thread,
                                    RawValue self,
                                    RawFunction function,
                                    RawValue* arguments,
                                    uint32_t argc,
                                    bool constructor_call,
                                    RawValue argument_tuple) {
  // find the correct overload to call
  if (function.overload_table().isTuple()) {
    auto overload_table = RawTuple::cast(function.overload_table());
    DCHECK(overload_table.length() > 0);
    function = overload_table.field_at<RawFunction>(std::min(argc, overload_table.length() - 1));
  }

  const SharedFunctionInfo* shared_info = function.shared_info();

  InterpreterFrame frame(thread);
  frame.function = function;
  frame.self = self;
  frame.argument_tuple = argument_tuple;
  frame.arguments = arguments;
  frame.argc = argc;
  frame.shared_function_info = shared_info;
  frame.ip = shared_info->bytecode_base_ptr;
  frame.oldip = frame.ip;

  if (frame.argument_tuple.isTuple()) {
    DCHECK((void*)frame.arguments == (void*)RawTuple::cast(frame.argument_tuple).data());
  } else {
    DCHECK(frame.argument_tuple.isNull());
  }

  //  if (frame.parent) {
  //    uintptr_t parent_base = bitcast<uintptr_t>(frame.parent);
  //    uintptr_t current_base = bitcast<uintptr_t>(&frame);
  //    size_t frame_size = parent_base - current_base;
  //    debuglnf("frame size: %", frame_size);
  //  }

  // stack overflow check
  if (Interpreter::stack_overflow_check(thread).is_error_exception()) {
    return kErrorException;
  }

  // allocate class instance and replace self value
  if (constructor_call) {
    DCHECK(shared_info->ir_info.is_constructor);
    frame.self = RawInstance::create(thread, RawClass::cast(frame.self));
  }

  // allocate stack space for local variables and the stack
  uint8_t localcount = shared_info->ir_info.local_variables;
  uint8_t stacksize = shared_info->ir_info.stacksize;
  uint16_t local_stack_buffer_slot_count = localcount + stacksize;
  auto* local_stack_buffer = static_cast<RawValue*>(alloca(kPointerSize * local_stack_buffer_slot_count));
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
    auto context = RawTuple::create(thread, RawFunction::kContextHeapVariablesOffset + heap_variables);
    context.set_field_at(RawFunction::kContextParentOffset, frame.function.context());
    context.set_field_at(RawFunction::kContextSelfOffset, frame.self);

    // initialize heap variables to null
    for (uint8_t i = 0; i < heap_variables; i++) {
      context.set_field_at(RawFunction::kContextHeapVariablesOffset + i, kNull);
    }

    frame.context = context;
  } else {
    frame.context = frame.function.context();
  }

  if (argc < shared_info->ir_info.minargc) {
    return thread->throw_message("Not enough arguments for function call, expected % but got %",
                                 (uint32_t)shared_info->ir_info.minargc, argc);
  }

  // arrow functions and functions with a spread argument may receive an arbitrary amount of arguments
  // functions which are neither cannot be called with more arguments than they were declared with
  if (!(shared_info->ir_info.spread_argument || shared_info->ir_info.arrow_function)) {
    if (argc > shared_info->ir_info.argc) {
      return thread->throw_message("Too many arguments for function call, expected at most % but got %",
                                   (uint32_t)shared_info->ir_info.argc, argc);
    }
  }

  // copy function arguments into local variables
  uint8_t func_argc = shared_info->ir_info.argc;
  bool func_has_spread = shared_info->ir_info.spread_argument;
  DCHECK(localcount >= func_argc);
  for (uint8_t i = 0; i < argc && i < func_argc; i++) {
    DCHECK(frame.arguments);
    frame.locals[i] = frame.arguments[i];
  }

  // initialize spread argument
  if (func_has_spread) {
    if (argc <= func_argc) {
      frame.locals[func_argc] = RawTuple::create_empty(thread);
    } else {
      uint32_t remaining_arguments = argc - func_argc;
      auto spread_args = RawTuple::create(thread, remaining_arguments);
      DCHECK(frame.arguments);
      for (uint8_t i = 0; i < remaining_arguments; i++) {
        spread_args.set_field_at(i, frame.arguments[func_argc + i]);
      }
      frame.locals[func_argc] = spread_args;
    }
  }

  // copy self from context if this is an arrow function
  if (shared_info->ir_info.arrow_function) {
    frame.self = frame.function.saved_self();
  }

  thread->checkpoint();

  return Interpreter::execute(thread);
}

RawValue Interpreter::call_builtin_function(Thread* thread,
                                            RawValue self,
                                            RawBuiltinFunction function,
                                            RawValue* arguments,
                                            uint32_t argc,
                                            RawValue argument_tuple) {
  BuiltinFrame frame(thread);
  frame.function = function;
  frame.self = self;
  frame.argument_tuple = argument_tuple;
  frame.arguments = arguments;
  frame.argc = argc;

  if (frame.argument_tuple.isTuple()) {
    DCHECK((void*)frame.arguments == (void*)RawTuple::cast(frame.argument_tuple).data());
  }

  // stack overflow check
  if (Interpreter::stack_overflow_check(thread).is_error_exception()) {
    return kErrorException;
  }

  // argc check
  auto expected_argc = function.argc();
  if (expected_argc != -1 && argc != expected_argc) {
    return thread->throw_message("Incorrect argument count for builtin function '%', expected % but got %",
                                 function.name(), expected_argc, argc);
  }

  thread->checkpoint();

  return function.function()(thread, &frame);
}

RawValue Interpreter::execute(Thread* thread) {
  InterpreterFrame* frame = static_cast<InterpreterFrame*>(thread->frame());

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

RawValue Interpreter::stack_overflow_check(Thread* thread) {
  const Stack* stack = thread->stack();
  auto frame_address = (uintptr_t)__builtin_frame_address(0);
  auto stack_bottom_address = (uintptr_t)stack->lo();
  size_t remaining_bytes_on_stack = frame_address - stack_bottom_address;
  if (remaining_bytes_on_stack <= kStackOverflowLimit) {
    debuglnf("thread % stack overflow", thread->id());
    return thread->throw_message("Maximum recursion depth exceeded", thread->id());
  }

  return kNull;
}

#define OP(name)                                                                                        \
  Interpreter::ContinueMode __attribute__((always_inline))                                              \
  Interpreter::opcode_##name([[maybe_unused]] Thread* thread, [[maybe_unused]] InterpreterFrame* frame, \
                             [[maybe_unused]] Instruction_##name* op)

OP(nop) {
  return ContinueMode::Next;
}

OP(panic) {
  debuglnf("panic in thread % in % at %", thread->id(), frame->function, (void*)frame->ip);
  thread->abort(1);
}

OP(import) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  String file_path_value(scope, frame->pop());
  String module_path_value(scope, frame->pop());

  auto file_path = fs::path(file_path_value.view());
  auto module_path = fs::path(module_path_value.view());

  // TODO: perform filesystem lookup and compilation in native mode

  // attempt to resolve the module path to a real file path
  std::optional<fs::path> resolve_result = runtime->resolve_module(module_path, file_path);
  if (!resolve_result.has_value()) {
    thread->throw_message("Could not resolve '%' to a valid path", module_path);
    return ContinueMode::Exception;
  }
  fs::path import_path = resolve_result.value();

  RawValue import_result = runtime->import_module_at_path(thread, import_path);
  if (import_result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->push(import_result);
  return ContinueMode::Next;
}

OP(stringconcat) {
  uint8_t count = op->arg();

  DCHECK(count > 0);

  size_t known_minimum_string_size = 0;
  for (int64_t depth = count - 1; depth >= 0; depth--) {
    RawValue value = frame->peek(depth);

    if (value.isString()) {
      auto string = RawString::cast(value);
      known_minimum_string_size += string.byte_length();
    }
  }

  if (known_minimum_string_size > RawString::kMaxByteLength) {
    thread->throw_message("String exceeds maximum allowed size");
    return ContinueMode::Exception;
  }

  utils::Buffer buffer;
  for (int64_t depth = count - 1; depth >= 0; depth--) {
    frame->peek(depth).to_string(buffer);
  }

  if (buffer.size() > RawString::kMaxByteLength) {
    thread->throw_message("String exceeds maximum allowed size");
    return ContinueMode::Exception;
  }

  frame->pop(count);
  frame->push(RawString::acquire(thread, buffer));

  return ContinueMode::Next;
}

OP(declareglobal) {
  uint8_t is_constant = op->arg1();
  uint16_t string_index = op->arg2();
  SYMBOL name = frame->get_string_table_entry(string_index).hash;

  RawValue value = frame->peek();
  RawValue result = thread->runtime()->declare_global_variable(thread, name, is_constant, value);

  if (result.is_error_exception()) {
    thread->throw_message("Duplicate declaration of global variable %", RawSymbol::create(name));
    return ContinueMode::Exception;
  }
  DCHECK(result.is_error_ok());

  return ContinueMode::Next;
}

OP(type) {
  RawValue value = frame->pop();
  frame->push(value.klass(thread));
  return ContinueMode::Next;
}

OP(instanceof) {
  RawValue expected_class_value = frame->pop();

  if (!expected_class_value.isClass()) {
    thread->throw_message("Expected right hand side of instanceof to be a class, got '%'",
                          expected_class_value.klass_name(thread));
    return ContinueMode::Exception;
  }

  RawClass expected_class = RawClass::cast(expected_class_value);

  // compiler inserts 'type' opcode for this value, so this will always be a class
  RawClass value_class = frame->pop().klass(thread);
  frame->push(RawBool::create(value_class.is_instance_of(expected_class)));
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

  if (value.isString()) {
    thread->throw_exception(RawException::create(thread, RawString::cast(value)));
  } else if (value.isException()) {
    thread->throw_exception(RawException::cast(value));
  } else {
    thread->throw_message("Expected thrown value to be an exception or a string");
  }

  return ContinueMode::Exception;
}

OP(rethrowex) {
  thread->rethrow_exception(RawException::cast(frame->pop()));
  return ContinueMode::Exception;
}

OP(assertcomparisonfailure) {
  auto message = RawString::cast(frame->pop());
  auto operation_name = RawString::cast(frame->pop());
  auto right = frame->pop();
  auto left = frame->pop();
  auto exception = RawAssertionException::create(thread, message, left, operation_name, right);
  thread->throw_exception(exception);
  return ContinueMode::Exception;
}

OP(asserttruthynessfailure) {
  auto message = RawString::cast(frame->pop());
  auto value = frame->pop();
  auto exception = RawAssertionException::create(thread, message, value);
  thread->throw_exception(exception);
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
  DCHECK(segment_count > 0);

  RawValue* segments = frame->top_n(segment_count);
  RawValue result = RawTuple::create_spread(thread, segments, segment_count);
  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  auto argument_tuple = RawTuple::cast(result);
  auto* arguments = argument_tuple.data();
  uint32_t argc = argument_tuple.length();
  RawValue callee = frame->peek(segment_count);
  RawValue self = frame->peek(segment_count + 1);
  RawValue rval = Interpreter::call_value(thread, self, callee, arguments, argc, argument_tuple);

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
  DCHECK(index < frame->shared_function_info->constant_table.size());
  RawValue value = frame->shared_function_info->constant_table[index];
  frame->push(value);
  return ContinueMode::Next;
}

OP(loadsmi) {
  auto value = static_cast<uintptr_t>(op->arg());
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
  uint16_t string_index = op->arg();
  SYMBOL name = frame->get_string_table_entry(string_index).hash;
  RawValue result = thread->runtime()->read_global_variable(thread, name);

  if (result.is_error_not_found()) {
    thread->throw_message("Unknown global variable %", RawSymbol::create(name));
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

  RawValue result = value.load_attr(thread, index);
  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->push(result);
  return ContinueMode::Next;
}

OP(loadattrsym) {
  RawValue value = frame->pop();
  uint16_t symbol_offset = op->arg();
  SYMBOL symbol = frame->get_string_table_entry(symbol_offset).hash;

  RawValue result = value.load_attr_symbol(thread, symbol);
  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->push(result);
  return ContinueMode::Next;
}

OP(loadsuperconstructor) {
  auto host_class = RawClass::cast(frame->function.host_class());
  auto parent_klass = RawClass::cast(host_class.parent());
  auto parent_constructor = parent_klass.constructor();
  frame->push(parent_constructor);
  return ContinueMode::Next;
}

OP(loadsuperattr) {
  uint16_t string_index = op->arg();
  SYMBOL name = frame->get_string_table_entry(string_index).hash;

  auto klass = RawClass::cast(frame->function.host_class());
  auto parent = RawClass::cast(klass.parent());
  auto func = parent.lookup_function(name);

  if (func.is_error_not_found()) {
    thread->throw_message("Super class '%' has no member function called '%'", parent.name(), RawSymbol::create(name));
    return ContinueMode::Exception;
  }

  frame->push(func);
  return ContinueMode::Next;
}

OP(setglobal) {
  uint16_t string_index = op->arg();
  SYMBOL name = frame->get_string_table_entry(string_index).hash;
  RawValue value = frame->peek();
  RawValue result = thread->runtime()->set_global_variable(thread, name, value);

  if (result.is_error_not_found()) {
    thread->throw_message("Unknown global variable %", RawSymbol::create(name));
    return ContinueMode::Exception;
  } else if (result.is_error_read_only()) {
    thread->throw_message("Cannot write to constant global variable %", RawSymbol::create(name));
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

  RawValue result = target.set_attr(thread, index, value);
  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->push(result);
  return ContinueMode::Next;
}

OP(setattrsym) {
  RawValue value = frame->pop();
  RawValue target = frame->pop();
  uint16_t symbol_offset = op->arg();
  SYMBOL symbol = frame->get_string_table_entry(symbol_offset).hash;

  RawValue result = target.set_attr_symbol(thread, symbol, value);
  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->push(result);
  return ContinueMode::Next;
}

OP(unpacksequence) {
  uint8_t count = op->arg();
  RawValue value = frame->pop();

  if (value.isTuple()) {
    auto tuple = RawTuple::cast(value);
    uint32_t tuple_length = tuple.length();

    if (tuple_length != count) {
      thread->throw_message("Expected tuple to be of length %, got %", (size_t)count, tuple_length);
      return ContinueMode::Exception;
    }

    // push values in reverse so that values can be assigned to their
    // target fields in source order
    for (int64_t i = tuple_length - 1; i >= 0; i--) {
      frame->push(tuple.field_at(i));
    }

    return ContinueMode::Next;
  } else if (value.isList()) {
    auto list = RawList::cast(value);
    uint32_t list_length = list.length();

    if (list_length != count) {
      thread->throw_message("Expected list to be of length %, got %", (size_t)count, list_length);
      return ContinueMode::Exception;
    }

    std::unique_lock locker(list);

    if (list.length() != list_length) {
      locker.unlock();
      thread->throw_message("List length changed during unpack");
      return ContinueMode::Exception;
    }

    // push values in reverse so that values can be assigned to their
    // target fields in source order
    RawValue* data = list.data();
    for (int64_t i = list_length - 1; i >= 0; i--) {
      frame->push(data[i]);
    }

    return ContinueMode::Next;
  } else {
    thread->throw_message("Value of type '%' cannot be the source of an unpack assignment", value.klass_name(thread));
    return ContinueMode::Exception;
  }
}

OP(unpacksequencespread) {
  uint8_t before_count = op->arg1();
  uint8_t after_count = op->arg2();
  uint16_t minimum_count = before_count + after_count;

  HandleScope scope(thread);
  Value value(scope, frame->pop());

  if (value.isTuple()) {
    Tuple tuple(scope, value);
    uint32_t tuple_length = tuple.length();

    if (tuple_length < minimum_count) {
      thread->throw_message("Tuple does not contain enough values to unpack");
      return ContinueMode::Exception;
    }

    uint32_t spread_count = tuple_length - minimum_count;
    Tuple spread_tuple(scope, RawTuple::create(thread, spread_count));

    // push the values after the spread
    for (uint8_t i = 0; i < after_count; i++) {
      frame->push(tuple.field_at(tuple_length - i - 1));
    }

    // put spread arguments into the spread tuple
    for (uint32_t i = 0; i < spread_count; i++) {
      spread_tuple.set_field_at(i, tuple.field_at(before_count + i));
    }
    frame->push(spread_tuple);

    // push the values before the spread
    for (uint8_t i = 0; i < before_count; i++) {
      frame->push(tuple.field_at(before_count - i - 1));
    }

    return ContinueMode::Next;
  } else if (value.isList()) {
    List list(scope, value);
    size_t list_length = list.length();

    if (list_length < minimum_count) {
      thread->throw_message("List does not contain enough values to unpack");
      return ContinueMode::Exception;
    }

    uint32_t spread_count = list_length - minimum_count;
    Tuple spread_tuple(scope, RawTuple::create(thread, spread_count));

    {
      std::unique_lock locker(list);

      if (list.length() != list_length) {
        locker.unlock();
        thread->throw_message("List length changed during unpack");
        return ContinueMode::Exception;
      }

      RawValue* data = list.data();

      // push the values after the spread
      for (uint8_t i = 0; i < after_count; i++) {
        frame->push(data[list_length - i - 1]);
      }

      // put spread arguments into the spread tuple
      for (uint32_t i = 0; i < spread_count; i++) {
        spread_tuple.set_field_at(i, data[before_count + i]);
      }
      frame->push(spread_tuple);

      // push the values before the spread
      for (uint8_t i = 0; i < before_count; i++) {
        frame->push(data[before_count - i - 1]);
      }
    }

    return ContinueMode::Next;
  } else {
    thread->throw_message("Value of type '%' cannot be the source of an unpack assignment", value.klass_name(thread));
    return ContinueMode::Exception;
  }
}

OP(unpackobject) {
  uint8_t key_count = op->arg();
  RawSymbol keys[key_count];

  for (uint8_t i = 0; i < key_count; i++) {
    keys[i] = frame->pop();
  }

  RawValue source_value = frame->pop();

  for (uint8_t i = 0; i < key_count; i++) {
    RawValue result = source_value.load_attr_symbol(thread, keys[i].value());
    if (result.is_error_exception()) {
      return ContinueMode::Exception;
    }
    frame->push(result);
  }

  return ContinueMode::Next;
}

OP(makefunc) {
  int16_t offset = op->arg();
  SharedFunctionInfo* shared_data = *bitcast<SharedFunctionInfo**>(op->ip() + offset);
  auto func = RawFunction::create(thread, frame->context, shared_data, frame->self);
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
  RawValue result = RawClass::create(thread, name, parent_value, constructor, member_props, member_functions,
                                     static_prop_keys, static_prop_values, static_functions, flags.value());

  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->push(RawClass::cast(result));
  return ContinueMode::Next;
}

OP(makestr) {
  uint16_t index = op->arg();

  const auto* shared_info = frame->shared_function_info;
  DCHECK(index < shared_info->string_table.size());
  const StringTableEntry& entry = shared_info->string_table[index];
  frame->push(RawString::create(thread, entry.value.data(), entry.value.size(), entry.hash));

  return ContinueMode::Next;
}

OP(makelist) {
  int16_t count = op->arg();
  DCHECK(count >= 0);
  auto list = RawList::create(thread, count);

  list.set_length(count);
  auto* data = list.data();
  for (int16_t i = count - 1; i >= 0; i--) {
    data[i] = frame->pop();
  }

  frame->push(list);

  return ContinueMode::Next;
}

OP(makelistspread) {
  uint32_t segment_count = op->arg();
  DCHECK(segment_count > 0);

  RawValue* segments = frame->top_n(segment_count);
  RawValue result = RawList::create_spread(thread, segments, segment_count);
  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->pop(segment_count);
  frame->push(result);
  return ContinueMode::Next;
}

OP(makedict) {
  THROW_NOT_IMPLEMENTED();
}

OP(makedictspread) {
  THROW_NOT_IMPLEMENTED();
}

OP(maketuple) {
  int16_t count = op->arg();
  auto tuple = RawTuple::create(thread, count);

  while (count--) {
    tuple.set_field_at(count, frame->pop());
  }

  frame->push(tuple);

  return ContinueMode::Next;
}

OP(maketuplespread) {
  uint32_t segment_count = op->arg();
  DCHECK(segment_count > 0);

  RawValue* segments = frame->top_n(segment_count);
  RawValue result = RawTuple::create_spread(thread, segments, segment_count);
  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->pop(segment_count);
  frame->push(result);
  return ContinueMode::Next;
}

OP(makefiber) {
  RawValue arg_argstuple = frame->pop();
  RawValue arg_function = frame->pop();
  RawValue arg_context = frame->pop();

  if (!arg_function.isFunction()) {
    thread->throw_message("Argument is not a function");
    return ContinueMode::Exception;
  }

  frame->push(RawFiber::create(thread, RawFunction::cast(arg_function), arg_context, arg_argstuple));

  return ContinueMode::Next;
}

OP(await) {
  RawValue value = frame->pop();
  RawValue result;

  if (value.isFiber()) {
    result = RawFiber::cast(value).await(thread);
  } else if (value.isFuture()) {
    result = RawFuture::cast(value).await(thread);
  } else {
    thread->throw_message("Value of type '%' cannot be awaited", value.klass_name(thread));
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
  frame->push(value.cast_to_bool());
  return ContinueMode::Next;
}

OP(caststring) {
  RawValue value = frame->pop();

  RawValue result = value.cast_to_string(thread);
  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->push(result);
  return ContinueMode::Next;
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
  frame->push(left.op_add(thread, right));
  return ContinueMode::Next;
}

OP(sub) {
  RawValue right = frame->pop();
  RawValue left = frame->pop();
  frame->push(left.op_sub(right));
  return ContinueMode::Next;
}

OP(mul) {
  RawValue right = frame->pop();
  RawValue left = frame->pop();
  frame->push(left.op_mul(thread, right));
  return ContinueMode::Next;
}

OP(div) {
  RawValue right = frame->pop();
  RawValue left = frame->pop();
  frame->push(left.op_div(right));
  return ContinueMode::Next;
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

  RawValue result = left.op_eq(thread, right);
  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->push(result);
  return ContinueMode::Next;
}

OP(neq) {
  RawValue right = frame->pop();
  RawValue left = frame->pop();

  RawValue result = left.op_neq(thread, right);
  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->push(result);
  return ContinueMode::Next;
}

OP(lt) {
  RawValue right = frame->pop();
  RawValue left = frame->pop();
  frame->push(left.op_lt(right));
  return ContinueMode::Next;
}

OP(gt) {
  RawValue right = frame->pop();
  RawValue left = frame->pop();
  frame->push(left.op_gt(right));
  return ContinueMode::Next;
}

OP(le) {
  RawValue right = frame->pop();
  RawValue left = frame->pop();
  frame->push(left.op_le(right));
  return ContinueMode::Next;
}

OP(ge) {
  RawValue right = frame->pop();
  RawValue left = frame->pop();
  frame->push(left.op_ge(right));
  return ContinueMode::Next;
}

OP(spaceship) {
  RawValue right = frame->pop();
  RawValue left = frame->pop();

  RawValue result = left.op_spaceship(right);
  if (result.is_error_exception()) {
    return ContinueMode::Exception;
  }

  frame->push(result);
  return ContinueMode::Next;
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
  frame->push(value.op_usub());
  return ContinueMode::Next;
}

OP(unot) {
  RawValue value = frame->pop();
  frame->push(value.op_unot());
  return ContinueMode::Next;
}

OP(ubnot) {
  THROW_NOT_IMPLEMENTED();
}

#undef OP

}  // namespace charly::core::runtime
