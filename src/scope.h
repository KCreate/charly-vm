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

#include <vector>
#include <unordered_map>

#include "defines.h"

#pragma once

namespace Charly {

  namespace Scope {

    /* Single entry of a container */
    class Entry {
      public:
        Entry(VALUE arg, bool is_constant) : value(arg), constant(is_constant) {};
        VALUE value;
        bool constant;
    };

    /*
    * Main hash-like data structure supporting fast access to known indices
    * and slightly-slower access when using hash-values
    * */
    class Container {
      public:

        /* Vector of entries in this scope */
        std::vector<Entry> entries;

        /* Map from values to offsets into the entries vector */
        std::unordered_map<VALUE, uint32_t> offset_table;

      public:
        Container(uint32_t initial_capacity = 4);

        /* Tries to read an entry from this container or a parent container */
        STATUS read(uint32_t index, VALUE* result);
        STATUS read(VALUE key, VALUE* result);

        /* Creates new entries to the offset table */
        STATUS register_offset(VALUE key, uint32_t index);

        /* Insert a new entry into this container */
        Entry& insert(VALUE value, bool is_constant = false);

        /* Writes to an already existing entry */
        STATUS write(uint32_t index, VALUE value);
        STATUS write(VALUE key, VALUE value, bool init_on_undefined = false);

        bool contains(VALUE key);
    };

  }

}
