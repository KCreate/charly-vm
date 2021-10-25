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

Thread::Thread(Runtime* runtime) {
  m_runtime = runtime;
  m_id = thread_id_counter++;
  clean();
}

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

const Stack& Thread::stack() const {
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
  m_is_main_thread = true;
  m_state = State::Waiting;
  m_fiber = kNull;
  m_context = make_fcontext(m_stack.hi(), m_stack.size(), [](transfer_t transfer) {
    Worker* worker = static_cast<Worker*>(transfer.data);
    worker->set_context(transfer.fctx);
    Thread* thread = worker->thread();
    Thread::set_current(thread);
    thread->m_worker = worker;
    thread->entry_main_thread();
    thread->enter_scheduler(State::Exited);
  });
}

void Thread::init_fiber_thread(RawFiber fiber) {
  fiber.set_thread(this);
  m_is_main_thread = false;
  m_state = State::Waiting;
  m_fiber = fiber;
  m_context = make_fcontext(m_stack.hi(), m_stack.size(), [](transfer_t transfer) {
    Worker* worker = static_cast<Worker*>(transfer.data);
    worker->set_context(transfer.fctx);
    Thread* thread = worker->thread();
    Thread::set_current(thread);
    thread->m_worker = worker;
    thread->entry_fiber_thread();
    thread->enter_scheduler(State::Exited);
  });
}

void Thread::clean() {
  m_is_main_thread = false;
  m_state = State::Free;
  m_stack.clear();
  m_exit_code = 0;
  m_fiber = kNull;
  m_worker = nullptr;
  m_last_scheduled_at = 0;
  m_frame = nullptr;
  m_pending_exception = kErrorOk;
}

void Thread::checkpoint() {
  Worker* worker = m_worker;
  DCHECK(worker);

  if (worker->has_stop_flag() || has_exceeded_timeslice()) {
    enter_scheduler(State::Ready);
  }
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
  transfer_t transfer = jump_fcontext(m_context, worker);
  m_context = transfer.fctx;
  DCHECK(m_worker == nullptr);
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
  Runtime* runtime = m_runtime;

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

  auto unit = Compiler::compile(boot_path, boot_file, CompilationUnit::Type::Module);
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
  DCHECK(module->function_table.size());
  runtime->register_module(module);

  HandleScope scope(this);
  Function function(scope, runtime->create_function(this, kNull, module->function_table.front()));
  Fiber fiber(scope, runtime->create_fiber(this, function));

  Thread* fiber_thread = runtime->scheduler()->get_free_thread();
  DCHECK(fiber_thread->id() == 1);
  fiber_thread->init_fiber_thread(fiber);
  fiber_thread->ready();
  runtime->scheduler()->schedule_thread(fiber_thread, worker()->processor());
}

void Thread::entry_fiber_thread() {
  RawFiber fiber = RawFiber::cast(this->fiber());
  RawFunction function = fiber.function();

  RawValue result = Interpreter::call_function(this, kNull, function, nullptr, 0);
  if (result == kErrorException) {
    debuglnf("unhandled exception in thread % (%)", id(), pending_exception());
    abort(1);
  }

  if (id() == 1) {
    debuglnf("main thread exited with value %", result);
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

void Thread::push_frame(Frame* frame) {
  DCHECK(frame->parent == m_frame);
  m_frame = frame;
}

void Thread::pop_frame(Frame* frame) {
  DCHECK(m_frame == frame);
  m_frame = m_frame->parent;
}

}  // namespace charly::core::runtime
