/*
 * This file is part of the Charly Virtual Machine (https://code.leonardschuetz.ch/r/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2018 Leonard Schütz
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

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

#include "memoryblock.h"

#pragma once

namespace Charly {

// Holds offset and length of a string in the string pool
struct StringOffsetInfo {
  uint32_t offset;
  uint32_t length;
};

class StringPool : public MemoryBlock {
  std::unordered_map<size_t, StringOffsetInfo> known_strings;

  inline size_t hash_string(const std::string& str) {
    return std::hash<std::string>{}(str);
  }

public:
  inline bool is_known(size_t strhash) {
    return this->known_strings.count(strhash) > 0;
  }
  inline StringOffsetInfo get_offsetinfo(size_t strhash) {
    return this->known_strings[strhash];
  }
  inline StringOffsetInfo get_offsetinfo(const std::string& str) {
    size_t str_hash = this->hash_string(str);
    if (this->is_known(str_hash)) {
      return this->get_offsetinfo(str_hash);
    } else {
      uint32_t start_offset = this->writeoffset;
      this->write_string(str);
      StringOffsetInfo info = {start_offset, static_cast<uint32_t>(str.size())};
      this->known_strings.emplace(str_hash, info);
      return info;
    }
  }
};
}  // namespace Charly
