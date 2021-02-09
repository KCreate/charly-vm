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

void Id::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Yellow, this->value);
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
}

void Function::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';

  if (this->arrow_function) {
    writer.fg(Color::Cyan, "anonymous");
  } else {
    writer.fg(Color::Green, this->name->value);
  }
}

void Class::dump_info(std::ostream& out) const {
  utils::ColorWriter writer(out);
  writer << ' ';
  writer.fg(Color::Green, this->name->value);
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

void MemberOp::dump_info(std::ostream& out) const {
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
}

}  // namespace charly::core::compiler
