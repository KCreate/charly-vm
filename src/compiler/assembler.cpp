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
    this->write_u8(Opcode::Branch);
    this->write_u32(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_u8(Opcode::Branch);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_u32(0);
  }
}

void Assembler::write_branchif_to_label(Label label) {
  if (this->labels.count(label) > 0) {
    this->write_u8(Opcode::BranchIf);
    this->write_u32(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_u8(Opcode::BranchIf);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_u32(0);
  }
}

void Assembler::write_branchlt_to_label(Label label) {
  if (this->labels.count(label) > 0) {
    this->write_u8(Opcode::BranchLt);
    this->write_u32(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_u8(Opcode::BranchLt);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_u32(0);
  }
}

void Assembler::write_branchgt_to_label(Label label) {
  if (this->labels.count(label) > 0) {
    this->write_u8(Opcode::BranchGt);
    this->write_u32(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_u8(Opcode::BranchGt);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_u32(0);
  }
}

void Assembler::write_branchle_to_label(Label label) {
  if (this->labels.count(label) > 0) {
    this->write_u8(Opcode::BranchLe);
    this->write_u32(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_u8(Opcode::BranchLe);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_u32(0);
  }
}

void Assembler::write_branchge_to_label(Label label) {
  if (this->labels.count(label) > 0) {
    this->write_u8(Opcode::BranchGe);
    this->write_u32(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_u8(Opcode::BranchGe);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_u32(0);
  }
}

void Assembler::write_brancheq_to_label(Label label) {
  if (this->labels.count(label) > 0) {
    this->write_u8(Opcode::BranchEq);
    this->write_u32(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_u8(Opcode::BranchEq);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_u32(0);
  }
}

void Assembler::write_branchneq_to_label(Label label) {
  if (this->labels.count(label) > 0) {
    this->write_u8(Opcode::BranchNeq);
    this->write_u32(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_u8(Opcode::BranchNeq);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_u32(0);
  }
}

void Assembler::write_branchunless_to_label(Label label) {
  if (this->labels.count(label) > 0) {
    this->write_u8(Opcode::BranchUnless);
    this->write_u32(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_u8(Opcode::BranchUnless);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_u32(0);
  }
}

void Assembler::write_registercatchtable_to_label(Label label) {
  if (this->labels.count(label) > 0) {
    this->write_u8(Opcode::RegisterCatchTable);
    this->write_u32(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_u8(Opcode::RegisterCatchTable);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_u32(0);
  }
}

void Assembler::write_putfunction_to_label(VALUE symbol,
                                           Label label,
                                           bool anonymous,
                                           bool needs_arguments,
                                           uint32_t argc,
                                           uint32_t lvarcount) {
  if (this->labels.count(label) > 0) {
    this->write_u8(Opcode::PutFunction);
    this->write_u64(symbol);
    this->write_u32(this->labels[label] - this->writeoffset + 1);
    this->write_u8(anonymous);
    this->write_u8(needs_arguments);
    this->write_u32(argc);
    this->write_u32(lvarcount);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_u8(Opcode::PutFunction);
    this->write_u64(symbol);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_u32(0);
    this->write_u8(anonymous);
    this->write_u8(needs_arguments);
    this->write_u32(argc);
    this->write_u32(lvarcount);
  }
}

void Assembler::write_putgenerator_to_label(VALUE symbol, Label label) {
  if (this->labels.count(label) > 0) {
    this->write_u8(Opcode::PutGenerator);
    this->write_u64(symbol);
    this->write_u32(this->labels[label] - this->writeoffset + 1);
  } else {
    uint32_t instruction_base = this->writeoffset;
    this->write_u8(Opcode::PutGenerator);
    this->write_u64(symbol);
    this->unresolved_label_references.push_back(UnresolvedReference({label, this->writeoffset, instruction_base}));
    this->write_u32(0);
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
    this->read<int32_t>(uref->target_offset) = relative_offset;
    uref = this->unresolved_label_references.erase(uref);
  }
}
}  // namespace Charly::Compilation
