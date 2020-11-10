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

#include "cliflags.h"
#include "charly.h"
#include "compiler.h"
#include "disassembler.h"
#include "parser.h"
#include "sourcefile.h"

extern char** environ;

using namespace Charly;

void print_help_page() {
  std::cout << kUsageMessage << std::endl;
  std::cout << std::endl;

  for (const FlagGroup& group : kDefinedFlagGroups) {
    std::cout << group.name << std::endl;

    for (const FlagDescriptor& flag : group.flags) {
      std::cout << "  ";
      if (flag.shortselector) {
        std::cout << "-" << flag.shortselector.value();
        std::cout << ", ";
      }
      std::cout << "--" << flag.name;
      if (flag.argument) {
        std::cout << " <" + flag.argument.value() + ">";
      }
      std::cout << std::endl;
      std::cout << "      " << flag.description;
      std::cout << std::endl;
      std::cout << std::endl;
    }
  }

  std::cout << kExampleUsages << std::endl;
}

int main(int argc, char** argv) {
  Charly::CLIFlags::init_argv(argc, argv);
  Charly::CLIFlags::init_env(environ);

  // Check that CHARLYVMDIR environment variable is set
  char* CHARLYVMDIR = std::getenv("CHARLYVMDIR");
  if (CHARLYVMDIR) {
    if (CLIFlags::is_flag_set("vmdir")) {
      std::cout << CHARLYVMDIR << '\n';
      return 0;
    }
  } else {
    std::cerr << "Missing CHARLYVMDIR environment variable!" << '\n';
    std::cerr << "Set CHARLYVMDIR to the charly-vm project root folder" << '\n';
    return 1;
  }

  std::optional<std::string> filename = CLIFlags::get_argument(0);

  // Check if a filename was given
  if (!filename) {
    std::cerr << "No filename given!" << '\n';
    print_help_page();
    return 1;
  }

  if (CLIFlags::is_flag_set("help")) {
    print_help_page();
    return 0;
  }

  if (CLIFlags::is_flag_set("license")) {
    std::cout << kLicense << '\n';
    return 0;
  }

  if (CLIFlags::is_flag_set("version")) {
    std::cout << kVersion << '\n';
    return 0;
  }

  CLIFlags::set_flag("dump_file_include", filename);

  std::string inputfile_path(std::string(std::getenv("PWD")) + "/" + filename.value());

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

  Compilation::CompilerManager cmanager;

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

  if (CLIFlags::is_flag_set("skipexec")) {
    return 0;
  }

  VMContext context({ .compiler_manager = cmanager });
  VM vm(context);
  GarbageCollector::set_host_vm(vm);

  VALUE fn_prelude = vm.register_module(cresult_prelude->instructionblock.value());
  VALUE fn_user = vm.register_module(cresult_userfile->instructionblock.value());

  vm.register_task(VMTask::init_callback(fn_prelude));
  vm.register_task(VMTask::init_callback(fn_user));
  uint8_t status_code = vm.start_runtime();

  // Display the instruction profile if requested
  if (CLIFlags::is_flag_set("instruction_profile")) {
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
