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

#include <iostream>
#include <optional>

#include "defines.h"
#include "gc.h"
#include "instructionblock.h"
#include "internals.h"
#include "opcode.h"
#include "status.h"
#include "stringpool.h"
#include "symboltable.h"
#include "value.h"

#pragma once

namespace Charly {

struct VMContext {
  SymbolTable& symtable;
  StringPool& stringpool;
  GarbageCollector& gc;

  bool trace_opcodes = false;
  bool trace_catchtables = false;
  bool trace_frames = false;

  std::ostream& out_stream;
  std::ostream& err_stream;
};

class VM {
  friend GarbageCollector;
  friend ManagedContext;

public:
  VM(VMContext& ctx) : context(ctx), frames(nullptr), catchstack(nullptr), ip(nullptr), halted(false) {
    ctx.gc.mark_ptr_persistent(reinterpret_cast<VALUE**>(&this->frames));
    ctx.gc.mark_ptr_persistent(reinterpret_cast<VALUE**>(&this->catchstack));
    ctx.gc.mark_vector_ptr_persistent(&this->stack);
    this->exec_prelude();
  }
  VM(const VM& other) = delete;
  VM(VM&& other) = delete;
  ~VM() {
    this->context.gc.unmark_ptr_persistent(reinterpret_cast<VALUE**>(&this->frames));
    this->context.gc.unmark_ptr_persistent(reinterpret_cast<VALUE**>(&this->catchstack));
    this->context.gc.unmark_vector_ptr_persistent(&this->stack);
    this->context.gc.collect();
  }

  // Methods that operate on the VM's frames
  Frame* pop_frame();
  Frame* create_frame(VALUE self, Function* calling_function, uint8_t* return_address);
  Frame* create_frame(VALUE self, Frame* parent_environment_frame, uint32_t lvarcount, uint8_t* return_address);

  // Stack manipulation
  std::optional<VALUE> pop_stack();
  void push_stack(VALUE value);

  // CatchStack manipulation
  CatchTable* create_catchtable(uint8_t* address);
  CatchTable* pop_catchtable();
  void unwind_catchstack();

  // Methods to create new data types
  VALUE create_object(uint32_t initial_capacity);
  VALUE create_array(uint32_t initial_capacity);
  VALUE create_float(double value);
  VALUE create_string(const char* data, uint32_t length);
  VALUE create_function(VALUE name, uint8_t* body_address, uint32_t argc, uint32_t lvarcount, bool anonymous);
  VALUE create_cfunction(VALUE name, uint32_t argc, uintptr_t pointer);

  // Methods to copy existing data types
  VALUE copy_value(VALUE value);
  VALUE deep_copy_value(VALUE value);
  VALUE copy_object(VALUE object);
  VALUE deep_copy_object(VALUE object);
  VALUE copy_array(VALUE array);
  VALUE deep_copy_array(VALUE array);
  VALUE copy_string(VALUE string);
  VALUE copy_function(VALUE function);
  VALUE copy_cfunction(VALUE cfunction);

  // Casting to different types
  VALUE cast_to_numeric(VALUE value);
  static int64_t cast_to_integer(VALUE value);
  double cast_to_double(VALUE value);

  // Methods that operate on the VALUE type
  VALUE create_number(double value);
  VALUE create_number(int64_t value);
  static double float_value(VALUE value);
  static double numeric_value(VALUE value);
  static VALUE type(VALUE value);
  static VALUE real_type(VALUE value);
  static bool truthyness(VALUE value);

  // Arithmetics
  VALUE add(VALUE left, VALUE right);
  VALUE sub(VALUE left, VALUE right);
  VALUE mul(VALUE left, VALUE right);
  VALUE div(VALUE left, VALUE right);
  VALUE mod(VALUE left, VALUE right);
  VALUE pow(VALUE left, VALUE right);
  VALUE uadd(VALUE value);
  VALUE usub(VALUE value);

  // Comparison operators
  VALUE eq(VALUE left, VALUE right);
  VALUE neq(VALUE left, VALUE right);
  VALUE lt(VALUE left, VALUE right);
  VALUE gt(VALUE left, VALUE right);
  VALUE le(VALUE left, VALUE right);
  VALUE ge(VALUE left, VALUE right);
  VALUE unot(VALUE value);

  // Bitwise operators
  VALUE shl(VALUE left, VALUE right);
  VALUE shr(VALUE left, VALUE right);
  VALUE band(VALUE left, VALUE right);
  VALUE bor(VALUE left, VALUE right);
  VALUE bxor(VALUE left, VALUE right);
  VALUE ubnot(VALUE value);

  // Execution
  Opcode fetch_instruction();
  void op_readlocal(uint32_t index, uint32_t level);
  void op_readmembersymbol(VALUE symbol);
  void op_readarrayindex(uint32_t index);
  void op_setlocal(uint32_t index, uint32_t level);
  void op_setmembersymbol(VALUE symbol);
  void op_setarrayindex(uint32_t index);
  void op_putself(uint32_t level);
  void op_putvalue(VALUE value);
  void op_putfloat(double value);
  void op_putstring(char* data, uint32_t length);
  void op_putfunction(VALUE symbol, uint8_t* body_address, bool anonymous, uint32_t argc, uint32_t lvarcount);
  void op_putcfunction(VALUE symbol, uintptr_t pointer, uint32_t argc);
  void op_putarray(uint32_t count);
  void op_puthash(uint32_t count);
  void op_pop();
  void op_dup();
  void op_swap();
  void op_topn(uint32_t offset);
  void op_setn(uint32_t offset);
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
  void run();
  void exec_prelude();
  void exec_block(InstructionBlock* block);
  void exec_module(InstructionBlock* block);

  // Private member access
  inline Frame* get_current_frame() {
    return this->frames;
  }

  VMContext context;
private:
  std::vector<VALUE> pretty_print_stack;
  std::vector<VALUE> stack;
  Frame* frames;
  CatchTable* catchstack;
  uint8_t* ip;
  bool halted;
};
}  // namespace Charly
