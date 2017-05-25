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

#ifndef VALUE_H
#define VALUE_H

#include "constants.h"
#include "buffer.h"
#include "hash.h"

/*
 * Types which are known to the machine
 * */
enum ch_type {
  ch_type_numeric,
  ch_type_string,
  ch_type_buffer,
  ch_type_hash,
  ch_type_function,
  ch_type_pointer,
  ch_type_voidptr,
  ch_num_types
};

// Main value type
struct ch_value_t {
  ch_type type;
  unsigned int ref_count;
  union {
    double numval; // numerics
    ch_buffer buffval; // strings, buffers
    ch_hash hashval; // hash-tables
    ch_value_t* ptrval; // functions, pointers
    void* voidptr; // pointer to internal stuff
  };
};

#endif
