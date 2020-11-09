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

#pragma once

namespace Charly {
using STATUS = uint16_t;

// Status codes used throughout the machine
enum Status : STATUS {
  Success,
  CantReturnFromTopLevel,
  CatchStackEmpty,
  ReadFailedOutOfBounds,
  ReadFailedTooDeep,
  WriteFailedOutOfBounds,
  WriteFailedTooDeep,
  InvalidArgumentType,
  CorruptedStack,
  RuntimeTaskNotCallable,
  InvalidInstructionPointer,
  InvalidThreadId,
  UndefinedGlobalReference,
  Debug
};

// Human-readable status messages
// clang-format off
static const std::string kStatusHumanReadable[] = {
  "Success",
  "Can't return from the top-level",
  "Catch stack is empty",
  "Reading local field failed, out of bounds",
  "Reading local field failed, too deep",
  "Writing local field failed, out of bounds",
  "Writing local field failed, too deep",
  "Argument for instruction has invalid type",
  "Corrupted Stack",
  "Callback of runtime task is not a callable type",
  "Invalid instruction pointer",
  "Invalid thread id",
  "UndefinedGlobalReference",
  "Debug"
};
// clang-format on
}  // namespace Charly
