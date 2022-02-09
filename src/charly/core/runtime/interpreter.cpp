/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2021 Leonard Schütz
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

#include "charly/core/compiler/compiler.h"
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"

namespace charly::core::runtime {

using namespace charly::core::compiler;
using namespace charly::core::compiler::ir;

Frame::Frame(Thread* thread, RawFunction function) : thread(thread), parent(thread->frame()), function(function) {
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

RawValue Interpreter::call_function(
  Thread* thread, RawValue self, RawFunction function, RawValue* arguments, uint8_t argc) {
  Runtime* runtime = thread->runtime();

  Frame frame(thread, function);
  frame.self = self;
  frame.locals = nullptr;
  frame.stack = nullptr;
  frame.return_value = kNull;
  frame.oldip = 0;
  frame.ip = 0;
  frame.sp = 0;
  frame.argc = argc;

  // if (frame.parent) {
  //   uintptr_t parent_base = bitcast<uintptr_t>(frame.parent);
  //   uintptr_t current_base = bitcast<uintptr_t>(&frame);
  //   size_t frame_size = parent_base - current_base;
  //   debuglnf("frame size: %", frame_size);
  // }

  SharedFunctionInfo* shared_info = function.shared_info();
  frame.shared_function_info = shared_info;
  frame.ip = shared_info->bytecode_base_ptr;
  frame.oldip = frame.ip;

  // stack overflow check
  const Stack* stack = thread->stack();
  uintptr_t frame_address = (uintptr_t)__builtin_frame_address(0);
  uintptr_t stack_bottom_address = (uintptr_t)stack->lo();
  size_t remaining_bytes_on_stack = frame_address - stack_bottom_address;
  if (remaining_bytes_on_stack <= kStackOverflowLimit) {
    // TODO: throw stack overflow exception
    debuglnf("thread % stack overflow", thread->id());
    thread->throw_value(RawSmallString::make_from_cstr("stackOF"));
    return kErrorException;
  }

  // allocate stack space for local variables and the stack
  uint8_t localcount = shared_info->ir_info.local_variables;
  uint8_t stacksize = shared_info->ir_info.stacksize;
  uint16_t local_stack_buffer_slot_count = localcount + stacksize;
  RawValue* local_stack_buffer = static_cast<RawValue*>(alloca(sizeof(RawValue) * local_stack_buffer_slot_count));
  RawValue* local_ptr = local_stack_buffer;
  RawValue* stack_ptr = local_stack_buffer + localcount;
  frame.locals = local_ptr;
  frame.stack = stack_ptr;
  for (uint16_t i = 0; i < local_stack_buffer_slot_count; i++) {
    local_stack_buffer[i] = kNull;
  }

  // setup frame context
  uint8_t heap_variables = shared_info->ir_info.heap_variables;
  if (heap_variables) {
    RawTuple context = RawTuple::cast(runtime->create_tuple(thread, heap_variables + 1));
    context.set_field_at(0, function.context());
    frame.context = context;
  } else {
    frame.context = function.context();
  }

  if (argc < shared_info->ir_info.minargc) {
    // TODO: throw not enough arguments exception
    debugln("not enough arguments for function call");
    thread->throw_value(RawSmallString::make_from_cstr("minargc"));
    return kErrorException;
  }

  // copy function arguments into local variables
  for (uint8_t i = 0; i < argc && i < localcount; i++) {
    DCHECK(arguments);
    frame.locals[i] = arguments[i];
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

  auto next_handler = [&]() __attribute__((always_inline)) -> void* {
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
      DCHECK(thread->has_pending_exception());

      // check if the current frame can handle this exception
      if (const ExceptionTableEntry* entry = frame->find_active_exception_table_entry(op->ip())) {
        frame->ip = entry->handler_ptr;
        frame->sp = 0;  // clear stack
        frame->push(thread->pending_exception());
        thread->reset_pending_exception();
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
  debuglnf("panic in thread % at ip %", thread->id(), (void*)frame->oldip);
  thread->abort(1);
}

OP(import) {
  UNIMPLEMENTED();
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
    debuglnf("duplicate declaration of global variable %", RawSymbol::make(name));
    thread->throw_value(RawSmallString::make_from_cstr("dupglob"));
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
    debuglnf("duplicate declaration of global variable %", RawSymbol::make(name));
    thread->throw_value(RawSmallString::make_from_cstr("dupglob"));
    return ContinueMode::Exception;
  }
  DCHECK(result.is_error_ok());

  return ContinueMode::Next;
}

OP(type) {
  UNIMPLEMENTED();
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

OP(testintjmp) {
  RawValue top = frame->pop();
  uint8_t check = op->arg1();

  DCHECK(top.isInt());
  if (RawInt::cast(top).value() == check) {
    int16_t offset = op->arg2();
    frame->ip = op->ip() + offset;
  } else {
    frame->push(top);
  }

  return ContinueMode::Next;
}

OP(throwex) {
  RawValue value = frame->pop();
  thread->throw_value(value);
  return ContinueMode::Exception;
}

OP(getexception) {
  // do nothing, exception value should already be on the stack
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
  uint8_t argc = op->arg();
  RawValue* args = frame->top_n(argc);
  RawValue callee = frame->peek(argc);
  RawValue self = frame->peek(argc + 1);

  if (callee.isFunction()) {
    RawFunction function = RawFunction::cast(callee);
    RawValue return_value = Interpreter::call_function(thread, self, function, args, argc);

    if (return_value.is_error_exception()) {
      return ContinueMode::Exception;
    }

    frame->pop(argc + 2);
    frame->push(return_value);
    return ContinueMode::Next;
  } else if (callee.isBuiltinFunction()) {
    RawBuiltinFunction function = RawBuiltinFunction::cast(callee);
    BuiltinFunctionType function_ptr = function.function();
    RawValue return_value = function_ptr(thread, args, argc);

    if (return_value.is_error_exception()) {
      return ContinueMode::Exception;
    }

    frame->pop(argc + 2);
    frame->push(return_value);
    return ContinueMode::Next;
  }

  debugln("called value is not a function");
  thread->throw_value(RawSmallString::make_from_cstr("notfunc"));
  return ContinueMode::Exception;
}

OP(callspread) {
  UNIMPLEMENTED();
}

OP(ret) {
  return ContinueMode::Return;
}

OP(load) {
  uint16_t index = op->arg();
  RawValue value = frame->shared_function_info->constant_table[index];
  frame->push(value);
  return ContinueMode::Next;
}

OP(loadsmi) {
  auto value = (uintptr_t)bitcast<int16_t>(op->arg());
  frame->push(RawValue(value));
  return ContinueMode::Next;
}

OP(loadself) {
  frame->push(frame->self);
  return ContinueMode::Next;
}

OP(loadargc) {
  frame->push(RawInt::make(frame->argc));
  return ContinueMode::Next;
}

OP(loadglobal) {
  uint8_t string_index = op->arg();
  SYMBOL name = frame->get_string_table_entry(string_index).hash;
  RawValue result = thread->runtime()->read_global_variable(thread, name);

  if (result.is_error_not_found()) {
    debugln("unknown global variable %", RawSymbol::make(name));
    thread->throw_value(RawSmallString::make_from_cstr("unkglob"));
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
    DCHECK(context.size() >= 1);
    context = RawTuple::cast(context.field_at(0));
    depth--;
  }

  // add one to index to skip over parent pointer
  frame->push(context.field_at(index + 1));
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
  UNIMPLEMENTED();
}

OP(loadsuperconstructor) {
  UNIMPLEMENTED();
}

OP(loadsuperattr) {
  UNIMPLEMENTED();
}

OP(setglobal) {
  uint8_t string_index = op->arg();
  SYMBOL name = frame->get_string_table_entry(string_index).hash;
  RawValue value = frame->pop();
  RawValue result = thread->runtime()->set_global_variable(thread, name, value);

  if (result.is_error_not_found()) {
    debugln("unknown global variable %", RawSymbol::make(name));
    thread->throw_value(RawSmallString::make_from_cstr("unkglob"));
    return ContinueMode::Exception;
  } else if (result.is_error_read_only()) {
    debugln("read-only global variable %", RawSymbol::make(name));
    thread->throw_value(RawSmallString::make_from_cstr("rdoglob"));
    return ContinueMode::Exception;
  }
  DCHECK(result.is_error_ok());

  frame->push(result);
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
    DCHECK(context.size() >= 1);
    context = RawTuple::cast(context.field_at(0));
    depth--;
  }

  // add one to index to skip over parent pointer
  context.set_field_at(index + 1, frame->peek());
  return ContinueMode::Next;
}

OP(setattr) {
  RawValue value = frame->pop();
  RawValue index = frame->pop();
  RawValue target = frame->pop();

  // tuple[int] = value
  if (target.isTuple() && index.isInt()) {
    RawTuple tuple = RawTuple::cast(target);
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

    tuple.set_field_at(index_int, value);
    frame->push(value);
    return ContinueMode::Next;
  }

  debugln("invalid combo, return null");

  frame->push(kNull);
  return ContinueMode::Next;
}

OP(setattrsym) {
  UNIMPLEMENTED();
}

OP(unpacksequence) {
  uint8_t count = op->arg();

  RawValue value = frame->pop();
  switch (value.shape_id()) {
    case ShapeId::kTuple: {
      RawTuple tuple(RawTuple::cast(value));
      size_t tuple_size = tuple.size();

      if (tuple_size != count) {
        debugln("expected tuple to be of size %, not %", (size_t)count, tuple_size);
        thread->throw_value(RawSmallString::make_from_cstr("invsize"));
        return ContinueMode::Exception;
      }

      for (size_t i = 0; i < tuple_size; i++) {
        frame->push(tuple.field_at(i));
      }

      return ContinueMode::Next;
    }
    default: {
      debugln("value is not a sequence");
      thread->throw_value(RawSmallString::make_from_cstr("notaseq"));
      return ContinueMode::Exception;
    }
  }
}

OP(unpacksequencespread) {
  UNIMPLEMENTED();
}

OP(unpackobject) {
  UNIMPLEMENTED();
}

OP(unpackobjectspread) {
  UNIMPLEMENTED();
}

OP(makefunc) {
  int16_t offset = op->arg();
  SharedFunctionInfo* shared_data = *bitcast<SharedFunctionInfo**>(op->ip() + offset);
  RawFunction func = RawFunction::cast(thread->runtime()->create_function(thread, frame->context, shared_data));
  frame->push(func);
  return ContinueMode::Next;
}

OP(makeclass) {
  UNIMPLEMENTED();
}

OP(makesubclass) {
  UNIMPLEMENTED();
}

OP(makestr) {
  uint16_t index = op->makestr()->arg();

  const SharedFunctionInfo& shared_info = *frame->shared_function_info;
  DCHECK(index < shared_info.string_table.size());
  const StringTableEntry& entry = shared_info.string_table[index];
  frame->push(thread->runtime()->create_string(thread, entry.value.data(), entry.value.size(), entry.hash));

  return ContinueMode::Next;
}

OP(makelist) {
  UNIMPLEMENTED();
}

OP(makelistspread) {
  UNIMPLEMENTED();
}

OP(makedict) {
  UNIMPLEMENTED();
}

OP(makedictspread) {
  UNIMPLEMENTED();
}

OP(maketuple) {
  Runtime* runtime = thread->runtime();
  uint8_t count = op->arg();
  RawTuple tuple = RawTuple::cast(runtime->create_tuple(thread, count));

  while (count--) {
    tuple.set_field_at(count, frame->pop());
  }

  frame->push(tuple);

  return ContinueMode::Next;
}

OP(maketuplespread) {
  UNIMPLEMENTED();
}

OP(fiberspawn) {
  UNIMPLEMENTED();
}

OP(fiberyield) {
  UNIMPLEMENTED();
}

OP(fibercall) {
  UNIMPLEMENTED();
}

OP(fiberpause) {
  UNIMPLEMENTED();
}

OP(fiberresume) {
  UNIMPLEMENTED();
}

OP(fiberawait) {
  UNIMPLEMENTED();
}

OP(caststring) {
  RawValue value = frame->pop();

  utils::Buffer buffer;
  value.to_string(buffer);
  frame->push(thread->runtime()->create_string(thread, buffer.data(), buffer.size(), buffer.hash()));

  return ContinueMode::Next;
}

OP(castsymbol) {
  UNIMPLEMENTED();
}

OP(castiterator) {
  UNIMPLEMENTED();
}

OP(iteratornext) {
  UNIMPLEMENTED();
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
  UNIMPLEMENTED();
}

OP(mul) {
  UNIMPLEMENTED();
}

OP(div) {
  UNIMPLEMENTED();
}

OP(mod) {
  UNIMPLEMENTED();
}

OP(pow) {
  UNIMPLEMENTED();
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
  UNIMPLEMENTED();
}

OP(gt) {
  UNIMPLEMENTED();
}

OP(le) {
  UNIMPLEMENTED();
}

OP(ge) {
  UNIMPLEMENTED();
}

OP(shl) {
  UNIMPLEMENTED();
}

OP(shr) {
  UNIMPLEMENTED();
}

OP(shru) {
  UNIMPLEMENTED();
}

OP(band) {
  UNIMPLEMENTED();
}

OP(bor) {
  UNIMPLEMENTED();
}

OP(bxor) {
  UNIMPLEMENTED();
}

OP(usub) {
  UNIMPLEMENTED();
}

OP(unot) {
  RawValue value = frame->pop();
  frame->push(value.truthyness() ? kFalse : kTrue);
  return ContinueMode::Next;
}

OP(ubnot) {
  UNIMPLEMENTED();
}

#undef OP

}  // namespace charly::core::runtime
