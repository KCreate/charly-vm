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

#include <filesystem>
#include <fstream>

#include "charly/utils/argumentparser.h"

#include "charly/core/compiler/compiler.h"
#include "charly/core/runtime/compiled_module.h"
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"
#include "charly/core/runtime/thread.h"
#include "charly/core/runtime/worker.h"

using boost::context::detail::jump_fcontext;
using boost::context::detail::make_fcontext;

namespace charly::core::runtime {

using namespace charly::core::compiler;
using namespace std::chrono_literals;

namespace fs = std::filesystem;

static atomic<uint64_t> thread_id_counter = 0;

thread_local static Thread* g_thread = nullptr;

Thread::Thread(Runtime* runtime) :
  m_id(thread_id_counter++),
  m_state(State::Free),
  m_stack(nullptr),
  m_runtime(runtime),
  m_exit_code(0),
  m_fiber(kNull),
  m_worker(nullptr),
  m_last_scheduled_at(0),
  m_context(nullptr),
  m_frame(nullptr),
  m_pending_exception(kErrorOk) {}

Thread* Thread::current() {
  return g_thread;
}

void Thread::set_current(Thread* thread) {
  g_thread = thread;
}

uint64_t Thread::id() const {
  return m_id;
}

Thread::State Thread::state() const {
  return m_state;
}

int32_t Thread::exit_code() const {
  return m_exit_code;
}

RawValue Thread::fiber() const {
  return m_fiber;
}

Worker* Thread::worker() const {
  return m_worker;
}

Runtime* Thread::runtime() const {
  return m_runtime;
}

uint64_t Thread::last_scheduled_at() const {
  return m_last_scheduled_at;
}

void Thread::extend_timeslice(uint64_t ms) {
  m_last_scheduled_at += ms;
}

bool Thread::has_exceeded_timeslice() const {
  uint64_t timestamp_now = get_steady_timestamp();
  return timestamp_now - m_last_scheduled_at >= kThreadTimeslice;
}

const Stack* Thread::stack() const {
  return m_stack;
}

ThreadLocalHandles* Thread::handles() {
  return &m_handles;
}

Frame* Thread::frame() const {
  return m_frame;
}

bool Thread::has_pending_exception() const {
  return m_pending_exception != kErrorOk;
}

RawValue Thread::pending_exception() const {
  DCHECK(has_pending_exception());
  return m_pending_exception;
}

void Thread::init_main_thread() {
  m_state = State::Waiting;
  m_fiber = kNull;
  DCHECK(m_stack == nullptr);
}

void Thread::init_fiber_thread(RawFiber fiber) {
  fiber.set_thread(this);
  m_state = State::Waiting;
  m_fiber = fiber;
  DCHECK(m_stack == nullptr);
}

void Thread::clean() {

  // prevent any other thread from getting the main thread id
  if (id() == kMainThreadId) {
    m_id = thread_id_counter++;
  }

  m_state = State::Free;
  if (m_stack != nullptr) {
    m_runtime->scheduler()->recycle_stack(m_stack);
  }
  m_stack = nullptr;
  m_exit_code = 0;
  m_fiber = kNull;
  m_worker = nullptr;
  m_last_scheduled_at = 0;
  m_frame = nullptr;
  m_pending_exception = kErrorOk;
  m_context = nullptr;
}

void Thread::checkpoint() {
  Worker* worker = m_worker;
  DCHECK(worker);

  if (worker->has_stop_flag() || has_exceeded_timeslice()) {
    enter_scheduler(State::Ready);
  }
}

void Thread::yield_to_scheduler() {
  enter_scheduler(State::Ready);
}

void Thread::abort(int32_t exit_code) {
  m_exit_code = exit_code;
  enter_scheduler(State::Aborted);
  UNREACHABLE();
}

void Thread::ready() {
  m_state.acas(State::Waiting, State::Ready);
}

void Thread::context_switch(Worker* worker) {
  DCHECK(m_state == State::Ready);
  m_state = State::Running;
  m_last_scheduled_at = get_steady_timestamp();

  if (m_stack == nullptr) {
    acquire_stack();
  }
  DCHECK(m_stack != nullptr);

  transfer_t transfer = jump_fcontext(m_context, worker);
  m_context = transfer.fctx;
  DCHECK(m_worker == nullptr);
}

void Thread::enter_native() {
  DCHECK(m_state == State::Running);
  DCHECK(m_worker != nullptr);
  m_state = State::Native;
  m_worker->enter_native();
}

void Thread::exit_native() {
  DCHECK(m_state == State::Native);
  DCHECK(m_worker != nullptr);
  m_state = State::Running;
  m_worker->exit_native();
  checkpoint();
}

void Thread::throw_value(RawValue value) {
  DCHECK(m_pending_exception == kErrorOk);
  m_pending_exception = value;
}

void Thread::reset_pending_exception() {
  DCHECK(m_pending_exception != kErrorOk);
  m_pending_exception = kErrorOk;
}

void Thread::entry_main_thread() {
  CHECK(id() == kMainThreadId);

  Runtime* runtime = m_runtime;
  Scheduler* scheduler = runtime->scheduler();

  // load the boot file
  std::optional<std::string> charly_vm_dir = utils::ArgumentParser::get_environment_for_key("CHARLYVMDIR");
  CHECK(charly_vm_dir.has_value());
  fs::path charly_dir(charly_vm_dir.value());
  fs::path boot_path = charly_dir / "src/charly/stdlib/boot.ch";
  std::ifstream boot_file(boot_path);

  if (!boot_file.is_open()) {
    debuglnf("Could not open the charly runtime boot file (%)", boot_path);
    runtime->abort(1);
    return;
  }

  utils::Buffer boot_file_buffer;
  std::string line;
  while (std::getline(boot_file, line)) {
    boot_file_buffer << line;
    boot_file_buffer.write_utf8_cp('\n');
  }

  debuglnf("boot_file_buffer.size() = %", boot_file_buffer.size());
  auto unit = Compiler::compile(boot_path, boot_file_buffer, CompilationUnit::Type::Module);
  if (unit->console.has_errors()) {
    unit->console.dump_all(std::cerr);
    debuglnf("Could not compile charly runtime boot file (%)", boot_path);
    runtime->abort(1);
    return;
  }

  if (utils::ArgumentParser::is_flag_set("dump_ast")) {
    unit->ast->dump(std::cout, true);
  }

  if (utils::ArgumentParser::is_flag_set("dump_ir")) {
    unit->ir_module->dump(std::cout);
  }

  if (utils::ArgumentParser::is_flag_set("dump_asm")) {
    unit->compiled_module->dump(std::cout);
  }

  if (utils::ArgumentParser::is_flag_set("skipexec")) {
    runtime->abort(0);
    return;
  }

  auto module = unit->compiled_module;
  CHECK(!module->function_table.empty());
  runtime->register_module(this, module);

  HandleScope scope(this);
  Function function(scope, runtime->create_function(this, kNull, module->function_table.front()));
  Fiber fiber(scope, runtime->create_fiber(this, function));

  // build the ARGV tuple
  auto& argv = utils::ArgumentParser::USER_FLAGS;
  Tuple argv_tuple(scope, runtime->create_tuple(this, argv.size()));
  for (uint32_t i = 0; i < argv.size(); i++) {
    const std::string& arg = argv[i];
    String arg_string(scope, runtime->create_string(this, arg.data(), arg.size(), crc32::hash_string(arg)));
    argv_tuple.set_field_at(i, arg_string);
  }
  CHECK(runtime->declare_global_variable(this, SYM("ARGV"), true).is_error_ok());
  CHECK(runtime->set_global_variable(this, SYM("ARGV"), argv_tuple).is_error_ok());

  fiber.thread()->ready();
  scheduler->schedule_thread(fiber.thread(), worker()->processor());
}

void Thread::entry_fiber_thread() {
  RawFiber fiber = RawFiber::cast(this->fiber());
  RawFunction function = fiber.function();

  RawValue result = Interpreter::call_function(this, kNull, function, nullptr, 0);
  if (result == kErrorException) {
    debuglnf("unhandled exception in thread % (%)", id(), pending_exception());
    abort(1);
  }

  if (id() == kMainFiberThreadId) {
    debuglnf("main fiber exited with value %", result);
    abort(0);
  }
}

void Thread::enter_scheduler(State state) {
  Thread::set_current(nullptr);
  m_state = state;
  fcontext_t& context = m_worker->context();
  m_worker = nullptr;
  transfer_t transfer = jump_fcontext(context, nullptr);
  m_worker = static_cast<Worker*>(transfer.data);
  Thread::set_current(this);
  m_worker->set_context(transfer.fctx);
}

void Thread::acquire_stack() {
  DCHECK(m_stack == nullptr);
  m_stack = m_runtime->scheduler()->get_free_stack();
  DCHECK(m_stack, "could not allocate thread stack");

  if (id() == kMainThreadId) {
    m_context = make_fcontext(m_stack->hi(), m_stack->size(), [](transfer_t transfer) {
      Worker* worker = static_cast<Worker*>(transfer.data);
      worker->set_context(transfer.fctx);
      Thread* thread = worker->thread();
      Thread::set_current(thread);
      thread->m_worker = worker;
      thread->entry_main_thread();
      thread->enter_scheduler(State::Exited);
    });
  } else {
    m_context = make_fcontext(m_stack->hi(), m_stack->size(), [](transfer_t transfer) {
      Worker* worker = static_cast<Worker*>(transfer.data);
      worker->set_context(transfer.fctx);
      Thread* thread = worker->thread();
      Thread::set_current(thread);
      thread->m_worker = worker;
      thread->entry_fiber_thread();
      thread->enter_scheduler(State::Exited);
    });
  }
}

void Thread::push_frame(Frame* frame) {
  DCHECK(frame->parent == m_frame);
  m_frame = frame;
}

void Thread::pop_frame(Frame* frame) {
  DCHECK(m_frame == frame);
  m_frame = m_frame->parent;
}

}  // namespace charly::core::runtime