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

#include "lvar-alloc.h"

namespace Charly::Compilation {

uint32_t FunctionScope::alloc_slot(bool constant) {
  if (this->active_slots.size() == 0) {
    this->active_slots.push_back({.constant = constant});
    return 0;
  }

  // Check if we have an unactive slot
  uint32_t allocated_slot = this->active_slots.size() - 1;
  for (auto slot = this->active_slots.rbegin(); slot != this->active_slots.rend(); slot++) {
    // If the slot is inactive, mark it as active and return the index
    if (!slot->active) {
      slot->active = true;
      return allocated_slot;
    }

    allocated_slot--;
  }

  this->active_slots.push_back({.constant = constant});
  return this->active_slots.size() - 1;
}

}  // namespace Charly::Compilation
