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

#include <algorithm>
#include <cmath>
#include <iomanip>

#include "disassembler.h"

namespace Charly::Compilation {

// TODO: Show literals of nan-boxed values
void Disassembler::dump(std::ostream& stream) {
  uint32_t offset = this->flags.start_offset;

  stream << "Disassembly of block at " << reinterpret_cast<void*>(this->block) << '\n';
  while (offset < this->block->get_writeoffset() && offset < this->flags.end_offset) {
    Opcode opcode = static_cast<Opcode>(this->block->read<uint8_t>(offset));

    // Print the branch arrows
    if (!this->flags.no_branches && this->highest_branch_density > 0) {
      this->draw_branchlines_for_offset(offset, stream);
    }

    // Print the offset
    if (!this->flags.no_offsets) {
      this->print_hex(reinterpret_cast<uint64_t>(this->block->get_data() + offset), stream, 12);
      stream << ": ";
    }

    // Print the mnemonic and its arguments
    stream << kOpcodeMnemonics[opcode] << " ";

#define i1 sizeof(bool)
#define i8 sizeof(uint8_t)
#define i32 sizeof(uint32_t)
#define i64 sizeof(uint64_t)

    switch (opcode) {
      case Opcode::ReadLocal:
      case Opcode::SetLocalPush:
      case Opcode::SetLocal: {
        stream << this->block->read<uint32_t>(offset + 1) << ", " << this->block->read<uint32_t>(offset + 1 + i32);
        break;
      }
      case Opcode::ReadMemberSymbol:
      case Opcode::ReadGlobal:
      case Opcode::SetMemberSymbolPush:
      case Opcode::SetMemberSymbol:
      case Opcode::SetGlobal:
      case Opcode::SetGlobalPush: {
        this->print_symbol(this->block->read<VALUE>(offset + 1), stream);
        break;
      }
      case Opcode::ReadArrayIndex:
      case Opcode::SetArrayIndexPush:
      case Opcode::SetArrayIndex: {
        stream << this->block->read<uint32_t>(offset + 1);
        break;
      }
      case Opcode::PutValue: {
        this->print_value(this->block->read<VALUE>(offset + 1), stream);
        break;
      }
      case Opcode::PutSuperMember: {
        this->print_symbol(this->block->read<VALUE>(offset + 1), stream);
        break;
      }
      case Opcode::PutString: {
        uint32_t str_offset = this->block->read<uint32_t>(offset + 1);
        uint32_t str_size = this->block->read<uint32_t>(offset + 1 + i32);
        char* blk_ptr = StringPool::get_char_ptr(str_offset);

        stream << '"';
        stream.write(blk_ptr, str_size);
        stream << '"';

        break;
      }
      case Opcode::PutFunction: {
        this->print_symbol(this->block->read<VALUE>(offset + 1), stream);
        stream << ", ";
        this->print_hex(this->block->get_data() + offset + this->block->read<int32_t>(offset + 1 + i64), stream, 12);
        stream << ", ";
        this->print(this->block->read<bool>(offset + 1 + i64 + i32), stream);
        stream << ", ";
        this->print(this->block->read<bool>(offset + 1 + i64 + i32 + i1), stream);
        stream << ", ";
        this->print(this->block->read<uint32_t>(offset + 1 + i64 + i32 + i1 + i1), stream);
        stream << ", ";
        this->print(this->block->read<uint32_t>(offset + 1 + i64 + i32 + i1 + i1 + i32), stream);
        break;
      }
      case Opcode::PutClass: {
        this->print_symbol(this->block->read<VALUE>(offset + 1), stream);
        stream << ", ";
        this->print(this->block->read<uint32_t>(offset + 1 + i64), stream);
        stream << ", ";
        this->print(this->block->read<uint32_t>(offset + 1 + i64 + i32 * 1), stream);
        stream << ", ";
        this->print(this->block->read<uint32_t>(offset + 1 + i64 + i32 * 2), stream);
        stream << ", ";
        this->print(this->block->read<uint32_t>(offset + 1 + i64 + i32 * 3), stream);
        stream << ", ";
        this->print(this->block->read<bool>(offset + 1 + i64 + i32 * 4), stream);
        stream << ", ";
        this->print(this->block->read<bool>(offset + 1 + i64 + i32 * 4 + i1), stream);
        break;
      }
      case Opcode::PutArray:
      case Opcode::PutHash:
      case Opcode::Dupn:
      case Opcode::Call:
      case Opcode::CallMember:
      case Opcode::New: {
        this->print(this->block->read<uint32_t>(offset + 1), stream);
        break;
      }
      case Opcode::RegisterCatchTable:
      case Opcode::Branch:
      case Opcode::BranchIf:
      case Opcode::BranchUnless:
      case Opcode::BranchLt:
      case Opcode::BranchGt:
      case Opcode::BranchLe:
      case Opcode::BranchGe:
      case Opcode::BranchEq:
      case Opcode::BranchNeq: {
        this->print_hex(this->block->get_data() + offset + this->block->read<int32_t>(offset + 1), stream, 12);
        break;
      }
      default: {
        // Do nothing
      }
    }

#undef i8
#undef i32
#undef i64

    stream << std::endl;
    offset += kInstructionLengths[opcode];
  }
}

void Disassembler::detect_branches() {
  uint32_t start_offset = this->flags.start_offset;
  uint32_t end_offset = this->flags.end_offset;
  uint32_t offset = start_offset;

  // Walk the block and detect all branches
  while (offset < this->block->get_writeoffset() && offset < end_offset) {
    Opcode opcode = static_cast<Opcode>(this->block->read<uint8_t>(offset));

    switch (opcode) {
      case Opcode::PutFunction: {
        if (!this->flags.no_func_branches) {
          this->branches.emplace_back(offset, offset + this->block->read<int32_t>(offset + 1 + sizeof(VALUE)));
        }

        break;
      }
      case Opcode::RegisterCatchTable:
      case Opcode::Branch:
      case Opcode::BranchIf:
      case Opcode::BranchUnless:
      case Opcode::BranchLt:
      case Opcode::BranchGt:
      case Opcode::BranchLe:
      case Opcode::BranchGe:
      case Opcode::BranchEq:
      case Opcode::BranchNeq: {
        this->branches.emplace_back(offset, offset + this->block->read<int32_t>(offset + 1));
        break;
      }
      default: {
        // Do nothing
      }
    }

    offset += kInstructionLengths[opcode];
  }

  std::sort(this->branches.begin(), this->branches.end(),
            [](Branch& l, Branch& r) { return l.lower_address() > r.lower_address(); });

  // Calculate the maximum amount of active branches at any given point
  // in the program
  //
  // Also allocates a branchline to all detected branches, used for drawing
  // them later
  uint32_t highest_active_branches = this->branches.size() > 0 ? 1 : 0;
  for (auto& br1 : this->branches) {
    uint32_t max_active = 1;
    uint32_t branchline = 0;

    // Move this branchline to the left while it overlaps with the branchline
    // of another branch
    auto overlaps_with_other_branchline = [&](uint32_t branchline) {
      return std::count_if(this->branches.begin(), this->branches.end(), [&](Branch& b) {
               if (b.has_allocated_branchline && br1.overlaps_with_branch(b)) {
                 return branchline == b.branchline;
               }

               return false;
             }) > 0;
    };

    while (overlaps_with_other_branchline(branchline)) {
      max_active++;
      branchline++;
    }

    br1.branchline = branchline;
    br1.has_allocated_branchline = true;

    // Update the highest branch count
    if (max_active > highest_active_branches) {
      highest_active_branches = max_active;
    }
  }
  this->highest_branch_density = highest_active_branches;
}

void Disassembler::draw_branchlines_for_offset(uint32_t offset, std::ostream& stream) {
  // Initialize the branchlanes row
  uint32_t branchlanewidth = this->highest_branch_density * 3;
  char branchlane[branchlanewidth];
  std::memset(branchlane, ' ', branchlanewidth);

  for (auto& branch : this->branches) {
    if (branch.in_range(offset)) {
      uint32_t start_offset = (branchlanewidth - 3) - branch.branchline * 3;

      if (branchlane[start_offset] == '-' || branchlane[start_offset] == '+') {
        branchlane[start_offset] = '+';
      } else {
        branchlane[start_offset] = '|';
      }

      // Draw a complete line to the right if this is either the start
      // or the end of a branch
      if (branch.is_start(offset) || branch.is_end(offset)) {
        uint32_t leftmost_offset = start_offset;

        // Draw a line to the right
        while (start_offset < branchlanewidth) {
          // Do not draw this line if there is either a star or an arrow already placed here
          if (branchlane[start_offset + 2] != '>' && branchlane[start_offset] != '*') {
            if (branchlane[start_offset] == '|' || branchlane[start_offset] == '+') {
              branchlane[start_offset] = '+';
            } else {
              branchlane[start_offset] = '-';
            }

            branchlane[start_offset + 1] = '-';
            branchlane[start_offset + 2] = '-';
          }
          start_offset += 3;
        }

        // Draw a little arrow if this is the end of a branch
        if (branch.is_end(offset)) {
          if (branchlane[branchlanewidth - 3] == '|' || branchlane[branchlanewidth - 3] == '+') {
            branchlane[branchlanewidth - 3] = '+';
          } else {
            if (branchlane[branchlanewidth - 3] != '*') {
              branchlane[branchlanewidth - 3] = '-';
            }
          }
          branchlane[branchlanewidth - 2] = '-';
          branchlane[branchlanewidth - 1] = '>';
        }

        branchlane[leftmost_offset] = '*';
      }
    }
  }

  stream << std::string(branchlane, branchlanewidth) << ' ';
}
}  // namespace Charly::Compilation
