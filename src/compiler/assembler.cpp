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

#include "assembler.h"

namespace Charly::Compilation {
Label Assembler::reserve_label() {
  return this->next_label_id++;
}

Label Assembler::place_label() {
  Label label = this->reserve_label();
  this->labels[label] = this->writeoffset;
  return label;
}

Label Assembler::place_label(Label label) {
  this->labels[label] = this->writeoffset;
  return label;
}

void Assembler::write_branch_to_label(Label label) {
  if (this->labels.count(label) > 0) {
    this->write_byte(Opcode::Branch);
    this->write_int(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_byte(Opcode::Branch);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_int(0);
  }
}

void Assembler::write_branchif_to_label(Label label) {
  if (this->labels.count(label) > 0) {
    this->write_byte(Opcode::BranchIf);
    this->write_int(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_byte(Opcode::BranchIf);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_int(0);
  }
}

void Assembler::write_branchunless_to_label(Label label) {
  if (this->labels.count(label) > 0) {
    this->write_byte(Opcode::BranchUnless);
    this->write_int(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_byte(Opcode::BranchUnless);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_int(0);
  }
}

void Assembler::write_registercatchtable_to_label(Label label) {
  if (this->labels.count(label) > 0) {
    this->write_byte(Opcode::RegisterCatchTable);
    this->write_int(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_byte(Opcode::RegisterCatchTable);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_int(0);
  }
}

void Assembler::write_putfunction_to_label(VALUE symbol, Label label, bool anonymous, uint32_t argc) {
  if (this->labels.count(label) > 0) {
    this->write_byte(Opcode::PutFunction);
    this->write_long(symbol);
    this->write_int(this->labels[label] - this->writeoffset + 1);
    this->write_byte(anonymous);
    this->write_int(argc);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_byte(Opcode::PutFunction);
    this->write_long(symbol);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_int(0);
    this->write_byte(anonymous);
    this->write_int(argc);
  }
}

void Assembler::resolve_unresolved_label_references() {
  for (auto uref = this->unresolved_label_references.begin(); uref != this->unresolved_label_references.end();) {
    // Check if the label exists
    if (this->labels.count(uref->id) == 0) {
      uref++;
      continue;
    }

    uint32_t label_offset = this->labels[uref->id];
    int32_t relative_offset = label_offset - uref->instruction_base;
    this->int32_at(uref->target_offset) = relative_offset;
    uref = this->unresolved_label_references.erase(uref);
  }
}
}  // namespace Charly::Compilation
