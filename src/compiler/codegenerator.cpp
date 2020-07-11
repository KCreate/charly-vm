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

#include <cmath>

#include "codegenerator.h"
#include "stringpool.h"

namespace Charly::Compilation {

InstructionBlock* CodeGenerator::compile(AST::AbstractNode* node) {
  this->visit_node(node);
  this->assembler.write_halt();

  // Codegen all blocks
  while (this->queued_functions.size() > 0) {
    QueuedFunction& queued_block = this->queued_functions.front();
    this->assembler.place_label(queued_block.label);

    // Generate a putgenerator instruction if this is a generator function
    if (queued_block.function->generator) {
      Label generator_label = this->assembler.reserve_label();
      this->assembler.write_nop();
      this->assembler.write_putgenerator_to_label(SymbolTable::encode(queued_block.function->name), generator_label);
      this->assembler.write_return();
      this->assembler.place_label(generator_label);
      this->visit_node(queued_block.function->body);
    } else {
      this->visit_node(queued_block.function->body);
    }

    this->queued_functions.pop_front();
  }
  this->assembler.resolve_unresolved_label_references();
  return new InstructionBlock(this->assembler);
}

AST::AbstractNode* CodeGenerator::visit_block(AST::Block* node, VisitContinue) {
  this->assembler.write_nop();
  for (auto& node : node->statements) {
    this->visit_node(node);

    // If the statement produces an expression, pop it off the stack now
    if (AST::yields_value(node)) {
      if (!AST::is_assignment(node)) {
        this->assembler.write_pop();
      }
    }
  }

  return node;
}

AST::AbstractNode* CodeGenerator::visit_ternaryif(AST::TernaryIf* node, VisitContinue) {
  // Codegen the condition
  if (AST::is_comparison(node->condition)) {
    this->codegen_cmp_arguments(node->condition);
  } else {
    this->visit_node(node->condition);
  }

  // Skip over the then expression if the condition was false
  Label else_exp_label = this->assembler.reserve_label();
  Label end_exp_label = this->assembler.reserve_label();

  if (AST::is_comparison(node->condition)) {
    this->codegen_cmp_branchunless(node->condition, else_exp_label);
  } else {
    this->assembler.write_branchunless_to_label(else_exp_label);
  }

  this->visit_node(node->then_expression);
  this->assembler.write_branch_to_label(end_exp_label);
  this->assembler.place_label(else_exp_label);
  this->visit_node(node->else_expression);
  this->assembler.place_label(end_exp_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_if(AST::If* node, VisitContinue) {
  // Codegen the condition
  if (AST::is_comparison(node->condition)) {
    this->codegen_cmp_arguments(node->condition);
  } else {
    this->visit_node(node->condition);
  }

  // Skip over the block if the condition was false
  Label end_block_label = this->assembler.reserve_label();

  if (AST::is_comparison(node->condition)) {
    this->codegen_cmp_branchunless(node->condition, end_block_label);
  } else {
    this->assembler.write_branchunless_to_label(end_block_label);
  }

  this->visit_node(node->then_block);
  this->assembler.place_label(end_block_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_ifelse(AST::IfElse* node, VisitContinue) {
  // Codegen the condition
  if (AST::is_comparison(node->condition)) {
    this->codegen_cmp_arguments(node->condition);
  } else {
    this->visit_node(node->condition);
  }

  // Skip over the block if the condition was false
  Label else_block_label = this->assembler.reserve_label();
  Label end_block_label = this->assembler.reserve_label();

  if (AST::is_comparison(node->condition)) {
    this->codegen_cmp_branchunless(node->condition, else_block_label);
  } else {
    this->assembler.write_branchunless_to_label(else_block_label);
  }

  this->visit_node(node->then_block);
  this->assembler.write_branch_to_label(end_block_label);
  this->assembler.place_label(else_block_label);
  this->visit_node(node->else_block);
  this->assembler.place_label(end_block_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_unless(AST::Unless* node, VisitContinue) {
  // Codegen the condition
  this->visit_node(node->condition);

  // Skip over the block if the condition was false
  Label end_block_label = this->assembler.reserve_label();
  this->assembler.write_branchif_to_label(end_block_label);
  this->visit_node(node->then_block);
  this->assembler.place_label(end_block_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_unlesselse(AST::UnlessElse* node, VisitContinue) {
  // Codegen the condition
  this->visit_node(node->condition);

  // Skip over the block if the condition was false
  Label else_block_label = this->assembler.reserve_label();
  Label end_block_label = this->assembler.reserve_label();
  this->assembler.write_branchif_to_label(else_block_label);
  this->visit_node(node->then_block);
  this->assembler.write_branch_to_label(end_block_label);
  this->assembler.place_label(else_block_label);
  this->visit_node(node->else_block);
  this->assembler.place_label(end_block_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_do_while(AST::DoWhile* node, VisitContinue) {
  // Setup labels
  Label block_label = this->assembler.reserve_label();
  Label condition_label = this->assembler.reserve_label();
  Label break_label = this->assembler.reserve_label();
  this->break_stack.push_back(break_label);
  this->continue_stack.push_back(condition_label);

  // Block codegen
  this->assembler.place_label(block_label);
  this->visit_node(node->block);

  // Condition codegen
  this->assembler.place_label(condition_label);
  if (AST::is_comparison(node->condition)) {
    this->codegen_cmp_arguments(node->condition);
    this->codegen_cmp_branchunless(node->condition, break_label);
  } else {
    this->visit_node(node->condition);
    this->assembler.write_branchunless_to_label(break_label);
  }
  this->assembler.write_branch_to_label(block_label);
  this->assembler.place_label(break_label);

  // Remove the break and continue labels from the stack again
  this->break_stack.pop_back();
  this->continue_stack.pop_back();

  return node;
}

AST::AbstractNode* CodeGenerator::visit_do_until(AST::DoUntil* node, VisitContinue) {
  // Setup labels
  Label block_label = this->assembler.reserve_label();
  Label condition_label = this->assembler.reserve_label();
  Label break_label = this->assembler.reserve_label();
  this->break_stack.push_back(break_label);
  this->continue_stack.push_back(condition_label);

  // Block codegen
  this->assembler.place_label(block_label);
  this->visit_node(node->block);

  // Condition codegen
  this->assembler.place_label(condition_label);
  this->visit_node(node->condition);
  this->assembler.write_branchif_to_label(break_label);
  this->assembler.write_branch_to_label(block_label);
  this->assembler.place_label(break_label);

  // Remove the break and continue labels from the stack again
  this->break_stack.pop_back();
  this->continue_stack.pop_back();

  return node;
}

AST::AbstractNode* CodeGenerator::visit_while(AST::While* node, VisitContinue) {
  // Setup labels
  Label condition_label = this->assembler.place_label();
  Label break_label = this->assembler.reserve_label();
  this->break_stack.push_back(break_label);
  this->continue_stack.push_back(condition_label);

  // Condition codegen
  if (AST::is_comparison(node->condition)) {
    this->codegen_cmp_arguments(node->condition);
    this->codegen_cmp_branchunless(node->condition, break_label);
  } else {
    this->visit_node(node->condition);
    this->assembler.write_branchunless_to_label(break_label);
  }

  // Block codegen
  this->visit_node(node->block);
  this->assembler.write_branch_to_label(condition_label);
  this->assembler.place_label(break_label);

  // Remove the break and continue labels from the stack again
  this->break_stack.pop_back();
  this->continue_stack.pop_back();

  return node;
}

AST::AbstractNode* CodeGenerator::visit_until(AST::Until* node, VisitContinue) {
  // Setup labels
  Label condition_label = this->assembler.place_label();
  Label break_label = this->assembler.reserve_label();
  this->break_stack.push_back(break_label);
  this->continue_stack.push_back(condition_label);

  // Condition codegen
  this->visit_node(node->condition);
  this->assembler.write_branchif_to_label(break_label);

  // Block codegen
  this->visit_node(node->block);
  this->assembler.write_branch_to_label(condition_label);
  this->assembler.place_label(break_label);

  // Remove the break and continue labels from the stack again
  this->break_stack.pop_back();
  this->continue_stack.pop_back();

  return node;
}

AST::AbstractNode* CodeGenerator::visit_loop(AST::Loop* node, VisitContinue) {
  // Setup labels
  Label block_label = this->assembler.place_label();
  Label break_label = this->assembler.reserve_label();
  this->break_stack.push_back(break_label);
  this->continue_stack.push_back(block_label);

  // Block codegen
  this->visit_node(node->block);
  this->assembler.write_branch_to_label(block_label);
  this->assembler.place_label(break_label);

  // Remove the break and continue labels from the stack again
  this->break_stack.pop_back();
  this->continue_stack.pop_back();

  return node;
}

AST::AbstractNode* CodeGenerator::visit_unary(AST::Unary* node, VisitContinue) {
  // Codegen expression
  this->visit_node(node->expression);
  this->assembler.write_operator(kOperatorOpcodeMapping[node->operator_type]);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_binary(AST::Binary* node, VisitContinue) {
  // Codegen expression
  this->visit_node(node->left);
  this->visit_node(node->right);
  this->assembler.write_operator(kOperatorOpcodeMapping[node->operator_type]);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_switch(AST::Switch* node, VisitContinue) {
  // Setup labels
  Label end_label = this->assembler.reserve_label();
  Label default_block = this->assembler.reserve_label();
  this->break_stack.push_back(end_label);

  // Codegen switch condition
  this->visit_node(node->condition);

  std::vector<Label> block_labels;
  block_labels.reserve(node->cases->size());

  // Codegen the switch conditions
  for (auto n : node->cases->children) {
    // Check if this is a switchnode (it should be)
    if (n->type() != AST::kTypeSwitchNode) {
      this->push_fatal_error(n, "Expected node to be a SwitchNode");
    }

    AST::SwitchNode* snode = n->as<AST::SwitchNode>();

    // Label of the block which runs if this node is selected
    Label node_block = this->assembler.reserve_label();
    block_labels.push_back(node_block);

    // Codegen each condition
    for (auto c : snode->conditions->children) {
      this->assembler.write_dup();
      this->visit_node(c);
      this->assembler.write_operator(Opcode::Eq);
      this->assembler.write_branchif_to_label(node_block);
    }
  }

  // Branch to the default block
  if (node->cases->size() > 0) {
    this->assembler.write_branch_to_label(default_block);
  }

  // Codegen the switch blocks
  int i = 0;
  for (auto n : node->cases->children) {
    AST::SwitchNode* snode = n->as<AST::SwitchNode>();

    // Codegen the block
    this->assembler.place_label(block_labels[i]);

    // Pop the condition off the stack
    this->assembler.write_pop();
    this->visit_node(snode->block);
    this->assembler.write_branch_to_label(end_label);

    i++;
  }

  // Codegen default block if there is one
  this->assembler.place_label(default_block);
  this->assembler.write_pop();
  if (node->default_block->type() != AST::kTypeEmpty) {
    this->visit_node(node->default_block);
  }
  this->assembler.place_label(end_label);

  this->break_stack.pop_back();

  return node;
}

AST::AbstractNode* CodeGenerator::visit_and(AST::And* node, VisitContinue) {
  // Label setup
  Label end_and_label = this->assembler.reserve_label();

  // Codegen expressions
  this->visit_node(node->left);
  this->assembler.write_dup();
  this->assembler.write_branchunless_to_label(end_and_label);
  this->assembler.write_pop();
  this->visit_node(node->right);

  this->assembler.place_label(end_and_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_or(AST::Or* node, VisitContinue) {
  // Label setup
  Label end_or_label = this->assembler.reserve_label();

  // Codegen expressions
  this->visit_node(node->left);
  this->assembler.write_dup();
  this->assembler.write_branchif_to_label(end_or_label);
  this->assembler.write_pop();
  this->visit_node(node->right);

  this->assembler.place_label(end_or_label);

  return node;
}

AST::AbstractNode* CodeGenerator::visit_typeof(AST::Typeof* node, VisitContinue cont) {
  cont();
  this->assembler.write_typeof();
  return node;
}

AST::AbstractNode* CodeGenerator::visit_new(AST::New* node, VisitContinue) {
  // Codegen target
  this->visit_node(node->klass);

  // Codegen arguments
  for (auto arg : node->arguments->children) {
    this->visit_node(arg);
  }

  this->assembler.write_new(node->arguments->size());

  return node;
}

AST::AbstractNode* CodeGenerator::visit_assignment(AST::Assignment* node, VisitContinue cont) {
  if (node->offset_info == nullptr) {
    this->push_fatal_error(node, "Missing offset info for assignment codegen");
  }

  // Codegen assignment
  cont();

  if (!this->codegen_write(*(node->offset_info), node->yielded_value_needed)) {
    this->push_fatal_error(node, "Invalid offset info generated by compiler");
  }

  return node;
}

AST::AbstractNode* CodeGenerator::visit_memberassignment(AST::MemberAssignment* node, VisitContinue) {
  // Codegen assignment
  this->visit_node(node->target);
  this->visit_node(node->expression);

  if (node->yielded_value_needed) {
    this->assembler.write_setmembersymbolpush(SymbolTable::encode(node->member));
  } else {
    this->assembler.write_setmembersymbol(SymbolTable::encode(node->member));
  }

  return node;
}

AST::AbstractNode* CodeGenerator::visit_andmemberassignment(AST::ANDMemberAssignment* node, VisitContinue) {
  // Codegen assignment
  this->visit_node(node->target);
  this->assembler.write_dup();
  this->assembler.write_readmembersymbol(SymbolTable::encode(node->member));
  this->visit_node(node->expression);
  this->assembler.write_operator(kOperatorOpcodeMapping[node->operator_type]);

  if (node->yielded_value_needed) {
    this->assembler.write_setmembersymbolpush(SymbolTable::encode(node->member));
  } else {
    this->assembler.write_setmembersymbol(SymbolTable::encode(node->member));
  }

  return node;
}

AST::AbstractNode* CodeGenerator::visit_indexassignment(AST::IndexAssignment* node, VisitContinue) {
  // Codegen assignment
  this->visit_node(node->target);
  this->visit_node(node->index);
  this->visit_node(node->expression);

  if (node->yielded_value_needed) {
    this->assembler.write_setmembervaluepush();
  } else {
    this->assembler.write_setmembervalue();
  }

  return node;
}

AST::AbstractNode* CodeGenerator::visit_andindexassignment(AST::ANDIndexAssignment* node, VisitContinue) {
  this->visit_node(node->target);
  this->visit_node(node->index);
  this->assembler.write_dupn(2);
  this->assembler.write_readmembervalue();
  this->visit_node(node->expression);
  this->assembler.write_operator(kOperatorOpcodeMapping[node->operator_type]);

  if (node->yielded_value_needed) {
    this->assembler.write_setmembervaluepush();
  } else {
    this->assembler.write_setmembervalue();
  }

  return node;
}

AST::AbstractNode* CodeGenerator::visit_call(AST::Call* node, VisitContinue) {
  // Codegen target
  this->visit_node(node->target);

  // Codegen arguments
  for (auto arg : node->arguments->children) {
    this->visit_node(arg);
  }

  this->assembler.write_call(node->arguments->size());

  return node;
}

AST::AbstractNode* CodeGenerator::visit_callmember(AST::CallMember* node, VisitContinue) {
  // Codegen target
  this->visit_node(node->context);

  // Codegen function
  this->assembler.write_dup();
  this->assembler.write_readmembersymbol(SymbolTable::encode(node->symbol));

  // Codegen arguments
  for (auto arg : node->arguments->children) {
    this->visit_node(arg);
  }

  this->assembler.write_callmember(node->arguments->size());

  return node;
}

AST::AbstractNode* CodeGenerator::visit_callindex(AST::CallIndex* node, VisitContinue) {
  // Codegen target
  this->visit_node(node->context);

  // Codegen function
  this->assembler.write_dup();
  this->visit_node(node->index);
  this->assembler.write_readmembervalue();

  // Codegen arguments
  for (auto arg : node->arguments->children) {
    this->visit_node(arg);
  }

  this->assembler.write_callmember(node->arguments->size());

  return node;
}

AST::AbstractNode* CodeGenerator::visit_identifier(AST::Identifier* node, VisitContinue) {
  // Check if we have the offset info for this identifier
  if (node->offset_info == nullptr) {
    this->push_fatal_error(node, "Missing offset info for identifier codegen");
  }

  if (!this->codegen_read(*(node->offset_info))) {
    this->push_fatal_error(node, "Invalid offset info generated by compiler");
  }

  return node;
}

AST::AbstractNode* CodeGenerator::visit_self(AST::Self* node, VisitContinue) {
  this->assembler.write_putself();
  return node;
}

AST::AbstractNode* CodeGenerator::visit_super(AST::Super* node, VisitContinue) {
  this->assembler.write_putsuper();
  return node;
}

AST::AbstractNode* CodeGenerator::visit_supermember(AST::SuperMember* node, VisitContinue) {
  this->assembler.write_putsupermember(SymbolTable::encode(node->symbol));
  return node;
}

AST::AbstractNode* CodeGenerator::visit_member(AST::Member* node, VisitContinue) {
  // Codegen target
  this->visit_node(node->target);
  this->assembler.write_readmembersymbol(SymbolTable::encode(node->symbol));

  return node;
}

AST::AbstractNode* CodeGenerator::visit_index(AST::Index* node, VisitContinue) {
  // Codegen target
  this->visit_node(node->target);
  this->visit_node(node->argument);
  this->assembler.write_readmembervalue();

  return node;
}

AST::AbstractNode* CodeGenerator::visit_null(AST::Null* node, VisitContinue) {
  this->assembler.write_putvalue(kNull);
  return node;
}

AST::AbstractNode* CodeGenerator::visit_nan(AST::Nan* node, VisitContinue) {
  this->assembler.write_putvalue(kNaN);
  return node;
}

AST::AbstractNode* CodeGenerator::visit_string(AST::String* node, VisitContinue) {
  if (node->value.size() <= 6) {
    this->assembler.write_putvalue(charly_create_istring(node->value.data(), node->value.size()));
  } else {
    StringOffsetInfo info = StringPool::encode_string(node->value);
    this->assembler.write_putstring(info.offset, info.length);
  }

  return node;
}

AST::AbstractNode* CodeGenerator::visit_floatnum(AST::FloatNum* node, VisitContinue) {
  this->assembler.write_putvalue(charly_create_number(node->value));
  return node;
}

AST::AbstractNode* CodeGenerator::visit_intnum(AST::IntNum* node, VisitContinue) {
  this->assembler.write_putvalue(charly_create_number(node->value));
  return node;
}

AST::AbstractNode* CodeGenerator::visit_boolean(AST::Boolean* node, VisitContinue) {
  this->assembler.write_putvalue(node->value ? kTrue : kFalse);
  return node;
}

AST::AbstractNode* CodeGenerator::visit_array(AST::Array* node, VisitContinue) {
  // Codegen array expressions
  for (auto child : node->expressions->children) {
    this->visit_node(child);
  }
  this->assembler.write_putarray(node->expressions->size());
  return node;
}

AST::AbstractNode* CodeGenerator::visit_hash(AST::Hash* node, VisitContinue) {
  // Codegen hash key and values expressions
  for (auto& pair : node->pairs) {
    this->visit_node(pair.second);
    this->assembler.write_putvalue(SymbolTable::encode(pair.first));
  }
  this->assembler.write_puthash(node->pairs.size());
  return node;
}

AST::AbstractNode* CodeGenerator::visit_function(AST::Function* node, VisitContinue) {
  // Label setup
  Label function_block_label = this->assembler.reserve_label();

  this->assembler.write_putfunction_to_label(SymbolTable::encode(node->name), function_block_label, node->anonymous,
                                             node->needs_arguments, node->parameters.size(), node->required_arguments,
                                             node->lvarcount);

  // Anonymous generators are invoked immediately
  if (node->generator && node->anonymous) {
    this->assembler.write_call(0);
  }

  // Codegen the block
  this->queued_functions.push_back(QueuedFunction({function_block_label, node}));

  return node;
}

AST::AbstractNode* CodeGenerator::visit_class(AST::Class* node, VisitContinue) {
  // Codegen all regular and static members
  for (auto n : node->member_properties->children) {
    if (n->type() != AST::kTypeIdentifier)
      this->push_fatal_error(n, "Expected node to be an identifier");
    this->assembler.write_putvalue(SymbolTable::encode(n->as<AST::Identifier>()->name));
  }
  for (auto n : node->static_properties->children) {
    if (n->type() == AST::kTypeAssignment) {
      this->assembler.write_putvalue(SymbolTable::encode(n->as<AST::Assignment>()->target));
      continue;
    }

    if (n->type() == AST::kTypeIdentifier) {
      this->assembler.write_putvalue(SymbolTable::encode(n->as<AST::Identifier>()->name));
      continue;
    }

    this->push_fatal_error(n, "Expected node to be an identifier");
  }
  for (auto n : node->member_functions->children) {
    this->visit_node(n);
  }
  for (auto n : node->static_functions->children) {
    this->visit_node(n);
  }
  if (node->parent_class->type() != AST::kTypeEmpty) {
    this->visit_node(node->parent_class);
  }

  if (node->constructor->type() != AST::kTypeEmpty) {
    this->visit_node(node->constructor);
  }

  this->assembler.write_putclass(SymbolTable::encode(node->name), node->member_properties->size(),
                                 node->static_properties->size(), node->member_functions->size(),
                                 node->static_functions->size(), node->parent_class->type() != AST::kTypeEmpty,
                                 node->constructor->type() != AST::kTypeEmpty);

  for (auto n : node->static_properties->children) {
    if (n->type() == AST::kTypeAssignment) {
      VALUE target_symbol = SymbolTable::encode(n->as<AST::Assignment>()->target);
      this->assembler.write_dup();
      this->visit_node(n->as<AST::Assignment>()->expression);
      this->assembler.write_setmembersymbol(target_symbol);
    }
  }

  return node;
}

AST::AbstractNode* CodeGenerator::visit_return(AST::Return* node, VisitContinue cont) {
  cont();
  this->assembler.write_return();
  return node;
}

AST::AbstractNode* CodeGenerator::visit_yield(AST::Yield* node, VisitContinue cont) {
  cont();
  this->assembler.write_yield();
  return node;
}

AST::AbstractNode* CodeGenerator::visit_throw(AST::Throw* node, VisitContinue cont) {
  cont();
  this->assembler.write_throw();
  return node;
}

AST::AbstractNode* CodeGenerator::visit_break(AST::Break* node, VisitContinue) {
  // Check if there is a label for the break instruction
  if (this->break_stack.size() == 0) {
    this->push_fatal_error(node, "Break has no jump target.");
  }

  this->assembler.write_branch_to_label(this->break_stack.back());
  return node;
}

AST::AbstractNode* CodeGenerator::visit_continue(AST::Continue* node, VisitContinue) {
  // Check if there is a label for the continue instruction
  if (this->continue_stack.size() == 0) {
    this->push_fatal_error(node, "Continue has no jump target.");
  }

  this->assembler.write_branch_to_label(this->continue_stack.back());
  return node;
}

AST::AbstractNode* CodeGenerator::visit_trycatch(AST::TryCatch* node, VisitContinue) {
  // Implementation of this method was inspired by:
  // http://lists.llvm.org/pipermail/llvm-dev/2008-April/013978.html

  // Check if we have the offset_info for the exception name
  if (node->exception_name->offset_info == nullptr) {
    this->push_fatal_error(node, "Missing offset info for exception identifier");
  }

  // Label setup
  Label handler_label = this->assembler.reserve_label();
  Label finally_label = this->assembler.reserve_label();

  // Codegen try block
  this->assembler.write_registercatchtable_to_label(handler_label);
  this->visit_node(node->block);
  this->assembler.write_popcatchtable();
  this->assembler.write_branch_to_label(finally_label);

  // Codegen handler block
  // If we don't have a handler block, we treat this try catch statement
  // as a cleanup landing pad and rethrow the exception after executing the finally block
  this->assembler.place_label(handler_label);
  if (node->handler_block->type() != AST::kTypeEmpty) {
    if (!this->codegen_write(*(node->exception_name->offset_info))) {
      this->push_fatal_error(node, "Invalid offset info generated by compiler");
    }
    this->visit_node(node->handler_block);

    // We don't emit a branch here because the end statement and finally block labels
    // would we generated after this node anyway.
  } else {
    if (node->finally_block->type() == AST::kTypeEmpty) {
      this->push_fatal_error(node, "Can't codegen try/catch statement with neither a handler nor finally block");
    }

    // Store the exception
    if (!this->codegen_write(*(node->exception_name->offset_info))) {
      this->push_fatal_error(node, "Invalid offset info generated by compiler");
    }
    this->visit_node(node->finally_block);
    if (!this->codegen_read(*(node->exception_name->offset_info))) {
      this->push_fatal_error(node, "Invalid offset info generated by compiler");
    }
    this->assembler.write_throw();
  }

  // Codegen finally block
  this->assembler.place_label(finally_label);
  if (node->finally_block->type() != AST::kTypeEmpty) {
    this->visit_node(node->finally_block);
  }

  return node;
}

bool CodeGenerator::codegen_read(ValueLocation& location) {
  switch (location.type) {
    case LocationType::LocFrame: {
      this->assembler.write_readlocal(
        location.as_frame.index,
        location.as_frame.level
      );
      break;
    }
    case LocationType::LocStack: {
      // In this case we do nothing because the value we are trying to summon
      // onto the stack here is already on the stack as guaranteed by the compiler.
      break;
    }
    case LocationType::LocArguments: {
      this->assembler.write_readlocal(0, location.as_arguments.level);
      this->assembler.write_readarrayindex(location.as_arguments.index);
      break;
    }
    case LocationType::LocGlobal: {
      this->assembler.write_readglobal(location.as_global.symbol);
      break;
    }
    case LocationType::LocInvalid: {
      return false;
      break;
    }
  }

  return true;
}

bool CodeGenerator::codegen_write(ValueLocation& location, bool keep_on_stack) {
  switch (location.type) {
    case LocationType::LocFrame: {
      if (keep_on_stack) {
        this->assembler.write_setlocalpush(
          location.as_frame.index,
          location.as_frame.level
        );
      } else {
        this->assembler.write_setlocal(
          location.as_frame.index,
          location.as_frame.level
        );
      }
      break;
    }
    case LocationType::LocStack: {
      // In this case we do nothing because the value that is being stored onto the stack
      // is by definition already on the stack
      break;
    }
    case LocationType::LocArguments: {
      this->assembler.write_readlocal(0, location.as_arguments.level);
      if (keep_on_stack) {
        this->assembler.write_setarrayindexpush(location.as_arguments.index);
      } else {
        this->assembler.write_setarrayindex(location.as_arguments.index);
      }
      break;
    }
    case LocationType::LocGlobal: {
      if (keep_on_stack) {
        this->assembler.write_setglobalpush(location.as_global.symbol);
      } else {
        this->assembler.write_setglobal(location.as_global.symbol);
      }
      break;
    }
    case LocationType::LocInvalid: {
      return false;
      break;
    }
  }

  return true;
}

void CodeGenerator::codegen_cmp_arguments(AST::AbstractNode* node) {
  this->visit_node(node->as<AST::Binary>()->left);
  this->visit_node(node->as<AST::Binary>()->right);
}

void CodeGenerator::codegen_cmp_branchunless(AST::AbstractNode* node, Label target_label) {
  AST::Binary* binexp = node->as<AST::Binary>();
  switch (binexp->operator_type) {
    case TokenType::Less: {
      this->assembler.write_branchge_to_label(target_label);
      break;
    }
    case TokenType::Greater: {
      this->assembler.write_branchle_to_label(target_label);
      break;
    }
    case TokenType::LessEqual: {
      this->assembler.write_branchgt_to_label(target_label);
      break;
    }
    case TokenType::GreaterEqual: {
      this->assembler.write_branchlt_to_label(target_label);
      break;
    }
    case TokenType::Equal: {
      this->assembler.write_branchneq_to_label(target_label);
      break;
    }
    case TokenType::Not: {
      this->assembler.write_brancheq_to_label(target_label);
      break;
    }
    default: {
      this->push_fatal_error(node, "Node doesn't have a comparison operator");
    }
  }
}

}  // namespace Charly::Compilation
