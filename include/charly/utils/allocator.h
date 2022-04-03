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

#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#pragma once

namespace charly {

static const size_t kPageSize = sysconf(_SC_PAGESIZE);

namespace utils {

  /*
   * Charly allocator class
   *
   * Handles heap memory allocations and guarded page access
   * */
  class Allocator {
  public:
    // allocate aligned heap memory
    static void* alloc(size_t size, size_t alignment = 8);

    // allocates memory via mmap
    // mapped memory region is initially protected with PROT_NONE
    static void* mmap_page_aligned(size_t size,
                                   int32_t protection = PROT_NONE,
                                   int32_t flags = MAP_PRIVATE | MAP_ANONYMOUS);
    static void* mmap_self_aligned(size_t size,
                                   int32_t protection = PROT_NONE,
                                   int32_t flags = MAP_PRIVATE | MAP_ANONYMOUS);
    static void* mmap_address(void* address,
                              size_t size,
                              int32_t protection = PROT_NONE,
                              int32_t flags = MAP_PRIVATE | MAP_ANONYMOUS);

    // reallocate pointer to fit a bigger size or change alignment
    // acts like alloc if pointer is null
    // returns old pointer if it fulfills the new requirements
    static void* realloc(void* old_pointer, size_t old_size, size_t new_size, size_t new_alignment = 8);

    // free a pointer allocated by alloc
    static void free(void* pointer);

    // munmap a region of memory
    static void munmap(void* pointer, size_t size);

    // protect a section of memory as either read-only or read and writeable
    static void protect_none(void* pointer, size_t size);
    static void protect_read(void* pointer, size_t size);
    static void protect_readwrite(void* pointer, size_t size);
    static void protect_exec(void* pointer, size_t size);
  };

}  // namespace utils
}  // namespace charly
