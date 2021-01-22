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

#include <vector>
#include <string>
#include <exception>

#include "charly/utils/buffer.h"
#include "charly/utils/colorwriter.h"
#include "charly/core/compiler/location.h"

#pragma once

namespace charly::core::compiler {

enum DiagnosticType : uint8_t {
  DiagnosticInfo,
  DiagnosticWarning,
  DiagnosticError
};

struct DiagnosticMessage {
  DiagnosticType type;
  std::string message;
  Location location;

  // write a formatted version of this error to the stream:
  //
  // <filename>:<row>:<col>: <message>
  friend std::ostream& operator<<(std::ostream& out, const DiagnosticMessage& message) {
    utils::ColorWriter writer(out);

    if (message.location.valid) {
      writer.write(message.location);
      writer.write(':');
    }

    writer.write(' ');

    switch (message.type) {
      case DiagnosticInfo: {
        writer.blue("info: ");
        break;
      }
      case DiagnosticWarning: {
        writer.yellow("warning: ");
        break;
      }
      case DiagnosticError: {
        writer.red("error: ");
        break;
      }
    }

    writer.write(message.message);

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
  DiagnosticConsole(const std::string& filename, const utils::Buffer& buffer);

  const std::vector<DiagnosticMessage>& messages() const {
    return m_messages;
  }

  bool has_errors();

  // write all diagnostic messages to the out stream
  void dump_all(std::ostream& out) const;

  // push message of specific type into the console
  void info(const std::string& msg, const Location& loc = Location{ .valid = false });
  void warning(const std::string& msg, const Location& loc = Location{ .valid = false });
  void error(const std::string& msg, const Location& loc = Location{ .valid = false });

  // pushing a fatal message throws a DiagnosticException
  [[noreturn]] void fatal(const std::string& msg, const Location& loc);

private:
  static const uint32_t kContextRows = 2;

  DiagnosticConsole(DiagnosticConsole&) = delete;
  DiagnosticConsole(DiagnosticConsole&&) = delete;

  // writes the section of the source code to which location
  // refers to to the out stream
  void write_annotated_source(std::ostream& out, DiagnosticType type, const Location& location) const;

  std::string m_filename;
  std::vector<std::string> m_source;
  std::vector<DiagnosticMessage> m_messages;
};

}  // namespace charly::core::compiler
