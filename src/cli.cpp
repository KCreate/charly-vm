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

#include <filesystem>
#include <fstream>

#include "charly.h"
#include "cli.h"
#include "compiler.h"
#include "disassembler.h"
#include "parser.h"
#include "sourcefile.h"

namespace Charly {
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

  // Check that CHARLYVMDIR environment variable is set
  char* CHARLYVMDIR = std::getenv("CHARLYVMDIR");
  if (CHARLYVMDIR) {
    if (this->flags.show_vmdir) {
      std::cout << CHARLYVMDIR << '\n';
      return 0;
    }
  } else {
    std::cout << "Missing CHARLYVMDIR environment variable!" << '\n';
    std::cout << "Set CHARLYVMDIR to the charly-vm project root folder" << '\n';
    return 1;
  }

  // Check if a filename was given
  if (this->flags.arguments.size() == 0) {
    std::cerr << "No filename given!" << '\n';
    std::cout << kHelpMessage << '\n';
    return 1;
  }

  this->flags.dump_files_include.push_back(this->flags.arguments[0]);

  std::string inputfile_path(std::string(std::getenv("PWD")) + "/" + this->flags.arguments[0]);

  // Read the userfile
  std::ifstream inputfile(inputfile_path);
  if (!inputfile.is_open()) {
    std::cerr << "Could not open file" << '\n';
    return 1;
  }
  std::string source_string((std::istreambuf_iterator<char>(inputfile)), std::istreambuf_iterator<char>());

  // Read the prelude
  char* stdlibpath = std::getenv("CHARLYVMDIR");

  if (stdlibpath == nullptr) {
    std::cerr << "Could not locate stdlib folder. Set the CHARLYVMDIR environment variable" << '\n';
    return 1;
  }

  std::string preludepath = std::string(stdlibpath) + "/src/stdlib/prelude.ch";
  std::ifstream preludefile(preludepath);
  if (!preludefile.is_open()) {
    std::cerr << "Could not open prelude" << '\n';
    return 1;
  }
  std::string prelude_string((std::istreambuf_iterator<char>(preludefile)), std::istreambuf_iterator<char>());

  Compilation::CompilerManager cmanager(this->flags);

  // Load symbols of internal vm methods
  for (auto& sig : Internals::Index::methods)
    SymbolTable::encode(sig.second.name);

  auto cresult_userfile = cmanager.compile(inputfile_path, source_string);
  auto cresult_prelude = cmanager.compile(preludepath, prelude_string);

  if (!cresult_userfile.has_value()) {
    return 1;
  }

  if (!cresult_prelude.has_value()) {
    return 1;
  }

  if (this->flags.skip_execution) {
    return 0;
  }

  VMContext context({.compiler_manager = cmanager,
                     .instruction_profile = this->flags.instruction_profile,
                     .trace_opcodes = this->flags.trace_opcodes,
                     .trace_catchtables = this->flags.trace_catchtables,
                     .trace_frames = this->flags.trace_frames,
                     .trace_gc = this->flags.trace_gc,
                     .argv = &this->flags.arguments,
                     .environment = &this->flags.environment});
  VM vm(context);

  VALUE fn_prelude = vm.register_module(cresult_prelude->instructionblock.value());
  VALUE fn_user = vm.register_module(cresult_userfile->instructionblock.value());

  vm.register_task(VMTask::init_callback(fn_prelude));
  vm.register_task(VMTask::init_callback(fn_user));
  uint8_t status_code = vm.start_runtime();

  // Display the instruction profile if requested
  if (this->flags.instruction_profile) {
    uint8_t opcode = 0;
    std::cerr << "Instruction Profile:" << '\n';
    while (opcode < OpcodeCount) {
      std::cerr << std::setw(26);
      std::cerr << kOpcodeMnemonics[opcode] << ": ";
      std::cerr << std::setw(1);
      std::cerr << vm.instruction_profile.entries[opcode].average_length << " nanoseconds ";
      std::cerr << "(" << vm.instruction_profile.entries[opcode].encountered << " samples)" << '\n';
      opcode++;
    }
  }

  return status_code;
}
}  // namespace Charly
