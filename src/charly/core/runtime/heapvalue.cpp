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

#include "charly/core/runtime/gc.h"
#include "charly/core/runtime/heapvalue.h"
#include "charly/core/runtime/worker.h"

namespace charly::core::runtime {

void HeapHeader::init(Type type) {
  m_forward_ptr.assert_cas(nullptr, this);
  m_type.assert_cas(kTypeDead, type);
}

void HeapHeader::init_dead() {
  m_forward_ptr.store(nullptr);
  m_type.store(kTypeDead);
  m_mark.store(kMarkColorBlack);
}

void HeapHeader::destroy() {
  // nothing
}

HeapHeader* HeapHeader::resolve() {
  if (GarbageCollector::instance->phase() == GCPhase::Evacuate) {
    // TODO: evacuate value
  }

  return m_forward_ptr;
}

Type HeapHeader::type() const {
  return m_type.load();
}

MarkColor HeapHeader::color() const {
  return m_mark.load();
}

void HeapHeader::set_color(MarkColor color) {
  m_mark.store(color);
}

void HeapFiber::init(Fiber::FiberTaskFn fn) {
  safeprint("initializing heapfiber %", this);
  HeapHeader::init(kTypeFiber);
  m_fiber = new Fiber(fn);
}

void HeapFiber::destroy() {
  if (m_fiber) {
    delete m_fiber;
    m_fiber = nullptr;
  }
}

Fiber::Status HeapFiber::status() const {
  return m_fiber->m_status.load();
}

}  // namespace charly::core::runtime
