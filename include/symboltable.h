/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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

#include <optional>
#include <thread>
#include <iostream>

#include "value.h"

#pragma once

namespace Charly {

const std::string kUndefinedSymbolString = "<undefined symbol>";

// Manages global symbol table contents,
// provides thread-safe access to it
class SymbolTable {
public:

  // Global symbol table
  inline static std::unordered_map<VALUE, std::string> global_symbol_table;

  // Synchronisation
  inline static std::mutex global_symbol_table_mutex;

  /*
   * Create a symbol via an input string
   * */
  inline static VALUE encode(const std::string& input) {
    VALUE symbol = charly_create_symbol(input);

    // Register previously unseen symbols in the global table
    {
      std::unique_lock<std::mutex> lk(global_symbol_table_mutex);
      if (SymbolTable::global_symbol_table.count(symbol) == 0)
        SymbolTable::global_symbol_table.insert({symbol, input});
    }


    return symbol;
  }

  /*
   * Create a symbol via a charly VALUE
   * */
  inline static VALUE encode(VALUE value) {
    uint8_t type = charly_get_type(value);
    switch (type) {

      case kTypeString: {
        char* str_data = charly_string_data(value);
        uint32_t str_length = charly_string_length(value);
        return SymbolTable::encode(std::string(str_data, str_length));
      }

      case kTypeNumber: {
        if (charly_is_float(value)) {
          double val = charly_double_to_double(value);
          std::string str = std::to_string(val);
          return SymbolTable::encode(str);
        } else {
          int64_t val = charly_int_to_uint64(value);
          std::string str = std::to_string(val);
          return SymbolTable::encode(str);
        }
      }

      case kTypeBoolean: {
        return SymbolTable::encode(value == kTrue ? "true" : "false");
      }

      case kTypeNull: {
        return SymbolTable::encode("null");
      }

      case kTypeSymbol: {
        return SymbolTable::encode(SymbolTable::decode(value));
      }

      default: {
        static const std::string symbol_type_symbol_table[] = {
          "<dead>",      "<class>",     "<object>", "<array>",      "<string>",   "<function>",
          "<cfunction>", "<generator>", "<frame>",  "<catchtable>", "<cpointer>", "<number>",
          "<boolean>",   "<null>",      "<symbol>", "<unknown>"
        };

        const std::string& symbol = symbol_type_symbol_table[type];
        return SymbolTable::encode(symbol);
      }
    }
  }

  /*
   * Returns an optional containing the decoded value of this symbol
   * */
  inline static std::optional<std::string> decode_optional(VALUE symbol) {
    std::optional<std::string> decoded_string;

    {
      std::unique_lock<std::mutex> lk(global_symbol_table_mutex);
      auto found_string = SymbolTable::global_symbol_table.find(symbol);
      if (found_string != SymbolTable::global_symbol_table.end()) {
        decoded_string = found_string->second;
      }
    }

    return decoded_string;
  }

  /*
   * Returns either the decoded value of this symbol or the fallback string
   * */
  inline static std::string decode(VALUE symbol) {
    return SymbolTable::decode_optional(symbol).value_or(kUndefinedSymbolString);
  }
};
}  // namespace Charly
