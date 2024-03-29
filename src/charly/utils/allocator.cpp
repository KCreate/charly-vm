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

#include "charly/utils/allocator.h"
#include "charly/debug.h"

namespace charly::utils {

void* Allocator::alloc(size_t size) {
  DCHECK(size > 0);
  void* memory = std::malloc(size);
  DCHECK(memory != nullptr);
  return memory;
}

void* Allocator::aligned_alloc(size_t size, size_t alignment) {
  DCHECK((alignment >= 8) && (alignment % 8 == 0));
  DCHECK(size > 0);

  void* memory = std::aligned_alloc(alignment, size);
  DCHECK(memory != nullptr);
  return memory;
}

void* Allocator::mmap_page_aligned(size_t size, int32_t protection, int32_t flags) {
  DCHECK(size >= kPageSize, "expected size to be at least the page size");
  DCHECK(size % kPageSize == 0, "expected size to be a multiple of the page size");
  void* memory = ::mmap(nullptr, size, protection, flags, -1, 0);
  CHECK(memory != nullptr, "could not mmap memory");
  return memory;
}

void* Allocator::mmap_self_aligned(size_t size, int32_t protection, int32_t flags) {
  DCHECK(size >= kPageSize, "expected size to be at least the page size");
  DCHECK(size % kPageSize == 0, "expected size to be a multiple of the page size");

  size_t alignment = size;
  auto memory = (uintptr_t)mmap_page_aligned(size * 2, protection, flags);
  size_t excess_upper = memory % alignment;
  size_t excess_lower = alignment - excess_upper;
  uintptr_t lower_excess_base = memory;
  uintptr_t upper_excess_base = memory + (alignment * 2) - excess_upper;
  uintptr_t aligned_base = memory + excess_lower;

  DCHECK(excess_lower % kPageSize == 0);
  DCHECK(excess_upper % kPageSize == 0);

  // allocation is optimally aligned already
  if (excess_lower == alignment) {
    munmap((void*)memory, alignment);
    return (void*)(memory + alignment);
  }

  // unmap excess pages
  munmap((void*)lower_excess_base, excess_lower);
  munmap((void*)upper_excess_base, excess_upper);
  DCHECK(aligned_base % alignment == 0);
  return (void*)aligned_base;
}

void* Allocator::mmap_address(void* address, size_t size, int32_t protection, int32_t flags) {
  DCHECK(size >= kPageSize, "expected size to be at least the page size");
  DCHECK(size % kPageSize == 0, "expected size to be a multiple of the page size");

  void* memory = ::mmap(address, size, protection, flags, -1, 0);

  if (memory == MAP_FAILED) {
    FAIL("could not map address %: %", address, std::strerror(errno));
  }

  return memory;
}

void* Allocator::realloc(void* old_pointer, size_t old_size, size_t new_size, size_t new_alignment) {
  // act like alloc if old pointer is null
  if (old_pointer == nullptr) {
    DCHECK(old_size == 0);
    return Allocator::aligned_alloc(new_size, new_alignment);
  }

  // return old pointer if it fulfills the new requirements
  if ((uintptr_t)old_pointer % new_alignment == 0) {
    if (old_size >= new_size) {
      return old_pointer;
    }
  }

  void* new_pointer = Allocator::aligned_alloc(new_size, new_alignment);
  std::memcpy(new_pointer, old_pointer, std::min(old_size, new_size));
  Allocator::free(old_pointer);
  return new_pointer;
}

void Allocator::free(void* pointer) {
  CHECK(pointer != nullptr);
  std::free(pointer);
}

void Allocator::munmap(void* pointer, size_t size) {
  CHECK(pointer != nullptr);
  CHECK((uintptr_t)pointer % kPageSize == 0);
  DCHECK(size >= kPageSize);
  DCHECK(size % kPageSize == 0);

  if (::munmap(pointer, size) != 0) {
    FAIL("could not munmap % bytes at pointer %", size, pointer);
  }
}

void protect_impl(void* pointer, size_t size, int32_t flags) {
  DCHECK(pointer != nullptr);
  DCHECK((uintptr_t)pointer % kPageSize == 0);
  DCHECK(size >= kPageSize);
  DCHECK(size % kPageSize == 0);

  if (mprotect(pointer, size, flags)) {
    FAIL("could not change memory protection");
  }
}

void Allocator::protect_none(void* pointer, size_t size) {
  protect_impl(pointer, size, PROT_NONE);
}

void Allocator::protect_read(void* pointer, size_t size) {
  protect_impl(pointer, size, PROT_READ);
}

void Allocator::protect_readwrite(void* pointer, size_t size) {
  protect_impl(pointer, size, PROT_READ | PROT_WRITE);
}

void Allocator::protect_exec(void* pointer, size_t size) {
  protect_impl(pointer, size, PROT_READ | PROT_EXEC);
}

}  // namespace charly::utils
