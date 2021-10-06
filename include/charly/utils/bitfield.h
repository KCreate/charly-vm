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

#include <cstdint>
#include <cstdlib>

#include "charly/charly.h"

#pragma once

namespace charly::utils {

template <int64_t SIZE>
class BitField {
public:
  static const int64_t kSize = SIZE;

  BitField() {
    reset();
  }

  bool get_bit(int64_t index) const {
    DCHECK(validate_index(index), "invalid index %", index);
    uint8_t entry = m_table[entry_for_index(index)];
    uint8_t mask = mask_for_index(index);
    return entry & mask;
  }

  void set_bit(int64_t index) {
    DCHECK(validate_index(index), "invalid index %", index);
    atomic<uint8_t>& entry = m_table[entry_for_index(index)];
    uint8_t mask = mask_for_index(index);
    uint8_t value = entry;

    // return early if the bit is already in the requested state
    if (value & mask) {
      return;
    }

    while (!entry.cas(value, value | mask)) {
      value = entry;
    }
  }

  void unset_bit(int64_t index) {
    DCHECK(validate_index(index), "invalid index %", index);
    atomic<uint8_t>& entry = m_table[entry_for_index(index)];
    uint8_t mask = mask_for_index(index);
    uint8_t value = entry;

    // return early if the bit is already in the requested state
    if ((value & mask) == 0) {
      return;
    }

    while (!entry.cas(value, value & ~mask)) {
      value = entry;
    }
  }

  void reset() {
    for (int64_t i = 0; i < kSize; i++) {
      m_table[i] = 0;
    }
  }

  int64_t find_next_set_bit(int64_t start = 0) const {
    if (start < 0 || start >= kSize) {
      return -1;
    }

    validate_index(start);

    while (start < kSize) {
      uint8_t entry = m_table[entry_for_index(start)];
      uint8_t mask = mask_for_index(start);

      if (entry & mask) {
        return start;
      }

      start++;
    }

    return -1;
  }

  int64_t size() const {
    return kSize;
  }

private:
  bool validate_index(int64_t index) const {
    return index >= 0 && index < kSize;
  }

  int64_t entry_for_index(int64_t index) const {
    DCHECK(validate_index(index), "invalid index %", index);
    return index / 8;
  }

  uint8_t mask_for_index(int64_t index) const {
    DCHECK(validate_index(index), "invalid index %", index);
    return 0x01 << (index % 8);
  }

private:
  static_assert(SIZE % 8 == 0, "size has to be divisible by 8");
  atomic<uint8_t> m_table[SIZE / 8 + 1];
};

}  // namespace charly::utils
