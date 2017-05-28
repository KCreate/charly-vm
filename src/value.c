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

#include "value.h"

/*
 * Creates a ch_value struct
 *
 * @param ch_type type - The type of the value
 * @return ch_value - The value
 * */
ch_value ch_value_create(ch_type type) {
  return { .type = type, .ref_count = 0 };
}

/*
 * Creates a new number value
 *
 * @param double number - The number
 * @return ch_value - The new value
 * */
ch_value ch_value_create_numeric(double number) {
  ch_value value = ch_value_create(ch_type_numeric);
  value.numval = number;
  return value;
}

/*
 * Creates a new string value
 *
 * @param char* value - The string
 * @return ch_value - The new value
 * */
ch_value ch_value_create_string(char* value) {
  ch_value value = ch_value_create(ch_type_string);
  value.buffval.buffer = value;
  value.buffval.size = strlen(value);
  value.buffval.segment_size = 1;
  return value;
}

/*
 * Creates a new buffer value
 *
 * @param ch_buffer* buffer - The buffer
 * @return ch_value - The new value
 * */
ch_value ch_value_create_buffer(ch_buffer* buffer) {
  ch_value value = ch_value_create(ch_type_buffer);
  value.buffval = buffer;
  return value;
}

/*
 * Creates a new pointer value
 *
 * @param ch_value* value - The value to be pointed at
 * @return ch_value - The new value
 * */
ch_value ch_value_create_pointer(ch_value* pointer) {
  ch_value value = ch_value_create(ch_type_pointer);
  value.ptrval = pointer;
  pointer->ref_count++;
  return value;
}

/*
 * Creates a new environment value
 *
 * @param ch_environment* environment - The environment to point to
 * @return ch_value - The new value
 * */
ch_value ch_value_create_environment(ch_environment* environment) {
  ch_value value = ch_value_create(ch_type_environment);
  value.envval = environment;
  return value;
}

/*
 * Creates a new void pointer value
 *
 * @param void* value - The void pointer
 * @return ch_value - The new value
 * */
ch_value ch_value_create_voidpointer(void* pointer) {
  ch_value value = ch_value_create(ch_type_voidptr);
  value.voidptr = pointer;
  return value;
}
