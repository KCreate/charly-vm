/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2021 Leonard Schütz
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
#include <functional>
#include <memory>

#include "charly/utils/map.h"
#include "charly/utils/vector.h"
#include "charly/utils/string.h"
#include "charly/core/compiler/token.h"
#include "charly/core/compiler/location.h"

#pragma once

namespace charly::core::compiler::ast {

template <typename T>
using ref = std::shared_ptr<T>;

template <typename T, typename... Args>
inline ref<T> make(Args... params) {
  return std::make_shared<T>(std::forward<Args>(params)...);
}

#define NAME(N)        \
  const char* name() { \
    return #N;         \
  }

struct Node;
using VisitorCallback = std::function<void(ref<Node>)>;

struct Node : std::enable_shared_from_this<Node> {
  LocationRange location;

  // set whole location
  void at(Location& loc)  { location.begin = loc; location.end = loc; }
  void at(ref<Node> node) { location = node->location; }

  // set begin location
  void begin(Location& loc)      { location.begin = loc; }
  void begin(LocationRange& loc) { location.begin = loc.begin; }
  void begin(ref<Node> node)     { location.begin = node->location.begin; }

  // set end location
  void end(Location& loc)      { location.end = loc; }
  void end(LocationRange& loc) { location.end = loc.end; }
  void end(ref<Node> node)     { location.end = node->location.end; }

  // dump this node to the output stream
  void dump(std::ostream& output, uint32_t depth = 0) {
    // indent
    for (int i = 0; i < depth; i++) {
      output << "  ";
    }

    output << '-';
    output << this->name();
    this->details(output);
    this->location.dump(output, false);
    output << '\n';

    this->children([&](std::shared_ptr<Node> node) {
      node->dump(output, depth + 1);
    });
  }

  // return the name of this node
  virtual const char* name() = 0;

  // write the details of this node into the output stream
  virtual void details(std::ostream& output) {
    output << " ";
  }

  // call the callback with all child nodes
  virtual void children(VisitorCallback cb) {}

  // checks wether a node is of a certain type
  template <typename T>
  bool is() {
    return dynamic_cast<T*>(this) != nullptr;
  }

  template <typename T>
  ref<T> as() {
    return std::dynamic_pointer_cast<T>(this->shared_from_this());
  }
};

struct Statement : public Node {};

struct Block : public Statement {
  NAME(Block);

  utils::vector<ref<Statement>> statements;

  void add_statement(ref<Statement> statement) {
    statements.push_back(statement);
  }

  Block() {}
  Block(utils::vector<ref<Statement>>&& statements) : statements(statements) {}

  void children(VisitorCallback cb) {
    for (ref<Statement>& node : statements) {
      cb(node);
    }
  }
};

struct Program : public Node {
  NAME(Program);

  utils::string filename;
  ref<Block> block;

  void set_block(ref<Block> block) {
    this->block = block;
    this->at(block);
  }

  Program(const utils::string& filename) : filename(filename) {}
  Program(const utils::string& filename, ref<Block> block) : filename(filename), block(block) {
    this->at(block);
  }

  void details(std::ostream& io) {
    io << " " << filename << " ";
  }

  void children(VisitorCallback cb) {
    cb(block);
  }
};

struct ControlStructure : public Statement {};

struct Expression : public Statement {};

struct Identifier : public Expression {
  NAME(Identifier);

  utils::string value;

  Identifier(const utils::string& value) : value(value) {}

  void details(std::ostream& io) {
    io << " " << value << " ";
  }
};

struct Int : public Expression {
  NAME(Int);

  int64_t value;

  Int(int64_t value) : value(value) {}

  void details(std::ostream& io) {
    io << " " << value << " ";
  }
};

struct Float : public Expression {
  NAME(Float);

  double value;

  Float(double value) : value(value) {}

  void details(std::ostream& io) {
    const double d = io.precision();
    io.precision(4);
    io << std::fixed;
    io << " " << value << " ";
    io << std::scientific;
    io.precision(d);
  }
};

struct Boolean : public Expression {
  NAME(Boolean);

  bool value;

  Boolean(bool value) : value(value) {}

  void details(std::ostream& io) {
    io << " " << (value ? "true" : "false") << " ";
  }
};

struct String : public Expression {
  NAME(String);

  utils::string value;

  String(const utils::string& value) : value(value) {}

  void details(std::ostream& io) {
    io << " \"" << value << "\" ";
  }
};

struct FormatString : public Expression {
  NAME(FormatString);

  utils::vector<ref<Expression>> parts;

  void add_part(ref<Expression> part) {
    parts.push_back(part);
    this->end(part);
  }

  FormatString() {}
  FormatString(utils::vector<ref<Expression>>&& parts) : parts(parts) {
    if (parts.size()) {
      this->begin(parts.front());
      this->end(parts.back());
    }
  }

  void children(VisitorCallback cb) {
    for (ref<Expression> node : parts) {
      cb(node);
    }
  }
};

struct Null : public Expression {
  NAME(Null);
};

struct Self : public Expression {
  NAME(Self);
};

struct Super : public Expression {
  NAME(Super);
};

struct Tuple : public Expression {
  NAME(Tuple);

  utils::vector<ref<Expression>> elements;

  void add_element(ref<Expression> node) {
    elements.push_back(node);
    this->end(node);
  }

  Tuple() {}
  Tuple(utils::vector<ref<Expression>>&& elements) : elements(elements) {}

  void children(VisitorCallback cb) {
    for (ref<Expression>& node : elements) {
      cb(node);
    }
  }
};

#undef NAME

}
