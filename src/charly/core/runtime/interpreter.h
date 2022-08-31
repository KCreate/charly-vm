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

#include "charly/charly.h"
#include "charly/value.h"

#include "charly/core/compiler/ir/bytecode.h"
#include "charly/core/runtime/compiled_module.h"

#pragma once

namespace charly::core::runtime {

struct Frame {
  enum Type {
    kInterpreterFrame = 0,
    kBuiltinFrame
  };

  Frame(Thread* thread, Type type);
  ~Frame();

  Type type;
  Thread* thread;
  Frame* parent;
  size_t depth;
  RawValue self;
  RawValue argument_tuple;
  RawValue* arguments = nullptr;
  uint32_t argc = 0;

  bool is_interpreter_frame() const {
    return type == Type::kInterpreterFrame;
  }

  bool is_builtin_frame() const {
    return type == Type::kBuiltinFrame;
  }
};

struct InterpreterFrame : public Frame {
  InterpreterFrame(Thread* thread) : Frame(thread, Frame::Type::kInterpreterFrame) {}

  RawFunction function;
  const SharedFunctionInfo* shared_function_info = nullptr;
  RawValue context;
  RawValue* locals = nullptr;
  RawValue* stack = nullptr;
  RawValue return_value;
  uintptr_t oldip = 0;
  uintptr_t ip = 0;
  uint32_t sp = 0;

  RawValue pop(uint8_t count = 1);
  RawValue peek(uint8_t depth = 0) const;
  RawValue* top_n(uint8_t count) const;
  void push(RawValue value);

  const ExceptionTableEntry* find_active_exception_table_entry(uintptr_t ip) const;
  const StringTableEntry& get_string_table_entry(uint16_t index) const;
};

struct BuiltinFrame : public Frame {
  BuiltinFrame(Thread* thread) : Frame(thread, Frame::Type::kBuiltinFrame) {}

  RawBuiltinFunction function;
};

// trigger an out of memory exception once this amount of remaining bytes on the stack
// has been crossed
static const size_t kStackOverflowLimit = 1024 * 32;  // 32 kilobytes

class Interpreter {
public:
  enum class ContinueMode {
    Next,      // execute next opcode
    Return,    // return from current frame
    Exception  // an exception was thrown, handle in current frame or return
  };

  static RawValue call_value(Thread* thread,
                             RawValue self,
                             RawValue target,
                             RawValue* arguments,
                             uint32_t argc,
                             RawValue argument_tuple = kNull);

  // if arguments_is_tuple is set, the arguments pointer is interpreted as a pointer to a tuple
  // if not set, arguments is assumed to be a pointer into the threads stack region
  static RawValue call_function(Thread* thread,
                                RawValue self,
                                RawFunction function,
                                RawValue* arguments,
                                uint32_t argc,
                                bool constructor_call = false,
                                RawValue argument_tuple = kNull);

  static RawValue call_builtin_function(Thread* thread,
                                        RawValue self,
                                        RawBuiltinFunction function,
                                        RawValue* arguments,
                                        uint32_t argc,
                                        RawValue argument_tuple = kNull);

private:
  static RawValue execute(Thread* thread);

  static RawValue stack_overflow_check(Thread* thread);

#define OP(name, ictype, stackpop, stackpush, ...) \
  static ContinueMode opcode_##name(Thread* thread, InterpreterFrame* frame, compiler::ir::Instruction_##name* op);
  FOREACH_OPCODE(OP)
#undef OP
};

}  // namespace charly::core::runtime
