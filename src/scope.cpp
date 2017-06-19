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
    using namespace Primitive;

    Container::Container(uint32_t initial_capacity) {
      this->entries = std::vector<Entry>();
      this->offset_table = std::unordered_map<std::string, uint32_t>();
      this->entries.reserve(initial_capacity);
    }

    STATUS Container::read(uint32_t index, VALUE* result) {
      if (index >= this->entries.size()) return Status::ReadFailedOutOfBounds;
      *result = this->entries[index].value;
      return Status::Success;
    }

    STATUS Container::read(std::string key, VALUE* result) {
      if (!this->contains(key)) return Status::ReadFailedVariableUndefined;
      uint32_t index = this->offset_table[key];
      return this->read(index, result);
    }

    STATUS Container::register_offset(std::string key, uint32_t index) {
      if (this->contains(key)) return Status::RegisterFailedAlreadyDefined;
      this->offset_table[key] = index;
      return Status::Success;
    }

    Entry& Container::insert(VALUE value, bool constant) {
      Entry entry = Entry(value, constant);
      this->entries.push_back(entry);
      return this->entries.back();
    }

    STATUS Container::write(uint32_t index, VALUE value) {
      if (index >= this->entries.size()) return Status::WriteFailedOutOfBounds;
      Entry& entry = this->entries[index];
      if (entry.constant) return Status::WriteFailedVariableIsConstant;
      entry.value = value;
      return Status::Success;
    }

    STATUS Container::write(std::string key, VALUE value) {
      if (!this->contains(key)) {
        this->insert(value, false);
        this->register_offset(key, this->entries.size() - 1);
        return Status::Success;
      }

      return this->write(this->offset_table[key], value);
    }

    bool Container::contains(std::string key) {
      return this->offset_table.count(key) == 1;
    }

  }

}
