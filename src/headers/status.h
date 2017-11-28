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

// Status codes used throughout the machine
static const STATUS kStatusSuccess = 0x00;
static const STATUS kStatusFieldIsConstant = 0x01;
static const STATUS kStatusUnspecifiedError = 0x02;
static const STATUS kStatusCantReturnFromTopLevel = 0x03;
static const STATUS kStatusUnknownThrowType = 0x04;
static const STATUS kStatusIpOutOfBounds = 0x05;
static const STATUS kStatusNotEnoughSpaceForInstructionArguments = 0x06;
static const STATUS kStatusUnknownOpcode = 0x07;
static const STATUS kStatusTooManyArgumentsForCFunction = 0x08;
static const STATUS kStatusNotEnoughArguments = 0x09;
static const STATUS kStatusCatchStackEmpty = 0x0a;
static const STATUS kStatusNoSuitableCatchTableFound = 0x0b;
static const STATUS kStatusReadFailedOutOfBounds = 0x0c;
static const STATUS kStatusReadFailedTooDeep = 0x0d;
static const STATUS kStatusWriteFailedOutOfBounds = 0x0e;
static const STATUS kStatusWriteFailedTooDeep = 0x0f;
static const STATUS kStatusStackEmpty = 0x10;

// Human-readable status messages
static const std::string kStatusHumanReadable[] = {"Success",
                                                   "Field is constant",
                                                   "Unspecified error",
                                                   "Can't return from the top-level",
                                                   "Unknown throw type",
                                                   "Instruction pointer is out of bounds",
                                                   "Not enough space for instruction arguments",
                                                   "Unknown opcode",
                                                   "Too many arguments for CFunction",
                                                   "Not enough arguments",
                                                   "Catch stack is empty",
                                                   "No suitable catch table found",
                                                   "Reading local field failed, out of bounds",
                                                   "Reading local field failed, too deep",
                                                   "Writing local field failed, out of bounds",
                                                   "Writing local field failed, too deep",
                                                   "Stack is empty"};
}  // namespace Charly
