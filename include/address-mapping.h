/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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

#include <tuple>
#include <vector>

#include "defines.h"
#include "instructionblock.h"

#pragma once

namespace Charly {

/*
 * Keeps track of which addresses belong to which files
 * */
class AddressMapping {
private:
  std::vector<std::tuple<uint8_t*, uint8_t*, std::string>> mappings;

public:

  // Register an instructionblocks address range under a specific filepath
  inline void register_instructionblock(InstructionBlock* block, const std::string& path) {
    uint8_t* begin = block->data;
    uint8_t* end = begin + block->capacity;
    this->mappings.push_back({begin, end, path});
  }

  // Return the filepath an address belongs to
  inline std::optional<std::string> resolve_address(uint8_t* address) {
    for (const auto& a : this->mappings) {
      if (address >= std::get<0>(a) && address < std::get<1>(a)) {
        return std::get<2>(a);
      }
    }

    return std::nullopt;
  }
};
}  // namespace Charly
