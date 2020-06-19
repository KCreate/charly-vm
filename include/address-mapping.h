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

#include <tuple>
#include <vector>
#include <optional>

#include "defines.h"

#pragma once

namespace Charly {

// Region of bytecode mapped to a source line
//
//                 +- Start offset
//                 |         +- End offset
//                 |         |         +- Line number
//                 v         v         v
typedef std::tuple<uint32_t, uint32_t, uint32_t> MappedSourceLine;

/*
 * Stores source mapping information for a specific file
 * */
struct SourceMap {
  std::string filename;
  uint8_t* begin_address = nullptr;
  uint8_t* end_address = nullptr;
  std::vector<MappedSourceLine> entries;

  // Fields used to create the entries list
  uint32_t last_line_number = 0;
  uint32_t last_bytecode_offset = 0;

  SourceMap(const std::string& f) : filename(f) {}

  SourceMap(SourceMap&& other) : filename(std::move(other.filename)), entries(std::move(other.entries)) {
  }

  // Start a new line number
  inline void begin_line_number(uint32_t line_number, uint32_t bytecode_offset) {
    this->entries.push_back({this->last_bytecode_offset, bytecode_offset, this->last_line_number});
    this->last_line_number = line_number;
    this->last_bytecode_offset = bytecode_offset;
  }

  // Creates the last mapped entry
  // Calling begin_line_number after this method has been called will result
  // in corrupted data
  inline void finish_file(uint32_t bytecode_offset) {
    this->entries.push_back({this->last_bytecode_offset, bytecode_offset, this->last_line_number});
    this->last_line_number = 0;
    this->last_bytecode_offset = 0;
  }

  // Returns the source line this offset belongs to
  inline uint32_t resolve_offset(uint32_t offset) {
    for (const MappedSourceLine& entry : this->entries) {
      uint32_t start_offset, end_offset, line_number;
      std::tie(start_offset, end_offset, line_number) = entry;

      if (start_offset <= offset && end_offset >= offset) {
        return line_number;
      }
    }

    return 0;
  }
};

// Manages sourcemaps
class AddressMapping {
  std::vector<SourceMap> mapped_files;

public:

  inline void begin_new_file(const std::string& filename) {
    this->mapped_files.emplace_back(filename);
  }

  inline SourceMap& active_file() {
    return this->mapped_files.back();
  }

  inline std::optional<std::string> filename_for_address(uint8_t* address) {
    for (SourceMap& map : this->mapped_files) {
      if (map.begin_address <= address && map.end_address >= address) {
        return map.filename;
      }
    }

    return std::nullopt;
  }

  inline std::optional<uint32_t> linenumber_for_address(uint8_t* address) {
    for (SourceMap& map : this->mapped_files) {
      if (map.begin_address <= address && map.end_address >= address) {
        return map.resolve_offset(map.begin_address - address);
      }
    }

    return std::nullopt;
  }

};
}  // namespace Charly
