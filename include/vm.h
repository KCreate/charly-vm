/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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
#include <queue>
#include <map>
#include <atomic>
#include <chrono>

#include "value.h"
#include "opcode.h"
#include "status.h"
#include "instructionblock.h"

#include "compiler-manager.h"
#include "compiler.h"

#pragma once

namespace Charly {
using Timestamp = std::chrono::time_point<std::chrono::steady_clock>;

struct VMInstructionProfileEntry {
  uint64_t encountered = 0;
  uint64_t average_length = 0;
};

// Stores how often each type of instruction was encountered
// and how it took on average
class VMInstructionProfile {
public:
  VMInstructionProfile() : entries(nullptr) {
    this->entries = new VMInstructionProfileEntry[OpcodeCount];
  }
  ~VMInstructionProfile() {
    delete[] this->entries;
  }

  void add_entry(Opcode opcode, uint64_t length) {
    VMInstructionProfileEntry& entry = this->entries[opcode];
    entry.average_length = (entry.average_length * entry.encountered + length) / (entry.encountered + 1);
    entry.encountered += 1;
  }

  VMInstructionProfileEntry* entries;
};

struct VMContext {
  Compilation::CompilerManager& compiler_manager;
};

/*
 * Stores information about a callback the VM needs to execute
 * */
struct VMTask {
  bool is_thread;
  uint64_t uid;
  union {
    struct {
      uint64_t id;
      VALUE argument;
    } thread;
    struct {
      Function* func;
      VALUE arguments[4];
    } callback;
  };

  /*
   * Initialize a VMTask which resumes a thread
   * */
  static inline VMTask init_thread(uint64_t id, VALUE argument) {
    return { .is_thread = true, .uid = 0, .thread = { id, argument } };
  }

  /*
   * Initialize a VMTask which calls a callback, with up to 4 arguments
   * */
  static inline VMTask init_callback_with_id(uint64_t id,
#include "instructionblock.h"
                                     Function* func,
                                     VALUE arg1 = kNull,
                                     VALUE arg2 = kNull,
                                     VALUE arg3 = kNull,
                                     VALUE arg4 = kNull) {
    return { .is_thread = false, .uid = id, .callback = { func, { arg1, arg2, arg3, arg4 } } };
  }
  static inline VMTask init_callback(Function* func,
                                     VALUE arg1 = kNull,
                                     VALUE arg2 = kNull,
                                     VALUE arg3 = kNull,
                                     VALUE arg4 = kNull) {
    return VMTask::init_callback_with_id(0, func, arg1, arg2, arg3, arg4);
  }
};

/*
 * Suspended VM thread
 * */
struct VMThread {
  uint64_t uid;
  std::vector<VALUE> stack;
  Frame* frame;
  CatchTable* catchstack;
  uint8_t* resume_address;

  VMThread(uint64_t u, std::vector<VALUE>&& s, Frame* f, CatchTable* c, uint8_t* r)
      : uid(u), stack(std::move(s)), frame(f), catchstack(c), resume_address(r) {
  }
};

class VM {
  friend class GarbageCollector;
public:
  VM(VMContext& ctx);
  VM(const VM& other) = delete;
  VM(VM&& other) = delete;

  // Stack manipulation
  VALUE pop_stack();
  void push_stack(VALUE value);

  // Misc. machine operations
  Frame* pop_frame();
  CatchTable* pop_catchtable();
  void unwind_catchstack(std::optional<VALUE> payload);

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

  // Machine functionality
  VALUE readmembersymbol(VALUE source, VALUE symbol);
  VALUE setmembersymbol(VALUE target, VALUE symbol, VALUE value);
  VALUE readmembervalue(VALUE source, VALUE value);
  VALUE setmembervalue(VALUE target, VALUE member_value, VALUE value);
  bool findprimitivevalue(VALUE value, VALUE symbol, VALUE* result);
  void call(uint32_t argc, bool with_target);
  void call_function(Function* function, uint32_t argc, VALUE* argv, VALUE self);
  void call_cfunction(CFunction* function, uint32_t argc, VALUE* argv);
  void call_class(Class* klass, uint32_t argc, VALUE* argv);
  void throw_exception(const std::string& message);
  void throw_exception(VALUE payload);
  VALUE get_global_symbol(VALUE symbol);

  void panic(STATUS reason);
  void debug_stackdump(std::ostream& io);
  void debug_stacktrace(std::ostream& io);

