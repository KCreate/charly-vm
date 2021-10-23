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

#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"
#include "charly/core/compiler/compiler.h"

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
  const Stack& stack = thread->stack();
  uintptr_t frame_address = (uintptr_t)__builtin_frame_address(0);
  uintptr_t stack_bottom_address = (uintptr_t)stack.lo();
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
  buf.emit_block(RawString::data(&left), left_size);
  buf.emit_block(RawString::data(&right), right_size);

  if (total_size <= RawSmallString::kMaxLength) {
    return RawSmallString::make_from_memory(buf.data(), total_size);
  } else if (total_size <= RawLargeString::kMaxLength) {
    return thread->runtime()->create_string(thread, buf.data(), total_size, buf.buffer_hash());
  } else {
    SYMBOL buf_hash = buf.buffer_hash();
    char* buffer = buf.release_buffer();
    CHECK(buffer, "could not release buffer");
    return thread->runtime()->acquire_string(thread, buffer, total_size, buf_hash);
  }
}

RawValue Interpreter::execute(Thread* thread) {
  Frame* frame = thread->frame();

  // dispatch table to opcode handlers
  static void* OPCODE_DISPATCH_TABLE[] = {
#define OP(name, ictype, stackpop, stackpush, ...) &&execute_handler_##name,
    FOREACH_OPCODE(OP)
#undef OP
  };

  ContinueMode continue_mode;
  InstructionDecoder* op = nullptr;

  auto next_handler = [&]() __attribute__((always_inline)) {
    op = bitcast<InstructionDecoder*>(frame->ip);
    Opcode opcode = op->opcode;
    size_t opcode_length = kOpcodeLength[opcode];
    frame->oldip = frame->ip;
    frame->ip += opcode_length;
    return OPCODE_DISPATCH_TABLE[opcode];
  };

  goto* next_handler();

#define OP(name, ictype, stackpop, stackpush, ...)                 \
  execute_handler_##name : {                                       \
    continue_mode = Interpreter::opcode_##name(thread, frame, op); \
    if (continue_mode == ContinueMode::Next)                       \
      goto* next_handler();                                        \
    goto handle_return_or_exception;                               \
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
        frame->sp = 0; // clear stack
        frame->push(thread->pending_exception());
        thread->reset_pending_exception();
        goto *next_handler();
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
    [[maybe_unused]] Thread* thread, [[maybe_unused]] Frame* frame, [[maybe_unused]] const InstructionDecoder* op)

OP(nop) {
  return ContinueMode::Next;
}

OP(panic) {
  debuglnf("panic in thread %", thread->id());
  thread->abort(1);
}

OP(import) {
  UNIMPLEMENTED();
}

OP(stringconcat) {
  Count8 count = op->stringconcat.count;

  utils::Buffer buffer;
  std::ostream stream(&buffer);
  for (int64_t depth = count - 1; depth >= 0; depth--) {
    frame->peek(depth).to_string(stream);
  }

  SYMBOL buf_hash = buffer.buffer_hash();
  size_t buf_size = buffer.size();
  char* buf_ptr = buffer.release_buffer();
  CHECK(buf_ptr, "could not release buffer");

  frame->pop(count);
  frame->push(thread->runtime()->acquire_string(thread, buf_ptr, buf_size, buf_hash));

  return ContinueMode::Next;
}

OP(declareglobal) {
  UNIMPLEMENTED();
}

OP(declareglobalconst) {
  UNIMPLEMENTED();
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
  int32_t offset = op->jmp.offset;
  frame->ip = op->ip() + offset;
  return ContinueMode::Next;
}

OP(jmpf) {
  RawValue condition = frame->pop();
  if (!condition.truthyness()) {
    int32_t offset = op->jmpf.offset;
    frame->ip = op->ip() + offset;
  }
  return ContinueMode::Next;
}

OP(jmpt) {
  RawValue condition = frame->pop();
  if (condition.truthyness()) {
    int32_t offset = op->jmpf.offset;
    frame->ip = op->ip() + offset;
  }
  return ContinueMode::Next;
}

OP(testjmp) {
  RawValue top = frame->peek();
  RawValue check = op->testjmp.value;

  if (top.raw() == check.raw()) {
    frame->ip = op->ip() + op->testjmp.offset;
    return ContinueMode::Next;
  }

  frame->push(top);
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
  Count8 argc = op->call.count;
  RawValue* args = frame->top_n(argc);
  RawValue callee = frame->peek(argc);
  RawValue self = frame->peek(argc + 1);

  if (callee.isFunction()) {
    RawFunction function = RawFunction::cast(callee);
    RawValue return_value = Interpreter::call_function(thread, self, function, args, argc);

    if (return_value == kErrorException) {
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
  frame->push(op->load.value);
  return ContinueMode::Next;
}

OP(loadsymbol) {
  frame->push(RawSymbol::make(op->loadsymbol.name));
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
  UNIMPLEMENTED();
}

OP(loadlocal) {
  Index8 index = op->loadlocal.index;
  DCHECK(index < frame->shared_function_info->ir_info.local_variables);
  frame->push(frame->locals[index]);
  return ContinueMode::Next;
}

OP(loadfar) {
  Count8 depth = op->loadfar.depth;
  Index8 index = op->loadfar.index;

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
  UNIMPLEMENTED();
}

OP(setlocal) {
  RawValue top = frame->peek();
  Index8 index = op->setlocal.index;
  DCHECK(index < frame->shared_function_info->ir_info.local_variables);
  frame->locals[index] = top;
  return ContinueMode::Next;
}

OP(setreturn) {
  frame->return_value = frame->pop();
  return ContinueMode::Next;
}

OP(setfar) {
  Count8 depth = op->setfar.depth;
  Index8 index = op->setfar.index;

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
  Count8 count = op->unpacksequence.count;

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
  int32_t offset = op->makefunc.offset;
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
  Index16 index = op->makestr.index;

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
  Count16 count = op->maketuple.count;
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

  std::stringstream stream;
  value.to_string(stream);
  std::string str = stream.str();
  frame->push(thread->runtime()->create_string(thread, str.data(), str.size(), crc32_string(str)));

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
    double lf = left.isInt() ? RawInt::cast(left).value() : RawFloat::cast(left).value();
    double rf = right.isInt() ? RawInt::cast(right).value() : RawFloat::cast(right).value();
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
