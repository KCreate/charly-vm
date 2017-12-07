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

#include <optional>
#include <iostream>
#include <cstdint>
#include <vector>

#include "token.h"

#pragma once

namespace Charly::Compiler::AST {

static const std::string kPaddingCharacters = "  ";

// Abstract base class of all ASTNodes
//
// TODO: Location information
struct AbstractNode {
public:
  AbstractNode() = delete;
  AbstractNode(const AbstractNode& other) = delete;
};

struct NodeList {
  // TODO: Figure out a good name for this
  // Other alternatives are: nodes, items
  std::vector<AbstractNode*> children;

  inline ~NodeList() {
    for (auto& node : this->children) delete node;
  }
};

// if <condition> {
//
// }
//
// if <condition> {
//   <then_block>
// } else {
//   <else_block>
// }
struct If: public AbstractNode {
  AbstractNode* condition;
  NodeList then_block;
  NodeList else_block;

  inline ~If() {
    delete condition;
  }
};

// unless <condition> {
//
// }
//
// unless <condition> {
//   <then_block>
// } else {
//   <else_block>
// }
struct Unless: public AbstractNode {
  AbstractNode* condition;
  NodeList then_block;
  NodeList else_block;

  inline ~Unless() {
    delete condition;
  }
};

// guard <condition> {
//   <block>
// }
struct Guard: public AbstractNode {
  AbstractNode* condition;
  NodeList block;

  inline ~Guard() {
    delete condition;
  }
};

// while <condition> {
//   <block>
// }
struct While: public AbstractNode {
  AbstractNode* condition;
  NodeList block;

  inline ~While() {
    delete condition;
  }
};

// until <condition> {
//   <block>
// }
struct Until: public AbstractNode {
  AbstractNode* condition;
  NodeList block;

  inline ~Until() {
    delete condition;
  }
};

// loop {
//   <block>
// }
struct Loop: public AbstractNode {
  NodeList block;
};

// <operator_type> <expression>
struct Unary: public AbstractNode {
  TokenType operator_type;
  AbstractNode* expression;

  inline ~Unary() {
    delete expression;
  }
};

// <left> <operator_type> <right>
struct Binary: public AbstractNode {
  TokenType operator_type;
  AbstractNode* left;
  AbstractNode* right;

  inline ~Binary() {
    delete left;
    delete right;
  }
};

// case <conditions> {
//   <block>
// }
struct SwitchNode: public AbstractNode {
  NodeList conditions;
  NodeList block;
};

// switch <condition> {
//   <cases>
//   default <default_block>
// }
struct Switch: public AbstractNode {
  AbstractNode* condition;
  std::vector<SwitchNode*> cases;
  NodeList default_block;

  inline ~Switch() {
    delete condition;
    for (auto& node : this->cases) delete node;
  }
};

// <left> && <right>
struct And: public AbstractNode {
  AbstractNode* left;
  AbstractNode* right;

  inline ~And() {
    delete left;
    delete right;
  }
};

// <left> || <right>
struct Or: public AbstractNode {
  AbstractNode* left;
  AbstractNode* right;

  inline ~Or() {
    delete left;
    delete right;
  }
};

// typeof <expression>
struct Typeof: public AbstractNode {
  AbstractNode* expression;

  inline ~Typeof() {
    delete expression;
  }
};

// <target> = <expression>
struct Assignment: public AbstractNode {
  AbstractNode* target;
  AbstractNode* expression;

  inline ~Assignment() {
    delete target;
    delete expression;
  }
};

// <target>(<arguments>)
struct Call: public AbstractNode {
  AbstractNode* target;
  NodeList arguments;

  inline ~Call() {
    delete target;
  }
};

// <identifier>
struct Identifier: public AbstractNode {
  std::string name;
};

// <target>.<identifier>
struct Member: public AbstractNode {
  AbstractNode* target;
  Identifier* identifier;

  inline ~Member() {
    delete target;
    delete identifier;
  }
};

// <target>[<argument>]
struct Index: public AbstractNode {
  AbstractNode* target;
  AbstractNode* argument;

  inline ~Index() {
    delete target;
    delete argument;
  }
};

// null
struct Null: public AbstractNode {};

// NAN
struct NAN: public AbstractNode {};

// "<value>"
//
// value is optional because we don't want to allocate any memory for an empty string
struct String: public AbstractNode {
  std::optional<std::string> value;
};

// <value>
struct Integer: public AbstractNode {
  int64_t value;
};

// <value>
struct Float: public AbstractNode {
  double value;
};

// <value>
struct Boolean: public AbstractNode {
  bool value;
};

// [<expressions>]
struct Array: public AbstractNode {
  NodeList expressions;
};

// {
//   <pairs>
// }
struct Hash: public AbstractNode {
  std::vector<std::pair<AbstractNode*, AbstractNode*>> pairs;

  inline ~Hash() {
    for (auto& pair : this->pairs) {
      delete pair.first;
      delete pair.second;
    }
  }
};

// func <name> (<parameters>) {
//   <body>
// }
//
// func (<parameters>) {
//   <body>
// }
//
// func {
//   <body>
// }
//
// ->(<parameters>) {
//   <body>
// }
//
// ->{
//   <body>
// }
//
// -><body>
struct Function: public AbstractNode {
  Identifier* name;
  NodeList parameters;
  NodeList body;
  bool anonymous;

  inline ~Function() {
    delete name;
  }
};

// property <identifier>;
struct PropertyDeclaration: public AbstractNode {
  Identifier* identifier;

  inline ~PropertyDeclaration() {
    delete identifier;
  }
};

// static <declaration>
struct StaticDeclaration: public AbstractNode {
  AbstractNode* declaration;

  inline ~StaticDeclaration() {
    delete declaration;
  }
};

// class {
//   <body>
// }
//
// class <name> {
//   <body>
// }
//
// class <name> extends <parents> {
//   <body>
// }
struct Class: public AbstractNode {
  Identifier* name;
  NodeList body;
  NodeList parents;

  inline ~Class() {
    delete name;
  }
};

// let <name>
//
// let <name> = <expression>
//
// const <name> = <expression>
struct LocalInitialisation: public AbstractNode {
  Identifier* name;
  AbstractNode* expression;
  bool constant;

  inline ~LocalInitialisation() {
    delete name;
    delete expression;
  }
};

// return
//
// return <expression>
struct Return: public AbstractNode {
  AbstractNode* expression;

  inline ~Return() {
    delete expression;
  }
};

// throw <expression>
struct Throw: public AbstractNode {
  AbstractNode* expression;

  inline ~Throw() {
    delete expression;
  }
};

// break
struct Break: public AbstractNode {};

// continue
struct Continue: public AbstractNode {};

// try {
//   <block>
// } catch (<exception_name>) {
//   <handler_block>
// }
struct TryCatch: public AbstractNode {
  NodeList block;
  Identifier* exception_name;
  NodeList handler_block;

  inline ~TryCatch() {
    delete exception_name;
  }
};
}
