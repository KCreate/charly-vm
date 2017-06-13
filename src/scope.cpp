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

#include "scope.h"

namespace Charly {

  namespace Scope {

    Container::Container(uint32_t initial_capacity, Container* parent) {
      this->parent = parent;
      this->entries = std::vector<Entry>();
      this->offset_table = std::unordered_map<std::string, uint32_t>();
      this->ref_count = 0;

      // Reserve initial_capacity elements in the entries vector
      this->entries.reserve(initial_capacity);

      // Update the ref_count of the parent container
      if (parent) parent->ref_count++;
    }

    Entry Container::insert(VALUE value, bool is_constant) {
      Entry entry = Entry(value, is_constant);
      this->entries.push_back(entry);
      return entry;
    }

    bool Container::contains(std::string key) {
      return this->offset_table.count(key) == 1;
    }

    bool Container::defined(std::string key) {
      if (this->contains(key)) return true;
      if (this->parent == NULL) return false;
      return this->parent->defined(key);
    }

  }

}
