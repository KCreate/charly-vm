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

#pragma once

namespace Charly {
// An opcode identifies a single instruction the machine can perform
// Opcodes can have arguments
enum Opcode : uint8_t {

  // Do nothing
  Nop,

  // Read a value at a given offset from a given frame
  // Pushes null if accessed with an out-of-bounds index
  //
  // args:
  // - index
  // - level
  ReadLocal,

  // Resolve symbol inside identifier
  //
  // args:
  // - symbol
  //
  // stack:
  // - identifier
  ReadMemberSymbol,

  // Resolve member inside identifier
  //
  // stack:
  // - identifier
  // - member
  ReadMemberValue,

  // Reads a value at a given index inside an array
  // The value popped from the stack needs to be an array, else this instruction does nothing
  // and the first stack argument will be pushed back onto the stack
  //
  // args:
  // - index
  //
  // stack:
  // - array
  ReadArrayIndex,

  // Reads a global symbol
  //
  // args:
  // - symbol
  ReadGlobal,

  // Set a value at a given offset inside a given frame
  // Will pop, but not write, if the index is out-of-bounds
  // Will push the assigned value onto the stack
  //
  // args:
  // - index
  // - level
  //
  // stack:
  // - value
  SetLocalPush,

  // Pop value and write to symbol of identifier
  //
  // args:
  // - symbol
  //
  // stack:
  // - identifier
  // - value
  SetMemberSymbolPush,

  // Pop value and write to member of identifier
  //
  // stack:
  // - identifier
  // - member
  // - value
  SetMemberValuePush,

  // Sets the value at a given index inside an array
  //
  // The array is pushed back onto the stack in any case
  //
  // args:
  // - index
  //
  // stack:
  // - array
  // - value
  SetArrayIndexPush,

  // Same as the SetLocalPush instruction, except it doesn't push the value onto the stack
  //
  // args:
  // - index
  // - level
  //
  // stack:
  // - value
  SetLocal,

  // Same as the SetMemberSymbolPush instruction, except it doesn't push the value onto the stack
  //
  // args:
  // - symbol
  //
  // stack:
  // - identifier
  // - value
  SetMemberSymbol,

  // Same as the SetMemberValuePush instruction, except it doesn't push the value onto the stack
  //
  // stack:
  // - identifier
  // - member
  // - value
  SetMemberValue,

  // Same as the SetArrayIndexPush instruction, except it doesn't push the value onto the stack
  SetArrayIndex,

  // Write to a global symbol
  // Will throw an exception if no such global symbol is found
  //
  // args:
  // - symbol
  //
  // stack:
  // - value
  SetGlobal,

  // Write to a global symbol
  // Will throw an exception if no such global symbol is found
  // Will push the assigned value onto the stack
  //
  // args:
  // - symbol
  //
  // stack:
  // - value
  SetGlobalPush,

  // Put the self value of a specified frame onto the stack
  //
  // args:
  // - level
  PutSelf,

  // If inside a class constructor, puts a copy of the parent constructor
  // onto the stack, with its bound_self value set to the current
  // self value
  PutSuper,

  // If inside a class member method, puts a copy of the parent member
  // function onto the stack, with its bound_self value set to
  // the current self value
  PutSuperMember,

  // Put value onto the stack
  //
  // args:
  // - value
  PutValue,

  // Put string onto the stack
  //
  // The actual string data lives in the TEXT segment of the compiled instructionblock
  //
  // args:
  // - offset
  // - length
  PutString,

  // Put a function onto the stack
  //
  // args:
  // - symbol
  // - block_offset
  // - anonymous
  // - needs_arguments
  // - argc
  // - minimum_argc
  // - lvarcount
  PutFunction,

  // Put an array onto the stack, popping a given amount of values from the stack
  // and inserting them into the array
  //
  // args:
  // - count
  PutArray,

  // Put a hash onto the stack, popping a given amount of key / value pairs from the stack
  // and inserting them into the array
  //
  // args:
  // - count
  PutHash,

