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

#include "value.h"

#pragma once

namespace Charly {
static const std::string kUnknownSymbol = "null";

class SymbolTable {
private:
  std::unordered_map<VALUE, std::string> table;

public:
  SymbolTable() = default;
  SymbolTable(const SymbolTable& other) : table(other.table) {
  }

  SymbolTable& operator=(SymbolTable& other) {
    if (this == &other)
      return *this;
    this->table.swap(other.table);
    return *this;
  }

  SymbolTable& operator=(SymbolTable&& other) {
    std::swap(this->table, other.table);
    return *this;
  }

  VALUE encode_string(const std::string& input);
  std::string decode_symbol(VALUE symbol);

  VALUE operator()(const std::string& input) {
    return this->encode_string(input);
  }
  std::string operator()(VALUE symbol) {
    return this->decode_symbol(symbol);
  }

  void copy_symbols_to_table(SymbolTable& other);
  inline void copy_symbols_from_table(SymbolTable& other) {
    other.copy_symbols_to_table(*this);
  }
};
}  // namespace Charly
