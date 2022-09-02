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

#include <execinfo.h>

#include "charly/charly.h"
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"

namespace charly {

std::recursive_mutex debugln_mutex;
auto program_startup_timestamp = std::chrono::steady_clock::now();

void print_runtime_debug_state(std::ostream& stream) {
  using namespace core::runtime;

  if (Thread* thread = Thread::current()) {
    debugln_impl_time(stream, "\n");
    debugln_impl_time(stream, "Thread: %\n", thread->id());

    if (Frame* frame = thread->frame()) {
      if (frame->is_builtin_frame()) {
        auto* builtin_frame = static_cast<BuiltinFrame*>(frame);
        debugln_impl_time(stream, "Builtin Function: %\n", builtin_frame->function);
      } else {
        auto* interpreter_frame = static_cast<InterpreterFrame*>(frame);
        debugln_impl_time(stream, "IP: %\n", (void*)interpreter_frame->oldip);
        debugln_impl_time(stream, "Function: %\n", interpreter_frame->function);
      }

      Frame* old = frame;
      debugln_impl_time(stream, "Frames:\n", frame);
      while (frame) {
        if (frame->is_builtin_frame()) {
          auto* builtin_frame = static_cast<BuiltinFrame*>(frame);
          debugln_impl_time(stream, "  - %\n", builtin_frame->function);
        } else {
          auto* interpreter_frame = static_cast<InterpreterFrame*>(frame);
          debugln_impl_time(stream, "  - %: %\n", interpreter_frame->function, (void*)interpreter_frame->oldip);
        }
        frame = frame->parent;
      }
      frame = old;

      if (frame->is_interpreter_frame()) {
        auto* interpreter_frame = static_cast<InterpreterFrame*>(frame);
        DCHECK(interpreter_frame->stack);
        if (interpreter_frame->sp > 0) {
          debugln_impl_time(stream, "\n");
          debugln_impl_time(stream, "Stack:\n");
          for (int64_t i = interpreter_frame->sp - 1; i >= 0; i--) {
            debugln_impl_time(stream, "  - %\n", interpreter_frame->stack[i]);
          }
        }
      }
    } else {
      debugln_impl_time(stream, "No active frame!\n");
    }
  } else {
    debugln_impl_time(stream, "No active thread!\n");
  }

  void* callstack[128];
  int frames = backtrace(callstack, 128);
  char** frame_symbols = backtrace_symbols(callstack, frames);
  debugln_impl_time(stream, "Hardware Stack:\n");
  for (int i = 0; i < frames; ++i) {
    debugln_impl_time(stream, "  - % %\n", i, frame_symbols[i] + 58);
  }
  utils::Allocator::free(frame_symbols);
}

}  // namespace charly
