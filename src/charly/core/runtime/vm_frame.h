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

#include <csetjmp>

#include "charly/charly.h"
#include "charly/atomic.h"
#include "charly/value.h"

#include "charly/core/runtime/scheduler.h"
#include "charly/core/runtime/allocator.h"
#include "charly/core/runtime/function.h"

#include "charly/core/compiler/ir/bytecode.h"

#pragma once

namespace charly::core::runtime {

static const uint32_t kLocalSelfIndex = 0;
static const uint32_t kLocalReturnIndex = 0;

static const size_t kStackOverflowLimit = 1024; // amount of free bytes which triggers an out of memory exception

struct StackFrame {
  StackFrame* parent;         // parent stack frame
  std::jmp_buf jump_buffer;   // jump buffer for exception handling
  Fiber* fiber;               // fiber that contains this stack frame
  VALUE self;                 // self value of the call
  Function* function;         // called function
  VALUE* stack = nullptr;     // pointer to stack space
  VALUE* locals = nullptr;    // pointer to locals space
  uintptr_t ip;               // pointer to next instruction
  uint32_t sp;                // stack pointer

  void stack_clear();

  VALUE stack_pop(uint8_t count = 1);

  VALUE stack_top() const;

  VALUE* stack_top_n(uint8_t count) const;

  VALUE stack_peek(uint8_t depth) const;

  void stack_push(VALUE value);

  CompiledFunction* fdata() const;

  ExceptionTableEntry* find_active_exception_table_entry() const;

  void unwind();
};

// builds up a charly call stack frame and begins executing the function
VALUE vm_call_function(StackFrame* parent, VALUE self, Function* function, VALUE* args, uint8_t argc);

// throws an exception inside the current stack frame
// if the call returns, it means the current function can handle the exception
// if the call does not return, it means the exception was passed to some higher frame
void vm_throw(StackFrame* frame, VALUE arg);

// passes an exception to some higher frame
[[noreturn]] void vm_throw_parent(StackFrame* parent, VALUE arg);

}  // namespace charly::core::runtime
