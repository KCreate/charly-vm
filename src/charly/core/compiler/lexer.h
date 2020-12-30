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

#include <memory>

#include "charly/utils/string.h"
#include "charly/utils/buffer.h"
#include "charly/utils/vector.h"
#include "charly/core/compiler/token.h"

#pragma once

namespace charly::core::compiler {

class Lexer {
public:
  Lexer(const utils::string& filename,
        const utils::string& source)
    : m_filename(filename), m_source(source) {
  }

private:
  Lexer(Lexer&) = delete;
  Lexer(Lexer&&) = delete;

  void reset_token();
  void increment_row();
  void increment_column();
  void reset_column();

  utils::Buffer m_filename;
  utils::Buffer m_source;

  Token    m_token;
  Location m_location;

  utils::vector<Token> m_tokens;
};

}
