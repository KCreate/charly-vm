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

#include "charly/utils/allocator.h"

#pragma once

namespace charly::utils {

class GuardedBuffer {
public:
  explicit GuardedBuffer(size_t size) : m_mapping(nullptr), m_size(size) {
    DCHECK(size >= kPageSize, "expected size to be at least the page size");
    DCHECK(size % kPageSize == 0, "expected size to be a multiple of the page size");
    m_mapping = Allocator::mmap_page_aligned(size + kPageSize * 2);
    Allocator::protect_readwrite((void*)((uintptr_t)m_mapping + kPageSize), size);
  }
  ~GuardedBuffer() {
    DCHECK(m_mapping != nullptr);
    Allocator::munmap(m_mapping, m_size);
    m_mapping = nullptr;
    m_size = 0;
  }

  void* data() const {
    return (void*)((uintptr_t)m_mapping + kPageSize);
  }

  size_t size() const {
    return m_size - kPageSize * 2;
  }

private:
  void* m_mapping;
  size_t m_size;
};

}  // namespace charly::utils
