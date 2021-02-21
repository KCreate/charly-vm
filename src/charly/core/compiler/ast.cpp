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

#include <sstream>
#include <vector>

#include "charly/utils/buffer.h"
#include "charly/core/compiler/ast.h"
#include "charly/utils/colorwriter.h"

namespace charly::core::compiler::ast {

using Color = utils::Color;

ref<Node> Node::search(const ref<Node>& node,
                        std::function<bool(const ref<Node>&)> compare,
                        std::function<bool(const ref<Node>&)> skip) {

  // check compare function
  if (compare(node)) {
    return node;
  }

  // skip certain node's children
  ref<Node> result = nullptr;
  if (!skip(node)) {
    node->children([&](const ref<Node>& child) {
      if (result.get())
        return;

      result = Node::search(child, compare, skip);
    });
  }

  return result;
}

void Node::dump(std::ostream& out, bool print_location) const {
  utils::ColorWriter writer(out);

  writer.fg(Color::Blue, node_name());

  dump_info(out);
  if (print_location) {
    writer << " <" << location() << ">";
  }
  writer << '\n';

  std::vector<std::stringstream> child_nodes;
  children([&](const ref<Node>& node) {
    child_nodes.emplace_back(std::stringstream{});
    std::stringstream& child_stream = child_nodes.back();
    if (termcolor::_internal::is_colorized(out)) {
      termcolor::colorize(child_stream);
    }
    node->dump(child_stream, print_location);
  });

  for (size_t i = 0; i < child_nodes.size(); i++) {
    auto& child_stream = child_nodes.at(i);

    std::string line;
    bool first_line = true;
    while (std::getline(child_stream, line)) {
      if (line.size()) {
        if (i != child_nodes.size() - 1) {
          if (first_line) {
            writer << "├─" << line << '\n';
          } else {
            writer << "│ " << line << '\n';
          }
        } else {
          if (first_line) {
            writer << "└─" << line << '\n';
          } else {
            writer << "  " << line << '\n';
          }
        }
      }

      first_line = false;
    }
  }

  writer << '\n';
}

void Block::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  if (this->needs_locals_table) {
    writer << ' ';
    writer.fg(Color::Red, "needs_locals_table");
  }
}

void Id::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Yellow, this->value);
  if (this->ir_location.valid()) {
    writer << ' ';
    writer.fg(Color::Magenta, this->ir_location);
  }
}

void Name::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Green, this->value);
}

void Int::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Red, this->value);
}

void Float::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Red, this->value);
}

void Bool::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Red, this->value ? "true" : "false");
}

void Char::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Red, '\'', utils::Buffer::u8(this->value), '\'');
}

void String::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Yellow, '\"', this->value, '\"');
}

bool Tuple::assignable() const {
  if (elements.size() == 0)
    return false;

  bool spread_passed = false;
  for (const ref<Expression>& node : elements) {
    if (isa<Name>(node)) {
      continue;
    }

    if (ref<Spread> spread = cast<Spread>(node)) {
      if (spread_passed)
        return false;

      spread_passed = true;

      if (!isa<Name>(spread->expression)) {
        return false;
      }

      continue;
    }

    return false;
  }

  return true;
}

bool DictEntry::assignable() const {
  if (value.get())
    return false;

  if (isa<Name>(key))
    return true;

  if (ref<Spread> spread = cast<Spread>(key)) {
    return isa<Name>(spread->expression);
  }

  return false;
}

bool Dict::assignable() const {
  if (elements.size() == 0)
    return false;

  bool spread_passed = false;
  for (const ref<DictEntry>& node : elements) {
    if (!node->assignable())
      return false;

    if (isa<Spread>(node->key)) {
      if (spread_passed)
        return false;
      spread_passed = true;
    }
  }

  return true;
}

void FunctionArgument::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  if (this->spread_initializer) {
    writer.fg(Color::Green, "...");
  }
  if (this->self_initializer) {
    writer.fg(Color::Green, "@");
  }
  writer.fg(Color::Green, this->name->value);
  if (this->ir_location.valid()) {
    writer << ' ';
    writer.fg(Color::Magenta, this->ir_location);
  }
}

