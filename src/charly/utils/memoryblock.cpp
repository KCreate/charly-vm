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

#include "charly/utils/memoryblock.h"
#include "charly/utils/colorwriter.h"

namespace charly::utils {

void MemoryBlock::seek(size_t offset) {

  // seek to buffer end (append mode)
  if (offset == (size_t)(-1)) {
    m_cursor = m_size;
    return;
  }

  assert(offset <= m_size);
  m_cursor = offset;
}

void MemoryBlock::write_block(const void* data, size_t length) {
  this->write_to(m_cursor, data, length);
  m_cursor += length;
}

void MemoryBlock::write_buffer(const MemoryBlock& other) {
  this->write_to(m_cursor, other.data(), other.size());
  m_cursor += other.size();
}

void MemoryBlock::write_string(const std::string& str) {
  this->write_to(m_cursor, str.c_str(), str.size());
  m_cursor += str.size();
}

void MemoryBlock::write_string(const char* str) {
  size_t length = std::strlen(str);
  this->write_to(m_cursor, str, length);
  m_cursor += length;
}

std::string MemoryBlock::buffer_string() const {
  if (m_size == 0)
    return std::string("");

  return std::string(m_data, m_size);
}

void MemoryBlock::dump(std::ostream& out) const {
  utils::ColorWriter writer(out);

  size_t offset = 0;
  for (; offset < m_size; offset += 16) {
    out << "0x";
    out << std::hex;
    out << std::setw(8);
    out << std::setfill('0');
    out << offset;
    out << std::setfill(' ');
    out << std::setw(1);
    out << std::dec;
    out << ":";

    for (size_t i = 0; i < 16 && (offset + i < m_size); i++) {
      if (i % 4 == 0) {
        out << " ";
      }

      out << std::hex;
      out << std::setw(2);
      out << std::setfill('0');
      out << ((uint32_t)(m_data[offset + i]) & 0x000000ff);
      out << std::setfill(' ');
      out << std::setw(1);
      out << std::dec;
      out << " ";
    }
    out << '\n';
  }
}

void MemoryBlock::reserve_space(size_t size) {
  if (m_capacity >= size)
    return;

  size_t new_capacity = m_capacity ? m_capacity * 2 : kInitialCapacity;
  while (new_capacity < size) {
    new_capacity *= 2;

    if (new_capacity >= kMaximumSize)
      assert(false && "reached maximum buffer size");
  }

  char* new_buffer = static_cast<char*>(std::realloc(m_data, new_capacity));
  if (new_buffer == nullptr)
    assert(false && "could not realloc buffer");

  m_data = new_buffer;
  m_capacity = new_capacity;
}

void MemoryBlock::write_to(size_t offset, const void* data, size_t length) {
  this->reserve_space(offset + length);
  std::memmove(m_data + offset, data, length);

  // update size if we moved past the buffer end
  if (offset + length > m_size) {
    m_size = offset + length;
  }
}

}  // namespace charly::utils
