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

#include <vector>
#include <unordered_map>

#include "ast.h"
#include "compiler-pass.h"
#include "assembler.h"
#include "symboltable.h"
#include "token.h"
#include "opcode.h"

#pragma once

namespace Charly::Compilation {
// Responsible for generating Charly bytecodes
class CodeGenerator : public CompilerPass {
  using CompilerPass::CompilerPass;

public:
  CodeGenerator(SymbolTable& s) : CompilerPass(s) {
    this->assembler = new Assembler();
  }
  ~CodeGenerator() {
    delete this->assembler;
  }

  // Main interface to the compiler
  InstructionBlock* compile(AST::AbstractNode* node);
  void reset();

  // Codegen specific AST nodes
  AST::AbstractNode* visit_if(AST::If* node, VisitContinue cont);
  AST::AbstractNode* visit_ifelse(AST::IfElse* node, VisitContinue cont);
  AST::AbstractNode* visit_unless(AST::Unless* node, VisitContinue cont);
  AST::AbstractNode* visit_unlesselse(AST::UnlessElse* node, VisitContinue cont);
  AST::AbstractNode* visit_guard(AST::Guard* node, VisitContinue cont);
  AST::AbstractNode* visit_while(AST::While* node, VisitContinue cont);
  AST::AbstractNode* visit_until(AST::Until* node, VisitContinue cont);
  AST::AbstractNode* visit_loop(AST::Loop* node, VisitContinue cont);
  AST::AbstractNode* visit_unary(AST::Unary* node, VisitContinue cont);
  AST::AbstractNode* visit_binary(AST::Binary* node, VisitContinue cont);
  //AST::AbstractNode* visit_switch(AST::Binary* node, VisitContinue cont);
  AST::AbstractNode* visit_and(AST::And* node, VisitContinue cont);
  AST::AbstractNode* visit_or(AST::Or* node, VisitContinue cont);
  AST::AbstractNode* visit_typeof(AST::Typeof* node, VisitContinue cont);
  AST::AbstractNode* visit_assignment(AST::Assignment* node, VisitContinue cont);
  AST::AbstractNode* visit_memberassignment(AST::MemberAssignment* node, VisitContinue cont);
  AST::AbstractNode* visit_indexassignment(AST::IndexAssignment* node, VisitContinue cont);
  AST::AbstractNode* visit_call(AST::Call* node, VisitContinue cont);
  AST::AbstractNode* visit_callmember(AST::CallMember* node, VisitContinue cont);
  AST::AbstractNode* visit_callindex(AST::CallIndex* node, VisitContinue cont);
  AST::AbstractNode* visit_identifier(AST::Identifier* node, VisitContinue cont);
  AST::AbstractNode* visit_self(AST::Self* node, VisitContinue cont);
  AST::AbstractNode* visit_member(AST::Member* node, VisitContinue cont);
  AST::AbstractNode* visit_index(AST::Index* node, VisitContinue cont);
  AST::AbstractNode* visit_null(AST::Null* node, VisitContinue cont);
  AST::AbstractNode* visit_nan(AST::Nan* node, VisitContinue cont);
  AST::AbstractNode* visit_string(AST::String* node, VisitContinue cont);
  AST::AbstractNode* visit_integer(AST::Integer* node, VisitContinue cont);
  AST::AbstractNode* visit_float(AST::Float* node, VisitContinue cont);
  AST::AbstractNode* visit_boolean(AST::Boolean* node, VisitContinue cont);
  AST::AbstractNode* visit_array(AST::Array* node, VisitContinue cont);
  AST::AbstractNode* visit_hash(AST::Hash* node, VisitContinue cont);
  AST::AbstractNode* visit_function(AST::Function* node, VisitContinue cont);
  AST::AbstractNode* visit_class(AST::Class* node, VisitContinue cont);
  AST::AbstractNode* visit_return(AST::Return* node, VisitContinue cont);
  AST::AbstractNode* visit_throw(AST::Throw* node, VisitContinue cont);
  AST::AbstractNode* visit_break(AST::Break* node, VisitContinue cont);
  AST::AbstractNode* visit_continue(AST::Continue* node, VisitContinue cont);
  AST::AbstractNode* visit_trycatch(AST::TryCatch* node, VisitContinue cont);

private:
  Assembler* assembler;
  std::vector<Label> break_stack;
  std::vector<Label> continue_stack;
};

// clang-format off
static std::unordered_map<TokenType, Opcode> kOperatorOpcodeMapping = {
  {TokenType::Plus, Opcode::Add},
  {TokenType::Minus, Opcode::Sub},
  {TokenType::Mul, Opcode::Mul},
  {TokenType::Div, Opcode::Div},
  {TokenType::Mod, Opcode::Mod},
  {TokenType::Pow, Opcode::Pow},
  {TokenType::Equal, Opcode::Eq},
  {TokenType::Not, Opcode::Neq},
  {TokenType::Less, Opcode::Lt},
  {TokenType::Greater, Opcode::Gt},
  {TokenType::LessEqual, Opcode::Le},
  {TokenType::GreaterEqual, Opcode::Ge},
  {TokenType::BitOR, Opcode::Or},
  {TokenType::BitXOR, Opcode::Xor},
  {TokenType::BitNOT, Opcode::UBNot},
  {TokenType::BitAND, Opcode::And},
  {TokenType::LeftShift, Opcode::Shl},
  {TokenType::RightShift, Opcode::Shr},
  {TokenType::UPlus, Opcode::UAdd},
  {TokenType::UMinus, Opcode::USub},
  {TokenType::UNot, Opcode::UNot}
};
// clang-format on
}  // namespace Charly::Compilation
