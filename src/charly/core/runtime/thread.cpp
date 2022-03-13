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
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"
#include "charly/core/runtime/thread.h"
#include "charly/core/runtime/worker.h"

#include "charly/core/runtime/builtins/core.h"
#include "charly/core/runtime/builtins/readline.h"

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
  m_pending_exception(kErrorOk),
  m_waiting_threads() {}

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
  m_state.acas(State::Free, State::Waiting);
  m_fiber = kNull;
  DCHECK(m_stack == nullptr);
}

void Thread::init_fiber_thread(RawFiber fiber) {
  fiber.set_thread(this);
  m_state.acas(State::Free, State::Waiting);
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
    m_state.acas(State::Running, State::Ready);
    enter_scheduler();
  }
}

void Thread::yield_to_scheduler() {
  m_state.acas(State::Running, State::Ready);
  enter_scheduler();
}

void Thread::abort(int32_t exit_code) {
  m_state.acas(State::Running, State::Aborted);
  m_exit_code = exit_code;
  enter_scheduler();
  UNREACHABLE();
}

void Thread::ready() {
  m_state.acas(State::Waiting, State::Ready);
}

void Thread::context_switch(Worker* worker) {
  DCHECK(m_state == State::Ready);
  m_state.acas(State::Ready, State::Running);
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
  m_state.acas(State::Running, State::Native);
  m_worker->enter_native();
}

void Thread::exit_native() {
  DCHECK(m_state == State::Native);
  DCHECK(m_worker != nullptr);
  m_state.acas(State::Native, State::Running);
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

  // load the boot file
  std::optional<std::string> charly_vm_dir = utils::ArgumentParser::get_environment_for_key("CHARLYVMDIR");
  CHECK(charly_vm_dir.has_value());
  fs::path charly_dir(charly_vm_dir.value());
  fs::path boot_path = charly_dir / "src/charly/stdlib/boot.ch";
  if (utils::ArgumentParser::is_flag_set("boot_path")) {
    const auto& values = utils::ArgumentParser::get_arguments_for_flag("boot_path");
    CHECK(values.size() == 1, "expected one argument for flag 'boot_path'");
    const std::string& value = values.front();
    boot_path = fs::current_path() / value;
  }

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

  auto unit = Compiler::compile(boot_path, boot_file_buffer, CompilationUnit::Type::ReplInput);
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

  // build ARGV tuple
  auto& argv = utils::ArgumentParser::USER_FLAGS;
  Tuple argv_tuple(scope, runtime->create_tuple(this, argv.size()));
  for (uint32_t i = 0; i < argv.size(); i++) {
    const std::string& arg = argv[i];
    String arg_string(scope, runtime->create_string(this, arg.data(), arg.size(), crc32::hash_string(arg)));
    argv_tuple.set_field_at(i, arg_string);
  }
  CHECK(runtime->declare_global_variable(this, SYM("ARGV"), true).is_error_ok());
  CHECK(runtime->set_global_variable(this, SYM("ARGV"), argv_tuple).is_error_ok());

  // initialize builtin functions
  builtin::core::initialize(this);
  builtin::readline::initialize(this);

  Fiber mainfiber(scope, runtime->create_fiber(this, function, kNull, kNull));
  CHECK(runtime->declare_global_variable(this, SYM("charly.mainfiber"), true).is_error_ok());
  CHECK(runtime->set_global_variable(this, SYM("charly.mainfiber"), mainfiber).is_error_ok());
}

void Thread::entry_fiber_thread() {
  HandleScope scope(this);
  Fiber fiber(scope, m_fiber);

  RawValue fiber_arguments = fiber.arguments();
  RawValue* argp = nullptr;
  int64_t argc = 0;
  if (fiber_arguments.isTuple()) {
    RawTuple tuple = RawTuple::cast(fiber_arguments);
    if (tuple.size()) {
      argp = bitcast<RawValue*>(tuple.address());
      CHECK(argc < 256);
      argc = tuple.size();
    }
  }

  RawValue result = Interpreter::call_function(this, fiber.context(), fiber.function(), argp, argc);

  // wake threads waiting for this thread to finish
  {
    std::lock_guard lock(fiber);
    fiber.thread()->wake_waiting_threads();
    fiber.set_result(result);
    fiber.set_thread(nullptr);
  }

  if (result == kErrorException) {
    debuglnf("unhandled exception in thread % (%)", id(), pending_exception());
    abort(1);
  }

  if (id() == kMainFiberThreadId) {
    debugln("main fiber exited with value %", result);

    if (result.isInt()) {
      abort(RawInt::cast(result).value());
    } else {
      abort(0);
    }
  }
}

void Thread::enter_scheduler() {
  Thread::set_current(nullptr);
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

  m_context = make_fcontext(m_stack->hi(), m_stack->size(), [](transfer_t transfer) {
    auto* worker = static_cast<Worker*>(transfer.data);
    worker->set_context(transfer.fctx);
    Thread* thread = worker->thread();
    Thread::set_current(thread);
    thread->m_worker = worker;

    if (thread->id() == kMainThreadId) {
      thread->entry_main_thread();
    } else {
      thread->entry_fiber_thread();
    }

    thread->m_state.acas(State::Running, State::Exited);
    thread->enter_scheduler();
  });
}

void Thread::push_frame(Frame* frame) {
  DCHECK(frame->parent == m_frame);
  m_frame = frame;
}

void Thread::pop_frame(Frame* frame) {
  DCHECK(m_frame == frame);
  m_frame = m_frame->parent;
}

void Thread::wake_waiting_threads() {
  auto scheduler = runtime()->scheduler();
  auto processor = worker()->processor();

  DCHECK(fiber().isFiber());
  DCHECK(RawFiber::cast(fiber()).is_locked());

  if (!m_waiting_threads.empty()) {
    for (Thread* thread : m_waiting_threads) {
      DCHECK(thread->state() == State::Waiting);
      thread->ready();
      scheduler->schedule_thread(thread, processor);
    }

    m_waiting_threads.clear();
  }
}

}  // namespace charly::core::runtime
