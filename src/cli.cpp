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

#include <fstream>
#include <iostream>

#include "charly.h"
#include "cli.h"
#include "compiler.h"
#include "disassembler.h"
#include "sourcefile.h"

namespace Charly {
using namespace Compilation;

int CLI::run() {
  if (this->flags.show_help) {
    std::cout << kHelpMessage << '\n';
    return 0;
  }

  if (this->flags.show_license) {
    std::cout << kLicense << '\n';
    return 0;
  }

  if (this->flags.show_version) {
    std::cout << kVersion << '\n';
    return 0;
  }

  // Check if a filename was given
  if (this->flags.arguments.size() == 0) {
    std::cerr << "No filename given!" << '\n';
    std::cout << kHelpMessage << '\n';
    return 1;
  }

  // Read the userfile
  std::ifstream inputfile(this->flags.arguments[0]);
  if (!inputfile.is_open()) {
    std::cerr << "Could not open file" << '\n';
    return 1;
  }
  std::string source_string((std::istreambuf_iterator<char>(inputfile)), std::istreambuf_iterator<char>());
  SourceFile userfile(this->flags.arguments[0], source_string);

  // Create the compiler
  CompilationConfig cconfig = {
      .known_top_level_vars = {"require_no_exec", "require", "Object", "Class", "Array", "String", "Number", "Function",
                               "Boolean", "Null", "stdin", "stdout", "stderr", "print", "write", "gets", "getc", "exit",
                               "sleep"},
      .flags = this->flags};
  CompilerContext compiler_context;
  Compiler compiler(compiler_context);

  // Compile the sourcefile
  InstructionBlock* compiled_block = compiler.compile(userfile, cconfig);

  if (this->flags.dump_asm) {
    if (compiled_block != nullptr) {
      Disassembler::Flags disassembler_flags =
          Disassembler::Flags({this->flags.asm_no_branches, this->flags.asm_no_offsets});
      Disassembler disassembler(compiled_block, disassembler_flags, &compiler_context);
      disassembler.dump(std::cout);
    }
  }

  if (this->flags.skip_execution) {
    return 0;
  }

  GarbageCollector gc({.initial_heap_count = 1, .heap_cell_count = 64, .heap_growth_factor = 1.5});
  SymbolTable symtable;
  StringPool stringpool;
  VMContext context({.symtable = symtable,
                     .stringpool = stringpool,
                     .gc = gc,
                     .trace_opcodes = this->flags.trace_opcodes,
                     .trace_catchtables = this->flags.trace_catchtables,
                     .trace_frames = this->flags.trace_frames,
                     .out_stream = std::cout,
                     .err_stream = std::cerr});
  VM vm(context);

  // VM vm(context);
  // vm.run();

  return 0;
}
}  // namespace Charly
