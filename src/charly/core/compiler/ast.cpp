/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

#include "charly/core/compiler/ast.h"
#include "charly/utils/buffer.h"
#include "charly/utils/colorwriter.h"

namespace charly::core::compiler::ast {

using Color = utils::Color;

void dump_string_with_escape_sequences(std::ostream& out, const char* begin, size_t length) {
  const char* begin_ptr = begin;
  const char* end = begin + length;

  uint32_t cp = 0;
  while (utf8::next(begin_ptr, end, cp)) {
    switch (cp) {
      case u'\a': {
        out << "\\a";
        break;
      }
      case u'\b': {
        out << "\\b";
        break;
      }
      case u'\f': {
        out << "\\f";
        break;
      }
      case u'\n': {
        out << "\\n";
        break;
      }
      case u'\r': {
        out << "\\r";
        break;
      }
      case u'\t': {
        out << "\\t";
        break;
      }
      case u'\v': {
        out << "\\v";
        break;
      }
      default: {
        utils::Buffer::write_utf8_cp_to_stream(out, cp);
        break;
      }
    }
  }
}

ref<Node> Node::search(const ref<Node>& node,
                       std::function<bool(const ref<Node>&)> compare,
                       std::function<bool(const ref<Node>&)> skip) {
  if (compare(node)) {
    return node;
  }

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

std::vector<ref<Node>> Node::search_all(const ref<Node>& node,
                                        std::function<bool(const ref<Node>&)> compare,
                                        std::function<bool(const ref<Node>&)> skip) {
  std::vector<ref<Node>> _result;
  search_all_impl(node, compare, skip, _result);
  return _result;
}

void Node::search_all_impl(const ref<Node>& node,
                           std::function<bool(const ref<Node>&)> compare,
                           std::function<bool(const ref<Node>&)> skip,
                           std::vector<ref<Node>>& result) {
  if (compare(node)) {
    result.push_back(node);
  }

  if (!skip(node)) {
    node->children([&](const ref<Node>& child) {
      Node::search_all_impl(child, compare, skip, result);
    });
  }
}

void Node::dump(std::ostream& out, bool print_location) const {
  utils::ColorWriter writer(out);

  Color name_color;
  switch (type()) {
    case Type::Function: {
      name_color = Color::Yellow;
      break;
    }
    case Type::Block: {
      name_color = Color::Red;
      break;
    }
    default: {
      name_color = Color::Blue;
      break;
    }
  }

  writer.fg(name_color, node_name());

  dump_info(out);

  auto truthy = truthyness();
  if (truthy == Truthyness::True) {
    writer.fg(Color::Green, " Truthy");
  } else if (truthy == Truthyness::False) {
    writer.fg(Color::Red, " Falsey");
  }

  auto sideeffects = has_side_effects();
  if (!sideeffects) {
    writer.fg(Color::Green, " side-effect-free");
  }

  if (print_location) {
    writer << " <" << location() << ">";
  }

  writer << '\n';

  std::vector<utils::Buffer> child_nodes;
  children([&](const ref<Node>& node) {
    utils::Buffer& child_stream = child_nodes.emplace_back();
    termcolor::colorlike(child_stream, out);
    node->dump(child_stream, print_location);
  });

  for (size_t i = 0; i < child_nodes.size(); i++) {
    utils::Buffer& child_stream = child_nodes.at(i);
    std::string line;
    bool first_line = true;
    while (std::getline(child_stream, line)) {
      if (!line.empty()) {
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
}

// NOTE: Update this method together with RawValue::op_equal
Truthyness Node::compares_equal(const ref<Node>& other) const {
  auto self_t = type();
  auto other_t = other->type();

  if (self_t == Type::Bool || other_t == Type::Bool) {
    return truthyness_equal(truthyness(), other->truthyness());
  }

  if (self_t == Type::Null && other_t == Type::Null) {
    return Truthyness::True;
  }

  // int <-> float comparisons
  if (self_t == Type::Int) {
    if (other_t == Type::Int) {
      auto self_int = cast<const Int>(shared_from_this());
      auto other_int = cast<Int>(other);
      return truthyness_create(self_int->value == other_int->value);
    } else if (other_t == Type::Float) {
      auto self_int = cast<const Int>(shared_from_this());
      auto other_float = cast<Float>(other);
      return truthyness_create((double)self_int->value == other_float->value);
    } else {
      return Truthyness::Unknown;
    }
  } else if (self_t == Type::Float) {
    if (other_t == Type::Int) {
      auto self_float = cast<const Float>(shared_from_this());
      auto other_int = cast<Int>(other);
      return truthyness_create(self_float->value == (double)other_int->value);
    } else if (other_t == Type::Float) {
      auto self_float = cast<const Float>(shared_from_this());
      auto other_float = cast<Float>(other);
      return truthyness_create(self_float->value == other_float->value);
    } else {
      return Truthyness::Unknown;
    }
  }

  // string <-> symbol comparisons
  if (self_t == Type::String) {
    if (other_t == Type::String) {
      auto self_sym = cast<const String>(shared_from_this());
      auto other_sym = cast<String>(other);
      return truthyness_create(self_sym->value == other_sym->value);
    } else if (other_t == Type::Symbol) {
      auto self_sym = cast<const String>(shared_from_this());
      auto other_sym = cast<Symbol>(other);
      return truthyness_create(self_sym->value == other_sym->value);
    } else {
      return Truthyness::Unknown;
    }
  } else if (self_t == Type::Symbol) {
    if (other_t == Type::String) {
      auto self_sym = cast<const Symbol>(shared_from_this());
      auto other_sym = cast<String>(other);
      return truthyness_create(self_sym->value == other_sym->value);
    } else if (other_t == Type::Symbol) {
      auto self_sym = cast<const Symbol>(shared_from_this());
      auto other_sym = cast<Symbol>(other);
      return truthyness_create(self_sym->value == other_sym->value);
    } else {
      return Truthyness::Unknown;
    }
  }

  // known false comparisons
  switch (self_t) {
    case Type::Symbol:
    case Type::String: {
      switch (other_t) {
        case Type::Null:
        case Type::Int:
        case Type::Float: return Truthyness::False;
        default: break;
      }
    }
    case Type::Int:
    case Type::Float: {
      if (other_t == Type::Null)
        return Truthyness::False;
      break;
    }
    default: break;
  }

  return Truthyness::Unknown;
}

void Block::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  if (this->repl_toplevel_block) {
    writer << ' ';
    writer.fg(Color::Red, "REPL");
  }
  if (this->allows_break) {
    writer << ' ';
    writer.fg(Color::Red, "breakable");
  }
}

void FarSelf::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Yellow, (uint32_t)this->depth);
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

void String::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.set_fg_color(Color::Yellow);
  writer << "\"";
  dump_string_with_escape_sequences(out, this->value);
  writer << "\"";
  writer.reset_color();
}

void Symbol::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Yellow, ":", this->value);
}

bool Tuple::assignable() const {
  if (elements.empty())
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

bool Tuple::has_spread_elements() const {
  for (const auto& exp : this->elements) {
    if (isa<Spread>(exp))
      return true;
  }

  return false;
}

bool List::has_spread_elements() const {
  for (const auto& exp : this->elements) {
    if (isa<Spread>(exp))
      return true;
  }

  return false;
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

bool Dict::has_spread_elements() const {
  for (const ref<DictEntry>& exp : this->elements) {
    if (isa<Spread>(exp->key)) {
      DCHECK(exp->value.get() == nullptr);
      return true;
    }
  }

  return false;
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

  writer << ' ';
  writer.fg(Color::Red, ir_info);
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
    writer.fg(Color::Red, "spread");
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

bool CallOp::has_spread_elements() const {
  for (const auto& exp : this->arguments) {
    if (isa<Spread>(exp))
      return true;
  }

  return false;
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
  writer.fg(Color::Red, ir::kBuiltinNames[(uint16_t)this->operation]);
}

}  // namespace charly::core::compiler::ast
