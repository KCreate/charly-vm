/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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

#include "utf8buffer.h"
#include <string>

namespace Charly {

// Append a utf8 codepoint
uint32_t UTF8Buffer::append_utf8(uint32_t cp) {
  this->grow_to_fit(this->writeoffset + 4);

  uint8_t* initial_write_pointer = this->data + this->writeoffset;
  uint8_t* write_pointer = this->data + this->writeoffset;
  write_pointer = utf8::append(cp, write_pointer);
  this->writeoffset += write_pointer - initial_write_pointer;

  return cp;
}

// Return the next utf8 codepoint
uint32_t UTF8Buffer::next_utf8() {
  if (this->readoffset >= this->writeoffset) {
    return L'\0';
  }

  uint8_t* initial_readpointer = this->data + this->readoffset;
  uint8_t* readpointer = this->data + this->readoffset;
  uint32_t cp = utf8::next(readpointer, this->data + this->writeoffset);
  this->readoffset += readpointer - initial_readpointer;
  return cp;
}

// Peek the next utf8 codepoint
uint32_t UTF8Buffer::peek_next_utf8() {
  if (this->readoffset >= this->writeoffset) {
    return L'\0';
  }

  uint8_t* initial_readpointer = this->data + this->readoffset;
  uint8_t* readpointer = this->data + this->readoffset;
  uint32_t cp = utf8::peek_next(readpointer, this->data + this->writeoffset);
  this->readoffset += readpointer - initial_readpointer;
  return cp;
}

// Return the amount of codepoints inside the buffer
size_t UTF8Buffer::codepointcount() {
  uint8_t* data_end = this->data + this->writeoffset;
  return utf8::distance(this->data, data_end);
}

size_t UTF8Buffer::utf8_byteoffset(uint32_t start) {
  uint8_t* start_iterator = this->data;

  while (start--) {
    if (start_iterator >= this->data + this->writeoffset) {
      return this->writeoffset;
    }

    utf8::advance(start_iterator, 1, this->data + this->writeoffset);
  }

  return start_iterator - this->data;
}
}  // namespace Charly
