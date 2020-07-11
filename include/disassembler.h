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

#include <iomanip>
#include <iostream>
#include <type_traits>
#include <vector>
#include <string_view>

#include "compiler.h"
#include "instructionblock.h"

#pragma once

namespace Charly::Compilation {
class Disassembler {
public:
  // Represents a branch from one point in the program to another
  struct Branch {
    uint32_t start_offset;
    uint32_t end_offset;

    bool has_allocated_branchline = false;
    uint64_t branchline = 0;

    Branch(uint32_t s, uint32_t e) : start_offset(s), end_offset(e) {
    }

    inline bool is_backwards_jump() {
      return end_offset <= start_offset;
    }

    inline bool in_range(uint32_t addr) {
      if (this->is_backwards_jump()) {
        return addr >= end_offset && addr <= start_offset;
      } else {
        return addr >= start_offset && addr <= end_offset;
      }
    }

    inline bool overlaps_with_branch(const Branch& other) {
      return this->in_range(other.start_offset) || this->in_range(other.end_offset);
    }

    inline bool is_start(uint32_t addr) {
      return addr == this->start_offset;
    }

    inline bool is_end(uint32_t addr) {
      return addr == this->end_offset;
    }

    inline uint32_t lower_address() {
      return this->is_backwards_jump() ? this->end_offset : this->start_offset;
    }

    inline uint32_t upper_address() {
      return this->is_backwards_jump() ? this->start_offset : this->end_offset;
    }
  };

  // Some flags for the disassembler
  struct Flags {
    bool no_branches = false;
    bool no_offsets = false;
    bool no_func_branches = false;
    uint32_t start_offset = 0;
    uint32_t end_offset = UINT32_MAX;
  };

public:
  Disassembler(InstructionBlock* b, Flags f) : block(b), flags(f) {
    if (!f.no_branches) {
      this->detect_branches();
    }
  }

  ~Disassembler() {
  }

  void dump(std::ostream& stream);

private:
  template <typename V>
  inline void print_hex(V value, std::ostream& stream, uint32_t width = 1) {
    stream.fill('0');

    if (std::is_pointer<V>::value) {
      stream << "0x" << std::hex << std::setw(width);
      stream << reinterpret_cast<void*>(value) << std::setw(1) << std::dec;
    } else {
      stream << "0x" << std::hex << std::setw(width) << value << std::setw(1) << std::dec;
    }
  }

  inline void print_symbol(VALUE value, std::ostream& stream) {
    std::optional<std::string> decoded_str = SymbolTable::decode(value);

    if (decoded_str.has_value()) {
      stream << "@\"" << decoded_str.value() << "\"";
    } else {
      this->print_hex(value, stream);
    }
  }


  inline void print_value(VALUE value, std::ostream& stream) {
    switch (charly_get_type(value)) {
      case kTypeNumber: {
        if (charly_is_float(value)) stream << charly_number_to_double(value);
        if (charly_is_int(value)) stream << charly_number_to_int64(value);
        return;
      }
      case kTypeString: {
        std::string_view data(charly_string_data(value), charly_string_length(value));
        stream << "\"" << data << "\"";
        return;
      }
      case kTypeBoolean: {
        if (value == kTrue) stream << "true";
        if (value == kFalse) stream << "false";
        return;
      }
      case kTypeNull: {
        stream << "null";
        return;
      }
      default: {
        this->print_hex(value, stream);
      }
    }
  }

  template <typename V>
  inline void print(V value, std::ostream& stream) {
    stream << value;
  }

  void detect_branches();
  void draw_branchlines_for_offset(uint32_t offset, std::ostream& stream);

private:
  InstructionBlock* block;
  std::vector<Branch> branches;
  uint32_t highest_branch_density = 0;
  Flags flags;
};

template <>
inline void Disassembler::print(bool value, std::ostream& stream) {
  stream << std::boolalpha << value << std::noboolalpha;
}
}  // namespace Charly::Compilation