  // Instructions
  Opcode fetch_instruction();
  void op_readlocal(uint32_t index, uint32_t level);
  void op_readmembersymbol(VALUE symbol);
  void op_readmembervalue();
  void op_readarrayindex(uint32_t index);
  void op_readglobal(VALUE symbol);
  void op_setlocalpush(uint32_t index, uint32_t level);
  void op_setmembersymbolpush(VALUE symbol);
  void op_setmembervaluepush();
  void op_setarrayindexpush(uint32_t index);
  void op_setlocal(uint32_t index, uint32_t level);
  void op_setmembersymbol(VALUE symbol);
  void op_setmembervalue();
  void op_setarrayindex(uint32_t index);
  void op_setglobal(VALUE symbol);
  void op_setglobalpush(VALUE symbol);
  void op_putself();
  void op_putsuper();
  void op_putsupermember(VALUE symbol);
  void op_putvalue(VALUE value);
  void op_putstring(char* data, uint32_t length);
  void op_putfunction(VALUE symbol,
                      uint8_t* body_address,
                      bool anonymous,
                      bool needs_arguments,
                      uint32_t argc,
                      uint32_t minimum_argc,
                      uint32_t lvarcount);
  void op_putarray(uint32_t count);
  void op_puthash(uint32_t count);
  void op_putclass(VALUE name,
                   uint32_t propertycount,
                   uint32_t staticpropertycount,
                   uint32_t methodcount,
                   uint32_t staticmethodcount,
                   bool has_parent_class,
                   bool has_constructor);
  void op_pop();
  void op_dup();
  void op_dupn(uint32_t count);
  void op_swap();
  void op_call(uint32_t argc);
  void op_callmember(uint32_t argc);
  void op_new(uint32_t argc);
  void op_return();
  void op_yield();
  void op_throw();
  void op_registercatchtable(int32_t offset);
  void op_popcatchtable();
  void op_branch(int32_t offset);
  void op_branchif(int32_t offset);
  void op_branchunless(int32_t offset);
  void op_branchlt(int32_t offset);
  void op_branchgt(int32_t offset);
  void op_branchle(int32_t offset);
  void op_branchge(int32_t offset);
  void op_brancheq(int32_t offset);
  void op_branchneq(int32_t offset);
  void op_typeof();
  void op_syscall(SyscallID id);

  VALUE syscall_timerinit(Function* function, uint32_t ms);
  VALUE syscall_timerclear(uint64_t id);
  VALUE syscall_tickerinit(Function* function, uint32_t period);
  VALUE syscall_tickerclear(uint64_t id);
  VALUE syscall_fibersuspend();
  VALUE syscall_fiberresume(uint64_t id, VALUE argument);

  VALUE syscall_calldynamic(VALUE function, Array* arguments);
  VALUE syscall_callmemberdynamic(VALUE function, VALUE context, Array* arguments);
  VALUE syscall_clearboundself(Function* function);

  VALUE syscall_caststring(VALUE value);
  VALUE syscall_copyvalue(VALUE value);

  VALUE syscall_containerlistkeys(Container* container);

  VALUE syscall_stringtriml(VALUE string);
  VALUE syscall_stringtrimr(VALUE string);
  VALUE syscall_stringlowercase(VALUE string);
  VALUE syscall_stringuppercase(VALUE string);

  void run();
  uint8_t start_runtime();
  void exit(uint8_t status_code);
  uint64_t get_next_thread_uid();
  void suspend_thread();
  void resume_thread(uint64_t uid, VALUE argument);
  void register_task(VMTask task);
  bool pop_task(VMTask* target);
  void clear_task_queue();
  uint64_t register_timer(Timestamp, VMTask task);
  uint64_t register_ticker(uint32_t, VMTask task);
  uint64_t get_next_timer_id();
  void clear_timer(uint64_t uid);
  void clear_ticker(uint64_t uid);

  VMContext context;
  VMInstructionProfile instruction_profile;
private:

  // References to the primitive classes of the VM
  Class* primitive_array    = nullptr;
  Class* primitive_boolean  = nullptr;
  Class* primitive_class    = nullptr;
  Class* primitive_function = nullptr;
  Class* primitive_null     = nullptr;
  Class* primitive_number   = nullptr;
  Class* primitive_object   = nullptr;
  Class* primitive_string   = nullptr;
  Class* primitive_value    = nullptr;
  Class* primitive_frame    = nullptr;

  // A function which handles uncaught exceptions
  Function* uncaught_exception_handler = nullptr; // function handling uncaught exceptions
  Class* internal_error_class = nullptr;          // error class used by internal exceptions
  Object* globals = nullptr;                      // container for global variables

  // Scheduled tasks and paused VM threads
  uint64_t next_thread_id = 0;
  std::map<uint64_t, VMThread> paused_threads;
  std::queue<VMTask> task_queue;
  std::mutex task_queue_m;
  std::condition_variable task_queue_cv;
  std::atomic<bool> running = true;

  // Remaining timers & tickers
  uint64_t next_timer_id = 0;
  std::map<Timestamp, VMTask> timers;
  std::map<Timestamp, std::tuple<VMTask, uint32_t>> tickers;

  // The uid of the current thread of execution
  uint64_t uid = 0;

  std::vector<VALUE> stack;
  Frame* frames = nullptr;
  CatchTable* catchstack = nullptr;
  uint8_t* ip = nullptr;
  bool halted = false;
  uint8_t status_code = 0;
};
}  // namespace Charly
