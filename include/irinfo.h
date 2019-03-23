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

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#pragma once

namespace Charly::Compilation {

// Contains a list of variables which, unless defined explicitly,
// should be rewritten to @<name> inside a particular function
struct IRKnownSelfVars {
  std::unordered_set<std::string> names;

  IRKnownSelfVars(const std::vector<std::string>& n) {
    for (auto& name : n) {
      this->names.insert(name);
    }
  }

  IRKnownSelfVars(std::vector<std::string>&& n) {
    for (auto& name : n) {
      this->names.insert(std::move(name));
    }
  }
};

// Contains information about this value on the stack
struct IRAssignmentInfo {
  bool assignment_value_required = true;

  IRAssignmentInfo(bool avr) : assignment_value_required(avr) {
  }
};
}  // namespace Charly::Compilation
