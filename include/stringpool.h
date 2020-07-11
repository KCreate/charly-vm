/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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
#include <mutex>
#include <shared_mutex>

#include "memoryblock.h"
#include "value.h"

#pragma once

namespace Charly {

// Holds offset and length of a string in the string pool
struct StringOffsetInfo {
  uint32_t offset;
  uint32_t length;
};

// Manages global string table
class StringPool {
  inline static MemoryBlock buffer;
  inline static std::unordered_map<size_t, StringOffsetInfo> offset_map;
  inline static std::shared_mutex buffer_mutex;

public:

  /*
   * Returns a char* to the referenced string
   * */
  inline static char* get_char_ptr(uint32_t offset) {
    return reinterpret_cast<char*>(StringPool::buffer.get_data() + offset);
  }

  /*
   * Returns the offset info for a given hash
   * */
  inline static StringOffsetInfo lookup_hash(size_t str_hash) {
    std::shared_lock<std::shared_mutex> lk(StringPool::buffer_mutex);
    return StringPool::offset_map[str_hash];
  }

  /*
   * Store a string in the buffer and return its offset info
   * Will return already existing reference if string has already
   * been encoded
   * */
  inline static StringOffsetInfo encode_string(const std::string& str) {
    size_t str_hash = std::hash<std::string>{}(str);

    // Return already existing mapping
    {
      std::shared_lock<std::shared_mutex> lk(StringPool::buffer_mutex);
      if (StringPool::offset_map.count(str_hash)) {
        return StringPool::lookup_hash(str_hash);
      }
    }

    // Store the symbol in the global buffer
    {
      std::unique_lock<std::shared_mutex> lk(StringPool::buffer_mutex);
      uint32_t start_offset = StringPool::buffer.writeoffset;
      StringPool::buffer.write_string(str);
      StringOffsetInfo info({ start_offset, static_cast<uint32_t>(str.size()) });
      StringPool::offset_map.emplace(str_hash, info);
      return info;
    }
  }
};
}  // namespace Charly
