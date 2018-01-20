/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2018 Leonard Schütz
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

#include "symboltable.h"

namespace Charly {
VALUE SymbolTable::encode_string(const std::string& input) {
  size_t hashvalue = std::hash<std::string>{}(input);

  VALUE symbol = static_cast<VALUE>((hashvalue & ~kSymbolMask) | kSymbolFlag);

  if (this->table.find(symbol) == this->table.end()) {
    this->table.insert({symbol, input});
  }

  return symbol;
}

std::optional<std::string> SymbolTable::decode_symbol(VALUE symbol) {
  std::optional<std::string> decoded_string;

  auto found_string = this->table.find(symbol);

  if (found_string != this->table.end()) {
    decoded_string = found_string->second;
  }

  return decoded_string;
}

void SymbolTable::copy_symbols_to_table(SymbolTable& other) {
  other.table.insert(this->table.begin(), this->table.end());
}
}  // namespace Charly
