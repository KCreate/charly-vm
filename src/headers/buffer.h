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

#include <cstring>
#include <utf8/utf8.h>

#pragma once

namespace Charly {
  const size_t kInitialBufferSize = 128;
  const size_t kBufferGrowthFactor = 2;

  // Handles UTF8 encoded text
  class Buffer {
    public:
      char* buffer;
      char* read_pointer;
      char* write_pointer;
      size_t used_bytesize;
      size_t bytesize;

    public:
      Buffer() {
        this->buffer = (char *)malloc(sizeof(char) * kInitialBufferSize);
        this->read_pointer = this->buffer;
        this->write_pointer = this->buffer;
        this->bytesize = kInitialBufferSize;
        this->used_bytesize = 0;
      };

      ~Buffer() {
        if (this->buffer != nullptr) free(this->buffer);
      }

      Buffer(const Buffer& other) {
        if (this->buffer != nullptr) free(this->buffer);
        this->buffer = (char *)malloc(sizeof(char) * other.bytesize);
        this->read_pointer = this->buffer + (other.read_pointer - other.buffer);
        this->write_pointer = this->buffer + (other.write_pointer - other.buffer);
        this->bytesize = other.bytesize;
        this->used_bytesize = other.used_bytesize;
        memcpy(this->buffer, other.buffer, other.used_bytesize);
      }

      Buffer& operator=(const Buffer& other) {
        if (this != &other) {
          free(this->buffer);
          this->buffer = (char *)malloc(sizeof(char) * other.bytesize);
          this->read_pointer = this->buffer + (other.read_pointer - other.buffer);
          this->write_pointer = this->buffer + (other.write_pointer - other.buffer);
          this->bytesize = other.bytesize;
          this->used_bytesize = other.used_bytesize;
          memcpy(this->buffer, other.buffer, other.bytesize);
        }

        return *this;
      }

      // Load raw data into the buffer
      Buffer& read(char* data, size_t length);
      Buffer& read(std::string& data);
      Buffer& read(const Buffer& data);

      // UTF8 methods
      uint32_t append_utf8(uint32_t cp);
      uint32_t next_utf8();
      uint32_t peek_next_utf8();
      uint32_t prior_utf8();
      uint32_t advance_utf8(uint32_t amount);
      bool is_valid_utf8(char* start, char* end);

      // Size of the used buffer in utf8 codepoints
      size_t charcount();

      // UTF8 Range methods
      size_t utf8_byteoffset(uint32_t start);

    private:

      // Memory handling
      void check_enough_size(size_t size);
      void grow_buffer_size(size_t minimum_size);
  };
}







