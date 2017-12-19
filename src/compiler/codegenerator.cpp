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

#include <iostream>

#include "codegenerator.h"

namespace Charly::Compilation {

InstructionBlock* CodeGenerator::compile(AST::AbstractNode* node) {
  this->visit_node(node);
  this->assembler->write_halt();
  this->assembler->resolve_unresolved_label_references();
  return new InstructionBlock(*static_cast<InstructionBlock*>(this->assembler));
}

void CodeGenerator::reset() {
  this->assembler->reset();
  this->break_stack.clear();
  this->continue_stack.clear();
}

AST::AbstractNode* CodeGenerator::visit_if(AST::If* node, VisitContinue cont) {
  (void)cont;

  // Codegen the condition
  this->visit_node(node->condition);

  // Skip over the block if the condition was false
  Label end_block_label = this->assembler->reserve_label();
  this->assembler->write_branchunless_to_label(end_block_label);
  this->visit_node(node->then_block);
  this->assembler->place_label(end_block_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_ifelse(AST::IfElse* node, VisitContinue cont) {
  (void)cont;

  // Codegen the condition
  this->visit_node(node->condition);

  // Skip over the block if the condition was false
  Label else_block_label = this->assembler->reserve_label();
  Label end_block_label = this->assembler->reserve_label();
  this->assembler->write_branchunless_to_label(else_block_label);
  this->visit_node(node->then_block);
  this->assembler->write_branch_to_label(end_block_label);
  this->assembler->place_label(else_block_label);
  this->visit_node(node->else_block);
  this->assembler->place_label(end_block_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_unless(AST::Unless* node, VisitContinue cont) {
  (void)cont;

  // Codegen the condition
  this->visit_node(node->condition);

  // Skip over the block if the condition was false
  Label end_block_label = this->assembler->reserve_label();
  this->assembler->write_branchif_to_label(end_block_label);
  this->visit_node(node->then_block);
  this->assembler->place_label(end_block_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_unlesselse(AST::UnlessElse* node, VisitContinue cont) {
  (void)cont;

  // Codegen the condition
  this->visit_node(node->condition);

  // Skip over the block if the condition was false
  Label else_block_label = this->assembler->reserve_label();
  Label end_block_label = this->assembler->reserve_label();
  this->assembler->write_branchunless_to_label(else_block_label);
  this->visit_node(node->then_block);
  this->assembler->write_branch_to_label(end_block_label);
  this->assembler->place_label(else_block_label);
  this->visit_node(node->else_block);
  this->assembler->place_label(end_block_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_guard(AST::Guard* node, VisitContinue cont) {
  (void)cont;

  // Codegen the condition
  this->visit_node(node->condition);

  // Skip over the block if the condition was false
  Label end_block_label = this->assembler->reserve_label();
  this->assembler->write_branchif_to_label(end_block_label);
  this->visit_node(node->block);
  this->assembler->place_label(end_block_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_identifier(AST::Identifier* node, VisitContinue cont) {
  (void)cont;

  // Check if we have the offset info for this identifier
  if (node->offset_info == nullptr) {
    this->fatal_error(node, "Missing offset info for codegen phase");
  }

  this->assembler->write_readlocal(node->offset_info->level, node->offset_info->index);

  return node;
}

}
