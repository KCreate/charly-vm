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

#ifndef BUFFER_H
#define BUFFER_H

#include "constants.h"

/*
 * Multi-purpose fixed-size buffer type
 * */
struct ch_buffer {
  char* buffer;
  size_t size; // size of the buffer field, has to be a multiple of segment_size
  size_t segment_size; // size of a single segment
};

ch_buffer* ch_buffer_create(size_t element_size, size_t element_count);
ch_buffer* ch_buffer_copy(ch_buffer* old_buffer);
ch_buffer* ch_buffer_init(ch_buffer* buffer, size_t element_size, size_t element_count);
void ch_buffer_free(ch_buffer* buffer);
void ch_buffer_clear(ch_buffer* buffer, char value);
bool ch_buffer_resize(ch_buffer* buffer, size_t element_count);
bool ch_buffer_double(ch_buffer* buffer);
void ch_buffer_reverse_bytes(ch_buffer* buffer);
void ch_buffer_reverse_segments(ch_buffer* buffer);
bool ch_buffer_index_out_of_bounds(ch_buffer* buffer, size_t index);
bool ch_buffer_change_segment_size(ch_buffer* buffer, size_t size);
size_t ch_buffer_segment_count(ch_buffer* buffer);
void* ch_buffer_index_ptr(ch_buffer* buffer, size_t index);
void* ch_buffer_index_last(ch_buffer* buffer);

#endif
