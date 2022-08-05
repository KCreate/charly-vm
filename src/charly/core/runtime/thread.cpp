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

#include <filesystem>
#include <fstream>

#include "charly/utils/argumentparser.h"

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
  m_pending_exception(kNull) {}

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

bool Thread::has_exceeded_timeslice() const {
  uint64_t timestamp_now = get_steady_timestamp_milli();
  return timestamp_now - last_scheduled_at() >= kThreadTimeslice;
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

RawValue Thread::pending_exception() const {
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
  m_pending_exception = kNull;
  m_context = nullptr;
}

void Thread::checkpoint() {
  Worker* worker = m_worker;
  DCHECK(worker);

  if (worker->has_stop_flag() || has_exceeded_timeslice()) {
    yield_to_scheduler();
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
  m_last_scheduled_at = get_steady_timestamp_milli();

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
  RawValue exception_result = runtime()->create_exception(this, value);
  if (exception_result.is_error_exception()) {
    return;
  }

  RawException exception = RawException::cast(exception_result);

  if (exception == pending_exception()) {
    return;
  }

  if (exception.cause() == kNull) {
    exception.set_cause(pending_exception());
  }
  set_pending_exception(exception);
}

void Thread::rethrow_value(RawValue value) {
  DCHECK(value.isException());
  set_pending_exception(value);
}

void Thread::set_pending_exception(RawValue value) {
  m_pending_exception = value;
}

int32_t Thread::entry_main_thread() {
  CHECK(id() == kMainThreadId);

  Runtime* runtime = m_runtime;
  runtime->initialize_symbol_table(this);
  runtime->initialize_builtin_types(this);
  runtime->initialize_argv_tuple(this);
  runtime->initialize_builtin_functions(this);

  fs::path boot_path = runtime->stdlib_directory() / "boot.ch";
  RawValue result = runtime->import_module(this, boot_path, true);
  if (result.is_error_exception()) {
    dump_exception_trace(RawException::cast(pending_exception()));
    return 1;
  }

  // execute user file or REPL depending on CLI arguments
  fs::path filename;
  bool load_as_repl = false;
  if (utils::ArgumentParser::USER_FILENAME.has_value()) {
    filename = utils::ArgumentParser::USER_FILENAME.value();
  } else {
    filename = runtime->stdlib_directory() / "repl.ch";
    load_as_repl = true;
  }

  RawValue user_result = runtime->import_module(this, filename, load_as_repl);
  if (user_result.is_error_exception()) {
    dump_exception_trace(RawException::cast(pending_exception()));
    return 1;
  }

  return 0;
}

void Thread::entry_fiber_thread() {
  HandleScope scope(this);
  Fiber fiber(scope, m_fiber);

  RawValue fiber_arguments = fiber.arguments();
  RawValue* argp = nullptr;
  size_t argc = 0;
  if (fiber_arguments.isTuple()) {
    auto tuple = RawTuple::cast(fiber_arguments);
    argp = bitcast<RawValue*>(tuple.address());
    argc = tuple.size();
  }

  RawValue result = Interpreter::call_function(this, fiber.context(), fiber.function(), argp, argc);
  {
    std::lock_guard lock(fiber);
    fiber.set_thread(nullptr);
    if (result.is_error_exception()) {
      runtime()->reject_future(this, fiber.result_future(), RawException::cast(pending_exception()));
    } else {
      DCHECK(!result.is_error());
      runtime()->resolve_future(this, fiber.result_future(), result);
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
      thread->abort(thread->entry_main_thread());
      UNREACHABLE();
    } else {
      thread->entry_fiber_thread();
      thread->m_state.acas(State::Running, State::Exited);
      thread->enter_scheduler();
    }
  });
}

void Thread::dump_exception_trace(RawException exception) const {
  std::stack<RawException> exception_stack;
  RawException next_exception = exception;
  bool chain_too_deep = false;
  while (true) {
    if (exception_stack.size() == kExceptionChainDepthLimit) {
      chain_too_deep = true;
      break;
    }

    exception_stack.push(next_exception);
    RawValue cause = RawException::cast(next_exception).cause();
    if (!cause.isException()) {
      break;
    }

    next_exception = RawException::cast(cause);
  }

  RawException first_exception = exception_stack.top();
  exception_stack.pop();
  debuglnf_notime("Unhandled exception in main thread:\n");
  debuglnf_notime("%", first_exception);

  while (!exception_stack.empty()) {
    RawException top = exception_stack.top();
    exception_stack.pop();
    debuglnf_notime("\nDuring handling of the above exception, another exception occured:\n");
    debuglnf_notime("%", top);
  }

  if (chain_too_deep) {
    debuglnf_notime("\nMore exceptions were thrown that are not shown here");
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

RawValue Thread::lookup_symbol(SYMBOL symbol) const {
  return worker()->processor()->lookup_symbol(symbol);
}

}  // namespace charly::core::runtime
