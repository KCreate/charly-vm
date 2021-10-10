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

#include <iomanip>
#include <sys/mman.h>
#include <cstdlib>
#include <utf8/utf8.h>

#include "charly/charly.h"
#include "charly/symbol.h"
#include "charly/utils/colorwriter.h"
#include "charly/utils/buffer.h"

namespace charly::utils {

void BufferBase::seek(size_t offset) {

  // -1 seeks to end of buffer
  if (offset == (size_t)(-1)) {
    offset = m_writeoffset;
  }

  CHECK(offset <= m_size);
  m_writeoffset = offset;

  if (m_readoffset > m_writeoffset) {
    m_readoffset = m_writeoffset;
  }

  reset_window();
}

void BufferBase::reset_window() {
  m_windowoffset = m_readoffset;
}

void BufferBase::emit_block(const void* data, size_t length) {
  write_to_offset(m_writeoffset, data, length);
  m_writeoffset += length;
}

void BufferBase::emit_zeroes(size_t size) {
  reserve_space(m_writeoffset + size);
  std::memset(data() + m_writeoffset, 0, size);
  m_writeoffset += size;
  update_size();
}

void BufferBase::emit_buffer(const BufferBase& other) {
  char* other_data = other.data();
  size_t other_size = other.size();

  // use a temporary buffer if we're copying ourselves
  if (data() == other.data()) {
    char* tmp_buf = (char*)alloca(other_size);
    std::memcpy(tmp_buf, other_data, other_size);
    write_to_offset(m_writeoffset, tmp_buf, other_size);
  } else {
    write_to_offset(m_writeoffset, other_data, other_size);
  }
  m_writeoffset += other_size;
}

void BufferBase::emit_string(const std::string& str) {
  write_to_offset(m_writeoffset, str.c_str(), str.size());
  m_writeoffset += str.size();
}

void BufferBase::emit_string(const char* str) {
  size_t length = std::strlen(str);
  write_to_offset(m_writeoffset, str, length);
  m_writeoffset += length;
}

void BufferBase::emit_string_view(const std::string_view& view) {
  write_to_offset(m_writeoffset, view.data(), view.size());
  m_writeoffset += view.size();
}

std::string BufferBase::buffer_string() const {
  if (m_size == 0)
    return std::string();

  return std::string(data(), m_size);
}

std::string BufferBase::window_string() const {
  size_t window_size = m_readoffset - m_windowoffset;

  if (window_size == 0)
    return std::string();

  return std::string(data() + m_windowoffset, window_size);
}

std::string_view BufferBase::buffer_view() const {
  if (m_size == 0)
    return std::string_view();

  return std::string_view(data(), m_size);
}

std::string_view BufferBase::window_view() const {
  size_t window_size = m_readoffset - m_windowoffset;

  if (window_size == 0)
    return std::string_view();

  return std::string_view(data() + m_windowoffset, window_size);
}

uint32_t BufferBase::buffer_hash() const {
  return SYM((const uint8_t*)data(), m_size);
}

uint32_t BufferBase::window_hash() const {
  return SYM((const uint8_t*)(data() + m_windowoffset), window_size());
}

void BufferBase::emit_utf8_cp(uint32_t cp) {
  reserve_space(m_writeoffset + 4);

  char* buffer_initial = data() + m_writeoffset;
  char* buffer_ptr = buffer_initial;
  buffer_ptr = utf8::append(cp, buffer_ptr);
  m_writeoffset += buffer_ptr - buffer_initial;
  update_size();
}

uint32_t BufferBase::peek_utf8_cp(uint32_t nth) const {
  char* buffer_initial = data() + m_readoffset;
  char* buffer_ptr = buffer_initial;

  if (m_readoffset >= m_writeoffset)
    return '\0';

  // walk forward to requested character
  while (nth--) {
    utf8::next(buffer_ptr, data() + m_writeoffset);

    // overflow check
    if (buffer_ptr >= data() + m_writeoffset) {
      return '\0';
    }
  }

  return utf8::peek_next(buffer_ptr, data() + m_writeoffset);
}

uint32_t BufferBase::read_utf8_cp() {
  if (m_readoffset >= m_writeoffset)
    return L'\0';

  // advance by one utf8 codepoint
  char* buffer_initial = data() + m_readoffset;
  char* buffer_ptr = buffer_initial;
  uint32_t codepoint = utf8::next(buffer_ptr, data() + m_writeoffset);
  m_readoffset += buffer_ptr - buffer_initial;
  return codepoint;
}

std::string BufferBase::utf8_encode_cp(uint32_t cp) {
  Buffer buffer;
  buffer.emit_utf8_cp(cp);
  return buffer.buffer_string();
}

void BufferBase::dump(std::ostream& out, bool absolute) const {
  BufferBase::hexdump(data(), m_size, out, absolute);
}

void BufferBase::hexdump(const char* buffer, size_t size, std::ostream& out, bool absolute) {
  utils::ColorWriter writer(out);

  for (size_t offset = 0; offset < size; offset += 16) {
    out << "0x";
    out << std::hex;
    out << std::setw(8);
    out << std::setfill('0');
    out << std::right;
    if (absolute) {
      out << (uint64_t)(uintptr_t)buffer + offset;
    } else {
      out << offset;
    }
    out << std::left;
    out << std::setfill(' ');
    out << std::setw(1);
    out << std::dec;
    out << ":";

    for (size_t i = 0; i < 16 && (offset + i < size); i++) {
      if (i % 4 == 0) {
        out << " ";
      }

      out << std::hex;
      out << std::setw(2);
      out << std::setfill('0');
      out << std::right;
      out << ((uint32_t)(buffer[offset + i]) & 0x000000ff);
      out << std::left;
      out << std::setfill(' ');
      out << std::setw(1);
      out << std::dec;
      out << " ";
    }
    out << '\n';
  }
}

void BufferBase::write_to_offset(size_t target_offset, const void* source, size_t size) {
  reserve_space(target_offset + size);
  std::memmove(data() + target_offset, source, size);

  // update size if we moved past the buffer end
  if (target_offset + size > m_size) {
    m_size = target_offset + size;
  }
}

void BufferBase::update_size() {
  if (m_writeoffset > m_size) {
    m_size = m_writeoffset;
  }
}

void Buffer::reserve_space(size_t size) {
  if (m_capacity >= size) {
    return;
  }

  size_t new_capacity = m_capacity ? m_capacity * 2 : kInitialCapacity;
  while (new_capacity < size) {
    new_capacity *= 2;
  }

  if (new_capacity >= kMaximumSize) {
    FAIL("reached maximum buffer size");
  }

  void* new_buffer = std::realloc(m_buffer, new_capacity);
  if (new_buffer == nullptr) {
    FAIL("could not realloc buffer");
  }

  // initialize new memory
  std::memset((char*)new_buffer + m_capacity, 0, new_capacity - m_capacity);

  m_buffer = new_buffer;
  m_capacity = new_capacity;
}

void BufferBase::clear() {
  std::memset(m_buffer, 0, m_capacity);
}

bool ProtectedBuffer::is_readonly() const {
  return m_readonly;
}

void ProtectedBuffer::set_readonly(bool option) {
  CHECK((uintptr_t)m_buffer % kPageSize == 0);
  CHECK(m_capacity % kPageSize == 0);

  // already in requested mode
  if (m_readonly == option) {
    return;
  }

  auto flags = option ? PROT_READ : PROT_READ | PROT_WRITE;
  if (mprotect(m_buffer, m_capacity, flags) != 0) {
    FAIL("could not mprotect region");
  }

  m_readonly = option;
}

void ProtectedBuffer::reserve_space(size_t size) {
  if (size < kPageSize) {
    size = kPageSize;
  }

  if (m_capacity >= size) {
    return;
  }

  size_t new_capacity = m_capacity ? m_capacity * 2 : kPageSize;
  while (new_capacity < size) {
    new_capacity *= 2;
  }

  if (new_capacity >= kMaximumSize) {
    FAIL("reached maximum buffer size");
  }

  DCHECK(new_capacity % kPageSize == 0);
  void* new_buffer = aligned_alloc(kPageSize, new_capacity);
  if (new_buffer == nullptr) {
    FAIL("could not realloc buffer");
  }

  // initialize new memory
  std::memset(new_buffer, 0, new_capacity);

  // copy old buffer contents
  std::memcpy(new_buffer, m_buffer, m_size);

  // free old buffer
  bool was_readonly = m_readonly;
  if (m_buffer) {
    set_readonly(false); // disable memory protections for the freed memory
    std::free(m_buffer);
  }

  m_buffer = new_buffer;
  m_capacity = new_capacity;
  set_readonly(was_readonly);
}

void ProtectedBuffer::clear() {
  bool was_readonly = is_readonly();
  set_readonly(false);
  BufferBase::clear();
  set_readonly(was_readonly);
}

void GuardedBuffer::reserve_space(size_t size) {
  DCHECK(size >= kPageSize);
  DCHECK((size % kPageSize) == 0, "size needs to be aligned to page size");

  if (m_capacity >= size) {
    return;
  }

  size_t new_capacity = m_capacity ? m_capacity * 2 : kPageSize;
  while (new_capacity < size) {
    new_capacity *= 2;
  }

  if (new_capacity >= kMaximumSize) {
    FAIL("reached maximum buffer size");
  }

  size_t mapping_size = new_capacity + kPageSize * 2;

  DCHECK(new_capacity % kPageSize == 0);
  DCHECK(mapping_size % kPageSize == 0);

  void* mapping = mmap(nullptr, mapping_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  void* mapping_buffer_base = (void*)((uintptr_t)mapping + kPageSize);
  if (mapping == nullptr) {
    FAIL("could not map buffer");
  }

  // copy old buffer
  if (m_buffer) {
    DCHECK(m_mapping_base && m_mapping_size);
    DCHECK(mprotect(mapping_buffer_base, new_capacity, PROT_READ | PROT_WRITE));
    std::memcpy(mapping, m_buffer, m_size);
    munmap(m_mapping_base, m_mapping_size);
  }

  m_mapping_base = mapping;
  m_mapping_size = mapping_size;
  m_buffer = (void*)((uintptr_t)mapping + kPageSize);
  m_capacity = new_capacity;

  // re-protect newly mapped region
  set_readonly(is_readonly());
}

void GuardedBuffer::dealloc_mapping() {
  if (m_mapping_base) {
    munmap(m_mapping_base, m_mapping_size);
    m_mapping_base = nullptr;
    m_mapping_size = 0;
  }

  m_buffer = nullptr;
  m_capacity = 0;
}

}  // namespace charly::utils