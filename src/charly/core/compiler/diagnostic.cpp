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

#include <algorithm>
#include <iomanip>

#include "charly/core/compiler/diagnostic.h"

using Color = charly::utils::Color;

namespace charly::core::compiler {

static const uint32_t kDiagnosticContextRows = 3;

DiagnosticConsole::DiagnosticConsole(const std::string& filepath, const utils::Buffer& buffer) : m_filepath(filepath) {
  std::string source = buffer.str();
  int begin = 0;
  int end;
  while ((end = source.find('\n', begin)) >= 0) {
    m_source.push_back(source.substr(begin, end - begin));
    begin = end + 1;
  }

  m_source.push_back(source.substr(begin));
}

bool DiagnosticConsole::has_errors() {
  return std::any_of(m_messages.begin(), m_messages.end(), [](auto& message) {
    return message.type == DiagnosticType::Error;
  });
}

void DiagnosticConsole::dump_all(std::ostream& out) const {
  utils::ColorWriter writer(out);

  size_t i = 0;
  for (const DiagnosticMessage& message : m_messages) {
    writer << message << '\n';

    if (message.location.valid) {
      write_annotated_source(out, message);
    }

    if (i < m_messages.size() - 1) {
      writer << '\n';
    }

    i++;
  }
}

void DiagnosticConsole::write_annotated_source(std::ostream& out, const DiagnosticMessage& message) const {
  utils::ColorWriter writer(out);

  const Location& location = message.location;
  if (!message.location.valid) {
    return;
  }

  // the first row to be printed
  uint32_t first_printed_row = kDiagnosticContextRows > location.row ? 0 : location.row - kDiagnosticContextRows;
  uint32_t last_printed_row = location.end_row + kDiagnosticContextRows;

  size_t offset = 0;
  uint32_t row = 0;
  for (const std::string& line : m_source) {
    bool contains_annotation = row >= location.row && row <= location.end_row;

    // skip lines outside the range of the location
    if (row < first_printed_row || row > last_printed_row) {
      row++;
      offset += line.size() + 1;
      continue;
    }

    if (contains_annotation) {

      // different indicator for multi-row highlights
      if (location.row != location.end_row) {
        writer << " |> ";
      } else {
        writer << " -> ";
      }
    } else {
      writer << "    ";
    }

    Color highlight_color = message.format_color();

    // print the line number
    writer << std::right;
    if (contains_annotation) {
      writer.fg(Color::White, std::setw(4), row + 1);
    } else {
      writer.fg(Color::Grey, std::setw(4), row + 1);
    }
    writer << std::left;

    // divider between line number and source code
    writer << " | ";

    if (contains_annotation) {
      if (row == location.row && location.row == location.end_row) {
        size_t annotate_start_offset = location.offset - offset;
        size_t annotate_end_offset = location.end_offset - offset;
        size_t annotate_length = annotate_end_offset - annotate_start_offset;

        writer << line.substr(0, annotate_start_offset);
        writer.bg(highlight_color, line.substr(annotate_start_offset, annotate_length));
        writer << line.substr(annotate_end_offset);
      } else {
        // check if we are on the first or last line
        if (row == location.row) {
          writer.write(line.substr(0, location.offset - offset));
          writer.bg(highlight_color, line.substr(location.offset - offset));
        } else if (row == location.end_row) {
          writer.bg(highlight_color, line.substr(0, location.end_offset - offset));
          writer.write(line.substr(location.end_offset - offset));
        } else {
          writer.bg(highlight_color, line);
        }
      }
    } else {
      writer.fg(Color::Grey, line);
    }
    writer << '\n';

    row++;
    offset += line.size() + 1;
  }
}

}  // namespace charly::core::compiler
