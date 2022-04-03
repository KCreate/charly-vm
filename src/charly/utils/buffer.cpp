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
#include <cstdlib>
#include <iomanip>

#include "charly/charly.h"
#include "charly/utils/buffer.h"

namespace charly::utils {

void Buffer::write_buffer(const Buffer& other) {
  write(other.data(), other.size());
}

void Buffer::write_utf8_cp(uint32_t cp) {
  char tmpbuf[4];
  char* endptr = utf8::append(cp, tmpbuf);
  DCHECK(endptr != nullptr);
  size_t length = endptr - tmpbuf;
  DCHECK(length <= 4);
  this->write(tmpbuf, length);
}

int64_t Buffer::read_utf8_cp() {
  setg(eback(), gptr(), pptr());

  if (gptr() == egptr()) {
    return -1;
  }

  char* read_original = gptr();
  char* read_head = read_original;
  uint32_t codepoint;
  CHECK(utf8::next(read_head, egptr(), codepoint));
  size_t sequence_length = read_head - read_original;
  DCHECK(sequence_length <= 4);
  gbump((int)sequence_length);
  return codepoint;
}

int64_t Buffer::peek_utf8_cp(uint32_t nth) {
  setg(eback(), gptr(), pptr());

  if (gptr() == egptr()) {
    return -1;
  }

  char* read_original = gptr();
  char* read_end = egptr();
  char* read_head = read_original;

  while (nth--) {
    CHECK(utf8::next(read_head, read_end));
  }

  if (read_head == read_end) {
    return -1;
  }

  uint32_t codepoint;
  CHECK(utf8::peek_next(read_head, read_end, codepoint));
  return codepoint;
}

int32_t Buffer::peek_char(uint32_t nth) {
  setg(eback(), gptr(), pptr());

  if (gptr() == egptr()) {
    return -1;
  }

  char* read_original = gptr();
  char* read_end = egptr();
  char* read_head = read_original;

  while (nth-- && read_head < read_end) {
    read_head++;
  }

  if (read_head == read_end) {
    return -1;
  }

  return *read_head;
}

void Buffer::protect() {
  if (!m_protected) {
    reserve_space(m_capacity, true);
    Allocator::protect_read(m_buffer, m_capacity);
    m_protected = true;
  }
}

void Buffer::unprotect() {
  if (m_protected) {
    DCHECK((uintptr_t)m_buffer % kPageSize == 0);
    DCHECK(m_capacity % kPageSize == 0);
    DCHECK(m_capacity >= kPageSize);
    Allocator::protect_readwrite(m_buffer, m_capacity);
    m_protected = false;
  }
}

void Buffer::clear() {
  unprotect();
  setp(m_buffer, m_buffer + m_capacity);
  setg(m_buffer, m_buffer, pptr());
  std::iostream::clear();
  std::memset(m_buffer, 0, m_capacity);
}

void Buffer::reset_window() {
  m_window_start = gptr() - eback();
}

char* Buffer::release_buffer() {
  char* buf = m_buffer;
  DCHECK(buf != nullptr);

  unprotect();
  m_buffer = nullptr;
  m_capacity = 0;

  setp(nullptr, nullptr);
  setg(nullptr, nullptr, nullptr);

  return buf;
}

void Buffer::dump(std::ostream& out, bool absolute) const {
  Buffer::hexdump(data(), size(), out, absolute);
}

void Buffer::hexdump(const char* buffer, size_t size, std::ostream& out, bool absolute) {
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

char* Buffer::data() const {
  return pbase();
}

size_t Buffer::capacity() const {
  return m_capacity;
}

size_t Buffer::size() const {
  return pptr() - pbase();
}

size_t Buffer::window_size() const {
  char* g = gptr();
  char* window_start = eback() + m_window_start;
  DCHECK(window_start <= g);
  return g - window_start;
}

size_t Buffer::write_offset() const {
  return pptr() - pbase();
}

size_t Buffer::read_offset() const {
  return gptr() - pbase();
}

size_t Buffer::window_offset() const {
  return m_window_start;
}

bool Buffer::is_protected() const {
  return m_protected;
}

bool Buffer::is_page_aligned() const {
  return ((uintptr_t)m_buffer % kPageSize) == 0;
}

SYMBOL Buffer::hash() const {
  return crc32::hash_block(data(), size());
}

std::string Buffer::str() const {
  return { data(), size() };
}

std::string Buffer::window_str() const {
  return { data() + m_window_start, window_size() };
}

std::string_view Buffer::view() const {
  return { data(), size() };
}

std::string_view Buffer::window_view() const {
  return { data() + m_window_start, window_size() };
}

Buffer* Buffer::setbuf(std::streambuf::traits_type::char_type* data, std::streamsize size) {
  clean();
  m_buffer = data;
  m_capacity = size;
  return this;
}

std::streambuf::traits_type::int_type Buffer::underflow() {
  if (gptr() < egptr()) {
    return std::streambuf::traits_type::to_int_type(*gptr());
  }

  if (egptr() < pptr()) {
    setg(pbase(), pbase() + (gptr() - eback()), pptr());
    return underflow();
  }

  return std::streambuf::traits_type::eof();
}

std::streambuf::traits_type::int_type Buffer::pbackfail(std::streambuf::traits_type::int_type c) {
  if (std::streambuf::traits_type::eq_int_type(c, std::streambuf::traits_type::eof())) {
    if (eback() < gptr()) {
      gbump(-1);
      return std::streambuf::traits_type::not_eof(c);
    } else {
      return c;
    }
  } else {
    if (eback() >= gptr()) {
      return std::streambuf::traits_type::eof();
    }
    gbump(-1);

    if (!std::streambuf::traits_type::eq_int_type(c, *gptr())) {
      *gptr() = std::streambuf::traits_type::to_char_type(c);
    }

    return c;
  }
}

std::streambuf::traits_type::int_type Buffer::overflow(std::streambuf::traits_type::int_type c) {
  if (std::streambuf::traits_type::eq_int_type(c, std::streambuf::traits_type::eof())) {
    return std::streambuf::traits_type::not_eof(c);
  }

  // write position available
  if (pptr() < epptr()) {
    sputc(std::streambuf::traits_type::to_char_type(c));
    return c;
  }

  // grow buffer
  reserve_space(m_capacity * 2);
  sputc(std::streambuf::traits_type::to_char_type(c));
  return c;
}

std::streambuf::traits_type::pos_type Buffer::seekpos(std::streambuf::traits_type::pos_type pos,
                                                      std::ios_base::openmode openmode) {
  return seekoff(pos, std::ios_base::beg, openmode);
}

char* calculate_target_offset(
  std::streambuf::traits_type::off_type off, std::ios_base::seekdir seekdir, char* base, char* cur, char* max) {
  switch (seekdir) {
    case std::ios_base::beg: {
      return base + off;
    }
    case std::ios_base::end: {
      return max + off;
    }
    case std::ios_base::cur: {
      return cur + off;
    }
    default: {
      FAIL("unexpected seekdir");
    }
  }
}

std::streambuf::traits_type::pos_type Buffer::seekoff(std::streambuf::traits_type::off_type off,
                                                      std::ios_base::seekdir seekdir,
                                                      std::ios_base::openmode openmode) {
  if (openmode & std::ios_base::in) {
    auto base = eback();
    auto cur = gptr();
    auto max = pptr();
    auto req = calculate_target_offset(off, seekdir, base, cur, max);
    if (req < base || req > max) {
      return { std::streambuf::traits_type::off_type(-1) };
    }
    setg(base, req, max);
    return std::streambuf::traits_type::pos_type{ req - base };
  }

  if (openmode & std::ios_base::out) {
    auto base = pbase();
    auto cur = pptr();
    auto max = epptr();
    auto req = calculate_target_offset(off, seekdir, base, cur, max);
    if (req < base || req > max) {
      return { std::streambuf::traits_type::off_type(-1) };
    }
    setp(base, max);

    size_t ppos_remaining = req - base;
    while (ppos_remaining >= kInt32Max) {
      pbump(kInt32Max);
      ppos_remaining -= kInt32Max;
    }
    pbump((int)ppos_remaining);

    return std::streambuf::traits_type::pos_type{ req - base };
  }

  FAIL("unexpected openmode");
}

void Buffer::reserve_space(size_t size, bool page_aligned) {
  bool was_protected = is_protected();

  unprotect();

  auto p = tellp();
  auto g = tellg();

  DCHECK(m_buffer != nullptr);
  DCHECK(m_capacity != 0);
  DCHECK(p != -1);
  DCHECK(g != -1);

  // determine new capacity
  size_t new_capacity = kDefaultCapacity;
  while (new_capacity < size) {
    new_capacity *= 2;
  }

  // determine if the new buffer needs to be aligned to the page size
  bool page_align_new_buffer = page_aligned;
  if (is_page_aligned() || was_protected) {
    page_align_new_buffer = true;
  }

  // minimum one page buffer when aligning to page size
  // we do not want mprotect to protect unrelated data later on
  if (page_align_new_buffer && new_capacity < kPageSize) {
    new_capacity = kPageSize;
  }

  size_t new_alignment = page_align_new_buffer ? kPageSize : 8;
  char* new_buffer = (char*)Allocator::realloc(m_buffer, m_capacity, new_capacity, new_alignment);
  CHECK(new_buffer != nullptr, "could not realloc buffer");

  m_buffer = new_buffer;
  m_capacity = new_capacity;

  setp(m_buffer, m_buffer + m_capacity);
  seekp(p);
  setg(m_buffer, m_buffer, pptr());
  seekg(g);

  CHECK(good());

  if (was_protected) {
    protect();
  }
}

void Buffer::clean() {
  if (m_buffer) {
    unprotect();
    Allocator::free(m_buffer);
    m_buffer = nullptr;
  }

  setp(nullptr, nullptr);
  setg(nullptr, nullptr, nullptr);
}

}  // namespace charly::utils