  // Put a new class onto the stack
  // Properties, methods, static properties, static methods and parent classes are popped
  // off of the stack
  //
  // args:
  // - symbol
  // - propertycount
  // - staticpropertycount
  // - methodcount
  // - staticmethodcount
  // - has_parent_class
  // - has_constructor
  //
  // stack:
  // - properties
  // - static properties
  // - methods
  // - static methods
  // - parent_class?
  // - constructor?
  PutClass,

  // Pop a value off the stack
  Pop,

  // Duplicate the top value of the stack
  Dup,

  // Duplicate the top n values of the stack
  //
  // args:
  // - n
  Dupn,

  // Swap the top two values of the stack
  Swap,

  // Call a function with argc arguments
  //
  // args:
  // - argc
  //
  // stack:
  // - function
  // - arguments
  Call,

  // Call a function with argc arguments and a target
  //
  // args:
  // - argc
  //
  // stack:
  // - target
  // - function
  // - arguments
  CallMember,

  // Construct a new object
  //
  // args:
  // - argc
  //
  // stack:
  // - klass
  // - arguments
  New,

  // Return from the current frame
  Return,

  // Currently unused opcode
  // TODO: Replace with something usable
  //
  // stack:
  // - expression
  Yield,

  // Throw a value
  //
  // stack:
  // - value
  Throw,

  // Push a new catch table onto the machine
  // WARNING: Offset is in bytes, no instruction length decoding is done
  //
  // args:
  // - offset
  RegisterCatchTable,

  // Pop the current catch table off the catchstack
  PopCatchTable,

  // Apply a given offset to the instruction pointer
  // WARNING: Offset is in bytes, no instruction length decoding is done
  //
  // args:
  // - offset
  Branch,

  // Pop test and apply a given offset to the instruction pointer
  // if test is truthy
  // WARNING: Offset is in bytes, no instruction length decoding is done
  //
  // args:
  // - offset
  //
  // stack:
  // - test
  BranchIf,

  // Pop test and apply a given offset to the instruction pointer
  // if test is falsey
  // WARNING: Offset is in bytes, no instruction length decoding is done
  //
  // args:
  // - offset
  //
  // stack:
  // - test
  BranchUnless,

  // Shorthand for
  //
  // lt
  // branchunless
  //
  // args:
  // - offset
  //
  // stack:
  // - left
  // - right
  BranchLt,

  // Shorthand for
  //
  // gt
  // branchunless
  //
  // args:
  // - offset
  //
  // stack:
  // - left
  // - right
  BranchGt,

  // Shorthand for
  //
  // le
  // branchunless
  //
  // args:
  // - offset
  //
  // stack:
  // - left
  // - right
  BranchLe,

  // Shorthand for
  //
  // ge
  // branchunless
  //
  // args:
  // - offset
  //
  // stack:
  // - left
  // - right
  BranchGe,

  // Shorthand for
  //
  // eq
  // branchunless
  //
  // args:
  // - offset
  //
  // stack:
  // - left
  // - right
  BranchEq,

  // Shorthand for
  //
  // neq
  // branchunless
  //
  // args:
  // - offset
  //
  // stack:
  // - left
  // - right
  BranchNeq,

  // Binary operators
  //
  // stack:
  // - left
  // - right
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Pow,
  Eq,
  Neq,
  Lt,
  Gt,
  Le,
  Ge,
  Shr,
  Shl,
  And,
  Or,
  Xor,

  // Unary operators
  //
  // stack:
  // - value
  UAdd,
  USub,
  UNot,
  UBNot,

  // Halt execution of the machine and return to the caller
  Halt,

  // Push the type of the uppermost value on the stack as a string
  Typeof,

