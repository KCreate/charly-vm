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

#include <list>

#include "normalizer.h"

namespace Charly::Compilation {
AST::AbstractNode* Normalizer::visit_block(AST::Block* node, VisitContinue cont) {
  (void)cont;

  std::list<AST::AbstractNode*> remaining_statements;

  // Remove all unneeded nodes
  bool append_to_block = true;
  for (auto statement : node->statements) {
    AST::AbstractNode* normalized_node = this->visit_node(statement);

    if (normalized_node != nullptr) {
      if (append_to_block) {
        if (!normalized_node->is_literal()) {
          remaining_statements.push_back(normalized_node);

          if (normalized_node->type() == AST::kTypeReturn || normalized_node->type() == AST::kTypeBreak ||
              normalized_node->type() == AST::kTypeContinue || normalized_node->type() == AST::kTypeThrow) {
            append_to_block = false;
          }
        } else {
          delete normalized_node;
        }
      } else {
        delete normalized_node;
      }
    }
  }

  // Wrap functions and classes in local initialisation nodes
  for (auto& statement : remaining_statements) {
    // Generate local initialisations for function and class nodes
    if (statement->type() == AST::kTypeFunction || statement->type() == AST::kTypeClass) {
      // Read the name of the node
      std::string name;
      if (statement->type() == AST::kTypeFunction) {
        name = AST::cast<AST::Function>(statement)->name;
      }
      if (statement->type() == AST::kTypeClass) {
        name = AST::cast<AST::Class>(statement)->name;
      }

      // Wrap the node in a local initialisation if it has a name
      if (name.size() > 0) {
        statement = (new AST::LocalInitialisation(name, statement, true))->at(statement);
      }
    }
  }

  node->statements = remaining_statements;
  return node;
}

AST::AbstractNode* Normalizer::visit_if(AST::If* node, VisitContinue cont) {
  cont();
  node->then_block = this->wrap_in_block(node->then_block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_ifelse(AST::IfElse* node, VisitContinue cont) {
  cont();
  node->then_block = this->wrap_in_block(node->then_block, cont);
  node->else_block = this->wrap_in_block(node->else_block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_unless(AST::Unless* node, VisitContinue cont) {
  cont();
  node->then_block = this->wrap_in_block(node->then_block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_unlesselse(AST::UnlessElse* node, VisitContinue cont) {
  cont();
  node->then_block = this->wrap_in_block(node->then_block, cont);
  node->else_block = this->wrap_in_block(node->else_block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_guard(AST::Guard* node, VisitContinue cont) {
  cont();
  node->block = this->wrap_in_block(node->block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_while(AST::While* node, VisitContinue cont) {
  cont();
  node->block = this->wrap_in_block(node->block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_until(AST::Until* node, VisitContinue cont) {
  cont();
  node->block = this->wrap_in_block(node->block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_loop(AST::Loop* node, VisitContinue cont) {
  cont();
  node->block = this->wrap_in_block(node->block, cont);
  return node;
}

AST::AbstractNode* Normalizer::visit_switch(AST::Switch* node, VisitContinue cont) {
  cont();

  if (node->default_block->type() != AST::kTypeEmpty) {
    node->default_block = this->wrap_in_block(node->default_block, cont);
  }

  for (auto& _child : node->cases->children) {
    AST::SwitchNode* switch_node = AST::cast<AST::SwitchNode>(_child);
    switch_node->block = this->wrap_in_block(switch_node->block, cont);
  }

  return node;
}

AST::AbstractNode* Normalizer::visit_function(AST::Function* node, VisitContinue cont) {
  cont();
  node->body = this->wrap_in_block(node->body, cont);

  AST::Block* body = AST::cast<AST::Block>(node->body);

  // Check if there are any statements inside the functions body
  if (body->statements.size() == 0) {
    body->append_node((new AST::Return(new AST::Null()))->at(body));
  } else {

    // Check that the last statement in the block would exit it,
    // either via a return, break, continue or throw
    // If not, insert a return null
    auto last_node_type = body->statements.back()->type();
    if (last_node_type != AST::kTypeReturn && last_node_type != AST::kTypeThrow) {
      body->append_node((new AST::Return(new AST::Null()))->at(body));
    }
  }

  return node;
}

AST::AbstractNode* Normalizer::visit_localinitialisation(AST::LocalInitialisation* node, VisitContinue cont) {
  cont();

  // Copy the name of the variable a function or class is assigned to
  // into it's name field, unless it already has a name
  if (node->expression->type() == AST::kTypeFunction) {
    AST::Function* func = AST::cast<AST::Function>(node->expression);
    if (func->name.size() == 0) {
      func->name = node->name;
    }
  } else if (node->expression->type() == AST::kTypeClass) {
    AST::Class* klass = AST::cast<AST::Class>(node->expression);
    if (klass->name.size() == 0) {
      klass->name = node->name;
    }
  }

  return node;
}

AST::AbstractNode* Normalizer::wrap_in_block(AST::AbstractNode* node, VisitContinue cont) {
  if (node->type() != AST::kTypeBlock) {
    node = (new AST::Block({node}))->at(node);
    node = this->visit_block(AST::cast<AST::Block>(node), cont);
  }

  return node;
}
}
