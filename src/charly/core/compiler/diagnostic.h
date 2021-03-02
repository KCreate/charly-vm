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

#include <exception>
#include <string>
#include <sstream>
#include <vector>

#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/location.h"
#include "charly/utils/buffer.h"
#include "charly/utils/colorwriter.h"

#pragma once

using Color = charly::utils::Color;

namespace charly::core::compiler {

enum DiagnosticType : uint8_t { DiagnosticInfo, DiagnosticWarning, DiagnosticError };

struct DiagnosticMessage {
  DiagnosticType type;
  std::string filepath;
  std::string message;
  Location location;

  utils::Color format_color() const {
    switch (type) {
      case DiagnosticInfo: {
        return utils::Color::Blue;
        break;
      }
      case DiagnosticWarning: {
        return utils::Color::Yellow;
        break;
      }
      case DiagnosticError: {
        return utils::Color::Red;
        break;
      }
    }
  }

  // write a formatted version of this error to the stream:
  //
  // <filepath>:<row>:<col>: <message>
  friend std::ostream& operator<<(std::ostream& out, const DiagnosticMessage& message) {
    utils::ColorWriter writer(out);

    writer << message.filepath << ':';

    if (message.location.valid) {
      writer << message.location << ':';
    }

    writer << ' ';

    switch (message.type) {
      case DiagnosticInfo: {
        writer.fg(message.format_color(), "info:");
        break;
      }
      case DiagnosticWarning: {
        writer.fg(message.format_color(), "warning:");
        break;
      }
      case DiagnosticError: {
        writer.fg(message.format_color(), "error:");
        break;
      }
    }

    writer << ' ' << message.message;

    return out;
  }
};

class DiagnosticException : std::exception {
  const char* what() const throw() {
    return "Fatal Diagnostic Exception";
  }
};

class DiagnosticConsole {
public:
  DiagnosticConsole(const std::string& filepath, const utils::Buffer& buffer);

  const std::vector<DiagnosticMessage>& messages() const {
    return m_messages;
  }

  bool has_errors();

  // write all diagnostic messages to the out stream
  void dump_all(std::ostream& out) const;

  // push message of specific type into the console
  template <typename... Args>
  void info(const Location& loc, Args&&... params) {
    std::stringstream stream;
    ((stream << std::forward<Args>(params)), ...);
    m_messages.push_back(DiagnosticMessage{ DiagnosticInfo, m_filepath, stream.str(), loc });
  }
  template <typename... Args>
  void info(const ast::ref<ast::Node>& node, Args&&... params) {
    info(node->location(), std::forward<Args>(params)...);
  }

  template <typename... Args>
  void warning(const Location& loc, Args&&... params) {
    std::stringstream stream;
    ((stream << std::forward<Args>(params)), ...);
    m_messages.push_back(DiagnosticMessage{ DiagnosticWarning, m_filepath, stream.str(), loc });
  }
  template <typename... Args>
  void warning(const ast::ref<ast::Node>& node, Args&&... params) {
    warning(node->location(), std::forward<Args>(params)...);
  }

  template <typename... Args>
  void error(const Location& loc, Args&&... params) {
    std::stringstream stream;
    ((stream << std::forward<Args>(params)), ...);
    m_messages.push_back(DiagnosticMessage{ DiagnosticError, m_filepath, stream.str(), loc });
  }
  template <typename... Args>
  void error(const ast::ref<ast::Node>& node, Args&&... params) {
    error(node->location(), std::forward<Args>(params)...);
  }

  // pushing a fatal message throws a DiagnosticException
  template <typename... Args>
  [[noreturn]] void fatal(const Location& loc, Args&&... params) {
    std::stringstream stream;
    ((stream << std::forward<Args>(params)), ...);
    m_messages.push_back(DiagnosticMessage{ DiagnosticError, m_filepath, stream.str(), loc });
    throw DiagnosticException();
  }
  template <typename... Args>
  [[noreturn]] void fatal(const ast::ref<ast::Node>& node, Args&&... params) {
    fatal(node->location(), std::forward<Args>(params)...);
  }

private:
  DiagnosticConsole(DiagnosticConsole&) = delete;
  DiagnosticConsole(DiagnosticConsole&&) = delete;

  // writes the section of the source code to which location
  // refers to to the out stream
  void write_annotated_source(std::ostream& out, const DiagnosticMessage& message) const;

  std::string m_filepath;
  std::vector<std::string> m_source;
  std::vector<DiagnosticMessage> m_messages;
};

}  // namespace charly::core::compiler
