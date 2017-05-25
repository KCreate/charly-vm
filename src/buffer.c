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

/*
 * Allocates a new ch_buffer struct on the heap
 * The ch_buffers internal buffer won't be initialized
 * Returns the allocated buffer or NULL on failure
 *
 * @param size_t element_size - The size of a single element in the buffer
 * @param size_t element_count - The amount of elements the buffer contains
 * @return ch_buffer* - The allocated buffer, can be null
 * */
ch_buffer* ch_buffer_create(size_t element_size, size_t element_count) {

  // Check arguments
  if (element_size == 0 || element_count == 0) return NULL;

  // Allocate memory for the ch_buffer structure
  ch_buffer* struct_buffer = malloc(sizeof(struct_buffer));
  if (!struct_buffer) return NULL;

  // Allocate memory for the ch_buffers internal buffer
  void* internal_buffer = malloc(element_size * element_count);
  if (!internal_buffer) return NULL;

  // Initialize the ch_buffer struct
  struct_buffer->buffer = internal_buffer;
  struct_buffer->size = element_size * element_count;
  struct_buffer->segment_size = element_size;

  return struct_buffer;
}

/*
 * Create a copy of a buffer and all of it's contents
 * Returns NULL if allocation or copying fails
 *
 * @param ch_buffer* old_buffer - The buffer to be copied
 * @return ch_buffer* - The newly copied buffer
 * */
ch_buffer* ch_buffer_copy(ch_buffer* old_buffer) {

  // Allocate memory for the new buffer
  ch_buffer* new_buffer = ch_buffer_create(old_buffer->segment_size, ch_buffer_segment_count(old_buffer));
  if (!new_buffer) return NULL;

  // Copy the old buffers memory to the new buffer
  memcpy(new_buffer->buffer, old_buffer->buffer, old_buffer->size);

  return new_buffer;
}

/*
 * Free a ch_buffer struct
 *
 * @param ch_buffer* buffer - The buffer to be freed
 * */
void ch_buffer_free(ch_buffer* buffer) {
  free(buffer->buffer);
  free(buffer);
}

/*
 * Set all bytes of a buffer to a given value
 *
 * @param ch_buffer* buffer - Buffer
 * @param char - Value to be set
 * */
void ch_buffer_clear(ch_buffer* buffer, char value) {
  memset(buffer->buffer, value, buffer->size);
}

/*
 * Resize the internal buffer of a ch_buffer struct to size
 *
 * @param ch_buffer* buffer - The buffer
 * @param size_t element_count - The amount of element that should fit into the new buffer
 * @return bool - Wether the buffer was resized or not
 * */
bool ch_buffer_resize(ch_buffer* buffer, size_t element_count) {

  // Check the size
  if (element_count == 0) return false;

  // Check if a resize needs to happen
  if (ch_buffer_segment_count(buffer) == element_count) {

    // Even though we don't actually resize anything we will
    // return true here, as from the outside it looks like we did
    return true;
  }

  // Reallocate the buffer
  void* new_buffer = realloc(buffer->buffer, buffer->segment_size * element_count);
  if (!new_buffer) return false;
  buffer->buffer = new_buffer;
  buffer->size = buffer->segment_size * element_count;

  return true;
}

/*
 * Tries to double the size of a buffer
 *
 * @param ch_buffer* buffer - The buffer
 * @return bool - Wether the buffer was resized or not
 * */
bool ch_buffer_double(ch_buffer* buffer) {
  return ch_buffer_resize(buffer, ch_buffer_segment_count(buffer) * 2);
}

/*
 * Reverse the bytes of a buffer
 *
 * @param ch_buffer* buffer - The buffer
 * */
void ch_buffer_reverse_bytes(ch_buffer* buffer) {
  u32 right = buffer->size - 1;
  u32 left = 0;

  while (left < right) {
    u8 byte = buffer->buffer[right];
    buffer->buffer[right] = buffer->buffer[left];
    buffer->buffer[left] = byte;
    ++left;
    --right;
  }
}

/*
 * Reverse the segments of a buffer
 *
 * @param ch_buffer* buffer - The buffer
 * */
void ch_buffer_reverse_segments(ch_buffer* buffer) {
  u32 right = buffer->size - buffer->segment_size;
  u32 left = 0;

  while (left < right) {
    u32 bleft = left;
    u32 bright = right;

    while (bleft < bright) {
      u8 byte = buffer->buffer[bright];
      buffer->buffer[bright] = buffer->buffer[bleft];
      buffer->buffer[bleft] = byte;
      ++bleft;
      ++bright;
    }

    left += buffer->segment_size;
    right -= buffer->segment_size;
  }
}

/*
 * Check wether a given index would be out of bounds
 *
 * @param ch_buffer* buffer - The buffer
 * @param size_t index - The index to check
 * @return bool - Wether the index is out of bounds
 * */
bool ch_buffer_index_out_of_bounds(ch_buffer* buffer, size_t index) {
  return index * buffer->segment_size >= buffer->size;
}

/*
 * Try to change the segment size of a buffer
 *
 * @param ch_buffer* buffer - The buffer
 * @param size_t size - The new segment size
 * @return bool - Wether the segment size could be changed
 * */
bool ch_buffer_change_segment_size(ch_buffer* buffer, size_t size) {
  if (size == 0) return false;
  if (buffer->size % size != 0) return false;
  buffer->segment_size = size;
  return true;
}

/*
 * Return the size of a single segment in the buffer
 *
 * @param ch_buffer* buffer
 * @return size_t - The size of a single segment
 * */
size_t ch_buffer_segment_count(ch_buffer* buffer) {
  return buffer->size / buffer->segment_size;
}

/*
 * Returns a pointer to the item at index
 * Performs no out of bounds checking at all
 *
 * @param ch_buffer* buffer - The buffer
 * @param size_t index - The index
 * @return void* - Pointer to the element at index
 * */
void* ch_buffer_index_ptr(ch_buffer* buffer, size_t index) {
  return buffer->buffer + (index * buffer->segment_size);
}

/*
 * Return a pointer to the last segment in the buffer
 *
 * @param ch_buffer* buffer - The buffer
 * @return void* - Pointer to the last segment
 * */
void* ch_buffer_index_last(ch_buffer* buffer) {
  return buffer->buffer - buffer->segment_size;
}
