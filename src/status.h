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

#pragma once

namespace Charly {
  namespace Status {
    static const STATUS Success                                = 0x00;
    static const STATUS WriteFailedVariableIsConstant          = 0x01;
    static const STATUS WriteFailedVariableUndefined           = 0x02;
    static const STATUS WriteFailedOutOfBounds                 = 0x03;
    static const STATUS WriteFailedTooDeep                     = 0x04;
    static const STATUS RegisterFailedAlreadyDefined           = 0x05;
    static const STATUS ReadFailedVariableUndefined            = 0x06;
    static const STATUS ReadFailedOutOfBounds                  = 0x07;
    static const STATUS ReadFailedTooDeep                      = 0x08;
    static const STATUS UnknownSymbol                          = 0x09;
    static const STATUS PopFailedStackEmpty                    = 0x0a;
    static const STATUS UnspecifiedError                       = 0x0b;
    static const STATUS CantReturnFromTopLevel                 = 0x0c;
    static const STATUS UnknownThrowType                       = 0x0d;
    static const STATUS IpOutOfBounds                          = 0x0e;
    static const STATUS NotEnoughSpaceForInstructionArguments  = 0x0f;
    static const STATUS UnknownOpcode                          = 0x10;
    static const STATUS TooManyArgumentsForCFunction           = 0x11;
    static const STATUS NotEnoughArguments                     = 0x12;
    static const STATUS CatchStackEmpty                        = 0x13;
    static const STATUS NoSuitableCatchTableFound              = 0x14;

    // Human-readable error messages
    static const std::string str[] = {
      "Success",
      "Write failed: Field is a constant",
      "Write failed: Field doesn't exist",
      "Write failed: Index out of bounds",
      "Write failed: Environment doesn't exist",
      "Register failed: Key already defined",
      "Read failed: Field doesn't exist",
      "Read failed: Index out of bounds",
      "Read failed: Environment doesn't exist",
      "Unknown symbol",
      "Pop failed: Stack empty",
      "Unspecified error",
      "Cannot return from top-level",
      "Unknown throw type",
      "Instruction pointer is out-of-bounds",
      "Not enough space for instruction arguments",
      "Unknown opcode",
      "Too many arguments for CFunction",
      "Not enough arguments",
      "Pop failed: Catchstack empty",
      "No suitable catch table found"
    };
  }
}
