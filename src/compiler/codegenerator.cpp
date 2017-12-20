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

AST::AbstractNode* CodeGenerator::visit_while(AST::While* node, VisitContinue cont) {
  (void)cont;

  // Setup labels
  Label condition_label = this->assembler->place_label();
  Label break_label = this->assembler->reserve_label();
  this->break_stack.push_back(break_label);
  this->continue_stack.push_back(condition_label);

  // Condition codegen
  this->visit_node(node->condition);
  this->assembler->write_branchunless_to_label(break_label);

  // Block codegen
  this->visit_node(node->block);
  this->assembler->write_branch_to_label(condition_label);
  this->assembler->place_label(break_label);

  // Remove the break and continue labels from the stack again
  this->break_stack.pop_back();
  this->continue_stack.pop_back();

  return node;
}

AST::AbstractNode* CodeGenerator::visit_until(AST::Until* node, VisitContinue cont) {
  (void)cont;

  // Setup labels
  Label condition_label = this->assembler->place_label();
  Label break_label = this->assembler->reserve_label();
  this->break_stack.push_back(break_label);
  this->continue_stack.push_back(condition_label);

  // Condition codegen
  this->visit_node(node->condition);
  this->assembler->write_branchif_to_label(break_label);

  // Block codegen
  this->visit_node(node->block);
  this->assembler->write_branch_to_label(condition_label);
  this->assembler->place_label(break_label);

  // Remove the break and continue labels from the stack again
  this->break_stack.pop_back();
  this->continue_stack.pop_back();

  return node;
}

AST::AbstractNode* CodeGenerator::visit_loop(AST::Loop* node, VisitContinue cont) {
  (void)cont;

  // Setup labels
  Label block_label = this->assembler->place_label();
  Label break_label = this->assembler->reserve_label();
  this->break_stack.push_back(break_label);
  this->continue_stack.push_back(block_label);

  // Block codegen
  this->visit_node(node->block);
  this->assembler->write_branch_to_label(block_label);
  this->assembler->place_label(break_label);

  // Remove the break and continue labels from the stack again
  this->break_stack.pop_back();
  this->continue_stack.pop_back();

  return node;
}

AST::AbstractNode* CodeGenerator::visit_unary(AST::Unary* node, VisitContinue cont) {
  (void)cont;

  // Codegen expression
  this->visit_node(node->expression);
  this->assembler->write_operator(kOperatorOpcodeMapping[node->operator_type]);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_binary(AST::Binary* node, VisitContinue cont) {
  (void)cont;

  // Codegen expression
  this->visit_node(node->left);
  this->visit_node(node->right);
  this->assembler->write_operator(kOperatorOpcodeMapping[node->operator_type]);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_and(AST::And* node, VisitContinue cont) {
  (void)cont;

  // Label setup
  Label end_and_label = this->assembler->reserve_label();

  // Codegen expressions
  this->visit_node(node->left);
  this->assembler->write_dup();
  this->assembler->write_branchunless_to_label(end_and_label);
  this->assembler->write_pop(1);
  this->visit_node(node->right);

  this->assembler->place_label(end_and_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_or(AST::Or* node, VisitContinue cont) {
  (void)cont;

  // Label setup
  Label end_or_label = this->assembler->reserve_label();

  // Codegen expressions
  this->visit_node(node->left);
  this->assembler->write_dup();
  this->assembler->write_branchif_to_label(end_or_label);
  this->assembler->write_pop(1);
  this->visit_node(node->right);

  this->assembler->place_label(end_or_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_typeof(AST::Typeof* node, VisitContinue cont) {
  cont();
  this->assembler->write_typeof();
  return node;
}

AST::AbstractNode* CodeGenerator::visit_assignment(AST::Assignment* node, VisitContinue cont) {

  // Check if we have the offset info for this identifier
  if (node->offset_info == nullptr) {
    this->fatal_error(node, "Missing offset info for assignment codegen");
  }

  // Codegen assignment
  cont();
  this->assembler->write_setlocal(node->offset_info->level, node->offset_info->index);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_memberassignment(AST::MemberAssignment* node, VisitContinue cont) {
  (void)cont;

  // Codegen assignment
  this->visit_node(node->target);
  this->visit_node(node->expression);
  this->assembler->write_setmembersymbol(this->symtable(node->member));

  return node;
}

AST::AbstractNode* CodeGenerator::visit_indexassignment(AST::IndexAssignment* node, VisitContinue cont) {
  (void)cont;

  // Codegen assignment
  this->visit_node(node->target);
  this->visit_node(node->index);
  this->visit_node(node->expression);
  this->assembler->write_setmembervalue();

  return node;
}

AST::AbstractNode* CodeGenerator::visit_call(AST::Call* node, VisitContinue cont) {
  (void)cont;

  // Codegen target
  this->visit_node(node->target);

  // Codegen arguments
  for (auto arg : node->arguments->children) {
    this->visit_node(arg);
  }

  this->assembler->write_call(node->arguments->children.size());

  return node;
}

AST::AbstractNode* CodeGenerator::visit_callmember(AST::CallMember* node, VisitContinue cont) {
  (void)cont;

  // Codegen target
  this->visit_node(node->context);

  // Codegen function
  this->assembler->write_dup();
  this->assembler->write_readmembersymbol(this->symtable(node->symbol));

  // Codegen arguments
  for (auto arg : node->arguments->children) {
    this->visit_node(arg);
  }

  this->assembler->write_callmember(node->arguments->children.size());

  return node;
}

AST::AbstractNode* CodeGenerator::visit_callindex(AST::CallIndex* node, VisitContinue cont) {
  (void)cont;

  // Codegen target
  this->visit_node(node->context);

  // Codegen function
  this->assembler->write_dup();
  this->visit_node(node->index);
  this->assembler->write_readmembervalue();

  // Codegen arguments
  for (auto arg : node->arguments->children) {
    this->visit_node(arg);
  }

  this->assembler->write_callmember(node->arguments->children.size());

  return node;
}

AST::AbstractNode* CodeGenerator::visit_identifier(AST::Identifier* node, VisitContinue cont) {
  (void)cont;

  // Check if we have the offset info for this identifier
  if (node->offset_info == nullptr) {
    this->fatal_error(node, "Missing offset info for identifier codegen");
  }

  this->assembler->write_readlocal(node->offset_info->level, node->offset_info->index);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_self(AST::Self* node, VisitContinue cont) {
  (void)cont;
  this->assembler->write_putself();
  return node;
}

AST::AbstractNode* CodeGenerator::visit_member(AST::Member* node, VisitContinue cont) {
  (void)cont;

  // Codegen target
  this->visit_node(node->target);
  this->assembler->write_readmembersymbol(this->symtable(node->symbol));

  return node;
}

AST::AbstractNode* CodeGenerator::visit_index(AST::Index* node, VisitContinue cont) {
  (void)cont;

  // Codegen target
  this->visit_node(node->target);
  this->visit_node(node->argument);
  this->assembler->write_readmembervalue();

  return node;
}

}