void Function::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';

  if (this->arrow_function) {
    writer.fg(Color::Cyan, "anonymous");
  } else {
    writer.fg(Color::Green, this->name->value);
  }

  if (this->ir_info.valid) {
    writer << ' ';
    writer.fg(Color::Magenta, this->ir_info);
  }
}

void ClassProperty::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  if (this->is_static) {
    writer << ' ';
    writer.fg(Color::Red, "static");
  }
  writer << ' ';
  writer.fg(Color::Yellow, this->name->value);
}

void Class::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Green, this->name->value);
}

void MemberOp::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Green, this->member->value);
}

void UnpackTargetElement::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  if (this->spread) {
    writer.fg(Color::Green, "...");
  }
  writer.fg(Color::Green, this->name->value);
  if (this->ir_location.valid()) {
    writer << ' ';
    writer.fg(Color::Magenta, this->ir_location);
  }
}

UnpackTarget::UnpackTarget(const ref<Node>& node) {
  switch (node->type()) {
    case Node::Type::Tuple: {
      ref<Tuple> tuple = cast<Tuple>(node);
      object_unpack = false;

      for (const ref<Expression>& element : tuple->elements) {
        switch (element->type()) {
          case Node::Type::Name: {
            this->elements.push_back(make<UnpackTargetElement>(cast<Name>(element), false));
            break;
          }
          case Node::Type::Spread: {
            ref<Spread> spread = cast<Spread>(element);
            this->elements.push_back(make<UnpackTargetElement>(cast<Name>(spread->expression), true));
            break;
          }
          default: {
            assert(false && "unexpected type");
            break;
          }
        }
      }

      break;
    }
    case Node::Type::Dict: {
      ref<Dict> dict = cast<Dict>(node);
      object_unpack = true;

      for (const ref<DictEntry>& entry : dict->elements) {
        const ref<Expression>& element = entry->key;

        switch (element->type()) {
          case Node::Type::Name: {
            this->elements.push_back(make<UnpackTargetElement>(cast<Name>(element), false));
            break;
          }
          case Node::Type::Spread: {
            ref<Spread> spread = cast<Spread>(element);
            this->elements.push_back(make<UnpackTargetElement>(cast<Name>(spread->expression), true));
            break;
          }
          default: {
            assert(false && "unexpected type");
            break;
          }
        }
      }

      break;
    }
    default: {
      assert(false && "unexpected type");
      break;
    }
  }
}

void UnpackTarget::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  if (this->object_unpack) {
    writer.fg(Color::Red, "object-unpack");
  } else {
    writer.fg(Color::Red, "sequence-unpack");
  }
}

void Assignment::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  if (this->operation != TokenType::Assignment) {
    writer << ' ';
    writer.fg(Color::Yellow, kTokenTypeStrings[static_cast<int>(this->operation)]);
  }
}

void MemberAssignment::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Green, this->member->value);
  if (this->operation != TokenType::Assignment) {
    writer << ' ';
    writer.fg(Color::Yellow, kTokenTypeStrings[static_cast<int>(this->operation)]);
  }
}

void IndexAssignment::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  if (this->operation != TokenType::Assignment) {
    writer << ' ';
    writer.fg(Color::Yellow, kTokenTypeStrings[static_cast<int>(this->operation)]);
  }
}

void BinaryOp::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Yellow, kTokenTypeStrings[static_cast<int>(this->operation)]);
}

void UnaryOp::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Blue, kTokenTypeStrings[static_cast<int>(this->operation)]);
}

void CallMemberOp::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Green, this->member->value);
}

void Declaration::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  if (this->constant) {
    writer << ' ';
    writer.fg(Color::Red, "const");
  }
  writer << ' ';
  writer.fg(Color::Green, this->name->value);
  if (this->ir_location.valid()) {
    writer << ' ';
    writer.fg(Color::Magenta, this->ir_location);
  }
}

void UnpackDeclaration::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  if (this->constant) {
    writer << ' ';
    writer.fg(Color::Red, "const");
  }
}

void Try::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Yellow, this->exception_name->value);
  if (this->exception_name->ir_location.valid()) {
    writer << ' ';
    writer.fg(Color::Magenta, this->exception_name->ir_location);
  }
}

void BuiltinOperation::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Red, this->name);
}

}  // namespace charly::core::compiler
