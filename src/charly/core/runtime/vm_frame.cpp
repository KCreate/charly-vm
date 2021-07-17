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

#include "charly/charly.h"

#include "charly/core/runtime/vm_frame.h"

namespace charly::core::runtime {

using namespace charly::core::compiler;
using namespace charly::core::compiler::ir;
using namespace charly::core::compiler::ir;

void not_implemented(Opcode opcode) {
  safeprint("opcode not implemented %", kOpcodeNames[opcode]);
  assert(false && "opcode not implemented");
}

// Todo list:
//
// - check for stack overflow
// - check for enough arguments

VALUE vm_call_function(StackFrame* parent, VALUE self, Function* function, VALUE* args, uint8_t argc) {
  assert(function);

  StackFrame frame;
  frame.parent = parent;
  frame.fiber = Scheduler::instance->fiber();
  frame.self = self;
  frame.function = function;

  auto fdata = function->shared_data;
  frame.ip = fdata->bytecode_base_ptr;

  // stack limit check
  Fiber::Stack& stack = frame.fiber->stack;
  uintptr_t frame_address = (uintptr_t)&frame;
  uintptr_t stack_bottom_address = (uintptr_t)stack.lo;
  if (frame_address - stack_bottom_address <= kStackOverflowLimit) {
    // TODO: throw not stack overflow exception
    vm_throw_parent(parent, VALUE::Char('S'));
    UNREACHABLE();
  }

  // setup frame stack
  uint8_t stacksize = fdata->ir_info.stacksize;
  frame.stack = (VALUE*)alloca(sizeof(VALUE) * stacksize);
  frame.sp = 0;

  // setup frame locals
  //
  // TODO: allocate on heap for leaked functions
  uint8_t localcount = fdata->ir_info.local_variables;
  frame.locals = (VALUE*)alloca(sizeof(VALUE) * localcount);
  for (uint8_t i = 0; i < localcount; i++) {
    frame.locals[i] = kNull;
  }

  // copy the arguments into the locals space
  if (args) {
    if (argc < fdata->ir_info.minargc) {
      // TODO: throw not enough arguments exception
      vm_throw_parent(parent, VALUE::Char('A'));
      UNREACHABLE();
    }

    for (uint8_t i = 0; i < argc; i++) {
      frame.locals[i + 2] = args[i];
    }
  }

  // child frames which jump into this stack frame will continue at this point
  setjmp(frame.jump_buffer);

  // sync with the scheduler
  Scheduler::instance->worker_checkpoint();

#define INC_IP() frame.ip += opcode_length; continue;
#define DISPATCH() continue;

  for (;;) {

    // fetch & decode current opcode
    InstructionDecoder* op = (InstructionDecoder*)frame.ip;
    Opcode opcode = op->opcode;
    assert(opcode < Opcode::__Count);
    size_t opcode_length = kOpcodeLength[opcode];

    // safeprint("%: % (% bytes)", (void*)frame.ip, kOpcodeNames[opcode], opcode_length);

    switch (opcode) {
      case Opcode::nop: {
        INC_IP();
      }

      case Opcode::panic: {
        Scheduler::instance->abort(1);
        UNREACHABLE();
      }

      case Opcode::import: { not_implemented(opcode); INC_IP(); }
      case Opcode::stringconcat: { not_implemented(opcode); INC_IP(); }
      case Opcode::declareglobal: { not_implemented(opcode); INC_IP(); }
      case Opcode::declareglobalconst: { not_implemented(opcode); INC_IP(); }
      case Opcode::type: { not_implemented(opcode); INC_IP(); }

      case Opcode::pop: {
        frame.stack_pop();
        INC_IP();
      }

      case Opcode::dup: {
        frame.stack_push(frame.stack_top());
        INC_IP();
      }

      case Opcode::dup2: {
        VALUE top1 = frame.stack_peek(1);
        VALUE top2 = frame.stack_peek(0);
        frame.stack_push(top1);
        frame.stack_push(top2);
        INC_IP();
      }

      case Opcode::jmp: {
        frame.ip += op->jmp.offset;
        DISPATCH();
      }

      case Opcode::jmpf: {
        VALUE condition = frame.stack_pop();
        if (condition.truthyness() == false) {
          frame.ip += op->jmpf.offset;
          DISPATCH();
        }
        INC_IP();
      }

      case Opcode::jmpt: {
        VALUE condition = frame.stack_pop();
        if (condition.truthyness() == true) {
          frame.ip += op->jmpf.offset;
          DISPATCH();
        }
        INC_IP();
      }

      case Opcode::testjmp: {
        VALUE top = frame.stack_pop();
        VALUE check = op->testjmp.value;

        if (top.compare(check)) {
          frame.ip += op->testjmp.offset;
          DISPATCH();
        }

        frame.stack_push(top);

        INC_IP();
      }

      case Opcode::testjmpstrict: {
        VALUE top = frame.stack_pop();
        VALUE check = op->testjmpstrict.value;

        if (top.compare_strict(check)) {
          frame.ip += op->testjmpstrict.offset;
          DISPATCH();
        }

        frame.stack_push(top);

        INC_IP();
      }

      case Opcode::throwex: {
        VALUE value = frame.stack_pop();
        // in the case that vm_throw returns, we can directly jump to the next opcode
        vm_throw(&frame, value);
        DISPATCH();
      }
      case Opcode::getexception: {
        // do nothing
        INC_IP();
      }

      case Opcode::call: {
        Count8 argc = op->call.count;
        VALUE* args = frame.stack_top_n(argc);
        VALUE callee = frame.stack_peek(argc);
        VALUE self = frame.stack_peek(argc + 1);

        if (callee.is_pointer_to(HeapType::Function)) {
          Function* function = callee.to_pointer<Function>();
          VALUE rval = vm_call_function(&frame, self, function, args, argc);

          frame.stack_pop(argc + 2);
          frame.stack_push(rval);
          INC_IP();
        }

        // TODO: not a function exception
        vm_throw(&frame, VALUE::Char('x'));
        DISPATCH();
      }

      case Opcode::callspread: { not_implemented(opcode); INC_IP(); }

      case Opcode::ret: {
        return frame.locals[kLocalReturnIndex];
      }

      case Opcode::load: {
        frame.stack_push(op->load.value);
        INC_IP();
      }

      case Opcode::loadsymbol: { not_implemented(opcode); INC_IP(); }

      case Opcode::loadself: {
        frame.stack_push(self);
        INC_IP();
      }

      case Opcode::loadargc: {
        frame.stack_push(VALUE::Int(argc));
        INC_IP();
      }

      case Opcode::loadglobal: { not_implemented(opcode); INC_IP(); }

      case Opcode::loadlocal: {
        Index8 index = op->loadlocal.index;
        assert(index < localcount);
        frame.stack_push(frame.locals[index]);
        INC_IP();
      }

      case Opcode::loadfar: { not_implemented(opcode); INC_IP(); }
      case Opcode::loadattr: { not_implemented(opcode); INC_IP(); }
      case Opcode::loadattrsym: { not_implemented(opcode); INC_IP(); }
      case Opcode::loadsuperconstructor: { not_implemented(opcode); INC_IP(); }
      case Opcode::loadsuperattr: { not_implemented(opcode); INC_IP(); }
      case Opcode::setglobal: { not_implemented(opcode); INC_IP(); }

      case Opcode::setlocal: {
        VALUE top = frame.stack_top();
        Index8 index = op->setlocal.index;
        assert(index < localcount);
        frame.locals[index] = top;
        INC_IP();
      }

      case Opcode::setreturn: {
        frame.locals[kLocalReturnIndex] = frame.stack_pop();
        INC_IP();
      }

      case Opcode::setfar: { not_implemented(opcode); INC_IP(); }
      case Opcode::setattr: { not_implemented(opcode); INC_IP(); }
      case Opcode::setattrsym: { not_implemented(opcode); INC_IP(); }
      case Opcode::unpacksequence: { not_implemented(opcode); INC_IP(); }
      case Opcode::unpacksequencespread: { not_implemented(opcode); INC_IP(); }
      case Opcode::unpackobject: { not_implemented(opcode); INC_IP(); }
      case Opcode::unpackobjectspread: { not_implemented(opcode); INC_IP(); }

      case Opcode::makefunc: {
        uintptr_t shared_data_ptr = frame.ip + op->makefunc.offset;
        CompiledFunction* shared_data = *(CompiledFunction**)shared_data_ptr;
        Function* function = MemoryAllocator::allocate<Function>(shared_data);
        frame.stack_push(VALUE::Pointer(function));
        INC_IP();
      }

      case Opcode::makeclass: { not_implemented(opcode); INC_IP(); }
      case Opcode::makesubclass: { not_implemented(opcode); INC_IP(); }
      case Opcode::makestr: { not_implemented(opcode); INC_IP(); }
      case Opcode::makelist: { not_implemented(opcode); INC_IP(); }
      case Opcode::makelistspread: { not_implemented(opcode); INC_IP(); }
      case Opcode::makedict: { not_implemented(opcode); INC_IP(); }
      case Opcode::makedictspread: { not_implemented(opcode); INC_IP(); }
      case Opcode::maketuple: { not_implemented(opcode); INC_IP(); }
      case Opcode::maketuplespread: { not_implemented(opcode); INC_IP(); }
      case Opcode::fiberspawn: { not_implemented(opcode); INC_IP(); }
      case Opcode::fiberyield: { not_implemented(opcode); INC_IP(); }
      case Opcode::fibercall: { not_implemented(opcode); INC_IP(); }
      case Opcode::fiberpause: { not_implemented(opcode); INC_IP(); }
      case Opcode::fiberresume: { not_implemented(opcode); INC_IP(); }
      case Opcode::fiberawait: { not_implemented(opcode); INC_IP(); }
      case Opcode::caststring: { not_implemented(opcode); INC_IP(); }
      case Opcode::castsymbol: { not_implemented(opcode); INC_IP(); }
      case Opcode::castiterator: { not_implemented(opcode); INC_IP(); }
      case Opcode::iteratornext: { not_implemented(opcode); INC_IP(); }

      case Opcode::add: {
        VALUE r = frame.stack_pop();
        VALUE l = frame.stack_pop();

        if (l.is_int() && r.is_int()) {
          VALUE result = VALUE::Int(l.to_int() + r.to_int());
          frame.stack_push(result);
          INC_IP();
        }

        if (l.is_float() && r.is_float()) {
          VALUE result = VALUE::Float(l.to_float() + r.to_float());
          frame.stack_push(result);
          INC_IP();
        }

        if ((l.is_int() || l.is_float()) && (r.is_int() || r.is_float())) {
          float lf = l.is_int() ? (float)l.to_int() : l.to_float();
          float rf = r.is_int() ? (float)r.to_int() : r.to_float();
          VALUE result = VALUE::Float(lf + rf);
          frame.stack_push(result);
          INC_IP();
        }

        frame.stack_push(kNaN);
        INC_IP();
      }

      case Opcode::sub: { not_implemented(opcode); INC_IP(); }
      case Opcode::mul: { not_implemented(opcode); INC_IP(); }
      case Opcode::div: { not_implemented(opcode); INC_IP(); }
      case Opcode::mod: { not_implemented(opcode); INC_IP(); }
      case Opcode::pow: { not_implemented(opcode); INC_IP(); }

      case Opcode::eq: {
        VALUE r = frame.stack_pop();
        VALUE l = frame.stack_pop();
        frame.stack_push(l.compare(r) ? kTrue : kFalse);
        INC_IP();
      }

      case Opcode::neq: {
        VALUE r = frame.stack_pop();
        VALUE l = frame.stack_pop();
        frame.stack_push(!l.compare(r) ? kTrue : kFalse);
        INC_IP();
      }

      case Opcode::lt: { not_implemented(opcode); INC_IP(); }
      case Opcode::gt: { not_implemented(opcode); INC_IP(); }
      case Opcode::le: { not_implemented(opcode); INC_IP(); }
      case Opcode::ge: { not_implemented(opcode); INC_IP(); }
      case Opcode::shl: { not_implemented(opcode); INC_IP(); }
      case Opcode::shr: { not_implemented(opcode); INC_IP(); }
      case Opcode::shru: { not_implemented(opcode); INC_IP(); }
      case Opcode::band: { not_implemented(opcode); INC_IP(); }
      case Opcode::bor: { not_implemented(opcode); INC_IP(); }
      case Opcode::bxor: { not_implemented(opcode); INC_IP(); }
      case Opcode::usub: { not_implemented(opcode); INC_IP(); }
      case Opcode::unot: { not_implemented(opcode); INC_IP(); }
      case Opcode::ubnot: { not_implemented(opcode); INC_IP(); }

      default: {
        UNREACHABLE();
      }
    }
  }

  UNREACHABLE();

#undef INC_IP
#undef DISPATCH
}

void vm_throw(StackFrame* frame, VALUE arg) {

  // handle local exception
  if (ExceptionTableEntry* entry = frame->find_active_exception_table_entry()) {
    frame->ip = entry->handler_ptr;
    frame->stack_clear();
    frame->stack_push(arg);
    return;
  }

  // throw into parent frame
  frame->unwind();
  vm_throw_parent(frame->parent, arg);
  UNREACHABLE();
}

void vm_throw_parent(StackFrame* parent, VALUE arg) {

  // traverse frame hierarchy and check if they handle an exception
  StackFrame* frame = parent;
  while (frame) {
    if (ExceptionTableEntry* entry = frame->find_active_exception_table_entry()) {
      frame->ip = entry->handler_ptr;
      frame->stack_clear();
      frame->stack_push(arg);
      std::longjmp(frame->jump_buffer, 1);
      UNREACHABLE();
    }
    frame->unwind();
    frame = frame->parent;
  }

  // no parent frame
  //
  // TODO: handle this in the runtime somehow via some global handler maybe?
  safeprint("uncaught exception in fiber %", Scheduler::instance->fiber()->id);
  Scheduler::instance->abort(1);
  UNREACHABLE();
}

void StackFrame::stack_clear() {
  this->sp = 0;
}

VALUE StackFrame::stack_pop(uint8_t count) {
  assert(this->sp >= count);
  VALUE top = kNull;
  while (count--) {
    top = this->stack[this->sp - 1];
    this->sp--;
  }
  return top;
}

VALUE StackFrame::stack_top() const {
  assert(this->sp > 0);
  return this->stack[this->sp - 1];
}

VALUE* StackFrame::stack_top_n(uint8_t count) const {
  assert(count <= fdata()->ir_info.stacksize);
  return &this->stack[this->sp - count];
}

VALUE StackFrame::stack_peek(uint8_t depth) const {
  assert(this->sp > depth);
  return this->stack[this->sp - 1 - depth];
}

void StackFrame::stack_push(VALUE value) {
  assert(this->sp < fdata()->ir_info.stacksize);
  this->stack[this->sp] = value;
  this->sp++;
}

CompiledFunction* StackFrame::fdata() const {
  return this->function->shared_data;
}

ExceptionTableEntry* StackFrame::find_active_exception_table_entry() const {
  for (ExceptionTableEntry& entry : this->fdata()->exception_table) {
    if (this->ip >= entry.begin_ptr && this->ip < entry.end_ptr) {
      return &entry;
    }
  }

  return nullptr;
}

void StackFrame::unwind() {
  safeprint("unwinding frame %", this);
}

}  // namespace charly::core::runtime
