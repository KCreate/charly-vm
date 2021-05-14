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

#include "charly/core/runtime/worker.h"
#include "charly/core/runtime/fiber.h"

namespace charly::core::runtime {

using namespace boost::context::detail;

FiberStack::FiberStack() {
  m_bottom = (void*)std::malloc(kFiberStackSize);
  m_top = (void*)((uintptr_t)m_bottom + (uintptr_t)kFiberStackSize);
  m_size = kFiberStackSize;

  safeprint("allocating stack at %", m_bottom);
}

FiberStack::~FiberStack() {
  safeprint("deallocating stack at %", m_bottom);
  if (m_bottom) {
    std::free(m_bottom);
  }
  m_bottom = nullptr;
  m_top = nullptr;
  m_size = 0;
}

void* FiberStack::top() const {
  return m_top;
}

void* FiberStack::bottom() const {
  return m_bottom;
}

size_t FiberStack::size() const {
  return m_size;
}

void Fiber::fiber_handler_function(transfer_t transfer) {
  g_worker->m_context = transfer.fctx;
  g_worker->current_fiber()->m_task_function();
  g_worker->fiber_exit();
}

void* Fiber::jump_context(void* argument) {
  transfer_t transfer = jump_fcontext(m_context, argument);
  m_context = transfer.fctx;
  return transfer.data;
}

}  // namespace charly::core::runtime
