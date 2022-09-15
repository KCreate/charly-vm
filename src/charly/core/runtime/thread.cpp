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

static atomic<size_t> thread_id_counter = 0;

thread_local static Thread* g_thread = nullptr;

Thread::Thread(Runtime* runtime) : m_id(thread_id_counter++), m_runtime(runtime) {}

Thread* Thread::current() {
  return g_thread;
}

void Thread::set_current(Thread* thread) {
  g_thread = thread;
}

size_t Thread::id() const {
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

size_t Thread::last_scheduled_at() const {
  return m_last_scheduled_at;
}

bool Thread::set_last_scheduled_at(size_t old_timestamp, size_t timestamp) {
  return m_last_scheduled_at.cas(old_timestamp, timestamp);
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
  m_last_scheduled_at = kNeverScheduledTimestamp;
  m_frame = nullptr;
  m_pending_exception = kNull;
  m_context = nullptr;
}

void Thread::checkpoint() {
  if (m_worker->has_stop_flag() || m_last_scheduled_at == kShouldYieldToSchedulerTimestamp) {
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
  m_last_scheduled_at = get_steady_timestamp();
  DCHECK(m_last_scheduled_at >= kFirstValidScheduledAtTimestamp);

  if (m_stack == nullptr) {
    acquire_stack();
  }
  DCHECK(m_stack != nullptr);

  transfer_t transfer = jump_fcontext(m_context, worker);
  m_context = transfer.fctx;
  m_last_scheduled_at = kNeverScheduledTimestamp;
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

RawValue Thread::throw_exception(RawException exception) {
  if (exception == pending_exception()) {
    return kErrorException;
  }

  if (exception.cause().isNull()) {
    exception.set_cause(pending_exception());
  }
  set_pending_exception(exception);
  return kErrorException;
}

void Thread::rethrow_exception(RawException exception) {
  set_pending_exception(exception);
}

void Thread::set_pending_exception(RawValue value) {
  m_pending_exception = value;
}

int32_t Thread::entry_main_thread() {
  CHECK(id() == kMainThreadId);

  Runtime* runtime = m_runtime;
  runtime->initialize_null_initialized_page();
  runtime->initialize_symbol_table(this);
  runtime->initialize_builtin_types(this);
  runtime->initialize_argv_tuple(this);
  runtime->initialize_builtin_functions(this);

  fs::path boot_path = runtime->stdlib_directory() / "boot.ch";
  RawValue result = runtime->import_module_at_path(this, boot_path, true);
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

  RawValue user_result = runtime->import_module_at_path(this, filename, load_as_repl);
  if (user_result.is_error_exception()) {
    dump_exception_trace(RawException::cast(pending_exception()));
    return 1;
  }

  return 0;
}

void Thread::entry_fiber_thread() {
  HandleScope scope(this);
  Fiber fiber(scope, m_fiber);
  Value arguments(scope, fiber.arguments());

  RawValue* arguments_pointer = nullptr;
  size_t argc = 0;
  if (arguments.isTuple()) {
    Tuple argtuple(scope, arguments);
    arguments_pointer = argtuple.data();
    argc = argtuple.length();
  } else {
    DCHECK(arguments.isNull());
  }

  RawValue result =
    Interpreter::call_function(this, fiber.context(), fiber.function(), arguments_pointer, argc, false, arguments);
  {
    std::lock_guard lock(fiber);
    fiber.set_thread(nullptr);
    if (result.is_error_exception()) {
      fiber.result_future().reject(this, RawException::cast(pending_exception()));
    } else {
      DCHECK(!result.is_error());
      fiber.result_future().resolve(this, result);
    }
  }
}

void Thread::enter_scheduler() {
  Thread* old_thread = Thread::current();
  Thread::set_current(nullptr);
  fcontext_t& context = m_worker->context();
  m_worker = nullptr;
  transfer_t transfer = jump_fcontext(context, nullptr);
  m_worker = static_cast<Worker*>(transfer.data);
  DCHECK(this == old_thread);
  DCHECK(m_worker->thread() == old_thread);
  Thread::set_current(m_worker->thread());
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
    RawValue cause = next_exception.cause();
    if (!cause.isException()) {
      break;
    }

    next_exception = RawException::cast(cause);
  }

  RawException first_exception = exception_stack.top();
  exception_stack.pop();
  debuglnf_notime("Unhandled exception in main thread:");
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

void Thread::acas_state(Thread::State old_state, Thread::State new_state) {
  m_state.acas(old_state, new_state);
}

RawTuple Thread::create_backtrace() {
  Frame* top_frame = frame();

  if (top_frame == nullptr) {
    return RawTuple::create_empty(this);
  }

  HandleScope scope(this);
  Tuple backtrace(scope, RawTuple::create(this, std::min(top_frame->depth + 1, kBacktraceDepthLimit)));
  size_t index = 0;
  for (Frame* frame = top_frame; frame != nullptr && index < kBacktraceDepthLimit; frame = frame->parent) {
    if (frame->is_interpreter_frame()) {
      auto* interpreter_frame = static_cast<InterpreterFrame*>(frame);

      uintptr_t oldip = interpreter_frame->oldip;
      auto loc = runtime()->source_location_for_ip(oldip);
      if (loc.has_value()) {
        fs::path path = loc->path;

        fs::path cwd = fs::current_path();
        if (cwd.compare(path) < 0) {
          path = fs::relative(path);
        }

        backtrace.set_field_at(index, RawTuple::create(this, interpreter_frame->function, RawString::create(this, path),
                                                       RawInt::create(loc->row + 1), RawInt::create(loc->column + 1)));
      } else {
        backtrace.set_field_at(index, RawTuple::create(this, interpreter_frame->function, RawString::create(this, "??"),
                                                       RawInt::create(0), RawInt::create(0)));
      }
    } else {
      auto* builtin_frame = static_cast<BuiltinFrame*>(frame);
      backtrace.set_field_at(index, RawTuple::create(this, builtin_frame->function));
    }
    index++;
  }

  return *backtrace;
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

uintptr_t Thread::allocate(size_t size, bool contains_external_heap_pointers) {
  checkpoint();
  auto* tab = m_worker->processor()->tab();
  return tab->allocate(this, size, contains_external_heap_pointers);
}

}  // namespace charly::core::runtime
