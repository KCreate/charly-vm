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

#include <iomanip>
#include <algorithm>

#include "charly/core/compiler/diagnostic.h"

namespace charly::core::compiler {

DiagnosticConsole::DiagnosticConsole(const std::string& filename, const utils::Buffer& buffer) : m_filename(filename) {
  int begin = 0;
  int end = 0;

  std::string source = buffer.buffer_string();
  while ((end = source.find('\n', begin)) >= 0) {
    m_source.push_back(source.substr(begin, end - begin));
    begin = end + 1;
  }

  m_source.push_back(source.substr(begin));
}

bool DiagnosticConsole::has_errors() {
  for (DiagnosticMessage& message : m_messages) {
    if (message.type == DiagnosticError) {
      return true;
    }
  }

  return false;
}

void DiagnosticConsole::dump_all(std::ostream& out) const {
  utils::ColorWriter writer(out);

  size_t i = 0;
  for (const DiagnosticMessage& message : m_messages) {
    writer.write(m_filename);
    writer.write(":");
    writer.write(message);
    writer.write('\n');

    if (message.location.valid) {
      write_annotated_source(out, message.type, message.location);
    }

    if (i < m_messages.size() - 1) {
      writer.write('\n');
    }

    i++;
  }
}

void DiagnosticConsole::write_annotated_source(std::ostream& out, DiagnosticType type, const Location& location) const {
  utils::ColorWriter writer(out);

  // the first row to be printed
  uint32_t first_printed_row = kContextRows > location.row ? 0 : location.row - kContextRows;
  uint32_t last_printed_row = location.end_row + kContextRows;

  size_t offset = 0;
  uint32_t row = 0;
  for (const std::string& line : m_source) {
    bool contains_annotation = row >= location.row && row <= location.end_row;

    // skip lines outside the range of the location
    if (row < first_printed_row || row > last_printed_row) {
      offset += line.size() + 1;
      row++;
      continue;
    }

    writer.write("    ");

    // print the line number
    writer.write(std::right);
    if (contains_annotation) {
      switch (type) {
        case DiagnosticError: {
          writer.red(std::setw(4), row + 1);
          break;
        }
        case DiagnosticWarning: {
          writer.yellow(std::setw(4), row + 1);
          break;
        }
        default: {
          writer.blue(std::setw(4), row + 1);
          break;
        }
      }
    } else {
      writer.write("    ");
    }
    writer.write(std::left);

    // divider between line number and source code
    writer.write(" | ");

    if (contains_annotation) {
      if (row == location.row && location.row == location.end_row) {
        size_t annotate_start_offset = location.offset - offset;
        size_t annotate_end_offset = location.end_offset - offset;
        size_t annotate_length = annotate_end_offset - annotate_start_offset;

        writer.write(line.substr(0, annotate_start_offset));

        switch (type) {
          case DiagnosticError: {
            writer.red_bg(line.substr(annotate_start_offset, annotate_length));
            break;
          }
          case DiagnosticWarning: {
            writer.yellow_bg(line.substr(annotate_start_offset, annotate_length));
            break;
          }
          default: {
            writer.blue_bg(line.substr(annotate_start_offset, annotate_length));
            break;
          }
        }

        if (annotate_end_offset < line.size())
          writer.write(line.substr(annotate_end_offset));
      } else {

        // check if we are on the first or last line
        if (row == location.row) {
          writer.write(line.substr(0, location.offset - offset));
          writer.red_bg(line.substr(location.offset - offset));
        } else if (row == location.end_row) {
          writer.red_bg(line.substr(0, location.end_offset - offset));
          writer.write(line.substr(location.end_offset - offset));
        } else {
          writer.red_bg(line);
        }
      }
    } else {
      writer.grey(line);
    }

    writer.write("\n");

    row++;
    offset += line.size() + 1;
  }
}

void DiagnosticConsole::info(const std::string& message, const Location& location) {
  m_messages.push_back(DiagnosticMessage{DiagnosticInfo, message, location});
}

void DiagnosticConsole::warning(const std::string& message, const Location& location) {
  m_messages.push_back(DiagnosticMessage{DiagnosticWarning, message, location});
}

void DiagnosticConsole::error(const std::string& message, const Location& location) {
  m_messages.push_back(DiagnosticMessage{DiagnosticError, message, location});
}

void DiagnosticConsole::fatal(const std::string& message, const Location& location) {
  m_messages.push_back(DiagnosticMessage{DiagnosticError, message, location});
  throw DiagnosticException();
}

}  // namespace charly::core::compiler
