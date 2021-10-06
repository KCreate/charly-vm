


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

#include <cstring>
#include <unistd.h>
#include <linux/memfd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "charly/charly.h"

#pragma once

namespace charly::utils {

class GuardedBuffer {
public:
  GuardedBuffer(size_t size, bool read_only = false) :
    m_fd(-1), m_mapping(nullptr), m_mapping_size(0), m_buffer(nullptr), m_buffer_size(0), m_readonly(read_only) {
    DCHECK(size > 0, "expected size to be non 0");
    DCHECK((size % kPageSize) == 0, "expected size () to be multiple of system page size (%)", size, kPageSize);

    size_t buffer_size = size;
    size_t mapping_size = size + (kPageSize * 2);

    // create anonymous file
    if ((m_fd = memfd_create("GuardedBuffer", 0)) == -1) {
      FAIL("could not create memfd file");
    }

    // set file size
    if (ftruncate(m_fd, buffer_size) != 0) {
      FAIL("could not truncate anonymous file");
    }

    // reserve enough address space for the entire buffer + guard pages
    // the leading and trailing pages of the buffer are always set to PROT_NON
    // to prevent overflows
    m_mapping = mmap(nullptr, mapping_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (m_mapping == MAP_FAILED) {
      FAIL("could not mmap address space");
    }

    // map the anonymous file into the address space
    void* buffer_address = (void*)((uintptr_t)m_mapping + kPageSize);
    if (mmap(buffer_address, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, m_fd, 0) == MAP_FAILED) {
      FAIL("could not map anonymous file");
    }

    m_buffer = buffer_address;
    m_buffer_size = buffer_size;
    m_mapping_size = mapping_size;

    set_readonly(read_only);
  }

  ~GuardedBuffer() {
    if (m_fd != -1) {
      close(m_fd);
      m_fd = -1;
    }

    if (m_mapping) {
      munmap(m_mapping, m_mapping_size);
      m_mapping = nullptr;
    }

    m_mapping = nullptr;
    m_buffer = nullptr;
    m_mapping_size = 0;
    m_buffer_size = 0;
  }

  bool is_readonly() const {
    return m_readonly;
  }

  void set_readonly(bool option) {
    if (option == m_readonly) {
      return;
    }

    if (option) {
      if (mprotect(m_buffer, m_buffer_size, PROT_READ) != 0) {
        FAIL("could not enable memory protection");
      }
    } else {
      if (mprotect(m_buffer, m_buffer_size, PROT_READ | PROT_WRITE) != 0) {
        FAIL("could not disable memory protection");
      }
    }

    m_readonly = option;
  }

  void* buffer() const {
    return m_buffer;
  }

  size_t size() const {
    return m_buffer_size;
  }

  void clear() {
    bool old_is_readonly = is_readonly();
    if (old_is_readonly) {
      set_readonly(false);
    }

    std::memset(buffer(), 0, size());

    if (old_is_readonly) {
      set_readonly(true);
    }
  }

private:
  int32_t m_fd;
  void* m_mapping;
  size_t m_mapping_size;
  void* m_buffer;
  size_t m_buffer_size;
  bool m_readonly;
};

}
