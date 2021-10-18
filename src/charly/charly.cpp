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

#include "charly/charly.h"
#include "charly/core/runtime/runtime.h"
#include "charly/core/runtime/interpreter.h"

namespace charly {

std::mutex debugln_mutex;
auto program_startup_timestamp = std::chrono::steady_clock::now();

void print_runtime_debug_state(std::ostream& stream) {
  using namespace core::runtime;

  if (Thread* thread = Thread::current()) {
    debugln_impl_time(stream, "\n");
    debugln_impl_time(stream, "Thread: %\n", thread->id());

    if (Frame* frame = thread->frame()) {
      debugln_impl_time(stream, "IP: %\n", (void*)frame->oldip);
      debugln_impl_time(stream, "Function: %\n", frame->function);

      Frame* old = frame;
      debugln_impl_time(stream, "Frames:\n", frame);
      while (frame) {
        debugln_impl_time(stream, "  - %: %\n", frame->function, (void*)frame->oldip);
        frame = frame->parent;
      }
      frame = old;

      if (frame->sp > 0) {
        debugln_impl_time(stream, "\n");
        debugln_impl_time(stream, "Stack:\n");
        for (int64_t i = frame->sp - 1; i >= 0; i--) {
          debugln_impl_time(stream, "  - %\n", frame->stack[i]);
        }
      }
    }

  }
}

}
