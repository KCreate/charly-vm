/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include <optional>

#include "block.h"
#include "context.h"
#include "defines.h"
#include "exception.h"
#include "frame.h"
#include "gc.h"
#include "internals.h"
#include "opcode.h"
#include "status.h"
#include "value.h"

#pragma once

namespace Charly {
class VM {
  friend MemoryManager;
  friend ManagedContext;

private:
  std::vector<VALUE> stack;
  std::vector<VALUE> pretty_print_stack;
  Context& context;
  Frame* frames;
  CatchTable* catchstack;
  uint8_t* ip;
  bool halted;

public:
  // Methods that operate on the VM's frames
  Frame* pop_frame();
  Frame* create_frame(VALUE self, Function* calling_function, uint8_t* return_address);

  // Instruction Blocks
  InstructionBlock* create_instructionblock();

  // Stack manipulation
  std::optional<VALUE> pop_stack();
  void push_stack(VALUE value);

  // CatchStack manipulation
  CatchTable* create_catchtable(uint8_t* address);
  CatchTable* pop_catchtable();
  void restore_catchtable(CatchTable* table);

  // Methods to create new data types
  VALUE create_object(uint32_t initial_capacity);
  VALUE create_array(uint32_t initial_capacity);
  VALUE create_float(double value);
  VALUE create_string(const char* data, uint32_t length);
  VALUE create_function(VALUE name,
                        uint8_t* body_address,
                        uint32_t argc,
                        uint32_t lvarcount,
                        bool anonymous,
                        InstructionBlock* block);
  VALUE create_cfunction(VALUE name, uint32_t argc, FPOINTER pointer);

  // Casting to different types
  VALUE cast_to_numeric(VALUE value);
  int64_t cast_to_integer(VALUE value);
  double cast_to_double(VALUE value);

  // Methods that operate on the VALUE type
  double float_value(VALUE value);
  double numeric_value(VALUE value);
  bool boolean_value(VALUE value);
  VALUE type(VALUE value);
  VALUE real_type(VALUE value);

  // Execution
  Opcode fetch_instruction();
  void op_readlocal(uint32_t index, uint32_t level);
  void op_readmembersymbol(VALUE symbol);
  void op_readarrayindex(uint32_t index);
  void op_setlocal(uint32_t index, uint32_t level);
  void op_setmembersymbol(VALUE symbol);
  void op_setarrayindex(uint32_t index);
  void op_putself();
  void op_putvalue(VALUE value);
  void op_putfloat(double value);
  void op_putstring(char* data, uint32_t length);
  void op_putfunction(VALUE symbol,
                      uint8_t* body_address,
                      InstructionBlock* block,
                      bool anonymous,
                      uint32_t argc,
                      uint32_t lvarcount);
  void op_putcfunction(VALUE symbol, FPOINTER pointer, uint32_t argc);
  void op_putarray(uint32_t count);
  void op_puthash(uint32_t count);
  void op_pop();
  void op_dup();
  void op_swap();
  void op_call(uint32_t argc);
  void op_callmember(uint32_t argc);
  void call(uint32_t argc, bool with_target);
  void call_function(Function* function, uint32_t argc, VALUE* argv, VALUE self);
  void call_cfunction(CFunction* function, uint32_t argc, VALUE* argv);
  void op_return();
  void op_throw();
  void throw_exception(VALUE payload);
  void op_registercatchtable(int32_t offset);
  void op_popcatchtable();
  void op_branch(int32_t offset);
  void op_branchif(int32_t offset);
  void op_branchunless(int32_t offset);
  void op_typeof();
  void panic(STATUS reason);
  void stacktrace(std::ostream& io);
  void catchstacktrace(std::ostream& io);
  void stackdump(std::ostream& io);
  void inline pretty_print(std::ostream& io, void* value) {
    this->pretty_print(io, (VALUE)value);
  }
  void pretty_print(std::ostream& io, VALUE value);

public:
  VM(Context& t_context) : context(t_context) {
    context.vm = this;
    this->frames = nullptr;
    this->catchstack = nullptr;
    this->ip = nullptr;
    this->halted = false;

    this->init_frames();
  }
  ~VM();
  void init_frames();
  void run();
};
}  // namespace Charly