  // The amount of opcodes that are defined, not an actual opcode
  OpcodeCount
};

#define i8 sizeof(uint8_t)
#define i32 sizeof(uint32_t)
#define i64 sizeof(uint64_t)

// clang-format off
// Constant lengths of all instructions
static constexpr uint32_t kInstructionLengths[]{
  /* Nop */                               1,
  /* ReadLocal */                         1 + i32 + i32,
  /* ReadMemberSymbol */                  1 + i64,
  /* ReadMemberValue */                   1,
  /* ReadArrayIndex */                    1 + i32,
  /* ReadGlobal */                        1 + i64,
  /* SetLocalPush */                      1 + i32 + i32,
  /* SetMemberSymbolPush */               1 + i64,
  /* SetMemberValuePush */                1,
  /* SetArrayIndexPush */                 1 + i32,
  /* SetLocal */                          1 + i32 + i32,
  /* SetMemberSymbol */                   1 + i64,
  /* SetMemberValue */                    1,
  /* SetArrayIndex */                     1 + i32,
  /* SetGlobal */                         1 + i64,
  /* SetGlobalPush */                     1 + i64,
  /* PutSelf */                           1,
  /* PutSuper */                          1,
  /* PutSuperMember */                    1 + i64,
  /* PutValue */                          1 + i64,
  /* PutString */                         1 + i32 + i32,
  /* PutFunction */                       1 + i64 + i32 + i8 * 2 + i32 + i32 + i32,
  /* PutArray */                          1 + i32,
  /* PutHash */                           1 + i32,
  /* PutClass */                          1 + i64 + i32 * 4 + i8 + i8,
  /* Pop */                               1,
  /* Dup */                               1,
  /* Dupn */                              1 + i32,
  /* Swap */                              1,
  /* Call */                              1 + i32,
  /* CallMember */                        1 + i32,
  /* New */                               1 + i32,
  /* Return */                            1,
  /* Yield */                             1,
  /* Throw */                             1,
  /* RegisterCatchTable */                1 + i32,
  /* PopCatchTable */                     1,
  /* Branch */                            1 + i32,
  /* BranchIf */                          1 + i32,
  /* BranchUnless */                      1 + i32,
  /* BranchLt */                          1 + i32,
  /* BranchGt */                          1 + i32,
  /* BranchLe */                          1 + i32,
  /* BranchGe */                          1 + i32,
  /* BranchEq */                          1 + i32,
  /* BranchNeq */                         1 + i32,
  /* Add, */                              1,
  /* Sub, */                              1,
  /* Mul, */                              1,
  /* Div, */                              1,
  /* Mod, */                              1,
  /* Pow, */                              1,
  /* Eq, */                               1,
  /* Neq, */                              1,
  /* Lt, */                               1,
  /* Gt, */                               1,
  /* Le, */                               1,
  /* Ge, */                               1,
  /* Shr, */                              1,
  /* Shl, */                              1,
  /* And, */                              1,
  /* Or, */                               1,
  /* Xor, */                              1,
  /* UAdd */                              1,
  /* USub */                              1,
  /* UNot */                              1,
  /* UBNot */                             1,
  /* Halt */                              1,
  /* Typeof */                            1
};

#undef i8
#undef i32
#undef i64

// String representations of instruction opcodes
static std::string kOpcodeMnemonics[]{
  "nop",
  "readlocal",
  "readmembersymbol",
  "readmembervalue",
  "readarrayindex",
  "readglobal",
  "setlocalpush",
  "setmembersymbolpush",
  "setmembervaluepush",
  "setarrayindexpush",
  "setlocal",
  "setmembersymbol",
  "setmembervalue",
  "setarrayindex",
  "setglobal",
  "setglobalpush",
  "putself",
  "putsuper",
  "putsupermember",
  "putvalue",
  "putstring",
  "putfunction",
  "putarray",
  "puthash",
  "putclass",
  "pop",
  "dup",
  "dupn",
  "swap",
  "call",
  "callmember",
  "new",
  "return",
  "yield",
  "throw",
  "registercatchtable",
  "popcatchtable",
  "branch",
  "branchif",
  "branchunless",
  "branchlt",
  "branchgt",
  "branchle",
  "branchge",
  "brancheq",
  "branchneq",
  "add",
  "sub",
  "mul",
  "div",
  "mod",
  "pow",
  "eq",
  "neq",
  "lt",
  "gt",
  "le",
  "ge",
  "shr",
  "shl",
  "and",
  "or",
  "xor",
  "uadd",
  "usub",
  "unot",
  "ubnot",
  "halt",
  "typeof"
};
// clang-format on

}  // namespace Charly
