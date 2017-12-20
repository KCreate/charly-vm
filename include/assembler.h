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

#include <functional>
#include <list>
#include <unordered_map>

#include "block.h"
#include "opcode.h"

#pragma once

namespace Charly::Compilation {

// A label for a given position in the generated instruction stream of the assembler
typedef uint32_t Label;

// The offset field of an instruction that needs to be resolved
struct UnresolvedReference {
  Label id;
  uint32_t target_offset;
  uint32_t instruction_base;
};

// Holds offset and length of a string in an instructionblock's static data section
struct StringOffsetInfo {
  uint32_t offset;
  uint32_t length;
};

// Handles label resolution and compile-time offset calculations
class Assembler : public InstructionBlock {
public:
  inline void reset() {
    this->labels.clear();
    this->unresolved_label_references.clear();
    this->next_label_id = 0;
  }

  // Label handling
  Label reserve_label();
  Label place_label();
  Label place_label(Label label);

  // Wrappers for instructions which branch by some byte offset
  void write_branch_to_label(Label label);
  void write_branchif_to_label(Label label);
  void write_branchunless_to_label(Label label);
  void write_registercatchtable_to_label(Label label);
  void write_putfunction_to_label(VALUE symbol, Label label, bool anonymous, uint32_t argc, uint32_t lvarcount);

  // Known string handling
  StringOffsetInfo write_putstring(const std::string& str);

  // Unresolved reference handling
  void resolve_unresolved_label_references();
  inline bool has_unresolved_label_references() {
    return this->unresolved_label_references.size() != 0;
  }

private:
  // Static data handling
  inline size_t hash_string(const std::string& str) {
    return std::hash<std::string>{}(str);
  }
  inline bool string_is_duplicate(size_t strhash) {
    return this->known_strings.count(strhash) > 0;
  }
  inline StringOffsetInfo get_staticdata_info(size_t strhash) {
    return this->known_strings[strhash];
  }

  // Mapping from label-id to an offset
  std::unordered_map<Label, uint32_t> labels;
  std::list<UnresolvedReference> unresolved_label_references;
  std::unordered_map<size_t, StringOffsetInfo> known_strings;
  uint32_t next_label_id = 0;
};
}  // namespace Charly::Compilation
