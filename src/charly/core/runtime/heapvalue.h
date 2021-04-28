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

#include <cstdlib>

#include "charly/charly.h"
#include "charly/atomic.h"
#include "charly/value.h"

#pragma once

namespace charly::core::runtime {

enum Type : uint8_t {
  kTypeDead = 0,
  kTypeTest
};

using MarkColor = uint8_t;
static const MarkColor kMarkColorWhite = 0x01;  // value not reachable
static const MarkColor kMarkColorGrey = 0x02;   // value currently being traversed
static const MarkColor kMarkColorBlack = 0x04;  // value reachable

class HeapHeader {
  friend class GCConcurrentWorker;
public:
  void init(Type type);
  void init_dead();

  // resolve a potential forward pointer
  // might concurrently evacuate cell to other heap region
  HeapHeader* resolve();

  Type type() const {
    return m_type.load();
  }

  MarkColor color() const {
    return m_mark.load();
  }

  void set_color(MarkColor color) {
    m_mark.store(color);
  }

private:
  HeapHeader() = delete;
  ~HeapHeader() = delete;

  charly::atomic<HeapHeader*> m_forward_ptr;
  charly::atomic<Type> m_type;
  charly::atomic<MarkColor> m_mark;
  charly::atomic<uint16_t> m_unused1;
  charly::atomic<uint16_t> m_unused2;
  charly::atomic<uint16_t> m_unused3;

  // TODO: add small lock type
};

class HeapTestType : public HeapHeader {
public:
  void init(uint64_t payload, VALUE other);

  VALUE other() const {
    return m_other.load();
  }

private:
  HeapTestType() = delete;
  ~HeapTestType() = delete;

  charly::atomic<uint64_t> m_payload;
  charly::atomic<VALUE> m_other;
};

}  // namespace charly::core::runtime
