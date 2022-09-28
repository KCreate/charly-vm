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

Thread::Type Thread::type() const {
  return m_type;
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

void Thread::set_worker(Worker* worker) {
  m_worker = worker;
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

void Thread::set_last_scheduled_at(size_t timestamp) {
  m_last_scheduled_at = timestamp;
}

fcontext_t& Thread::context() {
  return m_context;
}

void Thread::set_context(fcontext_t& context) {
  m_context = context;
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
  acas_state(State::Free, State::Waiting);
  m_type = Type::Main;
}

void Thread::init_fiber_thread(RawFiber fiber) {
  acas_state(State::Free, State::Waiting);
  m_type = Type::Fiber;
  m_fiber = fiber;
  fiber.set_thread(this);
}

void Thread::init_proc_scheduler_thread() {
  acas_state(State::Free, State::Waiting);
  m_type = Type::Scheduler;
}

void Thread::clean() {
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
  worker()->checkpoint();

  if (m_last_scheduled_at == kShouldYieldToSchedulerTimestamp) {
    Thread::context_switch_thread_to_scheduler(this, State::Ready);
  }
}

void Thread::abort(int32_t exit_code) {
  m_exit_code = exit_code;
  Thread::context_switch_thread_to_scheduler(this, State::Aborted);
  UNREACHABLE();
}

void Thread::wake_from_wait() {
  acas_state(State::Waiting, State::Ready);
}

void Thread::wake_from_future_wait() {
  acas_state(State::WaitingForFuture, State::Ready);
}

void Thread::enter_native() {
  DCHECK(m_state == State::Running);
  DCHECK(m_worker != nullptr);
  acas_state(State::Running, State::Native);
  m_worker->enter_native();
}

void Thread::exit_native() {
  DCHECK(m_state == State::Native);
  DCHECK(m_worker != nullptr);
  acas_state(State::Native, State::Running);
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

void Thread::context_handler(transfer_t transfer) {
  Thread* thread = static_cast<Thread*>(transfer.data);
  Worker* worker = thread->worker();
  Thread::set_current(thread);
  Thread* t_sched = worker->scheduler_thread();

  DCHECK(worker->thread() == thread);

  switch (thread->type()) {

    // initial jump to a ProcScheduler thread happens on the native
    // OS stack frame inside Worker::scheduler_loop
    case Type::Scheduler: {
      DCHECK(thread == t_sched);
      worker->set_context(transfer.fctx);
      thread->entry_proc_scheduler_thread();
      UNREACHABLE();
    }

    // initial jump to Main and Fiber threads happens within ProcScheduler
    // threads
    case Type::Main: {
      t_sched->set_context(transfer.fctx);
      thread->entry_main_thread();
      UNREACHABLE();
    }
    case Type::Fiber: {
      t_sched->set_context(transfer.fctx);
      thread->entry_fiber_thread();
      UNREACHABLE();
    }
  }
}

void Thread::context_switch_worker_to_scheduler(Worker* worker) {
  Thread* scheduler = worker->scheduler_thread();
  scheduler->wake_from_wait();

  DCHECK(worker->thread() == nullptr);
  worker->set_thread(scheduler);

  if (scheduler->stack() == nullptr) {
    scheduler->acquire_stack();
    DCHECK(scheduler->stack());
  }
  scheduler->acas_state(State::Ready, State::Running);
  scheduler->set_worker(worker);
  scheduler->set_last_scheduled_at(kNeverScheduledTimestamp, get_steady_timestamp());

  transfer_t transfer = jump_fcontext(scheduler->context(), scheduler);
  DCHECK(transfer.data == nullptr);
  scheduler->set_context(transfer.fctx);
  scheduler->set_last_scheduled_at(kNeverScheduledTimestamp);
  scheduler->set_worker(nullptr);
  worker->set_thread(nullptr);
  DCHECK(scheduler->state() == State::Waiting);
  DCHECK(worker->scheduler_thread() == scheduler);
}

void Thread::context_switch_scheduler_to_worker(Thread* scheduler) {
  Worker* worker = scheduler->worker();
  DCHECK(scheduler->is_scheduler());
  scheduler->acas_state(State::Running, State::Waiting);
  Thread::set_current(nullptr);
  transfer_t transfer = jump_fcontext(worker->context(), nullptr);
  DCHECK(transfer.data == scheduler);
  DCHECK(scheduler->worker() == worker);
  DCHECK(worker->thread() == scheduler);
  worker->set_context(transfer.fctx);
  Thread::set_current(scheduler);
}

void Thread::context_switch_scheduler_to_thread(Thread* from_scheduler, Thread* to_thread) {
  DCHECK(from_scheduler->is_scheduler());
  DCHECK(to_thread->type() != Type::Scheduler);

  Worker* worker = from_scheduler->worker();
  DCHECK(worker->scheduler_thread() == from_scheduler);
  DCHECK(worker->thread() == from_scheduler);
  worker->increase_context_switch_counter();
  worker->set_thread(to_thread);

  from_scheduler->acas_state(State::Running, State::Waiting);
  from_scheduler->set_last_scheduled_at(kNeverScheduledTimestamp);
  from_scheduler->set_worker(nullptr);

  if (to_thread->stack() == nullptr) {
    to_thread->acquire_stack();
    DCHECK(to_thread->stack());
  }

  to_thread->acas_state(State::Ready, State::Running);
  to_thread->set_last_scheduled_at(kNeverScheduledTimestamp, get_steady_timestamp());
  to_thread->set_worker(worker);

  transfer_t transfer = jump_fcontext(to_thread->context(), to_thread);
  DCHECK(transfer.data == to_thread);
  to_thread->set_context(transfer.fctx);
  Thread::set_current(from_scheduler);

  // reenter scheduler thread
  DCHECK(worker->scheduler_thread() == from_scheduler);
  DCHECK(from_scheduler->worker() == worker);
  DCHECK(worker->thread() == from_scheduler);
  from_scheduler->set_last_scheduled_at(kNeverScheduledTimestamp, get_steady_timestamp());
}

void Thread::context_switch_thread_to_scheduler(Thread* from_thread, Thread::State state) {
  Worker* worker = from_thread->worker();
  Thread* t_sched = worker->scheduler_thread();
  DCHECK(t_sched->state() == State::Waiting);
  DCHECK(t_sched->is_scheduler());

  DCHECK(from_thread->type() != Type::Scheduler);
  from_thread->acas_state(State::Running, state);
  from_thread->set_last_scheduled_at(kNeverScheduledTimestamp);
  from_thread->set_worker(nullptr);

  t_sched->acas_state(State::Waiting, State::Running);
  t_sched->set_worker(worker);
  worker->set_thread(t_sched);

  transfer_t transfer = jump_fcontext(t_sched->context(), from_thread);
  DCHECK(transfer.data == from_thread);
  worker = from_thread->worker();
  t_sched = worker->scheduler_thread();
  t_sched->set_context(transfer.fctx);
  Thread::set_current(from_thread);
}

void Thread::entry_main_thread() {
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
    abort(1);
    UNREACHABLE();
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
    abort(1);
    UNREACHABLE();
  }

  abort(0);
  UNREACHABLE();
}

void Thread::entry_fiber_thread() {
  {
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

  Thread::context_switch_thread_to_scheduler(this, State::Exited);
}

void Thread::entry_proc_scheduler_thread() {
  Runtime* runtime = this->runtime();
  Scheduler* scheduler = runtime->scheduler();

  for (;;) {
    Worker* worker = m_worker;
    Processor* proc = worker->processor();

    if (runtime->wants_exit()) {
      Thread::context_switch_scheduler_to_worker(this);
      UNREACHABLE();
    }

    proc->fire_timer_events(this);

    Thread* ready_thread = proc->get_ready_thread();
    if (ready_thread == nullptr) {
      size_t next_event = proc->timestamp_of_next_timer_event();

      if (next_event) {
        size_t now = get_steady_timestamp();

        if (next_event <= now) {
          continue;
        }

        size_t duration = next_event - now;
        native_section([&] {
          std::this_thread::sleep_for(std::chrono::milliseconds(duration));
        });
        continue;
      }

      Thread::context_switch_scheduler_to_worker(this);
      continue;
    }

    DCHECK(ready_thread->worker() == nullptr);
    DCHECK(ready_thread->state() == State::Ready);
    Thread::context_switch_scheduler_to_thread(this, ready_thread);

    switch (ready_thread->state()) {
      case State::Waiting: {
        continue;
      }
      case State::WaitingForFuture: {
        auto future = RawFuture::cast(ready_thread->m_waiting_on_future);
        DCHECK(future.has_finished() == false);
        DCHECK(future.is_locked());
        auto* wait_queue = future.wait_queue();
        future.set_wait_queue(wait_queue->append_thread(wait_queue, ready_thread));
        future.unlock();
        continue;
      }
      case State::Ready: {
        scheduler->schedule_thread(ready_thread, proc);
        continue;
      }
      case State::Exited: {
        scheduler->recycle_thread(ready_thread);
        continue;
      }
      case State::Aborted: {
        int32_t exit_code = ready_thread->exit_code();
        scheduler->recycle_thread(ready_thread);
        runtime->abort(exit_code);
        continue;
      }
      default: FAIL("unexpected thread state");
    }
  }
}

void Thread::wait_on_future(RawFuture future) {
  DCHECK(m_waiting_on_future.isNull());
  m_waiting_on_future = future;
  Thread::context_switch_thread_to_scheduler(this, State::WaitingForFuture);
  m_waiting_on_future = kNull;
}

void Thread::acquire_stack() {
  DCHECK(m_stack == nullptr);
  m_stack = m_runtime->scheduler()->get_free_stack();
  DCHECK(m_stack, "could not allocate thread stack");
  m_context = make_fcontext(m_stack->hi(), m_stack->size(), &Thread::context_handler);
}

void Thread::dump_exception_trace(RawException exception) const {
  debuglnf_notime("Unhandled exception in main thread:");
  debuglnf_notime("%", exception);

  size_t depth = 0;
  while (exception.cause().isException()) {
    if (depth++ > kExceptionChainDepthLimit) {
      debuglnf_notime("\nMore exceptions were thrown that are not shown here");
      break;
    }

    exception = exception.cause();
    debuglnf_notime("\nThe above exception was thrown during handling of this exception:");
    debuglnf_notime("%", exception);
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
