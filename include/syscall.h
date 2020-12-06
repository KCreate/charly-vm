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

#include <cstdint>
#include <unordered_map>

#pragma once

namespace Charly {
enum SyscallID : uint16_t {

  // Create a timer, pushes id onto the stack
  // - function
  // - timeout
  TimerInit,

  // Clear a timer
  // - id
  TimerClear,

  // Create a ticker, pushes id onto the stack
  // - function
  // - period
  TickerInit,

  // Clear a ticker
  // - id
  TickerClear,

  // Suspend the currently executing fiber
  // The return value of this syscall is the argument
  // passed to the corresponding FiberResume call
  FiberSuspend,

  // Resume a paused fiber
  // - id
  // - argument
  FiberResume,

  // Dynamically call a function
  // - function
  // - arguments
  CallDynamic,

  // Dynamically call a function with member value
  // - context
  // - function
  // - arguments
  CallMemberDynamic,

  // Remove the bound_self value of a function
  // - function
  ClearBoundSelf,

  // Cast input value to a given type
  // - value
  CastInt,
  CastFloat,
  CastNumber,
  CastString,

  // Check wether input value is a float number
  // - value
  IsFloat,

  // Copy a value
  // - value
  CopyValue,

  // Insert a value into an array
  // - array
  // - index
  // - value
  ArrayInsert,

  // Remove some element from an array
  // - array
  // - index
  ArrayRemove,

  // Clear the contents of an array
  // - array
  ArrayClear,

  // Return keys of object as array
  // - object
  ContainerListKeys,

  // Delete some key out of a container
  // - container
  // - key
  ContainerDeleteKey,

  // Trim whitespace off the beginning of a string
  // - string
  StringTrimL,

  // Trim whitespace off the end of a string
  // - string
  StringTrimR,

  // Return lowercased version of string
  // - string
  StringLowercase,

  // Return uppercased version of string
  // - string
  StringUppercase,

  // The amount of defined syscalls
  SyscallCount
};

// The amount of arguments each syscall requires
static constexpr uint16_t kSyscallArgumentCount[]{
  /* TimerInit */           2,
  /* TimerClear */          1,
  /* TickerInit */          2,
  /* TickerClear */         1,
  /* FiberSuspend */        0,
  /* FiberResume */         2,
  /* CallDynamic */         2,
  /* CallMemberDynamic */   3,
  /* ClearBoundSelf */      1,
  /* CastInt */             1,
  /* CastFloat */           1,
  /* CastNumber */          1,
  /* CastString */          1,
  /* IsFloat */             1,
  /* CopyValue */           1,
  /* ArrayInsert */         3,
  /* ArrayRemove */         2,
  /* ArrayClear */          1,
  /* ContainerListKeys */   1,
  /* ContainerDeleteKey */  2,
  /* StringTrimL */         1,
  /* StringTrimR */         1,
  /* StringLowercase */     1,
  /* StringUppercase */     1
};

// Plaintext names of syscalls
static std::string kSyscallNames[] = {
  "timerinit",
  "timerclear",
  "tickerinit",
  "tickerclear",
  "fibersuspend",
  "fiberresume",
  "calldynamic",
  "callmemberdynamic",
  "clearboundself",
  "castint",
  "castfloat",
  "castnumber",
  "caststring",
  "isfloat",
  "copyvalue",
  "arrayinsert",
  "arrayremove",
  "arrayclear",
  "containerlistkeys",
  "containerdeletekey",
  "stringtriml",
  "stringtrimr",
  "stringlowercase",
  "stringuppercase"
};

// Mapping from plaintext names to ids
static std::unordered_map<std::string, SyscallID> kSyscallNameMapping = {
  {"timerinit",          SyscallID::TimerInit},
  {"timerclear",         SyscallID::TimerClear},
  {"tickerinit",         SyscallID::TickerInit},
  {"tickerclear",        SyscallID::TickerClear},
  {"fibersuspend",       SyscallID::FiberSuspend},
  {"fiberresume",        SyscallID::FiberResume},
  {"calldynamic",        SyscallID::CallDynamic},
  {"callmemberdynamic",  SyscallID::CallMemberDynamic},
  {"clearboundself",     SyscallID::ClearBoundSelf},
  {"castint",            SyscallID::CastInt},
  {"castfloat",          SyscallID::CastFloat},
  {"castnumber",         SyscallID::CastNumber},
  {"caststring",         SyscallID::CastString},
  {"isfloat",            SyscallID::IsFloat},
  {"copyvalue",          SyscallID::CopyValue},
  {"arrayinsert",        SyscallID::ArrayInsert},
  {"arrayremove",        SyscallID::ArrayRemove},
  {"arrayclear",         SyscallID::ArrayClear},
  {"containerlistkeys",  SyscallID::ContainerListKeys},
  {"containerdeletekey", SyscallID::ContainerDeleteKey},
  {"stringtriml",        SyscallID::StringTrimL},
  {"stringtrimr",        SyscallID::StringTrimR},
  {"stringlowercase",    SyscallID::StringLowercase},
  {"stringuppercase",    SyscallID::StringUppercase}
};
// clang-format on
}  // namespace Charly
