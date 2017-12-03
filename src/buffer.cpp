/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include "buffer.h"
#include <string>

namespace Charly {

// Append a memory buffer
Buffer& Buffer::read(char* data, size_t length) {
  this->check_enough_size(length);

  memcpy(this->write_pointer, static_cast<char*>(data), length);
  this->write_pointer += length;
  this->used_bytesize += length;

  return *this;
}

// Append a string
Buffer& Buffer::read(std::string& data) {
  this->check_enough_size(data.size());

  memcpy(this->write_pointer, data.c_str(), data.size());
  this->write_pointer += data.size();
  this->used_bytesize += data.size();

  return *this;
}

// Append the contents of another buffer
Buffer& Buffer::read(const Buffer& data) {
  this->check_enough_size(data.used_bytesize);

  memcpy(this->write_pointer, data.buffer, data.used_bytesize);
  this->write_pointer += data.used_bytesize;
  this->used_bytesize += data.used_bytesize;

  return *this;
}

// Append a utf8 codepoint
uint32_t Buffer::append_utf8(uint32_t cp) {
  this->check_enough_size(4);

  char* initial_write_pointer = this->write_pointer;
  utf8::append(cp, this->write_pointer);
  uint32_t inserted_cp = utf8::prior(this->write_pointer, initial_write_pointer);
  utf8::advance(this->write_pointer, 1, this->buffer + this->used_bytesize);

  this->used_bytesize += this->write_pointer - initial_write_pointer;

  return inserted_cp;
}

// Return the next utf8 codepoint
uint32_t Buffer::next_utf8() {
  if (this->read_pointer >= this->write_pointer || this->read_pointer == nullptr) {
    return EOF;
  }

  return utf8::next(this->read_pointer, this->buffer + this->used_bytesize);
}

// Peek the next utf8 codepoint
uint32_t Buffer::peek_next_utf8() {
  if (this->read_pointer >= this->write_pointer || this->read_pointer == nullptr) {
    return EOF;
  }

  return utf8::peek_next(this->read_pointer, this->buffer + this->used_bytesize);
}

// Return the previous utf8 codepoint
uint32_t Buffer::prior_utf8() {
  if (this->read_pointer == nullptr || this->read_pointer < this->buffer) {
    return EOF;
  }

  return utf8::prior(this->read_pointer, this->read_pointer);
}

// Advance the read_pointer by amount utf8 codepoints
uint32_t Buffer::advance_utf8(uint32_t amount) {
  // Check if the read pointer is already at the end of the used
  // buffer
  uint32_t read_pointer_offset = this->read_pointer - this->buffer;
  if (read_pointer_offset >= this->used_bytesize) {
    return EOF;
  }

  if (this->read_pointer == nullptr) {
    return EOF;
  }

  utf8::advance(this->read_pointer, amount, this->buffer + this->used_bytesize);

  return this->peek_next_utf8();
}

// Check if a string inside the start-end range is valid utf8
bool Buffer::is_valid_utf8(char* start, char* end) {
  return utf8::is_valid(start, end);
}

// Return the amount of codepoints inside the buffer
size_t Buffer::charcount() {
  char* used_buffer_end = this->buffer + this->used_bytesize;
  return utf8::distance(this->buffer, used_buffer_end);
}

size_t Buffer::utf8_byteoffset(uint32_t start) {
  char* start_iterator = this->buffer;

  while (start--) {
    if (start_iterator >= this->buffer + this->used_bytesize) {
      return start_iterator - this->buffer;
    }

    utf8::advance(start_iterator, 1, this->buffer + this->used_bytesize);
  }

  return start_iterator - this->buffer;
}

// Check if there is enough size and if not allocate more memory
void Buffer::check_enough_size(size_t size) {
  if (this->used_bytesize + size >= this->bytesize) {
    this->grow_buffer_size(this->used_bytesize + size);
  }
}

// Allocate more memory
void Buffer::grow_buffer_size(size_t minimum_size) {
  // Allocate the new buffer
  size_t old_size = this->bytesize;
  size_t new_size = old_size * kBufferGrowthFactor;

  // Double the buffers size until we have at least *minimum_bytes* reserved
  while (new_size < minimum_size) {
    new_size *= kBufferGrowthFactor;
  }

  char* new_buffer = static_cast<char*>(malloc(sizeof(char) * new_size));
  memcpy(new_buffer, this->buffer, this->used_bytesize);

  // Update offset pointers
  this->read_pointer = new_buffer + (this->read_pointer - this->buffer);
  this->write_pointer = new_buffer + (this->write_pointer - this->buffer);
  this->bytesize = new_size;

  // Deallocate the old buffer
  free(this->buffer);
  this->buffer = new_buffer;
}

}  // namespace Charly
