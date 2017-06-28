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

#include <vector>

#include "defines.h"

#pragma once

namespace Charly {
  namespace Machine {

    struct InstructionBlock {
      const uint32_t INITIAL_BLOCK_SIZE = 256; // TODO: Find a better value for this

      ID id;
      uint32_t lvarcount;
      uint8_t* data;
      uint32_t data_size;

      InstructionBlock(ID id, uint32_t lvarcount) : id(id), lvarcount(lvarcount) {
        this->data = (uint8_t *)malloc(INITIAL_BLOCK_SIZE * sizeof(uint8_t));
        this->data_size = INITIAL_BLOCK_SIZE * sizeof(uint8_t);
      }

      ~InstructionBlock() {
        free(this->data);
      }
    };

  };
};
