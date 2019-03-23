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

#pragma once

namespace Charly::Compilation {

  // Different types of locations
  //
  // Values can either be stored inside a stack frame, on the stack or inside a frames arguments.
  enum LocationType : uint8_t {
    LocFrame,
    LocStack,
    LocArguments,
    LocSelf
  };

  // Stores the location of a value
  struct ValueLocation {
    static ValueLocation frame(uint32_t level, uint32_t index) {
      return { .type = LocationType::LocFrame, .as_frame = { level, index } };
    }

    static ValueLocation stack(uint32_t offset) {
      return { .type = LocationType::LocStack, .as_stack = { offset } };
    }

    static ValueLocation arguments(uint32_t offset, uint32_t level = 0) {
      return { .type = LocationType::LocArguments, .as_arguments = { offset, level } };
    }

    static ValueLocation self(uint64_t symbol, uint32_t level = 0) {
      return { .type = LocationType::LocSelf, .as_self = { symbol, level } };
    }

    static ValueLocation patch_level(ValueLocation location, uint32_t new_level) {
      switch (location.type) {
        case LocationType::LocFrame: {
          location.as_frame.level = new_level;
          break;
        }
        case LocationType::LocArguments: {
          location.as_arguments.level = new_level;
          break;
        }
        case LocationType::LocSelf: {
          location.as_self.level = new_level;
          break;
        }
        default: { break; }
      }

      return location;
    }

    LocationType type;

    union {
      struct {
        uint32_t level;
        uint32_t index;
      } as_frame;

      struct {
        uint32_t offset;
      } as_stack;

      struct {
        uint32_t offset;
        uint32_t level;
      } as_arguments;

      struct {
        uint64_t symbol;
        uint32_t level;
      } as_self;
    };
  };

  // This struct stores information about the relative location of a variable during the compilation process.
  struct LocalOffsetInfo {
    ValueLocation location;
    bool valid = true;
    bool constant = false;
    bool overwriteable = false;
  };
}
